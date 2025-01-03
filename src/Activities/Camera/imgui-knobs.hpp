#pragma once

/*
 MIT License

 Copyright (c) 2022 Simon Altschuler

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
 */
#include <cstdlib>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"

typedef int ImGuiKnobFlags;

enum ImGuiKnobFlags_ {
    ImGuiKnobFlags_NoTitle = 1 << 0,
    ImGuiKnobFlags_NoInput = 1 << 1,
    ImGuiKnobFlags_ValueTooltip = 1 << 2,
    ImGuiKnobFlags_DragHorizontal = 1 << 3,
    ImGuiKnobFlags_RotateRelative = 1 << 4,
    ImGuiKnobFlags_RotateAbsolute = 1 << 5,
    ImGuiKnobFlags_WrapAround = 1 << 6,
};

typedef int ImGuiKnobVariant;

enum ImGuiKnobVariant_ {
    ImGuiKnobVariant_Tick = 1 << 0,
    ImGuiKnobVariant_Dot = 1 << 1,
    ImGuiKnobVariant_Wiper = 1 << 2,
    ImGuiKnobVariant_WiperOnly = 1 << 3,
    ImGuiKnobVariant_WiperDot = 1 << 4,
    ImGuiKnobVariant_Stepped = 1 << 5,
    ImGuiKnobVariant_Space = 1 << 6,
};

namespace ImGuiKnobs {

    struct color_set {
        ImColor base;
        ImColor hovered;
        ImColor active;

        color_set(ImColor base, ImColor hovered, ImColor active) : base(base), hovered(hovered), active(active) {}

        color_set(ImColor color) {
            base = color;
            hovered = color;
            active = color;
        }
    };

    bool Knob(const char *label, float *p_value, float v_min, float v_max, float speed = 0, const char *format = NULL, ImGuiKnobVariant variant = ImGuiKnobVariant_Tick, float size = 0, ImGuiKnobFlags flags = 0, int steps = 10
              ,float aAngleMin=3.14159265358979323846f*0.75f,float aAngleMax=3.14159265358979323846f*2.25f);
    bool KnobInt(const char *label, int *p_value, int v_min, int v_max, float speed = 0, const char *format = NULL, ImGuiKnobVariant variant = ImGuiKnobVariant_Tick, float size = 0, ImGuiKnobFlags flags = 0, int steps = 10);
}// namespace ImGuiKnobs
