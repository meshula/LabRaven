
#include "UIElements.hpp"
#include <mutex>
#include "nanocolor.h"

void ColorChipWithPicker(ImVec4& color, char id) {
    static ImVec4 saved_palette[32] = {};
    static std::once_flag init_palette;
    std::call_once(init_palette, []() {
        // Generate a default palette. The palette will persist and can be edited.
        for (int n = 0; n < IM_ARRAYSIZE(saved_palette); n++)
        {
            ImGui::ColorConvertHSVtoRGB(n / 31.0f, 0.8f, 0.8f,
                                        saved_palette[n].x, saved_palette[n].y, saved_palette[n].z);
            saved_palette[n].w = 1.0f; // Alpha
        }
    });
    
    const int misc_flags = ImGuiColorEditFlags_PickerHueWheel;
    static ImVec4 backup_color;
    static char button_id[] = "Col##x";
    button_id[5] = id;
    static char picker_id[] = "Pic##x";
    picker_id[5] = id;

    bool open_popup = ImGui::ColorButton(button_id, color, misc_flags);
    ImGui::SameLine(0, ImGui::GetStyle().ItemInnerSpacing.x);
    //open_popup |= ImGui::Button("Palette");
    if (open_popup)
    {
        ImGui::OpenPopup(picker_id);
        backup_color = color;
    }
    if (ImGui::BeginPopup(picker_id))
    {
        ImGui::ColorPicker4("##picker", (float*)&color, misc_flags | ImGuiColorEditFlags_NoSidePreview | ImGuiColorEditFlags_NoSmallPreview);
        ImGui::SameLine();
        
        ImGui::BeginGroup(); // Lock X position
        ImGui::Text("Current");
        ImGui::ColorButton("##current", color, ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(60, 40));
        ImGui::Text("Previous");
        if (ImGui::ColorButton("##previous", backup_color, ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_AlphaPreviewHalf, ImVec2(60, 40)))
            color = backup_color;
        ImGui::Separator();
        ImGui::Text("Palette");
        for (int n = 0; n < IM_ARRAYSIZE(saved_palette); n++)
        {
            ImGui::PushID(n);
            if ((n % 8) != 0)
                ImGui::SameLine(0.0f, ImGui::GetStyle().ItemSpacing.y);
            
            ImGuiColorEditFlags palette_button_flags = ImGuiColorEditFlags_NoAlpha | ImGuiColorEditFlags_NoPicker | ImGuiColorEditFlags_NoTooltip;
            if (ImGui::ColorButton("##palette", saved_palette[n], palette_button_flags, ImVec2(20, 20)))
                color = ImVec4(saved_palette[n].x, saved_palette[n].y, saved_palette[n].z, color.w); // Preserve alpha!
            
            // Allow user to drop colors into each palette entry. Note that ColorButton() is already a
            // drag source by default, unless specifying the ImGuiColorEditFlags_NoDragDrop flag.
            if (ImGui::BeginDragDropTarget())
            {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_3F))
                    memcpy((float*)&saved_palette[n], payload->Data, sizeof(float) * 3);
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(IMGUI_PAYLOAD_TYPE_COLOR_4F))
                    memcpy((float*)&saved_palette[n], payload->Data, sizeof(float) * 4);
                ImGui::EndDragDropTarget();
            }
            
            ImGui::PopID();
        }
        ImGui::EndGroup();
        ImGui::EndPopup();
    }
}

bool ColorSpacePicker(const char* label, int* cs, const char** csName) {
    static const char** colorspaces = NcRegisteredColorSpaceNames();
    const char* initialCS = *csName;
    bool changed = false;
    ImGui::BeginGroup();
    ImGui::PushItemWidth(110);
    if (ImGui::BeginCombo(label, colorspaces[*cs]))
    {
        ImGui::Selectable(*csName); // always put the current name at the top
        int n = 0;
        if (colorspaces[n] != nullptr) {
            do {
                bool is_selected = (*cs == n);
                if (ImGui::Selectable(colorspaces[n], is_selected)) {
                    *cs = n;
                    if (csName) {
                        *csName = colorspaces[n];
                        changed = true;
                    }
                }
                //if (is_selected)
                //    ImGui::SetItemDefaultFocus();
                ++n;
            } while (colorspaces[n] != nullptr);
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    ImGui::EndGroup();
    return changed && (0 != strcmp(*csName, initialCS));
}
