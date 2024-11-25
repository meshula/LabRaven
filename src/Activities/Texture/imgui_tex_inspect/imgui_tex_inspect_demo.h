// ImGuiTexInspect, a texture inspector widget for dear imgui

#pragma once
#include "imgui.h"
#include <string>

namespace ImGuiTexInspect
{
struct Texture
{
    void *texture; // a bindable texture handle
    ImVec2 size;
};

Texture LoadTexture(const char *path);

void FocusTexture(Texture);
void ShowDemoWindow(const std::string& renderingCS);
} //namespace ImGuiTexInspect
