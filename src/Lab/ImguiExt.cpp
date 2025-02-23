
#include "ImguiExt.hpp"

#include <mutex>
#include <string>
#include <vector>
#include <fstream>
#include <iostream>

#include "imgui.h"
#include "imgui_internal.h"
#include "IconsFontaudio.h"

using namespace ImGui;

namespace imgui_fonts
{
    unsigned int getCousineRegularCompressedSize();
    const unsigned int * getCousineRegularCompressedData();
}

void imgui_fixed_window_begin(const char * name, float min_x, float min_y, float max_x, float max_y)
{
    ImGui::SetNextWindowPos({min_x, min_y});
    ImGui::SetNextWindowSize({max_x - min_x, max_y - min_y});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowMinSize, ImVec2(0, 0));
    bool result = ImGui::Begin(name, NULL, ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);
    assert(result);
}

void imgui_fixed_window_end()
{
    ImGui::End();
    ImGui::PopStyleVar(2);
}

std::vector<uint8_t> read_file_binary(const std::string & pathToFile)
{
    std::ifstream file(pathToFile, std::ios::binary);
    std::vector<uint8_t> fileBufferBytes;

    if (file.is_open())
    {
        file.seekg(0, std::ios::end);
        size_t sizeBytes = file.tellg();
        file.seekg(0, std::ios::beg);
        fileBufferBytes.resize(sizeBytes);
        if (file.read((char*)fileBufferBytes.data(), sizeBytes)) return fileBufferBytes;
    }
    else throw std::runtime_error("could not open binary ifstream to path " + pathToFile);
    return fileBufferBytes;
}

ImFont * append_audio_icon_font(const std::vector<uint8_t> & buffer)
{
    static std::vector<uint8_t> s_audioIconFontBuffer;

    ImGuiIO & io = ImGui::GetIO();
    s_audioIconFontBuffer = buffer;
    static const ImWchar icons_ranges[] = { ICON_MIN_FAD, ICON_MAX_FAD, 0 };
    ImFontConfig icons_config; 
    icons_config.MergeMode = true; 
    icons_config.PixelSnapH = true;
    icons_config.FontDataOwnedByAtlas = false;
    icons_config.GlyphOffset = {0, 5};
    auto g_audio_icon = io.Fonts->AddFontFromMemoryTTF((void *)s_audioIconFontBuffer.data(), (int)s_audioIconFontBuffer.size(), 20.f, &icons_config, icons_ranges);
    IM_ASSERT(g_audio_icon != NULL);
    return g_audio_icon;
}

// This Icon drawing is adapted from thedmd's imgui_node_editor blueprint sample because the icons are nice

void DrawIcon(ImDrawList* drawList,
              const ImVec2& ul, const ImVec2& lr,
              IconType type, bool filled,
              ImU32 color, ImU32 innerColor)
{
    struct rectf {
        float x, y, w, h;
        float center_x() const { return x + w * 0.5f; }
        float center_y() const { return y + h * 0.5f; }
    };

    rectf rect = { ul.x, ul.y, lr.x - ul.x, lr.y - ul.y };
    const float outline_scale = rect.w / 24.0f;
    const int extra_segments = static_cast<int>(2 * outline_scale); // for full circle

    if (type == IconType::Flow)
    {
        const auto origin_scale = rect.w / 24.0f;

        const auto offset_x = 1.0f * origin_scale;
        const auto offset_y = 0.0f * origin_scale;
        const auto margin = (filled ? 2.0f : 2.0f) * origin_scale;
        const auto rounding = 0.1f * origin_scale;
        const auto tip_round = 0.7f; // percentage of triangle edge (for tip)
                                     //const auto edge_round = 0.7f; // percentage of triangle edge (for corner)
        const rectf canvas{
            rect.x + margin + offset_x,
            rect.y + margin + offset_y,
            rect.w - margin * 2.0f,
            rect.h - margin * 2.0f };

        const auto left = canvas.x + canvas.w * 0.5f * 0.3f;
        const auto right = canvas.x + canvas.w - canvas.w * 0.5f * 0.3f;
        const auto top = canvas.y + canvas.h * 0.5f * 0.2f;
        const auto bottom = canvas.y + canvas.h - canvas.h * 0.5f * 0.2f;
        const auto center_y = (top + bottom) * 0.5f;
        //const auto angle = AX_PI * 0.5f * 0.5f * 0.5f;

        const auto tip_top = ImVec2(canvas.x + canvas.w * 0.5f, top);
        const auto tip_right = ImVec2(right, center_y);
        const auto tip_bottom = ImVec2(canvas.x + canvas.w * 0.5f, bottom);

        drawList->PathLineTo(ImVec2(left, top) + ImVec2(0, rounding));
        drawList->PathBezierCubicCurveTo(
            ImVec2(left, top),
            ImVec2(left, top),
            ImVec2(left, top) + ImVec2(rounding, 0));
        drawList->PathLineTo(tip_top);
        drawList->PathLineTo(tip_top + (tip_right - tip_top) * tip_round);
        drawList->PathBezierCubicCurveTo(
            tip_right,
            tip_right,
            tip_bottom + (tip_right - tip_bottom) * tip_round);
        drawList->PathLineTo(tip_bottom);
        drawList->PathLineTo(ImVec2(left, bottom) + ImVec2(rounding, 0));
        drawList->PathBezierCubicCurveTo(
            ImVec2(left, bottom),
            ImVec2(left, bottom),
            ImVec2(left, bottom) - ImVec2(0, rounding));

        if (!filled)
        {
            if (innerColor & 0xFF000000)
                drawList->AddConvexPolyFilled(drawList->_Path.Data, drawList->_Path.Size, innerColor);

            drawList->PathStroke(color, true, 2.0f * outline_scale);
        }
        else
            drawList->PathFillConvex(color);
    }
    else
    {
        auto triangleStart = rect.center_x() + 0.32f * rect.w;

        rect.x -= static_cast<int>(rect.w * 0.25f * 0.25f);

        if (type == IconType::Circle)
        {
            const ImVec2 c{ rect.center_x(), rect.center_y() };

            if (!filled)
            {
                const auto r = 0.5f * rect.w / 2.0f - 0.5f;

                if (innerColor & 0xFF000000)
                    drawList->AddCircleFilled(c, r, innerColor, 12 + extra_segments);
                drawList->AddCircle(c, r, color, 12 + extra_segments, 2.0f * outline_scale);
            }
            else
                drawList->AddCircleFilled(c, 0.5f * rect.w / 2.0f, color, 12 + extra_segments);
        }
        else if (type == IconType::Square)
        {
            if (filled)
            {
                const auto r = 0.5f * rect.w / 2.0f;
                const auto p0 = ImVec2{ rect.center_x(), rect.center_y() } -ImVec2(r, r);
                const auto p1 = ImVec2{ rect.center_x(), rect.center_y() } +ImVec2(r, r);

                drawList->AddRectFilled(p0, p1, color, 0, 15 + extra_segments);
            }
            else
            {
                const auto r = 0.5f * rect.w / 2.0f - 0.5f;
                const auto p0 = ImVec2{ rect.center_x(), rect.center_y() } -ImVec2(r, r);
                const auto p1 = ImVec2{ rect.center_x(), rect.center_y() } +ImVec2(r, r);

                if (innerColor & 0xFF000000)
                    drawList->AddRectFilled(p0, p1, innerColor, 0, 15 + extra_segments);

                drawList->AddRect(p0, p1, color, 0, 15 + extra_segments, 2.0f * outline_scale);
            }
        }
        else if (type == IconType::Grid)
        {
            const auto r = 0.5f * rect.w / 2.0f;
            const auto w = ceilf(r / 3.0f);

            const auto baseTl = ImVec2(floorf(rect.center_x() - w * 2.5f), floorf(rect.center_y() - w * 2.5f));
            const auto baseBr = ImVec2(floorf(baseTl.x + w), floorf(baseTl.y + w));

            auto tl = baseTl;
            auto br = baseBr;
            for (int i = 0; i < 3; ++i)
            {
                tl.x = baseTl.x;
                br.x = baseBr.x;
                drawList->AddRectFilled(tl, br, color);
                tl.x += w * 2;
                br.x += w * 2;
                if (i != 1 || filled)
                    drawList->AddRectFilled(tl, br, color);
                tl.x += w * 2;
                br.x += w * 2;
                drawList->AddRectFilled(tl, br, color);

                tl.y += w * 2;
                br.y += w * 2;
            }

            triangleStart = br.x + w + 1.0f / 24.0f * rect.w;
        }
        else if (type == IconType::RoundSquare)
        {
            if (filled)
            {
                const auto r = 0.5f * rect.w / 2.0f;
                const auto cr = r * 0.5f;
                const auto p0 = ImVec2{ rect.center_x(), rect.center_y() } -ImVec2(r, r);
                const auto p1 = ImVec2{ rect.center_x(), rect.center_y() } +ImVec2(r, r);

                drawList->AddRectFilled(p0, p1, color, cr, 15);
            }
            else
            {
                const auto r = 0.5f * rect.w / 2.0f - 0.5f;
                const auto cr = r * 0.5f;
                const auto p0 = ImVec2{ rect.center_x(), rect.center_y() } -ImVec2(r, r);
                const auto p1 = ImVec2{ rect.center_x(), rect.center_y() } +ImVec2(r, r);

                if (innerColor & 0xFF000000)
                    drawList->AddRectFilled(p0, p1, innerColor, cr, 15);

                drawList->AddRect(p0, p1, color, cr, 15, 2.0f * outline_scale);
            }
        }
        else if (type == IconType::Diamond)
        {
            if (filled)
            {
                const auto r = 0.607f * rect.w / 2.0f;
                const auto c = ImVec2{ rect.center_x(), rect.center_y() };

                drawList->PathLineTo(c + ImVec2(0, -r));
                drawList->PathLineTo(c + ImVec2(r, 0));
                drawList->PathLineTo(c + ImVec2(0, r));
                drawList->PathLineTo(c + ImVec2(-r, 0));
                drawList->PathFillConvex(color);
            }
            else
            {
                const auto r = 0.607f * rect.w / 2.0f - 0.5f;
                const auto c = ImVec2{ rect.center_x(), rect.center_y() };

                drawList->PathLineTo(c + ImVec2(0, -r));
                drawList->PathLineTo(c + ImVec2(r, 0));
                drawList->PathLineTo(c + ImVec2(0, r));
                drawList->PathLineTo(c + ImVec2(-r, 0));

                if (innerColor & 0xFF000000)
                    drawList->AddConvexPolyFilled(drawList->_Path.Data, drawList->_Path.Size, innerColor);

                drawList->PathStroke(color, true, 2.0f * outline_scale);
            }
        }
        else if (type == IconType::Plus) {
            const float r = rect.w / 3.f;
            const float half_r = r * 0.5f;
            const auto c = ImVec2{ rect.center_x(), rect.center_y() };
            if (filled) {
                // p is the center of each of the rectangles, offset from c by r,
                // and each rectangle spans from p - half_r to p + half_r
                // three of the rectangles are horizontal, and two are vertical
                const ImVec2 p1 = c + ImVec2(-r, 0);
                const ImVec2 p2 = c + ImVec2(r, 0);
                const ImVec2 p3 = c + ImVec2(0, -r);
                const ImVec2 p4 = c + ImVec2(0, r);
                drawList->AddRectFilled(p1 - ImVec2(half_r, half_r), p1 + ImVec2(half_r, half_r), color);
                drawList->AddRectFilled(p2 - ImVec2(half_r, half_r), p2 + ImVec2(half_r, half_r), color);
                drawList->AddRectFilled(p3 - ImVec2(half_r, half_r), p3 + ImVec2(half_r, half_r), color);
                drawList->AddRectFilled(p4 - ImVec2(half_r, half_r), p4 + ImVec2(half_r, half_r), color);
                drawList->AddRectFilled(c - ImVec2(half_r, half_r), c + ImVec2(half_r, half_r), color);
            }
            else {
                // draw an outline around the plus shape similar to the boxes,
                // each of the four outside boxes will be drawn as three lines, not including the center box.
                // there will therefore be 12 lines.
                // the first box will be the upper box, then the right, then the bottom, then the left.

                // draw the top box, from bottom left to top left to top right to bottom right.
                ImVec2 p1 = c - ImVec2(half_r, half_r);
                ImVec2 p2 = p1 - ImVec2(0, r);
                ImVec2 p3 = p2 + ImVec2(r, 0);
                ImVec2 p4 = p3 + ImVec2(0, r);
                drawList->AddLine(p1, p2, color);
                drawList->AddLine(p2, p3, color);
                drawList->AddLine(p3, p4, color);

                // draw the right box, from top left to top right to bottom right to bottom left.
                p1 = p4;
                p2 = p1 + ImVec2(r, 0);
                p3 = p2 + ImVec2(0, r);
                p4 = p3 - ImVec2(r, 0);
                drawList->AddLine(p1, p2, color);
                drawList->AddLine(p2, p3, color);
                drawList->AddLine(p3, p4, color);

                // draw the bottom box, from top right to bottom right to bottom left to top left.
                p1 = p4;
                p2 = p1 + ImVec2(0, r);
                p3 = p2 - ImVec2(r, 0);
                p4 = p3 - ImVec2(0, r);
                drawList->AddLine(p1, p2, color);
                drawList->AddLine(p2, p3, color);
                drawList->AddLine(p3, p4, color);

                // draw the left box, from bottom right to bottom left to top left to top right.
                p1 = p4;
                p2 = p1 - ImVec2(r, 0);
                p3 = p2 - ImVec2(0, r);
                p4 = p3 + ImVec2(r, 0);
                drawList->AddLine(p1, p2, color);
                drawList->AddLine(p2, p3, color);
                drawList->AddLine(p3, p4, color);
            }
        }
        else
        {
            const auto triangleTip = triangleStart + rect.w * (0.45f - 0.32f);
            drawList->AddTriangleFilled(
                ImVec2(ceilf(triangleTip), rect.center_y() * 0.5f),
                ImVec2(triangleStart, rect.center_y() + 0.15f * rect.h),
                ImVec2(triangleStart, rect.center_y() - 0.15f * rect.h),
                color);
        }
    }
}

// dpad. If the mouse button is not pressed, p_x and p_y will be zero, and false will be returned.
// when the mouse button is pressed initially, init_x and init_y will be set to the position of the mouse.
// if the mouse is held down and moved, p_x and p_y will be set to the relative motion from init_x and init_y, scaled to the range -1 to 1.
// if the mouse is held down and moved less than deadzone from init_x and init_y, p_x and p_y will be zero.
// label is the label for the widget.
// p_x and p_y are the output values.
// init_x and init_y are the initial position of the mouse when the button is pressed.
// size is the width and height of the widget.
// deadzone is the amount of motion away from init_x and init_y that is required to change p_x and p_y.
// returns true if the dpad was used.
// it will be drawn using DrawIcon, using the Plus sign. If it is not used, it will be drawn in a light gray color.
// if it is used, it will be drawn in a darker gray color.
// is_active is a pointer to a bool that will be set to true if the dpad is being used. It used by the algorithm
// to determine, if the mouse is down, whether init_x and init_y should be set to the current mouse position.
bool imgui_dpad(const char* label, float* p_x, float* p_y,
    bool drawBg, float size,
    float deadzone)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    const ImGuiID id = window->GetID(label);
    const ImGuiStyle& style = ImGui::GetStyle();

    // Calculate layout
    ImVec2 pos = window->DC.CursorPos;
    ImVec2 button_sz(size, size);
    const ImVec2 label_size = ImGui::CalcTextSize(label, NULL, true);
    const float total_height = button_sz.y + style.ItemSpacing.y + label_size.y;

    // Create the invisible button to handle input
    bool is_active = ImGui::InvisibleButton(label, button_sz);
    is_active = ImGui::IsItemActive();
    bool hovered = ImGui::IsItemHovered();

    if (p_x && p_y) {
        // Input handling
        if (is_active)
        {
            ImVec2 delta = ImGui::GetIO().MousePos - pos;
            ImVec2 center(button_sz.x * 0.5f, button_sz.y * 0.5f);

            // Calculate normalized coordinates (-1 to 1)
            *p_x = (delta.x - center.x) / center.x;
            *p_y = (delta.y - center.y) / center.y;

            // Clamp values
            *p_x = ImClamp(*p_x, -1.0f, 1.0f);
            *p_y = ImClamp(*p_y, -1.0f, 1.0f);

            // Apply deadzone
            if (fabsf(*p_x) < deadzone) *p_x = 0.0f;
            if (fabsf(*p_y) < deadzone) *p_y = 0.0f;
        }
        else
        {
            // Reset when released
            *p_x = 0.0f;
            *p_y = 0.0f;
        }
    }

    // Rendering
    ImDrawList* draw_list = window->DrawList;
    const ImU32 bg_col = ImGui::GetColorU32(is_active ? ImGuiCol_FrameBgActive :
                                          hovered ? ImGuiCol_FrameBgHovered :
                                          ImGuiCol_FrameBg);

    // Draw background
    if (drawBg) {
        draw_list->AddRectFilled(pos, pos + button_sz,
                                 bg_col, style.FrameRounding);
    }

    // Draw icon
    DrawIcon(draw_list, pos, pos + button_sz,
             IconType::Plus, is_active,
             ImGui::GetColorU32(ImGuiCol_Text),
             ImGui::GetColorU32(ImGuiCol_TextDisabled));

    // Draw label
    ImVec2 label_pos(
        pos.x + (button_sz.x - label_size.x) * 0.5f,
        pos.y + button_sz.y + style.ItemSpacing.y
    );
    draw_list->AddText(label_pos, ImGui::GetColorU32(ImGuiCol_Text), label);

    // Advance cursor past the control including label
    //ImGui::ItemSize(ImVec2(button_sz.x, total_height), style.FramePadding.y);

    return is_active;
}

bool imgui_knob(const char* label, float* p_value, float v_min, float v_max, bool zero_on_release)
{
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();

    float radius_outer = 20.0f;
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 center = ImVec2(pos.x + radius_outer, pos.y + radius_outer);
    float line_height = ImGui::GetTextLineHeight();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    float ANGLE_MIN = 3.141592f * 0.75f;
    float ANGLE_MAX = 3.141592f * 2.25f;

    ImGui::InvisibleButton(label, ImVec2(radius_outer * 2, 
                                         radius_outer * 2 + line_height + style.ItemInnerSpacing.y));
    bool value_changed = false;
    bool is_active = ImGui::IsItemActive();
    bool is_hovered = ImGui::IsItemActive();
    if (is_active && io.MouseDelta.x != 0.0f)
    {
        float step = (v_max - v_min) / 200.0f;
        *p_value += io.MouseDelta.x * step;
        if (*p_value < v_min) *p_value = v_min;
        if (*p_value > v_max) *p_value = v_max;
        value_changed = true;
    }

    float t = (*p_value - v_min) / (v_max - v_min);
    float angle = ANGLE_MIN + (ANGLE_MAX - ANGLE_MIN) * t;
    float angle_cos = cosf(angle), angle_sin = sinf(angle);
    float radius_inner = radius_outer * 0.40f;
    draw_list->AddCircleFilled(center, radius_outer, ImGui::GetColorU32(ImGuiCol_FrameBg), 16);
    draw_list->AddLine(ImVec2(center.x + angle_cos * radius_inner, center.y + angle_sin * radius_inner),
                       ImVec2(center.x + angle_cos * (radius_outer - 2),
                              center.y + angle_sin * (radius_outer - 2)),
                       ImGui::GetColorU32(ImGuiCol_SliderGrabActive), 2.0f);
    draw_list->AddCircleFilled(center, radius_inner, 
                               ImGui::GetColorU32(is_active ? ImGuiCol_FrameBgActive :
                                                  is_hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), 16);

    auto ts = ImGui::CalcTextSize(label);
    draw_list->AddText(ImVec2(center.x - 0.5f * ts.x, pos.y + radius_outer * 2 + style.ItemInnerSpacing.y),
                       ImGui::GetColorU32(ImGuiCol_Text), label);

    if (is_active || is_hovered)
    {
        ImGui::SetNextWindowPos(ImVec2(pos.x - style.WindowPadding.x, 
                                       pos.y - line_height - style.ItemInnerSpacing.y - style.WindowPadding.y));
        ImGui::BeginTooltip();
        ImGui::Text("%.3f", *p_value);
        ImGui::EndTooltip();
    }
    
    if (!is_active && zero_on_release) {
        *p_value = 0.f;
    }

    return value_changed;
}

bool imgui_splitter(bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size)
{
    using namespace ImGui;
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = g.CurrentWindow;
    ImGuiID id = window->GetID("##Splitter");
    ImRect bb;
    bb.Min = window->DC.CursorPos + (split_vertically ? ImVec2(*size1, 0.0f) : ImVec2(0.0f, *size1));
    bb.Max = bb.Min + CalcItemSize(split_vertically ? ImVec2(thickness, splitter_long_axis_size) : ImVec2(splitter_long_axis_size, thickness), 0.0f, 0.0f);
    window->DrawList->AddRect(bb.Min, bb.Max, IM_COL32(255,255,0,255));

    return ImGui::SplitterBehavior(bb, id, split_vertically ? ImGuiAxis_X : ImGuiAxis_Y, size1, size2, min_size1, min_size2, 0.0f);
}

// Piano display: https://github.com/shric/midi
// The MIT License (MIT)
// Copyright (c) 2020 Chris Young

class Piano {
    int key_states[256] = {0};
public:
    void up(int key);
    void draw(bool *show);
    void down(int key, int velocity);
    std::vector<int> current_notes();
};


static bool has_black(int key) {
    return (!((key - 1) % 7 == 0 || (key - 1) % 7 == 3) && key != 51);
}

void Piano::up(int key) {
    key_states[key] = 0;
}

void Piano::down(int key, int velocity) {
    key_states[key] = velocity;

}

void Piano::draw(bool *show) {
    ImU32 Black = IM_COL32(0, 0, 0, 255);
    ImU32 White = IM_COL32(255, 255, 255, 255);
    ImU32 Red = IM_COL32(255,0,0,255);
    ImGui::Begin("Keyboard", show);
    ImDrawList *draw_list = ImGui::GetWindowDrawList();
    ImVec2 p = ImGui::GetCursorScreenPos();
    int width = 20;
    int cur_key = 21;
    for (int key = 0; key < 52; key++) {
        ImU32 col = White;
        if (key_states[cur_key]) {
            col = Red;
        }
        draw_list->AddRectFilled(
                ImVec2(p.x + key * width, p.y),
                ImVec2(p.x + key * width + width, p.y + 120),
                col, 0, ImDrawFlags_RoundCornersAll);
        draw_list->AddRect(
                ImVec2(p.x + key * width, p.y),
                ImVec2(p.x + key * width + width, p.y + 120),
                Black, 0, ImDrawFlags_RoundCornersAll);
        cur_key++;
        if (has_black(key)) {
            cur_key++;
        }
    }
    cur_key = 22;
    for (int key = 0; key < 52; key++) {
        if (has_black(key)) {
            ImU32 col = Black;
            if (key_states[cur_key]) {
                col = Red;
            }
            draw_list->AddRectFilled(
                    ImVec2(p.x + key * width + width * 3 / 4, p.y),
                    ImVec2(p.x + key * width + width * 5 / 4 + 1, p.y + 80),
                    col, 0, ImDrawFlags_RoundCornersAll);
            draw_list->AddRect(
                    ImVec2(p.x + key * width + width * 3 / 4, p.y),
                    ImVec2(p.x + key * width + width * 5 / 4 + 1, p.y + 80),
                    Black, 0, ImDrawFlags_RoundCornersAll);

            cur_key += 2;
        } else {
            cur_key++;
        }
    }
    ImGui::End();
}

std::vector<int> Piano::current_notes() {
    std::vector<int> result{};
    for (int i = 0; i < 256; i++) {
        if (key_states[i]) {
            result.push_back(i);
        }
    }
    return result;
}

// https://github.com/bkaradzic/bgfx/blob/master/3rdparty/dear-imgui/widgets/range_slider.inl
// https://github.com/ocornut/imgui/issues/76
// Taken from: https://github.com/wasikuss/imgui/commit/a50515ace6d9a62ebcd69817f1da927d31c39bb1


template<typename TYPE>
static const char* ImAtoi(const char* src, TYPE* output)
{
    int negative = 0;
    if (*src == '-') { negative = 1; src++; }
    if (*src == '+') { src++; }
    TYPE v = 0;
    while (*src >= '0' && *src <= '9')
        v = (v * 10) + (*src++ - '0');
    *output = negative ? -v : v;
    return src;
}

template<typename TYPE, typename SIGNEDTYPE>
TYPE RoundScalarWithFormatT(const char* format, ImGuiDataType data_type, TYPE v)
{
    const char* fmt_start = ImParseFormatFindStart(format);
    if (fmt_start[0] != '%' || fmt_start[1] == '%') // Don't apply if the value is not visible in the format string
        return v;
    char v_str[64];
    ImFormatString(v_str, IM_ARRAYSIZE(v_str), fmt_start, v);
    const char* p = v_str;
    while (*p == ' ')
        p++;
    if (data_type == ImGuiDataType_Float || data_type == ImGuiDataType_Double)
        v = (TYPE)ImAtof(p);
    else
        ImAtoi(p, (SIGNEDTYPE*)&v);
    return v;
}

float RoundScalarWithFormatFloat(const char* format, ImGuiDataType data_type, float v)
{
    return RoundScalarWithFormatT<float, float>(format, data_type, v);
}

float SliderCalcRatioFromValueFloat(ImGuiDataType data_type, float v,
                                    float v_min, float v_max, float power, float linear_zero_pos)
{
    return ScaleRatioFromValueT<float, float, float>(data_type, v, v_min, v_max, false, power, linear_zero_pos);
}

// ~80% common code with ImGui::SliderBehavior
bool RangeSliderBehavior(const ImRect& frame_bb, ImGuiID id, float* v1, float* v2, 
                         float v_min, float v_max, float power, int decimal_precision, ImGuiSliderFlags flags)
{
    ImGuiContext& g = *GImGui;
    ImGuiWindow* window = GetCurrentWindow();
    const ImGuiStyle& style = g.Style;

    // Draw frame
    RenderFrame(frame_bb.Min, frame_bb.Max, GetColorU32(ImGuiCol_FrameBg), true, style.FrameRounding);

    const bool is_non_linear = (power < 1.0f-0.00001f) || (power > 1.0f+0.00001f);
    const bool is_horizontal = (flags & ImGuiSliderFlags_Vertical) == 0;

    const float grab_padding = 2.0f;
    const float slider_sz = is_horizontal ? (frame_bb.GetWidth() - grab_padding * 2.0f) : (frame_bb.GetHeight() - grab_padding * 2.0f);

    // Integer sliders, if possible have the grab size represent 1 unit
    float grab_sz;
    if (decimal_precision > 0)
        grab_sz = ImMin(style.GrabMinSize, slider_sz);
    else
        grab_sz = ImMin(ImMax(1.0f * (slider_sz / ((v_min < v_max ? v_max - v_min : v_min - v_max) + 1.0f)), style.GrabMinSize), slider_sz); 

    const float slider_usable_sz = slider_sz - grab_sz;
    const float slider_usable_pos_min = (is_horizontal ? frame_bb.Min.x : frame_bb.Min.y) + grab_padding + grab_sz*0.5f;
    const float slider_usable_pos_max = (is_horizontal ? frame_bb.Max.x : frame_bb.Max.y) - grab_padding - grab_sz*0.5f;

    // For logarithmic sliders that cross over sign boundary we want the exponential increase to be symmetric around 0.0f
    float linear_zero_pos = 0.0f;   // 0.0->1.0f
    if (v_min * v_max < 0.0f)
    {
        // Different sign
        const float linear_dist_min_to_0 = powf(fabsf(0.0f - v_min), 1.0f/power);
        const float linear_dist_max_to_0 = powf(fabsf(v_max - 0.0f), 1.0f/power);
        linear_zero_pos = linear_dist_min_to_0 / (linear_dist_min_to_0+linear_dist_max_to_0);
    }
    else
    {
        // Same sign
        linear_zero_pos = v_min < 0.0f ? 1.0f : 0.0f;
    }

    // Process clicking on the slider
    bool value_changed = false;
    if (g.ActiveId == id)
    {
        if (g.IO.MouseDown[0])
        {
            const float mouse_abs_pos = is_horizontal ? g.IO.MousePos.x : g.IO.MousePos.y;
            float clicked_t = (slider_usable_sz > 0.0f) ? ImClamp((mouse_abs_pos - slider_usable_pos_min) / slider_usable_sz, 0.0f, 1.0f) : 0.0f;
            if (!is_horizontal)
                clicked_t = 1.0f - clicked_t;

            float new_value;
            if (is_non_linear)
            {
                // Account for logarithmic scale on both sides of the zero
                if (clicked_t < linear_zero_pos)
                {
                    // Negative: rescale to the negative range before powering
                    float a = 1.0f - (clicked_t / linear_zero_pos);
                    a = powf(a, power);
                    new_value = ImLerp(ImMin(v_max,0.0f), v_min, a);
                }
                else
                {
                    // Positive: rescale to the positive range before powering
                    float a;
                    if (fabsf(linear_zero_pos - 1.0f) > 1.e-6f)
                        a = (clicked_t - linear_zero_pos) / (1.0f - linear_zero_pos);
                    else
                        a = clicked_t;
                    a = powf(a, power);
                    new_value = ImLerp(ImMax(v_min,0.0f), v_max, a);
                }
            }
            else
            {
                // Linear slider
                new_value = ImLerp(v_min, v_max, clicked_t);
            }

            char fmt[64];
            snprintf(fmt, 64, "%%.%df", decimal_precision);

            // Round past decimal precision
            new_value = RoundScalarWithFormatFloat(fmt, ImGuiDataType_Float, new_value);
            if (*v1 != new_value || *v2 != new_value)
            {
                if (fabsf(*v1 - new_value) < fabsf(*v2 - new_value))
                {
                    *v1 = new_value;
                }
                else
                {
                    *v2 = new_value;
                }
                value_changed = true;
            }
        }
        else
        {
            ClearActiveID();
        }
    }

    // Calculate slider grab positioning
    float grab_t = SliderCalcRatioFromValueFloat(ImGuiDataType_Float, *v1, v_min, v_max, power, linear_zero_pos);

    // Draw
    if (!is_horizontal)
        grab_t = 1.0f - grab_t;
    float grab_pos = ImLerp(slider_usable_pos_min, slider_usable_pos_max, grab_t);
    ImRect grab_bb1;
    if (is_horizontal)
        grab_bb1 = ImRect(ImVec2(grab_pos - grab_sz*0.5f, frame_bb.Min.y + grab_padding), ImVec2(grab_pos + grab_sz*0.5f, frame_bb.Max.y - grab_padding));
    else
        grab_bb1 = ImRect(ImVec2(frame_bb.Min.x + grab_padding, grab_pos - grab_sz*0.5f), ImVec2(frame_bb.Max.x - grab_padding, grab_pos + grab_sz*0.5f));
    window->DrawList->AddRectFilled(grab_bb1.Min, grab_bb1.Max, GetColorU32(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab), style.GrabRounding);

    // Calculate slider grab positioning
    grab_t = SliderCalcRatioFromValueFloat(ImGuiDataType_Float, *v2, v_min, v_max, power, linear_zero_pos);

    // Draw
    if (!is_horizontal)
        grab_t = 1.0f - grab_t;
    grab_pos = ImLerp(slider_usable_pos_min, slider_usable_pos_max, grab_t);
    ImRect grab_bb2;
    if (is_horizontal)
        grab_bb2 = ImRect(ImVec2(grab_pos - grab_sz*0.5f, frame_bb.Min.y + grab_padding), ImVec2(grab_pos + grab_sz*0.5f, frame_bb.Max.y - grab_padding));
    else
        grab_bb2 = ImRect(ImVec2(frame_bb.Min.x + grab_padding, grab_pos - grab_sz*0.5f), ImVec2(frame_bb.Max.x - grab_padding, grab_pos + grab_sz*0.5f));
    window->DrawList->AddRectFilled(grab_bb2.Min, grab_bb2.Max, GetColorU32(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab), style.GrabRounding);

    ImRect connector(grab_bb1.Min, grab_bb2.Max);
    connector.Min.x += grab_sz;
    connector.Min.y += grab_sz*0.3f;
    connector.Max.x -= grab_sz;
    connector.Max.y -= grab_sz*0.3f;

    window->DrawList->AddRectFilled(connector.Min, connector.Max, GetColorU32(ImGuiCol_SliderGrab), style.GrabRounding);

    return value_changed;
}

// ~95% common code with ImGui::SliderFloat
bool RangeSliderFloat(const char* label, float* v1, float* v2, float v_min, float v_max, const char* display_format, float power)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;
    const ImGuiID id = window->GetID(label);
    const float w = CalcItemWidth();

    const ImVec2 label_size = CalcTextSize(label, NULL, true);
    const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y*2.0f));
    const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

    // NB- we don't call ItemSize() yet because we may turn into a text edit box below
    if (!ItemAdd(total_bb, id))
    {
        ItemSize(total_bb, style.FramePadding.y);
        return false;
    }

    const bool hovered = ItemHoverable(frame_bb, id, g.LastItemData.ItemFlags);
    if (hovered)
        SetHoveredID(id);

    int decimal_precision = 3;
    if (display_format)
        decimal_precision = ImParseFormatPrecision(display_format, 3);

    // Tabbing or CTRL-clicking on Slider turns it into an input box
    bool start_text_input = false;
    if (hovered && g.IO.MouseClicked[0])
    {
        SetActiveID(id, window);
        FocusWindow(window);

        if (g.IO.KeyCtrl)
        {
            start_text_input = true;
            g.TempInputId = 0;
        }
    }

    if (start_text_input || (g.ActiveId == id && g.TempInputId == id))
    {
        char fmt[64];
        snprintf(fmt, 64, "%%.%df", decimal_precision);
        return TempInputScalar(frame_bb, id, label, ImGuiDataType_Float, v1, fmt);
    }

    ItemSize(total_bb, style.FramePadding.y);

    // Actual slider behavior + render grab
    const bool value_changed = RangeSliderBehavior(frame_bb, id, v1, v2, v_min, v_max, power, decimal_precision, 0);

    if (display_format) {
        // Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
        char value_buf[64];
        const char* value_buf_end = value_buf + ImFormatString(value_buf, IM_ARRAYSIZE(value_buf), display_format, *v1, *v2);
        RenderTextClipped(frame_bb.Min, frame_bb.Max, value_buf, value_buf_end, NULL, ImVec2(0.5f,0.5f));
    }

    if (label_size.x > 0.0f)
        RenderText(ImVec2(frame_bb.Max.x + style.ItemInnerSpacing.x, frame_bb.Min.y + style.FramePadding.y), label);

    return value_changed;
}


// per https://github.com/ocornut/imgui/issues/1167#issuecomment-310176321

bool ImageButton(ImTextureID img,
    const ImVec2& size,
    const ImVec4& bg_col,
    const ImVec4& drawCol_normal,
    const ImVec4& drawCol_hover,
    const ImVec4& drawCol_Down,
    int frame_padding)
{
    ImGuiWindow* window = GetCurrentWindow();
    if (window->SkipItems)
        return false;
    const ImVec2& uv0 = ImVec2(0, 0);
    const ImVec2& uv1 = ImVec2(1, 1);

    ImGuiContext& g = *GImGui;
    const ImGuiStyle& style = g.Style;

    // Default to using texture ID as ID. User can still push string/integer prefixes.
    // We could hash the size/uv to create a unique ID but that would prevent the user from animating UV.
    PushID((int) img);
    const ImGuiID id = window->GetID("#image");
    PopID();

    const ImVec2 padding = (frame_padding >= 0) ? ImVec2((float)frame_padding, (float)frame_padding) : style.FramePadding;
    const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size + padding * 2);
    const ImRect image_bb(window->DC.CursorPos + padding, window->DC.CursorPos + padding + size);
    ItemSize(bb);
    if (!ItemAdd(bb, id))
        return false;

    bool hovered, held;
    bool pressed = ButtonBehavior(bb, id, &hovered, &held);
    // Render
    /*const ImU32 col = GetColorU32((hovered && held) ? ImGuiCol_ButtonActive : hovered ? ImGuiCol_ButtonHovered : ImGuiCol_Button);
    RenderFrame(bb.Min, bb.Max, col, true, ImClamp((float)ImMin(padding.x, padding.y), 0.0f, style.FrameRounding));
    if (bg_col.w > 0.0f)
        window->DrawList->AddRectFilled(image_bb.Min, image_bb.Max, GetColorU32(bg_col));*/

    window->DrawList->AddImage(img, image_bb.Min, image_bb.Max, uv0, uv1, GetColorU32(
        (hovered && held) ? drawCol_Down : hovered ? drawCol_hover : drawCol_normal));

    return pressed;
}

