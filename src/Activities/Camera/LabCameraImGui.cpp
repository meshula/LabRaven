
/*
 Copyright (c) 2013 Nick Porcino, All rights reserved.
 License is MIT: http://opensource.org/licenses/MIT

 LabCameraImgui has no external dependencies besides LabCamera and Dear ImGui.
 Include LabCamera.cpp, and LabCameraImgui.cpp in your project.
*/


/*-----------------------------------------------------------------------------
     Demonstrate navigation via a virtual joystick hosted in a little window
 */

#include "LabCameraImGui.h"
#define TRACE_INTERACTION 0
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui.h"
#include "imgui_internal.h"

#include "ImGuizmo.h"

#include <algorithm>
#include <stdio.h>



struct LCNav_MouseState
{
    float initial_mousex{ 0 };          // set when mouse transitions from not dragging to dragging
    float initial_mousey{ 0 };
    float mousex{ 0 };                  // current mouse position in window space
    float mousey{ 0 };
    bool  click_initiated{ false };     // true only on the frame when the mouse transitioned from not dragging to dragging
    bool  dragging{ false };            // true as long as the button is held
    bool  click_ended{ false };         // true only on the frame when the mouse transitioned from dragging to not dragging
};


enum LCNav_Component
{
    LCNav_None, LCNav_Roll, LCNav_Joystick, LCNav_Vernier, LCNav_LensKit, LCNav_Zoom
};


struct LCNav_Panel
{
    float nav_radius = 6;
    lc_radians roll{ 0 };
    lc_interaction* interactive_controller = nullptr;
    LabCameraNavigatorPanelInteraction state = LCNav_Inactive;
    lc_i_Mode camera_interaction_mode = lc_i_ModeTurnTableOrbit;

    LCNav_Panel()
    {
        interactive_controller = lc_i_create_interactive_controller();
        lc_i_set_speed(interactive_controller, 0.01f, 0.005f);
        lc_camera_set_defaults(&trackball_camera);
    }

    ~LCNav_Panel()
    {
        lc_i_free_interactive_controller(interactive_controller);
    }

    // this camera is for displaying a little gimbal in the trackball widget
    lc_camera trackball_camera;

    LCNav_Component component = LCNav_None;
    LCNav_MouseState mouse_state;
    const lc_v2f size = { 512, 256 };
    const lc_v2f trackball_size = { 128, 128 };
    const lc_v2f lenskit_size = { 120, 200 };
};

extern "C"
LCNav_Panel* create_navigator_panel()
{
    return new LCNav_Panel();
}

extern "C"
void release_navigator_panel(LCNav_Panel* p)
{
    LCNav_Panel* ptr = reinterpret_cast<LCNav_Panel*>(p);
    delete ptr;
}



extern "C"
lc_interaction* LCNav_Panel_interaction_controller(const LCNav_Panel* p)
{
    return p->interactive_controller;
}

extern "C"
lc_i_Mode LCNav_Panel_interaction_mode(const LCNav_Panel* p)
{
    return p->camera_interaction_mode;
}

extern "C"
lc_radians LCNav_Panel_roll(const LCNav_Panel* p)
{
    return lc_radians{ p->roll };
}


static bool DialControl(const char* label, float* p_value)
{
    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();

    float radius_outer = 20.0f;
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 center = ImVec2(pos.x + radius_outer, pos.y + radius_outer);
    float line_height = ImGui::GetTextLineHeight();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    ImGui::InvisibleButton(label, ImVec2(radius_outer * 2, radius_outer * 2 + line_height + style.ItemInnerSpacing.y));

    bool value_changed = false;
    bool is_active = ImGui::IsItemActive();
    bool is_hovered = ImGui::IsItemActive();
    if (is_active && (io.MouseDelta.x != 0.f || io.MouseDelta.y != 0.f))
    {
        ImVec2 mouse_pos = io.MousePos - center;
        *p_value = atan2f(mouse_pos.x, -mouse_pos.y);
        value_changed = true;
    }

    float angle_cos = cosf(*p_value - 3.141592f * 0.5f), angle_sin = sinf(*p_value - 3.141592f * 0.5f);
    float radius_inner = radius_outer * 0.40f;
    draw_list->AddCircleFilled(center, radius_outer, ImGui::GetColorU32(ImGuiCol_FrameBg), 16);
    draw_list->AddLine(ImVec2(center.x + angle_cos * radius_inner, center.y + angle_sin * radius_inner),
                       ImVec2(center.x + angle_cos * (radius_outer - 2), center.y + angle_sin * (radius_outer - 2)),
                       ImGui::GetColorU32(ImGuiCol_SliderGrabActive), 2.0f);
    draw_list->AddCircleFilled(center, radius_inner, ImGui::GetColorU32(is_active ? ImGuiCol_FrameBgActive : is_hovered ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), 16);
    draw_list->AddText(ImVec2(pos.x, pos.y + radius_outer * 2 + style.ItemInnerSpacing.y), ImGui::GetColorU32(ImGuiCol_Text), label);

    if (is_active || is_hovered)
    {
        ImGui::SetNextWindowPos(ImVec2(pos.x - style.WindowPadding.x, pos.y - line_height - style.ItemInnerSpacing.y - style.WindowPadding.y));
        ImGui::BeginTooltip();
        ImGui::Text("%d", (int) (*p_value * 180.f / 3.141592f));
        ImGui::EndTooltip();
    }

    return value_changed;
}

static bool LensKit_FocalSlider(const char* label, float* p_value, float* lenses, int lens_count, ImVec2 const& sz)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiContext& g = *GImGui;
    const ImGuiID id = window->GetID(label);

    ImGuiIO& io = ImGui::GetIO();

    // remember where the cursor was before the invisible button
    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImGui::Button(label, ImVec2{ sz.x, sz.y });
    bool is_clicked = ImGui::IsItemClicked();
    bool is_active = ImGui::IsItemActive();

    float line_height = ImGui::GetTextLineHeight();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();


    float offset = sz.x * 0.5f;
    float mid_x = pos.x + offset;
    float scale = (sz.y - offset * 2.f) / (lenses[lens_count - 1] - lenses[0]);
    float radius = 6.f;

    const uint32_t col_active = 0xffffffff;
    const uint32_t col_hovered = 0xffffff00;
    const uint32_t col_inactive = 0xffc02000;

    ImVec2 mouse_pos = io.MousePos;
    bool is_hovered = false;

    ImVec2 p0;

    draw_list->AddLine({ mid_x, pos.y + offset }, { mid_x, pos.y + sz.y - offset }, 0xffffffff, 2.f);

    radius = 10.f;
    p0 = { mid_x, pos.y + (*p_value - lenses[0]) * scale + offset };
    bool thumb_is_hovered = !is_hovered && (fabsf(p0.x - mouse_pos.x) < radius * 0.5f) && (fabsf(p0.y - mouse_pos.y) < radius * 0.5f);
    is_hovered = !is_hovered && (fabsf(p0.x - mouse_pos.x) < radius * 0.5f) && (mouse_pos.y >= pos.y + offset) && (mouse_pos.y <= pos.y + sz.y - offset);

    is_active = g.ActiveId == id;

    draw_list->AddCircleFilled(p0, radius,
        ImGui::GetColorU32((is_hovered || is_active) ? col_hovered : col_inactive), 8);
    draw_list->AddCircle(p0, radius, 0xffcccccc, 8);

    float possible_value = (mouse_pos.y - (pos.y + offset)) / scale + lenses[0];
    possible_value = std::min(std::max(possible_value, lenses[0]), lenses[lens_count-1]);

    bool ret = false;
    if (is_active)
    {
        *p_value = possible_value;
        ret = true;
    }

    if (is_hovered || is_active)
    {
        ImGui::BeginTooltip();
        char str[16];
        snprintf(str, 16, "%d", static_cast<int>(possible_value));
        ImGui::TextUnformatted(str);
        ImGui::EndTooltip();
    }

    return ret;
}


static bool LensKit_Picker(const char* label, float* p_value, float* lenses, int lens_count, ImVec2 const& sz)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiIO& io = ImGui::GetIO();

    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    ImGui::Button(label, ImVec2{ sz.x, sz.y });
    bool is_clicked = ImGui::IsItemClicked();
    //bool is_active = ImGui::IsItemActive();

    float offset = sz.x * 0.5f;
    float mid_x = pos.x + offset;
    float scale = (sz.y - offset * 2.f) / (lenses[lens_count - 1] - lenses[0]);
    float radius = 6.f;

    draw_list->AddLine({ mid_x, pos.y + offset }, { mid_x, pos.y + sz.y - offset }, 0xffffffff, 2.f);

    const uint32_t col_active = 0xffffffff;
    const uint32_t col_hovered = 0xffffff00;
    const uint32_t col_inactive = 0xff000000;

    ImVec2 mouse_pos = io.MousePos;
    bool is_hovered = false;

    ImVec2 p0;
    int hovered_element = lens_count;

    float dist_min = 1e4f;
    for (int i = 0; i < lens_count; ++i)
    {
        p0 = { mid_x, pos.y + (lenses[i] - lenses[0]) * scale + offset };
        //bool is_kit_active = fabsf(*p_value - lenses[i]) < 1.f;
        float test_dist = fabsf(p0.x - mouse_pos.x);
        is_hovered = (test_dist < radius * 0.5f) && (fabsf(p0.y - mouse_pos.y) < radius * 0.5f);
        if (is_hovered && (test_dist < dist_min))
        {
            dist_min = test_dist;
            hovered_element = i;
        }
    }

    for (int i = 0; i < lens_count; ++i)
    {
        p0 = { mid_x, pos.y + (lenses[i] - lenses[0]) * scale + offset };
        bool is_kit_active = fabsf(*p_value - lenses[i]) < 1.f;
        is_hovered = i == hovered_element;

        draw_list->AddCircleFilled(p0, radius,
            ImGui::GetColorU32(is_kit_active ? col_active : is_hovered ? col_hovered : col_inactive), 8);
        draw_list->AddCircle(p0, radius, 0xffcccccc, 8);

        if (is_hovered)
        {
            ImGui::BeginTooltip();
            char str[16];
            snprintf(str, 16, "%d", static_cast<int>(lenses[i]));
            ImGui::TextUnformatted(str);
            ImGui::EndTooltip();
        }
    }

    bool ret = false;
    if (is_clicked && hovered_element < lens_count)
    {
        *p_value = lenses[hovered_element];
        ret = true;
    }
    return ret;
}

static bool LensKit_Vernier(const char* label, float* p_value,
                            const lc_camera& camera, float* lenses, int lens_count, ImVec2 const& sz)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    ImGuiIO& io = ImGui::GetIO();
    ImGuiStyle& style = ImGui::GetStyle();

    ImVec2 pos = ImGui::GetCursorScreenPos();
    float line_height = ImGui::GetTextLineHeight();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    float radius_outer = sz.x * 0.5f;
    ImVec2 center = ImVec2(pos.x + radius_outer, pos.y + radius_outer);

    ImGui::InvisibleButton(label, ImVec2(radius_outer * 2, radius_outer * 2 + line_height + style.ItemInnerSpacing.y));
    bool is_clicked = ImGui::IsItemClicked();
    bool is_active = ImGui::IsItemActive();
    bool is_hovered = ImGui::IsItemHovered();
    ImVec2 mouse_pos = io.MousePos - center;

    float fov_h = lc_camera_horizontal_FOV(&camera).rad * 0.5f;

    bool value_changed = false;
    if (is_active && (io.MouseDelta.x != 0.f || io.MouseDelta.y != 0.f))
    {
        //*p_value = atan2f(mouse_pos.x, -mouse_pos.y);
        value_changed = true;
    }

    float radius_inner = radius_outer * 0.40f;
    draw_list->AddCircleFilled(center, radius_outer, ImGui::GetColorU32(ImGuiCol_FrameBg), 16);

    float angle_cos, angle_sin;
    float a = -fov_h;
    for (; a < fov_h; a += 0.2f)
    {
        angle_cos = cosf(a - 3.141592f * 0.5f);
        angle_sin = sinf(a - 3.141592f * 0.5f);
        draw_list->AddLine(ImVec2(center.x + angle_cos * radius_inner, center.y + angle_sin * radius_inner),
            ImVec2(center.x + angle_cos * (radius_outer - 2), center.y + angle_sin * (radius_outer - 2)),
            ImGui::GetColorU32(ImGuiCol_SliderGrabActive), 2.0f);
    }
    angle_cos = cosf(fov_h - 3.141592f * 0.5f);
    angle_sin = sinf(fov_h - 3.141592f * 0.5f);
    draw_list->AddLine(ImVec2(center.x + angle_cos * radius_inner, center.y + angle_sin * radius_inner),
        ImVec2(center.x + angle_cos * (radius_outer - 2), center.y + angle_sin * (radius_outer - 2)),
        ImGui::GetColorU32(ImGuiCol_SliderGrabActive), 2.0f);


    draw_list->AddCircleFilled(center, radius_inner,
        ImGui::GetColorU32(is_active ? ImGuiCol_FrameBgActive : is_active ? ImGuiCol_FrameBgHovered : ImGuiCol_FrameBg), 16);

    static ImVec2 initial_click = { 0,0 };
    if (is_clicked)
    {
        initial_click = mouse_pos;
    }
    if (is_active)
    {
        float dy = (initial_click.y - mouse_pos.y) * 0.0125f;
        float val = *p_value - dy;
        *p_value = std::min(std::max(val, lenses[0]), lenses[lens_count - 1]);
        value_changed = true;
    }


    if (is_hovered)
    {
        ImGui::SetNextWindowPos(ImVec2(pos.x - style.WindowPadding.x, pos.y - line_height - style.ItemInnerSpacing.y - style.WindowPadding.y));
        ImGui::BeginTooltip();
        ImGui::Text("%d", (int)(fov_h * 2.f * 180.f / 3.141592f));
        ImGui::EndTooltip();
    }

    return value_changed;
}


static bool LensKit(const char* label, float* p_value,
                    const lc_camera& camera, float* lenses, int lens_count, ImVec2 const& sz)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems)
        return false;

    bool ret = false;

    ImGui::BeginChild("###LensKitChld"); // to nest columns

    ImGui::Columns(3, "###LensKitCols", true);
    ImGui::SetColumnWidth(0, sz.x / 3);

    // kit bar
    ret |= LensKit_Picker("###LensKitKit", p_value, lenses, lens_count, ImVec2{ sz.x / 3, sz.y });

    ImGui::NextColumn();

    // focal length slider
    ret |= LensKit_FocalSlider("###LensKitFL", p_value, lenses, lens_count, ImVec2{ sz.x / 3, sz.y });

    ImGui::NextColumn();

    // vernier slider
    ret |= LensKit_Vernier("###LensKitVrn", p_value, camera, lenses, lens_count, ImVec2{ sz.x / 3, sz.y });

    ImGui::EndChild();

    return ret;
}



extern "C"
void LCNav_mouse_state_update(LCNav_MouseState* ms,
    float mousex, float mousey, bool left_button_down)
{
    ms->click_ended = ms->dragging && !left_button_down;
    ms->click_initiated = !ms->dragging && left_button_down;
    ms->dragging = left_button_down;
    ms->mousex = mousex;
    ms->mousey = mousey;

    if (ms->click_initiated)
    {
        ms->initial_mousex = mousex;
        ms->initial_mousey = mousey;
    };

    if (TRACE_INTERACTION && ms->click_ended)
        printf("button released\n");
    if (TRACE_INTERACTION && ms->click_initiated)
        printf("button is_clicked\n");
}


const float roll_track_sz = 8.f;

extern "C"
void
draw_roll_widget(LCNav_Panel* navigator_panel, const lc_camera& camera,
    ImVec2 const& p0, ImVec2 const& p1)
{
    ImGuiIO& io = ImGui::GetIO();

    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    bool roll_active = false;
    draw_list->PushClipRect(p0, p1);
    {
        const float roll_track_sz = 8.f;
        float width = p1.x - p0.x - 2.f * roll_track_sz;
        draw_list->AddCircle((p0 + p1) * 0.5f, width * 0.5f, 0xffffffff, 24, 1.f);
        draw_list->AddCircle((p0 + p1) * 0.5f, width * 0.5f + roll_track_sz, 0xffffffff, 24, 1.f);

        //lc_v3f ypr = lc_mount_ypr(&camera.mount);
        float roll = navigator_panel->roll.rad; // ypr.z
        float s = sinf(roll) * ((width + roll_track_sz) * 0.5f);
        float c = cosf(roll) * ((width + roll_track_sz) * 0.5f);
        ImVec2 p_roll = { s, -c };
        ImVec2 roll_indicator = ((p0 + p1) * 0.5f) + p_roll;
        ImVec2 roll_dist = roll_indicator - io.MousePos;
        float roll_hovered = roll_dist.x * roll_dist.x + roll_dist.y * roll_dist.y;
        roll_active = roll_hovered < 36.f;
        if (roll_active)
            draw_list->AddCircleFilled(roll_indicator, roll_track_sz * 0.5f, 0xffffffff, 6);
        else
            draw_list->AddCircle(roll_indicator, roll_track_sz * 0.5f, 0xffffffff, 6, 2.f);
    }
    draw_list->PopClipRect();
}

extern "C"
void
draw_joystick_widget(LCNav_Panel* navigator_panel, const lc_camera& camera,
    ImVec2 const& p0, ImVec2 const& p1)
{
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    bool roll_active = false;
    draw_list->PushClipRect(p0, p1);
    {
        float width = p1.x - p0.x - 2.f * roll_track_sz;

        const lc_rigid_transform* tr = &camera.mount.transform;
        lc_mount_set_view(&navigator_panel->trackball_camera.mount, 5.f, tr->orientation, lc_v3f{ 0,0,0 }, lc_v3f{ 0, 1, 0 });

        lc_v3f pnt{ 0, 1, 0 };
        lc_v2f xy0 = lc_camera_project_to_viewport(&navigator_panel->trackball_camera, { -roll_track_sz,0 }, { width, p1.y - p0.y }, pnt);
        pnt = lc_v3f{ 0, -1, 0 };
        lc_v2f xy1 = lc_camera_project_to_viewport(&navigator_panel->trackball_camera, { -roll_track_sz,0 }, { width, p1.y - p0.y }, pnt);

        ImVec2 v0 = p0 + ImVec2{ xy0.x, xy0.y };
        ImVec2 v1 = p0 + ImVec2{ xy1.x, xy1.y };
        draw_list->AddLine(v0, v1, 0xff00ff00);

        pnt = lc_v3f{ 1, 0, 0 };
        xy0 = lc_camera_project_to_viewport(&navigator_panel->trackball_camera, { -roll_track_sz,0 }, { width, p1.y - p0.y }, pnt);
        pnt = lc_v3f{ -1, 0, 0 };
        xy1 = lc_camera_project_to_viewport(&navigator_panel->trackball_camera, { -roll_track_sz,0 }, { width, p1.y - p0.y }, pnt);

        v0 = p0 + ImVec2{ xy0.x, xy0.y };
        v1 = p0 + ImVec2{ xy1.x, xy1.y };
        draw_list->AddLine(v0, v1, 0xff0000ff);

        pnt = lc_v3f{ 0, 0, 1 };
        xy0 = lc_camera_project_to_viewport(&navigator_panel->trackball_camera, { -roll_track_sz,0 }, { width, p1.y - p0.y }, pnt);
        pnt = lc_v3f{ 0, 0, -1 };
        xy1 = lc_camera_project_to_viewport(&navigator_panel->trackball_camera, { -roll_track_sz,0 }, { width, p1.y - p0.y }, pnt);

        v0 = p0 + ImVec2{ xy0.x, xy0.y };
        v1 = p0 + ImVec2{ xy1.x, xy1.y };
        draw_list->AddLine(v0, v1, 0xffffBB00);
    }
    draw_list->PopClipRect();
}

extern "C"
bool check_joystick_gizmo(LCNav_Panel* navigator_panel, ImVec2& p0, ImVec2& p1)
{
    ImVec2 sz{ navigator_panel->trackball_size.x, navigator_panel->trackball_size.y };

    // assuming the 3d viewport is the current window, fetch the content region
    ImGuiWindow* win = ImGui::GetCurrentWindow();
    ImRect edit_rect = win->ContentRegionRect;
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 imgui_cursor_pos = ImGui::GetCursorPos();
    ImGui::SetCursorScreenPos(edit_rect.Min);
    //ImVec2 mouse_pos = io.MousePos - ImGui::GetCurrentWindow()->Pos - imgui_cursor_pos;

    // detect that the mouse is clicked in the content region
    bool click_finished = ImGui::Button("###NAVGIZMOREGION", sz);

    p0 = ImGui::GetItemRectMin();
    p1 = ImGui::GetItemRectMax();
    return click_finished;
}


static bool nav_visible = true;
extern "C"
void activate_navigator_panel(bool v) { nav_visible = v; }

extern "C"
LabCameraNavigatorPanelInteraction
run_navigator_panel(LCNav_Panel* navigator_panel_, lc_camera* camera, float dt)
{
    if (!nav_visible)
        return LCNav_Inactive;
    
    LCNav_Panel* navigator_panel = reinterpret_cast<LCNav_Panel*>(navigator_panel_);
    LabCameraNavigatorPanelInteraction result = LCNav_Inactive;
    ImGui::SetNextWindowSize(ImVec2(navigator_panel->size.x, navigator_panel->size.y),
                             ImGuiCond_FirstUseEver);
    ImGuiIO& io = ImGui::GetIO();
    //ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x - navigator_panel->size.x, 10));

    if (!ImGui::Begin("Navigator###LCNav", &nav_visible, ImGuiWindowFlags_NoScrollWithMouse)) {
        ImGui::End();
        return LCNav_Inactive;
    }

    ImVec2 winPos = ImGui::GetWindowPos();
    ImVec2 cursor_screen_pos = ImGui::GetCursorScreenPos();

    {
        lc_v3f orbit =
            lc_i_orbit_center_constraint(navigator_panel->interactive_controller);
        lc_v3f pos = camera->mount.transform.position;
        lc_v3f pole = { pos.x - orbit.x, pos.y - orbit.y, pos.z - orbit.z };
        float orbit_distance = sqrtf(pole.x*pole.x + pole.y*pole.y + pole.z*pole.z);
        
        auto m = lc_rt_matrix(&camera->mount.transform);
        auto m0 = m;
        ImGuizmo::SetDrawlist();
        ImGuizmo::ViewManipulate(
                     (float*)&m, // view matrix
                     orbit_distance,        // camera distance constraint
                     ImVec2(winPos.x + 12, winPos.y + 142), // position
                     ImVec2(128, 128), IM_COL32_BLACK_TRANS);
        float matrixTranslation[3], matrixRotation[3], matrixScale[3];
        ImGuizmo::DecomposeMatrixToComponents((float*)&m,
                                              matrixTranslation,
                                              matrixRotation, 
                                              matrixScale);
        float y = 40;
        ImGui::SetNextItemWidth(120);
        ImGui::SetCursorPos({200, y});
        ImGui::InputFloat3("Translation", &camera->mount.transform.position.x);
        ImGui::SetNextItemWidth(120);
        y += 20;
        ImGui::SetCursorPos({200, y});
        ImGui::InputFloat3("Orbit", &orbit.x);
        ImGui::SetNextItemWidth(120);
        y += 20;
        ImGui::SetCursorPos({200, y});
        float pre_distance = orbit_distance;
        ImGui::InputFloat("Distance", &orbit_distance);
        if (orbit_distance != pre_distance) {
            pole.x = pole.x * orbit_distance / pre_distance;
            pole.y = pole.y * orbit_distance / pre_distance;
            pole.z = pole.z * orbit_distance / pre_distance;
            pole.x += orbit.x;
            pole.y += orbit.y;
            pole.z += orbit.z;
            camera->mount.transform.position = pole;
        }
        ImGui::SetNextItemWidth(120);
        y += 20;
        ImGui::SetCursorPos({200, y});
        const float d2r = 2.f * float(M_PI) / 360.f;
        lc_v3f ypr = lc_mount_ypr(&camera->mount);
        ypr.x /= d2r; ypr.y /= -d2r; ypr.z /= d2r;
        //float t = ypr.x; ypr.x = ypr.y; ypr.y = t; // swap ypr to euler
        ImGui::InputFloat3("Rotation", &ypr.x);
        ImGui::SetCursorScreenPos({cursor_screen_pos.x, cursor_screen_pos.y + 60});

        float* before = (float*) &m0;
        float* after  = (float*) &m;
        bool changed  = false;
        for (int i = 0; i < 16 && !changed; ++i) {
            if (before[i] != after[i]) {
                changed = true;
            }
        }
        if (changed) {
            lc_mount_set_view_transform_ypr_eye(&camera->mount,
                {matrixRotation[1] * d2r, // yaw is y axis
                 matrixRotation[0] * d2r, matrixRotation[2] * d2r},
                {matrixTranslation[0],
                 matrixTranslation[1], matrixTranslation[2]});
        }
    }

    ImGui::Columns(3, "###NavCol", true);
    ImGui::SetColumnWidth(0, navigator_panel->trackball_size.x + 16);

    auto& ms = navigator_panel->mouse_state;
    LCNav_mouse_state_update(&ms, io.MousePos.x, io.MousePos.y, io.MouseDown[0]);

    ImVec2 p0, p1;
    /*bool joystick_gizmo_clicked =*/ check_joystick_gizmo(navigator_panel, p0, p1);
    bool joystick_gizmo_hovered = ImGui::IsItemHovered();
    draw_roll_widget(navigator_panel, *camera, p0, p1);
    draw_joystick_widget(navigator_panel, *camera, p0, p1);

    if (!(ms.click_initiated || ms.dragging))
        navigator_panel->component = LCNav_None;

    ImVec2 joystick_center = (p0 + p1) * 0.5f;
    if (joystick_gizmo_hovered && navigator_panel->component == LCNav_None && ms.click_initiated)
    {
        ImVec2 center_dist = joystick_center - io.MousePos;
        float radius = (p1.x - p0.x) * 0.5f - roll_track_sz;
        float r2 = radius * radius;
        if (center_dist.x * center_dist.x + center_dist.y * center_dist.y > r2)
            navigator_panel->component = LCNav_Roll;
        else
            navigator_panel->component = LCNav_Joystick;
    }

    float roll_hint = 0.f;
    if (navigator_panel->component == LCNav_Roll)
    {
        ImVec2 center_dist = joystick_center - io.MousePos;
        float radius = (p1.x - p0.x) * 0.5f - roll_track_sz;
        float r2 = radius * radius;
        r2 = sqrtf(r2);
        center_dist *= 1.f / r2;
        roll_hint = -atan2f(center_dist.x, center_dist.y);
    }

    ImGui::SetCursorScreenPos({ cursor_screen_pos.x, cursor_screen_pos.y + navigator_panel->trackball_size.y });

    ImGui::NextColumn();
    ImGui::SetColumnWidth(1, 100);

    if (ImGui::Button("Home###NavHome")) {
        auto up = lc_i_world_up_constraint(navigator_panel->interactive_controller);
        lc_mount_look_at(&camera->mount, { 0.f, 0.2f, navigator_panel->nav_radius }, { 0, 0, 0 }, lc_i_world_up_constraint(navigator_panel->interactive_controller));
        lc_i_set_orbit_center_constraint(navigator_panel->interactive_controller, { 0,0,0 });
        navigator_panel->roll = lc_radians{ 0 };
        result = LCNav_Inactive;
    }
    if (ImGui::Button(navigator_panel->camera_interaction_mode == lc_i_ModeCrane ? "-Crane-" : " Crane ")) {
        navigator_panel->camera_interaction_mode = lc_i_ModeCrane;
        result = LCNav_ModeChange;
    }
    if (ImGui::Button(navigator_panel->camera_interaction_mode == lc_i_ModeDolly ? "-Dolly-" : " Dolly ")) {
        navigator_panel->camera_interaction_mode = lc_i_ModeDolly;
        result = LCNav_ModeChange;
    }
    if (ImGui::Button(navigator_panel->camera_interaction_mode == lc_i_ModeTurnTableOrbit ? "-Orbit-" : " Orbit ")) {
        navigator_panel->camera_interaction_mode = lc_i_ModeTurnTableOrbit;
        result = LCNav_ModeChange;
    }
    if (ImGui::Button(navigator_panel->camera_interaction_mode == lc_i_ModePanTilt ? "-PanTilt-" : " PanTilt ")) {
        navigator_panel->camera_interaction_mode = lc_i_ModePanTilt;
        result = LCNav_ModeChange;
    }
    if (ImGui::Button(navigator_panel->camera_interaction_mode == lc_i_ModeArcball ? "-Arcball-" : " Arcball ")) {
        navigator_panel->camera_interaction_mode = lc_i_ModeArcball;
        result = LCNav_ModeChange;
    }

    ImGui::NextColumn();

    static float focal_length = 50.f;
    static float lenses[] = { 8, 16, 24, 35, 50, 80, 100, 120, 200 };
    static int lens_count = sizeof(lenses) / sizeof(float);

    ImVec2 pos = ImGui::GetCursorPos();
    ImVec2 sz = { navigator_panel->lenskit_size.x, navigator_panel->lenskit_size.y };
    ImGui::SetCursorPosY(pos.y + 8);
    if (LensKit("Lens###NavHome", &focal_length, *camera, lenses, lens_count, sz))
    {
        camera->optics.focal_length = lc_millimeters{ focal_length };
    }

    // end of columns.

    ImGui::End();

    if (navigator_panel->component == LCNav_Joystick)
    {
        lc_i_Phase phase = lc_i_PhaseContinue;
        if (ms.click_initiated)
            phase = lc_i_PhaseStart;
        else if (ms.click_ended)
            phase = lc_i_PhaseFinish;

        if (ms.click_initiated)
            ImGui::SetNextFrameWantCaptureMouse(true);

        if (navigator_panel->camera_interaction_mode == lc_i_ModeArcball)
        {
            ImVec2 center_dist = joystick_center - io.MousePos;
            InteractionToken tok = lc_i_begin_interaction(navigator_panel->interactive_controller, navigator_panel->trackball_size);
            lc_i_ttl_interaction(
                navigator_panel->interactive_controller,
                camera,
                tok, phase, navigator_panel->camera_interaction_mode,
                { navigator_panel->trackball_size.x - (center_dist.x + navigator_panel->trackball_size.x * 0.5f),
                  navigator_panel->trackball_size.y - (center_dist.y + navigator_panel->trackball_size.y * 0.5f) },
                  lc_radians{ navigator_panel->roll }, dt);
            lc_i_end_interaction(navigator_panel->interactive_controller, tok);
        }
        else
        {
            const float speed_scaler = 10.f;
            float scale = speed_scaler / navigator_panel->trackball_size.x;
            float dx = (navigator_panel->mouse_state.mousex - navigator_panel->mouse_state.initial_mousex) * scale;
            float dy = (navigator_panel->mouse_state.mousey - navigator_panel->mouse_state.initial_mousey) * -scale;
            InteractionToken tok = lc_i_begin_interaction(navigator_panel->interactive_controller, navigator_panel->trackball_size);
            lc_i_single_stick_interaction(
                navigator_panel->interactive_controller,
                camera,
                tok, navigator_panel->camera_interaction_mode, { dx, dy },
                lc_radians{ navigator_panel->roll }, dt);
            lc_i_end_interaction(navigator_panel->interactive_controller, tok);
        }
    }

    if (navigator_panel->component == LCNav_Roll)
    {
        lc_i_Phase phase = lc_i_PhaseContinue;
        if (ms.click_initiated)
            phase = lc_i_PhaseStart;
        else if (ms.click_ended)
            phase = lc_i_PhaseFinish;

        if (ms.click_initiated)
            ImGui::SetNextFrameWantCaptureMouse(true);

        navigator_panel->roll = lc_radians{ roll_hint };

        // renormalize transform, then apply the camera roll
        lc_mount_look_at(&camera->mount, camera->mount.transform.position,
            lc_i_orbit_center_constraint(navigator_panel->interactive_controller),
            lc_i_world_up_constraint(navigator_panel->interactive_controller));
        InteractionToken tok = lc_i_begin_interaction(navigator_panel->interactive_controller, navigator_panel->trackball_size);
        lc_i_set_roll(navigator_panel->interactive_controller, camera, tok, navigator_panel->roll);
        lc_i_end_interaction(navigator_panel->interactive_controller, tok);
    }

    return result;
}

static float len(float x, float y)
{
    return sqrtf(x * x + y * y);
}

extern "C"
void FX_minimap(ImDrawList* d, ImVec2 a, ImVec2 b, ImVec2 sz, ImVec4 mouse, float t, const lc_rigid_transform* cam, const lc_v3f lookat)
{
    float min_dim = std::min(sz.x, sz.y);
    lc_v3f eye = cam->position;
    lc_v3f relative_eye = { eye.x - lookat.x, eye.y - lookat.y, eye.z - lookat.z };
    float scale = 1.f / 10.f;
    ImVec2 p_eye = { relative_eye.x, relative_eye.z };
    p_eye *= scale;
    p_eye *= min_dim * 0.5f;
    p_eye += a;
    p_eye += sz * 0.5f;

    ImVec2 p_lookat = a + sz * 0.5f;
    d->AddRectFilled(p_lookat - ImVec2{ 4,4 }, p_lookat + ImVec2{ 4,4 }, 0xffffd050);
    d->AddRectFilled(p_eye - ImVec2{ 4,4 }, p_eye + ImVec2{ 4,4 }, 0xffffd050);
}

// this is a canonical way to create an ImGui rendering window with an invisible button
// to extract a drawable region covering the whole window.
// https://gist.github.com/ocornut/51367cc7dfd2c41d607bb0acfa6caf66
//
extern "C"
void camera_minimap(int w, int h, const lc_rigid_transform* cam, const lc_v3f lookat)
{
    ImGuiIO& io = ImGui::GetIO();
    ImGui::Begin("Camera Minimap", NULL, ImGuiWindowFlags_AlwaysAutoResize);
    ImVec2 size{ float(w), float(h) };
    ImGui::InvisibleButton("cm_canvas", size);
    ImVec2 p0 = ImGui::GetItemRectMin();
    ImVec2 p1 = ImGui::GetItemRectMax();
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    draw_list->PushClipRect(p0, p1);

    ImVec4 mouse_data;
    mouse_data.x = (io.MousePos.x - p0.x) / size.x;
    mouse_data.y = (io.MousePos.y - p0.y) / size.y;
    mouse_data.z = io.MouseDownDuration[0];
    mouse_data.w = io.MouseDownDuration[1];

    {
        // draw here
        //FX_glass(draw_list, p0, p1, size, mouse_data, (float)ImGui::GetTime());
        FX_minimap(draw_list, p0, p1, size, mouse_data, (float)ImGui::GetTime(), cam, lookat);
    }

    draw_list->PopClipRect();
    ImGui::End();
}
