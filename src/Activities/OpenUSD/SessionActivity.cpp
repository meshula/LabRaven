#include "SessionActivity.hpp"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"

#include "ImGuiHydraEditor/src/views/editor.h"
#include "Providers/OpenUSD/OpenUSDProvider.hpp"

namespace lab {

struct SessionActivity::data {
    data() = default;
    ~data() = default;
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
    auto model = usd->Model();
    if (!model) {
        ImGui::SetNextWindowSize(ImVec2(200, 400), ImGuiCond_FirstUseEver);
        ImGui::Begin("Session Editor##A1");
        ImGui::TextUnformatted("Session Editor: No stage opened yet.");
        ImGui::End();
    }
    else {
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Session Editor##A3")) {
            usd->GetSessionLayerManager()->Update();
        }
        ImGui::End();
    }
}

} // lab
