//
//  TfDebugActivity.cpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 24/02/09
//  Copyright © 2024 Nick Porcino. All rights reserved.
//

#include "TfDebugActivity.hpp"
#include "imgui.h"

namespace lab {
struct TfDebugActivity::data {
};

TfDebugActivity::TfDebugActivity() : Activity(TfDebugActivity::sname())
{
    _self = new TfDebugActivity::data();
    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<TfDebugActivity*>(instance)->RunUI(*vi);
    };
}

TfDebugActivity::~TfDebugActivity()
{
    delete _self;
}

void TfDebugActivity::RunUI(const LabViewInteraction&)
{
    // draw the debugger
    static std::vector<std::string> debugStrings = PXR_NS::TfDebug::GetDebugSymbolNames();
    ImGui::Begin("TfDebugger");
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

} // lab
