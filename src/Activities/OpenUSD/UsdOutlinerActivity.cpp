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
#include "ImGuiHydraEditor/src/views/outliner.h"
#include <pxr/usd/usd/stage.h>

#include <vector>

// on Apple the LabCreateTextues come from the MetalProvider
#if defined(__APPLE__)
#include "Providers/Metal/MetalProvider.h"
#endif


/// @todo
// when selection changes, update the outliner by closing everything and
// opening tree nodes that are in the selection path
// https://github.com/ocornut/imgui/issues/1131

namespace {
lab::SpriteProvider::Frame gGhost[2];
void* gGhostTextures[2];
}

PXR_NAMESPACE_OPEN_SCOPE

Outliner::Outliner(Model* model, const string label) : View(model, label) {}

const string Outliner::GetViewType()
{
    return VIEW_TYPE;
};

void Outliner::_Draw()
{
    SdfPath root = SdfPath::AbsoluteRootPath();
    SdfPathVector paths = _sceneIndex->GetChildPrimPaths(root);
    for (auto primPath : paths) _DrawPrimHierarchy(primPath);
}

// returns the node's rectangle
ImRect Outliner::_DrawPrimHierarchy(SdfPath primPath)
{
    bool recurse = _DrawHierarchyNode(primPath);

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
       GetModel()->SetSelection({primPath});
    }

    const ImRect curItemRect =
        ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());

    if (recurse) {
        // draw all children and store their rect position
        vector<ImRect> rects;
        SdfPathVector primPaths =
            _sceneIndex->GetChildPrimPaths(primPath);

        for (auto child : primPaths) {
            ImRect childRect = _DrawPrimHierarchy(child.GetPrimPath());
            rects.push_back(childRect);
        }

        if (rects.size() > 0) {
            // draw hierarchy decoration for all children
            _DrawChildrendHierarchyDecoration(curItemRect, rects);
        }

        ImGui::TreePop();
    }
    
    return curItemRect;
}

ImGuiTreeNodeFlags Outliner::_ComputeDisplayFlags(SdfPath primPath)
{
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;

    // set the flag if leaf or not
    SdfPathVector primPaths = _sceneIndex->GetChildPrimPaths(primPath);

    if (primPaths.size() == 0) {
        flags |= ImGuiTreeNodeFlags_Leaf;
        flags |= ImGuiTreeNodeFlags_Bullet;
    }
    else flags = ImGuiTreeNodeFlags_OpenOnArrow;

    // if selected prim, set highlight flag
    bool isSelected = _IsInModelSelection(primPath);
    if (isSelected) flags |= ImGuiTreeNodeFlags_Selected;

    return flags;
}

bool Outliner::_DrawHierarchyNode(SdfPath primPath)
{
    bool recurse = false;
    const char* primName = primPath.GetName().c_str();
    ImGuiTreeNodeFlags flags = _ComputeDisplayFlags(primPath);

    ImRect curItemRect; // this is the size we really need

    // print node in blue if parent of selection
    if (_IsParentOfModelSelection(primPath)) {
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
    else if (_IsInModelSelection(primPath)) {
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
        auto stage = lab::OpenUSDProvider::instance()->Stage();
        UsdPrim prim = stage->GetPrimAtPath(primPath);
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
        if (ImGui::ImageButton("###outliner_ghost", (ImTextureID) gGhostTextures[ghostIdx],
                    ImVec2(gGhost[ghostIdx].w, gGhost[ghostIdx].h),
                               ImVec2(0, 0), ImVec2(1, 1), bg_color)) {

            /// @TODO This should be a transaction

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

bool Outliner::IsParentOf(SdfPath primPath, SdfPath childPrimPath)
{
    return primPath.GetCommonPrefix(childPrimPath) == primPath;
}

bool Outliner::_IsParentOfModelSelection(SdfPath primPath)
{
    // check if primPath is parent of selection
    for (auto&& p : GetModel()->GetSelection())
        if (IsParentOf(primPath, p)) return true;

    return false;
}

bool Outliner::_IsInModelSelection(SdfPath primPath)
{
    SdfPathVector sel = GetModel()->GetSelection();
    // check if primPath in model selection
    return find(sel.begin(), sel.end(), primPath) != sel.end();
}

void Outliner::_DrawChildrendHierarchyDecoration(ImRect parentRect,
                                                 vector<ImRect> childrenRects)
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

PXR_NAMESPACE_CLOSE_SCOPE

namespace lab {

struct UsdOutlinerActivity::data {
    data() {
        memset(&gGhost, 0, sizeof(gGhost));
    }
    std::unique_ptr<pxr::Outliner> outliner;
};

void UsdOutlinerActivity::RunUI(const LabViewInteraction&) {
    if (!IsActive())
        return;

    pxr::UsdStageRefPtr stage = OpenUSDProvider::instance()->Stage();
    pxr::Model* model = OpenUSDProvider::instance()->Model();
    if (!stage || !model) {
        ImGui::SetNextWindowSize(ImVec2(200, 400), ImGuiCond_FirstUseEver);
        ImGui::Begin("Outliner##A1");
        ImGui::TextUnformatted("Outliner: No stage opened yet.");
        ImGui::End();
    }
    else {
        if (!_self->outliner) {
            _self->outliner = std::make_unique<pxr::Outliner>(model, "Outliner##A2");
        }

        ImGui::SetNextWindowSize(ImVec2(200, 400), ImGuiCond_FirstUseEver);
#if 0
        ImGui::Begin("Outliner");
        pxr::UsdStageRefPtr stage = OpenUSDProvider::instance()->Stage();
        if (stage) {
            for (auto prim : stage->GetPseudoRoot().GetChildren()) {
                DrawPrimHierarchy(prim);
            }
        }
        ImGui::End();
#endif
        _self->outliner->Update();
    }
}

void UsdOutlinerActivity::_activate() {
    if (!gGhost[0].h) {
        auto sprite = SpriteProvider::instance();
        auto assets = AssetsProvider::instance();
        std::string path = assets->resolve("$(LAB_BUNDLE)/ghost.aseprite");
        sprite->cache_sprite(path.c_str(), "ghost");
        gGhost[0] = sprite->frame("ghost", 0);
        gGhost[1] = sprite->frame("ghost", 1);
        int i = LabCreateRGBA8Texture(gGhost[0].w, gGhost[0].h, gGhost[0].pixels);
        gGhostTextures[0] = LabTextureHardwareHandle(i);
        i = LabCreateRGBA8Texture(gGhost[1].w, gGhost[1].h, gGhost[1].pixels);
        gGhostTextures[1] = LabTextureHardwareHandle(i);
    }
}

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

} // lab
