#include "SessionActivity.hpp"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui.h"

#include "Activities/OpenUSD/HydraEditor.hpp"
#include "Providers/OpenUSD/OpenUSDProvider.hpp"
#include "UsdSessionLayer.hpp"

namespace lab {

struct SessionActivity::data {
    data() {
        sessionLayer = std::unique_ptr<UsdSessionLayer>(new UsdSessionLayer());
    }
    ~data() = default;

    std::unique_ptr<UsdSessionLayer> sessionLayer;
};

SessionActivity::SessionActivity() : Activity(SessionActivity::sname()) {
    _self = new SessionActivity::data();

    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<SessionActivity*>(instance)->RunUI(*vi);
    };
}

SessionActivity::~SessionActivity() {
    delete _self;
}

void SessionActivity::RunUI(const LabViewInteraction&) {
    auto usd = OpenUSDProvider::instance();
    auto stage = usd->Stage();
    if (!stage) {
        ImGui::SetNextWindowSize(ImVec2(200, 400), ImGuiCond_FirstUseEver);
        ImGui::Begin("Session Editor##A1");
        ImGui::TextUnformatted("Session Editor: No stage opened yet.");
        ImGui::End();
    }
    else {
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
        _self->sessionLayer->Update();
    }
}

} // lab
