#include "PropertiesActivity.hpp"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui.h"

#include "ImGuiHydraEditor/src/views/editor.h"
#include "Providers/OpenUSD/OpenUSDProvider.hpp"

namespace lab {

struct PropertiesActivity::data {
    data() = default;
    ~data() = default;
    std::unique_ptr<pxr::Editor> editor;
};

PropertiesActivity::PropertiesActivity() : Activity(PropertiesActivity::sname()) {
    _self = new PropertiesActivity::data();

    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<PropertiesActivity*>(instance)->RunUI(*vi);
    };
    activity.Menu = [](void* instance) {
        static_cast<PropertiesActivity*>(instance)->Menu();
    };
}

PropertiesActivity::~PropertiesActivity() {
    delete _self;
}

void PropertiesActivity::RunUI(const LabViewInteraction&) {
    if (!_self->editor) {
        auto usd = OpenUSDProvider::instance();
        auto model = usd->Model();
        if (model) {
            ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
            _self->editor = std::unique_ptr<pxr::Editor>(
                                     new pxr::Editor(model, "Properties Editor##A1"));
        }
    }

    // make a window 800, 600
    if (_self->editor) {
        _self->editor->Update();
    }
    else {
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Properties Editor##A2")) {
            ImGui::Text("No USD stage loaded.");
        }
        ImGui::End();

    }
}

void PropertiesActivity::Menu() {
}

} // lab
