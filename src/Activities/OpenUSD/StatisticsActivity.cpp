//
//  StatisticsActivity.cpp
//  LabExcelsior
//
//  Created by Nick Porcino on 12/20/23.
//  Copyright © 2023 Nick Porcino. All rights reserved.
//

#include "StatisticsActivity.hpp"

#include "Lab/App.h"
#include "Lab/tinycolormap.hpp"
#include "Providers/Selection/SelectionProvider.hpp"
#include "Providers/OpenUSD/OpenUSDProvider.hpp"

#include "parallel_hashmap/phmap.h"
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <string>
#include <vector>
#include <stdio.h>

#include "imgui.h"
#include "imgui_internal.h"

using phmap::flat_hash_map;
using phmap::flat_hash_set;


namespace lab {
PXR_NAMESPACE_USING_DIRECTIVE

using namespace std;

struct usd_prim_data_t {
    int face_count = 0;
};

struct usd_statistics_t {
    // a map of prim types to a set of prims of that type
    flat_hash_map<std::string, flat_hash_set<UsdPrim>> prim_sets;

    flat_hash_map<UsdPrim, usd_prim_data_t> data;
    flat_hash_map<UsdPrim, usd_prim_data_t> accumulated_data;
    
    void clear() {
        prim_sets.clear();
        data.clear();
        accumulated_data.clear();
    }
};

struct usd_traverse_t {
    vector<UsdPrim> prims;
    vector<UsdPrim> instances;
    flat_hash_map<SdfPath, UsdPrim> prototypes;
    
    bool skip_guides = true;
    bool skip_invisible = true;
    int frame = 0;
    int invalid_instance_count = 0;
};

struct usd_filter_t {
    bool count_faces = false;
};



void RenderRingSlice(ImDrawList& draw_list,
                     const ImVec2& center,
                     double inner_radius, double radius,
                     double a0, double a1, ImU32 col) {
    static const float resolution = 50 / (2 * IM_PI);
    int n = ImMax(3, (int)((a1 - a0) * resolution));
    draw_list.PathClear();
    draw_list.PathArcTo(center, radius, a0, a1, n);
    draw_list.PathArcTo(center, inner_radius, a1, a0, n);

    // calculate the first point of the first arc:
    ImVec2 p0 = center + ImVec2(cosf(a0) * float(radius), sinf(a0) * float(radius));
    draw_list.PathLineTo(p0);
    //draw_list.PathStroke(IM_COL32(255, 255, 0, 255));
    //draw_list.PathStroke(col);
    
    draw_list.PathFillConcave(col);
}


float lerp(float x0, float x1, float u) {
    return x0 * (1.f - u) + x1 * u;
}

struct RenderSlice {
    UsdPrim prim;
    int level;
    float a0, a1;
    float r, g, b, a;
    float r0, r1;
    float x, y;         // <- the location in the middle of the "macaroni"
};

vector<RenderSlice> slices;
bool create_slices = false;

void RenderRadialChart(
        int depth, float t,
        UsdPrim root, UsdPrim selected, usd_statistics_t& stats,
        float x, float y,
        float a0, float a1, float r0, float r1)
{
    auto dl = ImGui::GetWindowDrawList();
    if (!create_slices) {
        for (auto& sl : slices) {
            ImU32 c;
            if (sl.prim == selected) {
                int lum = 128 + int(fabsf(sinf(t)) * 127.f);
                c = IM_COL32(lum, lum, 0, 255);
            }
            else {
                c = IM_COL32((int)(sl.r * 255.f),
                             (int)(sl.g * 255.f),
                             (int)(sl.b * 255.f),
                             (int)(sl.a * 255.f));
            }
            RenderRingSlice(*dl, ImVec2(x, y),
                            sl.r0, sl.r1, sl.a0, sl.a1,
                            c);
        }
        return;
    }

    auto children = root.GetChildren();
    float total = (float) stats.accumulated_data[root].face_count;
    if (total < 1.f)
        return;

    float a2 = a0;
    for (auto i = children.begin(); i != children.end(); ++i) {
        int fc = stats.accumulated_data[*i].face_count;
        if (!fc)
            continue;

        float ctotal = (float) fc;
        float ratio = ctotal / total;
        float a3 = a2 + lerp(a0, a1, ratio) - a0;
        tinycolormap::Color c = tinycolormap::GetHeatColor(ratio);
        if (a3 - a2 > 1e-2f) {
            ImVec2 center = { x, y };
            RenderRingSlice(*dl, center, 
                            r0, r1, a2, a3,
                            IM_COL32((int)(c.r() * 255.f), (int)(c.g() * 255.f), (int)(c.b() * 255.f), 255));
            float halfa = (a2 + a3)/2;
            float midr = (r0 + r1)/2;
            ImVec2 p0 = ImVec2(cosf(halfa) * float(midr), sinf(halfa) * float(midr));

            slices.push_back((RenderSlice)
                             {*i, depth, a2, a3,
                              (float) c.r(), (float) c.g(), (float) c.b(), 1.f,
                              r0, r1,
                              p0.x, p0.y});
        }
        RenderRadialChart(depth + 1, t, *i, selected,
                          stats, x, y, a2, a3, r1 + 1, r1 + 20);
        a2 = a3;
    }
}

struct StatisticsActivity::data {
    data() = default;
    ~data() = default;

    UsdPrim rootPrim;
    float t = 0;
    
    usd_statistics_t stats;
    void usd_statistics(usd_traverse_t& traverse, usd_statistics_t& stats);
    void usd_traverse(UsdPrim root_prim, usd_traverse_t& traverse);
    int  prepare_render_data(UsdPrim root, usd_statistics_t& stats);
};

//  prepare_render_data(scene3.stage->GetPseudoRoot(), stats);

int StatisticsActivity::data::prepare_render_data(UsdPrim root, usd_statistics_t& stats) {
     auto children = root.GetChildren();
     int count = stats.data[root].face_count;
     for (auto i = children.begin(); i != children.end(); ++i) {
         count += prepare_render_data(*i, stats);
     }
     stats.accumulated_data[root].face_count = count;
     return count;
 }

void StatisticsActivity::data::usd_traverse(UsdPrim root_prim, usd_traverse_t& traverse) {
    printf("usd_traverse begin\n");
    auto prim_range = UsdPrimRange::PreAndPostVisit(root_prim);
    for (auto prim_it = prim_range.begin(); 
            prim_it != prim_range.end(); ++prim_it) {
        if ((*prim_it).IsValid() && !prim_it.IsPostVisit()) {
            bool skip = false;
            if (*prim_it != root_prim) {
                if (traverse.skip_guides) {
                    TfToken purpose;
                    if (prim_it->GetAttribute(UsdGeomTokens->purpose).
                            Get(&purpose, traverse.frame)) {
                        skip = purpose == UsdGeomTokens->guide;
                    }
                }
                TfToken visibility;
                if (!skip && traverse.skip_invisible &&
                        prim_it->GetAttribute(UsdGeomTokens->visibility).
                            Get(&visibility, traverse.frame)) {
                    skip = visibility == UsdGeomTokens->invisible;
                }
            }
            if (skip) {
                prim_it.PruneChildren();
            }
            else {
                traverse.prims.push_back(*prim_it);
                if (prim_it->IsInstance()) {
                    UsdPrim prototype = prim_it->GetPrototype();
                    if (prototype.IsValid()) {
                        traverse.instances.push_back(*prim_it);
                        auto path = prototype.GetPath();
                        traverse.prototypes[path] = prototype;
                    }
                    else
                        ++traverse.invalid_instance_count;
                }
            }
        }
    }
    printf("usd_traverse end\n");
}

void StatisticsActivity::data::usd_statistics(usd_traverse_t& traverse, usd_statistics_t& stats)
{
    printf("usd_statistics begin\n");

    
    // create this array outside the loop because .Get will re-use a container
    // to avoid allocation overhead of repeatedly creating a new one.
    VtIntArray face_vert_counts;
    for (auto& i : traverse.prims) {
        // is typename in prim_sets?
        auto tn = i.GetTypeName().GetString();
        if (!stats.prim_sets.contains(tn)) {
            stats.prim_sets[tn] = flat_hash_set<UsdPrim>();
        }
        stats.prim_sets[tn].insert(i);
        if (i.IsA<UsdGeomMesh>()) {
            UsdGeomMesh geom(i);
            geom.GetFaceVertexCountsAttr().Get(&face_vert_counts, 0);
            stats.data[i].face_count = (int) face_vert_counts.size();
        }
    }
    printf("usd_statistics end\n");
}

StatisticsActivity::StatisticsActivity()
: Activity(StatisticsActivity::sname())
, _self(new data)
{
    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<StatisticsActivity*>(instance)->RunUI(*vi);
    };
}

StatisticsActivity::~StatisticsActivity() {
    delete _self;
}

void StatisticsActivity::RunUI(const LabViewInteraction&) {
    bool must_create_slices = false;

    auto selection = SelectionProvider::instance();
    auto selectedPrims = selection->GetSelectionPrims();

    ImGui::Begin("Statistics");
    if (ImGui::Button("Gather")) {
        usd_traverse_t traverse;
        if (selectedPrims.size()) {
            _self->rootPrim = selectedPrims[0];
        }
        else {
            auto usd = OpenUSDProvider::instance();
            auto stage = usd->Stage();
            _self->rootPrim = stage->GetPseudoRoot();
        }
        if (_self->rootPrim) {
            must_create_slices = true;
            slices.clear();
            _self->stats.clear();
            _self->usd_traverse(_self->rootPrim, traverse);
            _self->usd_statistics(traverse, _self->stats);
            _self->prepare_render_data(_self->rootPrim, _self->stats);
        }
    }

    if (_self->rootPrim) {
        // Get the window position in screen coordinates
        ImVec2 windowPos = ImGui::GetWindowPos();

        // Get the window padding
        ImVec2 windowPadding = ImGui::GetStyle().WindowPadding;
        
        // Get the available content region within the window, considering window padding
        ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

        float cx = contentRegionAvailable.x/2;
        float cy = contentRegionAvailable.y/2 + 20;
        
        create_slices = must_create_slices;
        
        UsdPrim selectedPrim;
        if (selectedPrims.size() > 0)
            selectedPrim = selectedPrims[0];
        
        RenderRadialChart(0, _self->t, _self->rootPrim, selectedPrim, _self->stats,
                          windowPos.x + windowPadding.x + cx,
                          windowPos.y + windowPadding.y + cy,
                          0, 2*M_PI, 0,10);
        create_slices = false;
        
        _self->t += 1.f/30.f;
        if (_self->t >= 6.283f) {
            _self->t -= 6.283f;
        }

        auto usd = OpenUSDProvider::instance();
        auto stage = usd->Stage();
        float lh = ImGui::GetTextLineHeight() / 2;
        int id = 19999;
        for (auto& i : slices) {
            ImGui::SetCursorPos((ImVec2) { cx + i.x - lh, cy + i.y - lh });
            ImGui::PushID(id++);
            if (ImGui::InvisibleButton(" *", (ImVec2){lh*2,lh*2})) {
                std::vector<UsdPrim> prims;
                prims.push_back(i.prim);
                selection->SetSelectionPrims(&(*stage), prims);
            }
            ImGui::PopID();
            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::SetTooltip("%s", i.prim.GetPath().GetString().c_str());
                ImGui::EndTooltip();
            }
        }
    }
    ImGui::End();
}


} //lab
