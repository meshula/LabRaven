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
};

HydraActivity::HydraActivity() : Activity(HydraActivity::sname()) {
    _self = new HydraActivity::data();

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

void HydraActivity::RunUI(const LabViewInteraction&) {
    if (!_self->viewport) {
        auto usd = OpenUSDProvider::instance();
        auto model = usd->Model();
        if (model) {
            ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
            _self->viewport = std::unique_ptr<HydraViewport>(
                                     new HydraViewport(model, "Hydra Viewport##A1"));
        }
    }

    // make a window 800, 600
    if (_self->viewport) {
        ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
        _self->viewport->Update();
    }
    else {
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Hydra Viewport##A2")) {
            ImGui::Text("No USD stage loaded.");
        }
        ImGui::End();

    }
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
