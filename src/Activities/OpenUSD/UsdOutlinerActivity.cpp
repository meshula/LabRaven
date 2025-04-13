//
//  OutlinerActivity.cpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 12/16/23.
//  Copyright © 2023 Nick Porcino. All rights reserved.
//

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
#include "Lab/CoreProviders/Metal/MetalProvider.hpp"
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



struct UsdOutlinerActivity::data {
    data() {
        memset(&gGhost, 0, sizeof(gGhost));
    }
};

void UsdOutlinerActivity::USDOutlinerUI(const LabViewInteraction& vi) {
    pxr::UsdStageRefPtr stage = OpenUSDProvider::instance()->Stage();
    if (!stage) {
        return;
    }
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
}

void UsdOutlinerActivity::HydraOutlinerUI(const LabViewInteraction& vi) {
}

void UsdOutlinerActivity::RunUI(const LabViewInteraction& vi) {
    if (!IsActive() || !UIVisible())
        return;

    ImGui::SetNextWindowSize(ImVec2(200, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin("Stage Outliner");

    USDOutlinerUI(vi);

    ImGui::End();
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
