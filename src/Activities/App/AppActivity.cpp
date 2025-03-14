
#include "AppActivity.hpp"
#include "Lab/App.h"
#include "imgui.h"

namespace lab {

struct AppActivity::data {
};

AppActivity::AppActivity() 
: Activity(AppActivity::sname())
, _self(new data) {
    activity.Menu = [](void* instance) {
        if (ImGui::BeginMenu("Window")) {
            if (ImGui::MenuItem("Reset Window Layout")) {
                LabApp::instance()->ResetWindowPositions();
            }
            ImGui::EndMenu();
        }
    };
}

AppActivity::~AppActivity() {
    delete _self;
}

} // namespace lab
