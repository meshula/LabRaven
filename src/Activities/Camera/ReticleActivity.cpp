//
//  ReticleActivity.cpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 11/29/23.
//  Copyright © 2023 Nick Porcino. All rights reserved.
//

#include "ReticleActivity.hpp"
#include "imgui.h"
#include "../Color/UIElements.hpp"

namespace lab {

struct ReticleActivity::data {
    bool letterbox = false;
    bool thirds = false;
    bool split = false;
    bool crosshair = false;
    bool action = false;
    bool custom = false;
    bool safe = false;
    ImVec4 actionColor = ImVec4(1, 0, 0, 1);
    ImVec4 customColor = ImVec4(1, 1, 0, 1);
    ImVec4 safeColor = ImVec4(0, 1, 0, 1);
    ImVec4 letterboxColor = ImVec4(0, 0, 0, 0.6f);
    float desired_aspect = 2.35f;
    float actionValue = 0.91f;
    float safeValue = 0.86f;
    float customValue = 0.79f;
};

ReticleActivity::ReticleActivity()
: Activity(ReticleActivity::sname()) {
    _self = new ReticleActivity::data;
        
    //activity.Render = [](void* instance, const LabViewInteraction* vi) {
    //    static_cast<ReticleActivity*>(instance)->Render(*vi);
    //};
    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<ReticleActivity*>(instance)->RunUI(*vi);
    };
    activity.ToolBar = [](void* instance) {
        static_cast<ReticleActivity*>(instance)->ToolBar();
    };
}

ReticleActivity::~ReticleActivity() {
    delete _self;
}

void ReticleActivity::Render(ImDrawList* draw_list, const LabViewInteraction& vi)
{
    const ImU32 RED = ImGui::ColorConvertFloat4ToU32(ImVec4(1, 0, 0, 1));
    const ImU32 BLACK = ImGui::ColorConvertFloat4ToU32(ImVec4(0, 0, 0, 1));

    ImU32 bar_color = ImGui::ColorConvertFloat4ToU32(_self->letterboxColor);

    auto& d = vi.view;

    bool draw_vertical_bars = false;
    bool draw_horizontal_bars = false;
    float aspect = d.w / d.h;
    if (_self->letterbox) {
        if (aspect > _self->desired_aspect) {
            draw_vertical_bars = true;
        }
        else {
            draw_horizontal_bars = true;
        }
    }

    float h = d.wh;
    float w = d.ww;

    if (draw_horizontal_bars) {
        h = d.wh / _self->desired_aspect;
        w = d.ww;
        const float bar_h = (d.wh - h) * 0.5f;
        draw_list->AddRectFilled(
            ImVec2(d.wx, d.wy),
            ImVec2(d.wx + d.ww, d.wy + bar_h),
            bar_color);
        draw_list->AddRectFilled(
            ImVec2(d.wx, d.wy + d.wh - bar_h),
            ImVec2(d.wx + d.ww, d.wy + d.wh),
            bar_color);
    }
    if (draw_vertical_bars) {
        h = d.wh;
        w = d.wh * _self->desired_aspect;
        const float bar_w = (d.ww - w) * 0.5f;
        draw_list->AddRectFilled(
            ImVec2(d.wx, d.wy),
            ImVec2(d.wx + bar_w, d.wy + d.wh),
            bar_color);
        draw_list->AddRectFilled(
            ImVec2(d.wx + d.ww - bar_w, d.wy),
            ImVec2(d.wx + d.ww, d.wy + d.wh),
            bar_color);
    }
    const float w2 = w * 0.5f;
    const float h2 = h * 0.5f;
    
    const float action = _self->actionValue; // red
    const float title = _self->safeValue;  // yellow
    const float custom = _self->customValue; // green
    float midx = d.wx + 0.5f * d.ww;
    float midy = d.wy + 0.5f * d.wh;

    if (_self->action) {
        ImU32 color = ImGui::ColorConvertFloat4ToU32(_self->actionColor);
        float rw = w*action;
        float rh = h*action;
        float x = midx - 0.5*rw;
        float y = midy - 0.5*rh;
        draw_list->AddRect(
            ImVec2(x, y),
            ImVec2(x + rw, y + rh),
            color);
    }
    if (_self->safe) {
        ImU32 color = ImGui::ColorConvertFloat4ToU32(_self->safeColor);
        float rw = w*title;
        float rh = h*title;
        float x = midx - 0.5*rw;
        float y = midy - 0.5*rh;
        draw_list->AddRect(
            ImVec2(x, y),
            ImVec2(x + rw, y + rh),
            color);
    }
    if (_self->custom) {
        ImU32 color = ImGui::ColorConvertFloat4ToU32(_self->customColor);
        float rw = w*custom;
        float rh = h*custom;
        float x = midx - 0.5*rw;
        float y = midy - 0.5*rh;
        draw_list->AddRect(
            ImVec2(x, y),
            ImVec2(x + rw, y + rh),
            color);
    }

    // thirds
    if (_self->thirds) {
        const float w3 = w / 3.f;
        const float h3 = h / 3.f;
        draw_list->AddLine(
            ImVec2(midx - w3*0.5f, midy - h*0.5f),
            ImVec2(midx - w3*0.5f, midy + h*0.5f),
            RED);
        draw_list->AddLine(
            ImVec2(midx + w3*0.5f, midy - h*0.5f),
            ImVec2(midx + w3*0.5f, midy + h*0.5f),
            RED);
        draw_list->AddLine(
            ImVec2(midx - w*0.5f, midy - h3*0.5f),
            ImVec2(midx + w*0.5f, midy - h3*0.5f),
            RED);
        draw_list->AddLine(
            ImVec2(midx - w*0.5f, midy + h3*0.5f),
            ImVec2(midx + w*0.5f, midy + h3*0.5f),
            RED);
    }

    // quadrants
    if (_self->split) {
        draw_list->AddLine(
            ImVec2(midx, midy - h2),
            ImVec2(midx, midy + h2),
            BLACK);
        draw_list->AddLine(
            ImVec2(midx - w2, midy),
            ImVec2(midx + w2, midy),
            BLACK);
    }

    // crosshair
    if (_self->crosshair) {
        const float ch = 0.5f * (h2 > w2 ? w2 : h2);
        draw_list->AddLine(
            ImVec2(midx, midy - ch),
            ImVec2(midx, midy + ch),
            BLACK);
        draw_list->AddLine(
            ImVec2(midx - ch, midy),
            ImVec2(midx + ch, midy),
            BLACK);
    }
    
   // draw_list->AddText(ImVec2(d.wx + 100, d.wy + 180), BLACK, "Hydra test");
}

void ReticleActivity::RunUI(const LabViewInteraction&)
{
    ImGuiStyle& style = ImGui::GetStyle();
    float bsy = style.ItemSpacing.y;  // Store the current vertical spacing

    ImGui::Begin("Reticle");

    ImVec2 pos = ImGui::GetCursorPos();
    
    ImGui::Checkbox("Thirds", &_self->thirds);
    ImVec2 bsize = ImGui::GetItemRectSize();
    ImGui::Checkbox("Split", &_self->split);
    ImGui::Checkbox("Crosshair", &_self->crosshair);
    
    ImGui::Separator();
    
    ColorChipWithPicker(_self->letterboxColor, 'l');
    ImGui::SameLine();
    ImGui::Checkbox("LetterBox", &_self->letterbox);
    ImGui::SameLine();
    ImGui::InputFloat("###aspect", &_self->desired_aspect);

    ImVec2 chipPos = pos;
    chipPos.x += bsize.x + 25;
    ImGui::SetCursorPos(chipPos);
    ColorChipWithPicker(_self->actionColor, 'a');

    ImVec2 newPos = pos;
    newPos.x += bsize.x + 50;
    ImGui::SetCursorPos(newPos);
    ImGui::Checkbox("Action", &_self->action);
    ImGui::SameLine();
    ImGui::InputFloat("###Action", &_self->actionValue);

    newPos.y += bsy + bsize.y;
    chipPos.y = newPos.y;
    ImGui::SetCursorPos(chipPos);
    ColorChipWithPicker(_self->customColor, 'c');

    ImGui::SetCursorPos(newPos);
    ImGui::Checkbox("Custom", &_self->custom);
    ImGui::SameLine();
    ImGui::InputFloat("###Custom", &_self->customValue);

    newPos.y += bsy + bsize.y;
    chipPos.y = newPos.y;
    ImGui::SetCursorPos(chipPos);
    ColorChipWithPicker(_self->safeColor, 's');

    ImGui::SetCursorPos(newPos);
    ImGui::Checkbox("Safe", &_self->safe);
    ImGui::SameLine();
    ImGui::InputFloat("###Safe", &_self->safeValue);

    ImGui::End();
}

void ReticleActivity::ToolBar() {
    if (ImGui::Button("Letterbox", ImVec2(0, 37))) {
        _self->letterbox = !_self->letterbox;
    }
}

} //lab
