#include "HydraActivity.hpp"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui.h"

#include "HydraViewport.hpp"
#include "Providers/OpenUSD/OpenUSDProvider.hpp"
#include "Providers/Camera/CameraProvider.hpp"

namespace lab {

PXR_NAMESPACE_USING_DIRECTIVE

struct HydraActivity::data {
    data() = default;
    ~data() = default;
    std::unique_ptr<HydraViewport> viewport;
    int stage_signature = 0;
};

HydraActivity::HydraActivity() : Activity(HydraActivity::sname()) {
    _self = new HydraActivity::data();

    _self->viewport = std::unique_ptr<HydraViewport>(
                             new HydraViewport("Hydra Viewport##A1"));

    SetTime(UsdTimeCode::Default());

    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<HydraActivity*>(instance)->RunUI(*vi);
    };
    activity.Menu = [](void* instance) {
        static_cast<HydraActivity*>(instance)->Menu();
    };

    activity.Update = [](void* instance, float dt) {
        auto cp = CameraProvider::instance();
        cp->Update(dt);
    };
}

HydraActivity::~HydraActivity() {
    delete _self;
}
PXR_NS::HdSceneIndexBaseRefPtr HydraActivity::GetFinalSceneIndex() {
    return _self->viewport->GetFinalSceneIndex();
}

SdfPathVector HydraActivity::GetHdSelection() {
    return _self->viewport->GetHdSelection();
}

void HydraActivity::SetHdSelection(const SdfPathVector& spv) {
    _self->viewport->SetHdSelection(spv);
}

void HydraActivity::RemoveSceneIndex(HdSceneIndexBaseRefPtr p) {
    _self->viewport->RemoveSceneIndex(p);
}

void HydraActivity::SetStage(UsdStageRefPtr stage) {
    _self->viewport->SetStage(stage);
}

int HydraActivity::GetHit(GfVec3f& hitPoint, GfVec3f& hitNormal) {
    _self->viewport->GetHit(hitPoint, hitNormal);
}

void HydraActivity::SetTime(UsdTimeCode tc) {
    _self->viewport->SetTime(tc);
}

HdSceneIndexBaseRefPtr HydraActivity::GetEditableSceneIndex() {
    _self->viewport->GetEditableSceneIndex();
}

void HydraActivity::SetEditableSceneIndex(PXR_NS::HdSceneIndexBaseRefPtr sceneIndex) {
    _self->viewport->SetEditableSceneIndex(sceneIndex);
}

void HydraActivity::RunUI(const LabViewInteraction& vi) {
    if (!_self->viewport) {
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Hydra Viewport##A2")) {
            ImGui::Text("No USD stage loaded.");
        }
        ImGui::End();
        return;
    }

    auto usd = OpenUSDProvider::instance();
    int current_gen = usd->StageGeneration();
    if (current_gen != _self->stage_signature) {
        SetStage(usd->Stage());
        _self->stage_signature = current_gen;
    }

    // make a window 800, 600
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    _self->viewport->Update(vi);
}

void HydraActivity::Menu() {
}

void HydraActivity::SetNearFar(float znear, float zfar) {
    _self->viewport->SetNearFar(znear, zfar);
}

GfVec2d HydraActivity::GetNearFar() const {
    return _self->viewport->GetNearFar();
}

float HydraActivity::GetAspect() const {
    return _self->viewport->GetAspect();
}

PXR_NS::GfCamera HydraActivity::GetGfCamera() const {
    return _self->viewport->GetGfCamera();
}

void HydraActivity::SetCameraFromGfCamera(const GfCamera& gfCam) {
    _self->viewport->SetCameraFromGfCamera(gfCam);
}


} // lab
