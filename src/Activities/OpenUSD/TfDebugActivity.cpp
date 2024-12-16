//
//  DebuggerActivity.cpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 24/02/09
//  Copyright © 2024 Nick Porcino. All rights reserved.
//

#include "TfDebugActivity.hpp"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"

namespace lab {
    struct DebuggerActivity::data {
        bool ui_visible = true;
    };

    DebuggerActivity::DebuggerActivity() : Activity(DebuggerActivity::sname())
    {
        _self = new DebuggerActivity::data();
        activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
            static_cast<DebuggerActivity*>(instance)->RunUI(*vi);
        };
        activity.Menu = [](void* instance) {
            static_cast<DebuggerActivity*>(instance)->Menu();
        };
    }

    DebuggerActivity::~DebuggerActivity()
    {
        delete _self;
    }

    void DebuggerActivity::RunUI(const LabViewInteraction&)
    {
        if (!_self->ui_visible)
            return;
    
        // draw the debugger
        static std::vector<std::string> debugStrings = PXR_NS::TfDebug::GetDebugSymbolNames();
        ImGui::Begin("TfDebugger", &_self->ui_visible);
        for (auto& s : debugStrings) {
            bool enabled = PXR_NS::TfDebug::IsDebugSymbolNameEnabled(s);
            if (ImGui::Checkbox(s.c_str(), &enabled)) {
                PXR_NS::TfDebug::SetDebugSymbolsByName(s, enabled);
            }
            if (ImGui::BeginItemTooltip())
            {
                static std::string desc;
                desc = PXR_NS::TfDebug::GetDebugSymbolDescription(s);
                ImGui::TextUnformatted(desc.c_str());
                ImGui::EndTooltip();
            }
        }
        ImGui::End();
    }

    void DebuggerActivity::Menu()
    {
        if (ImGui::BeginMenu("Modes")) {
            if (ImGui::MenuItem("Debugger", nullptr, _self->ui_visible, true)) {
                _self->ui_visible = !_self->ui_visible;
            }
            ImGui::EndMenu();
        }
    }

} // lab
