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
#include "HydraViewport.hpp"

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui.h"
#include "Lab/App.h"
#include "Lab/StudioCore.hpp"
#include "Lab/ImguiExt.hpp"
#include "Activities/OpenUSD/HydraActivity.hpp"
#include "Providers/Assets/AssetsProvider.hpp"
#include "Providers/Selection/SelectionProvider.hpp"
#include "Providers/OpenUSD/OpenUSDProvider.hpp"
#include "Providers/OpenUSD/UsdUtils.hpp"
#include "Providers/Sprite/SpriteProvider.hpp"
#include <pxr/usd/usd/stage.h>

#include "usdtweak/src/Selection.h"
#include "usdtweak/src/widgets/StageOutliner.h"

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

namespace lab {



/**
 * @brief Outliner view that acts as an outliner. it allows to preview and
 * navigate into the UsdStage hierarchy.
 *
 */
class Outliner : public View {
    public:
        inline static const string VIEW_TYPE = "Outliner";

        /**
         * @brief Construct a new Outliner object
         *
         * @param model the Model of the new Outliner view
         * @param label the ImGui label of the new Outliner view
         */
        Outliner(const string label = VIEW_TYPE);

        /**
         * @brief Override of the View::GetViewType
         *
         */
        const string GetViewType() override;

    private:
        /**
         * @brief Override of the View::Draw
         *
         */
        void _Draw(const LabViewInteraction& vi) override;

        /**
         * @brief Draw the hierarchy of all the descendant UsdPrims of the
         * given UsdPrim in the outliner
         *
         * @param primPath the SdfPath of the prim for which all the descendant
         * hierarchy will be drawn in the outliner
         * @return the ImRect rectangle of the tree node corresponding to the
         * given 'primPath'
         */
        ImRect _DrawPrimHierarchy(pxr::SdfPath primPath);

        /**
         * @brief Compute the display flags of the given UsdPrim
         *
         * @param primPath the SdfPath of the prim to compute the dislay flags
         * from
         * @return an ImGuiTreeNodeFlags object.
         * Default is ImGuiTreeNodeFlags_None.
         * If 'primPath' has no children, flag contains ImGuiTreeNodeFlags_Leaf
         * If 'primPath' has children, flags contains
         * ImGuiTreeNodeFlags_OpenOnArrow
         * if 'primPath' is part of selection, flags
         * contains ImGuiTreeNodeFlags_Selected
         *
         */
        ImGuiTreeNodeFlags _ComputeDisplayFlags(pxr::SdfPath primPath);

        /**
         * @brief Draw the hierarchy tree node of the given UsdPrim. The color
         * and the behavior of the node will bet set accordingly.
         *
         * @param primPath the SdfPath of the prim that will be drawn next on
         * the outliner
         * @return true if children 'primPath' must be drawn too
         * @return false otherwise
         */
        bool _DrawHierarchyNode(pxr::SdfPath primPath);

        /**
         * @brief Check if a given prim path is parent of another prim path
         *
         * @param primPath SdfPath to check if it is parent of
         * 'childPrimPath'
         * @param childPrimPath the SdfPath that acts as the child prim path
         * @return true if 'primPath' is parent of 'childPrimPath'
         * @return false otherwise
         */
        bool IsParentOf(pxr::SdfPath primPath, pxr::SdfPath childPrimPath);

        /**
         * @brief Check if the given UsdPrim is parent of a UsdPrim within the
         * current Model Selection.
         *
         * @param primPath SdfPath to check with
         * @return true if 'primPath' is parent of a prim within the current
         * Model selection
         * @return false otherwise
         */
        bool _IsParentOfModelSelection(pxr::SdfPath primPath);

        /**
         * @brief Check if the given UsdPrim is part of the current Model
         * selection
         *
         * @param prim SdfPath to check with
         * @return true if 'primPath' is part of the current Model selection
         * @return false otherwise
         */
        bool _IsInModelSelection(pxr::SdfPath primPath);

        /**
         * @brief Draw the children hierarchy decoration of the outliner view
         * (aka the vertical and horizontal lines that connect parent and child
         * nodes).
         *
         * @param parentRect the ImRect rectangle of the parent node
         * @param childrenRects a vector of ImRect rectangles of the direct
         * children node of 'parentRect'
         */
        void _DrawChildrendHierarchyDecoration(ImRect parentRect,
                                               vector<ImRect> childrenRects);
};



Outliner::Outliner(const string label) : View(label) {}

const string Outliner::GetViewType()
{
    return VIEW_TYPE;
};

void Outliner::_Draw(const LabViewInteraction& vi)
{
    SdfPath root = SdfPath::AbsoluteRootPath();
    lab::Orchestrator* mm = lab::Orchestrator::Canonical();
    std::weak_ptr<HydraActivity> hact;
    auto hydra = mm->LockActivity(hact);
    SdfPathVector paths = hydra->GetFinalSceneIndex()->GetChildPrimPaths(root);
    for (auto primPath : paths)
        _DrawPrimHierarchy(primPath);
}

// returns the node's rectangle
ImRect Outliner::_DrawPrimHierarchy(SdfPath primPath)
{
    bool recurse = _DrawHierarchyNode(primPath);

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        lab::Orchestrator* mm = lab::Orchestrator::Canonical();
        std::weak_ptr<HydraActivity> hact;
        auto hydra = mm->LockActivity(hact);
        hydra->SetHdSelection({primPath});
    }

    const ImRect curItemRect =
        ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());

    if (recurse) {
        // draw all children and store their rect position
        lab::Orchestrator* mm =lab:: Orchestrator::Canonical();
        std::weak_ptr<HydraActivity> hact;
        auto hydra = mm->LockActivity(hact);
        vector<ImRect> rects;
        SdfPathVector primPaths = hydra->GetFinalSceneIndex()->GetChildPrimPaths(primPath);

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
    lab::Orchestrator* mm =lab:: Orchestrator::Canonical();
    std::weak_ptr<HydraActivity> hact;
    auto hydra = mm->LockActivity(hact);

    // set the flag if leaf or not
    SdfPathVector primPaths = hydra->GetFinalSceneIndex()->GetChildPrimPaths(primPath);

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
    lab::Orchestrator* mm =lab:: Orchestrator::Canonical();
    std::weak_ptr<HydraActivity> hact;
    auto hydra = mm->LockActivity(hact);
    // check if primPath is parent of selection
    for (auto&& p : hydra->GetHdSelection())
        if (IsParentOf(primPath, p)) return true;

    return false;
}

bool Outliner::_IsInModelSelection(SdfPath primPath)
{
    lab::Orchestrator* mm =lab:: Orchestrator::Canonical();
    std::weak_ptr<HydraActivity> hact;
    auto hydra = mm->LockActivity(hact);
    SdfPathVector sel = hydra->GetHdSelection();
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


struct UsdOutlinerActivity::data {
    data() {
        memset(&gGhost, 0, sizeof(gGhost));
    }
    std::unique_ptr<Outliner> outliner;
};

void UsdOutlinerActivity::RunUI(const LabViewInteraction& vi) {
    if (!IsActive())
        return;

    pxr::UsdStageRefPtr stage = OpenUSDProvider::instance()->Stage();
    if (!stage) {
        ImGui::SetNextWindowSize(ImVec2(200, 400), ImGuiCond_FirstUseEver);
        ImGui::Begin("Hydra Outliner##A1");
        ImGui::TextUnformatted("Hydra Outliner: No stage opened yet.");
        ImGui::End();
    }
    else {
        if (!_self->outliner) {
            _self->outliner = std::make_unique<Outliner>("Hydra Outliner##A2");
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
        _self->outliner->Update(vi);

        ImGui::SetNextWindowSize(ImVec2(200, 400), ImGuiCond_FirstUseEver);
        ImGui::Begin("Stage Outliner");
        lab::Orchestrator* mm =lab:: Orchestrator::Canonical();
        std::weak_ptr<HydraActivity> hact;
        auto hydra = mm->LockActivity(hact);

        SelectionHash sh = 0;
        Selection selection;
        SdfPathVector sel = hydra->GetHdSelection();
        selection.Clear(stage);
        for (auto& s : sel) {
            selection.AddSelected(stage, s);
        }
        selection.UpdateSelectionHash(stage, sh);
        DrawStageOutliner(stage, selection);
        if (selection.UpdateSelectionHash(stage, sh)) {
            hydra->SetHdSelection(selection.GetSelectedPaths(stage));
        }

        ImGui::End();
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
