#include "HydraActivity.hpp"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"

namespace lab {

struct HydraActivity::data {
    data() = default;
    ~data() = default;
};

HydraActivity::HydraActivity() : Activity(HydraActivity::sname()) {
    _self = new HydraActivity::data();

    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<HydraActivity*>(instance)->RunUI(*vi);
    };
    activity.Menu = [](void* instance) {
        static_cast<HydraActivity*>(instance)->Menu();
    };
}

HydraActivity::~HydraActivity() {
    delete _self;
}

void HydraActivity::RunUI(const LabViewInteraction&) {
    // make a window 800, 600
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    if (ImGui::Begin("Hydra##Activity")) {
        ImGui::Text("Hydra");
    }
    ImGui::End();
}

void HydraActivity::Menu() {
}

} // lab
