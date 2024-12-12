//
//  OutlinerActivity.cpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 12/16/23.
//  Copyright © 2023 Nick Porcino. All rights reserved.
//
// The outliner is originally by Raphael Jouretz at
// https://github.com/raph080/ImGuiHydraEditor, under the MIT license


#include "UsdOutlinerActivity.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "Lab/ImguiExt.hpp"
#include "Providers/Assets/AssetsProvider.hpp"
#include "Providers/Selection/SelectionProvider.hpp"
#include "Providers/OpenUSD/OpenUSDProvider.hpp"
#include "Providers/OpenUSD/UsdUtils.hpp"
#include "Providers/Sprite/SpriteProvider.hpp"
#include <vector>

// on apple the LabCreateTextues come from the MetalProvider
#if defined(__APPLE__)
#include "Providers/Metal/MetalProvider.h"
#endif


/// @todo
// when selection changes, update the outliner by closing everything and
// opening tree nodes that are in the selection path
// https://github.com/ocornut/imgui/issues/1131

namespace lab {

PXR_NAMESPACE_USING_DIRECTIVE

struct UsdOutlinerActivity::data {
    data() {
        memset(&ghost, 0, sizeof(ghost));
    }
    SpriteProvider::Frame ghost[2];
    void* ghost_textures[2];
};

UsdOutlinerActivity::UsdOutlinerActivity()
: Activity(UsdOutlinerActivity::sname()) {
    _self = new UsdOutlinerActivity::data();
    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<UsdOutlinerActivity*>(instance)->RunUI(*vi);
    };
}

UsdOutlinerActivity::~UsdOutlinerActivity() {
    delete _self;
}

ImGuiTreeNodeFlags UsdOutlinerActivity::ComputeDisplayFlags(pxr::UsdPrim prim)
{
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
    
    // set the flag if leaf or not
    pxr::UsdPrimSiblingRange children = prim.GetChildren();
    if (GetSize(children) == 0) {
        flags |= ImGuiTreeNodeFlags_Leaf;
        flags |= ImGuiTreeNodeFlags_Bullet;
    }
    else flags = ImGuiTreeNodeFlags_OpenOnArrow;
    
    // if selected prim, set highlight flag
    auto selection = SelectionProvider::instance();
    if (selection) {
        auto sel = selection->GetSelectionPrims();
        if (std::find(sel.begin(), sel.end(), prim) != sel.end()) {
            flags |= ImGuiTreeNodeFlags_Selected;
        }
    }
    return flags;
}

bool UsdOutlinerActivity::IsParentOfModelSelection(pxr::UsdPrim prim) const
{
    // check if prim is parent of selection
    auto selection = SelectionProvider::instance();
    if (selection) {
        for (auto&& p : selection->GetSelectionPrims())
            if (IsParentOf(prim, p))
                return true;
    }
    return false;
}

bool UsdOutlinerActivity::IsSelected(pxr::UsdPrim prim) const {
    // check if prim is parent of selection
    auto selection = SelectionProvider::instance();
    if (selection) {
        for (auto&& p : selection->GetSelectionPrims())
            if (prim == p)
                return true;
    }
    return false;
}

void UsdOutlinerActivity::_activate() {
    if (!_self->ghost[0].h) {
        auto sprite = SpriteProvider::instance();
        auto assets = AssetsProvider::instance();
        std::string path = assets->resolve("$(LAB_BUNDLE)/ghost.aseprite");
        sprite->cache_sprite(path.c_str(), "ghost");
        _self->ghost[0] = sprite->frame("ghost", 0);
        _self->ghost[1] = sprite->frame("ghost", 1);
        int i = LabCreateRGBA8Texture(_self->ghost[0].w, _self->ghost[0].h, _self->ghost[0].pixels);
        _self->ghost_textures[0] = LabTextureHardwareHandle(i);
        i = LabCreateRGBA8Texture(_self->ghost[1].w, _self->ghost[1].h, _self->ghost[1].pixels);
        _self->ghost_textures[1] = LabTextureHardwareHandle(i);
    }
}

bool UsdOutlinerActivity::DrawHierarchyNode(pxr::UsdPrim prim, ImRect& curItemRect)
{
    bool recurse = false;
    const char* primName = prim.GetName().GetText();
    ImGuiTreeNodeFlags flags = ComputeDisplayFlags(prim);
    
    // print node in blue if parent of selection
    if (IsParentOfModelSelection(prim)) {
        ImU32 color = ImGui::GetColorU32(ImGuiCol_HeaderActive, 1.f);
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        recurse = ImGui::TreeNodeEx(primName, flags);
        ImGui::PopStyleColor();
        curItemRect = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
        
#if 0
        // adornment to the right of a "folder"
        ImGui::SameLine();
        ImGui::TextUnformatted("\\O/");
#endif
    }
    else if (IsSelected(prim)) {
        ImU32 color = IM_COL32(255, 255, 0, 255);
        ImGui::PushStyleColor(ImGuiCol_Text, color);
        recurse = ImGui::TreeNodeEx(primName, flags);
        ImGui::PopStyleColor();
        curItemRect = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
        ImGui::SameLine();
        ImVec4 bg_color(0, 0, 0, 0);
        
        bool isVisibilityInherited = true;
        bool visVaries = false;
        bool isInvisible = false;
        auto img = pxr::UsdGeomImageable(prim);
        if (img) {
            UsdAttributeQuery query(img.GetVisibilityAttr());
            TfToken visibility = UsdGeomTokens->inherited;
            query.Get(&visibility, UsdTimeCode::Default());
            isVisibilityInherited = visibility == UsdGeomTokens->inherited;
            visVaries = query.ValueMightBeTimeVarying();
            isInvisible = visibility == UsdGeomTokens->invisible;
        }
        
        int ghostIdx = isInvisible? 1 : 0;
        if (ImGui::ImageButton("###outliner_ghost", (ImTextureID) _self->ghost_textures[ghostIdx],
                    ImVec2(_self->ghost[ghostIdx].w, _self->ghost[ghostIdx].h),
                               ImVec2(0, 0), ImVec2(1, 1), bg_color)) {
            // hide/show
            if (isInvisible)
                img.MakeVisible();
            else
                img.MakeInvisible();
        }
    }
    else {
        recurse = ImGui::TreeNodeEx(primName, flags);
        curItemRect = ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    }
    return recurse;
}


void UsdOutlinerActivity::DrawChildrendHierarchyDecoration(ImRect parentRect,
                                                std::vector<ImRect> childrenRects)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImColor lineColor = ImGui::GetColorU32(ImGuiCol_Text, 0.25f);
    
    // all decoration for children will start at horizStartPos
    float horizStartPos = parentRect.Min.x + 10.0f;  // hard coded
    
    // save the lineStart of the last children to draw the vertical
    // line from the parent to the last child
    ImVec2 lineStart;
    
    // loop over all children and draw every horizontal line
    for (auto rect : childrenRects) {
        const float midpoint = (rect.Min.y + rect.Max.y) / 2.0f;
        const float lineSize = 8.0f;  // hard coded
        
        lineStart = ImVec2(horizStartPos, midpoint);
        ImVec2 lineEnd = lineStart + ImVec2(lineSize, 0);
        
        drawList->AddLine(lineStart, lineEnd, lineColor);
    }
    
    // draw the vertical line from the parent to the last child
    ImVec2 lineEnd = ImVec2(horizStartPos, parentRect.Max.y + 2.0f);
    drawList->AddLine(lineStart, lineEnd, lineColor);
}

// returns the node's rectangle
ImRect UsdOutlinerActivity::DrawPrimHierarchy(pxr::UsdPrim prim)
{
    ImRect curItemRect;
    bool recurse = DrawHierarchyNode(prim, curItemRect);

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        auto selection = SelectionProvider::instance();
        UsdStageRefPtr stagePtr = OpenUSDProvider::instance()->Stage();
        UsdStage* stage = stagePtr.operator->();
        if (selection)
            selection->SetSelectionPrims(stage, {prim});
    }
    
    if (recurse) {
        // draw all children and store their rect position
        std::vector<ImRect> rects;
        for (auto child : prim.GetChildren()) {
            ImRect childRect = DrawPrimHierarchy(child);
            rects.push_back(childRect);
        }
        
        if (rects.size() > 0) {
            // draw hierarchy decoration for all children
            DrawChildrendHierarchyDecoration(curItemRect, rects);
        }
        
        ImGui::TreePop();
    }
    
    return curItemRect;
}

void UsdOutlinerActivity::RunUI(const LabViewInteraction&) {
    if (!IsActive())
        return;

    ImGui::SetNextWindowSize(ImVec2(200, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin("Outliner");
    pxr::UsdStageRefPtr stage = OpenUSDProvider::instance()->Stage();
    if (stage) {
        for (auto prim : stage->GetPseudoRoot().GetChildren()) {
            DrawPrimHierarchy(prim);
        }
    }
    ImGui::End();
}


} // lab
