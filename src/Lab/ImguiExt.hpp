#pragma once

#ifndef lab_imgui_ext_hpp
#define lab_imgui_ext_hpp

#include <string>
#include <vector>
#include <fstream>
#include <iostream>

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui.h"
#include "imgui_internal.h"
#include "IconsFontaudio.h"

namespace imgui_fonts
{
    unsigned int getCousineRegularCompressedSize();
    const unsigned int * getCousineRegularCompressedData();
}
ImFont * append_audio_icon_font(const std::vector<uint8_t> & buffer);

void imgui_fixed_window_begin(const char * name, float min_x, float min_y, float max_x, float max_y);
void imgui_fixed_window_end();

std::vector<uint8_t> read_file_binary(const std::string & pathToFile);

enum class IconType { 
    Flow, Circle, Square, Grid, RoundSquare, Diamond, Plus };
void DrawIcon(ImDrawList* drawList, const ImVec2& ul, const ImVec2& lr, IconType type, bool filled, ImU32 color, ImU32 innerColor);

bool imgui_knob(const char* label, float* p_value, float v_min, float v_max, bool zero_on_release);

bool imgui_splitter(bool split_vertically, float thickness, float* size1, float* size2, float min_size1, float min_size2, float splitter_long_axis_size = -1.0f);

bool RangeSliderFloat(const char* label, float* v1, float* v2,
                      float v_min, float v_max,
                      const char* display_format = "(%.3f, %.3f)", float power = 1.0f);

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
                float size,
                float deadzone = 0.2f);

/*
bool ImageButton(
    ImTextureID img,
    const ImVec2& size,
    const ImVec4& bg_col = ImColor(255, 255, 255, 255),
    const ImVec4& drawCol_normal = ImColor(225, 225, 225, 255),
    const ImVec4& drawCol_hover = ImColor(255, 255, 255, 255),
    const ImVec4& drawCol_Down = ImColor(180, 180, 160, 255), int frame_padding = 0);
*/
// run time type dispatch to Slider by gugrob9000
// https://github.com/ocornut/imgui/issues/7081#issuecomment-1901774784
namespace ImGui {
template<typename> constexpr ImGuiDataType DataTypeEnum = ImGuiDataType_COUNT;
template<> constexpr inline ImGuiDataType DataTypeEnum<int8_t>   = ImGuiDataType_S8;
template<> constexpr inline ImGuiDataType DataTypeEnum<uint8_t>  = ImGuiDataType_U8;
template<> constexpr inline ImGuiDataType DataTypeEnum<int16_t>  = ImGuiDataType_S16;
template<> constexpr inline ImGuiDataType DataTypeEnum<uint16_t> = ImGuiDataType_U16;
template<> constexpr inline ImGuiDataType DataTypeEnum<int32_t>  = ImGuiDataType_S32;
template<> constexpr inline ImGuiDataType DataTypeEnum<uint32_t> = ImGuiDataType_U32;
template<> constexpr inline ImGuiDataType DataTypeEnum<int64_t>  = ImGuiDataType_S64;
template<> constexpr inline ImGuiDataType DataTypeEnum<uint64_t> = ImGuiDataType_U64;
template<> constexpr inline ImGuiDataType DataTypeEnum<float>    = ImGuiDataType_Float;
template<> constexpr inline ImGuiDataType DataTypeEnum<double>   = ImGuiDataType_Double;

template<typename T> concept ScalarType = (DataTypeEnum<T> != ImGuiDataType_COUNT);

template<ScalarType T>
bool Drag(const char* l, T* p, float speed, T min = std::numeric_limits<T>::lowest(), T max = std::numeric_limits<T>::max(), const char* fmt = nullptr, ImGuiSliderFlags flags = 0) {
  return DragScalar(l, DataTypeEnum<T>, p, speed, &min, &max, fmt, flags);
}

template<ScalarType T>
bool DragN(const char* l, T* p, int n, float speed, T min = std::numeric_limits<T>::lowest(), T max = std::numeric_limits<T>::max(), const char* fmt = nullptr, ImGuiSliderFlags flags = 0) {
  return DragScalarN(l, DataTypeEnum<T>, p, n, speed, &min, &max, fmt, flags);
}

template<ScalarType T>
bool Slider(const char* l, T* p, T min = std::numeric_limits<T>::lowest(), T max = std::numeric_limits<T>::max(), const char* fmt = nullptr, ImGuiSliderFlags flags = 0) {
  return SliderScalar(l, DataTypeEnum<T>, p, &min, &max, fmt, flags);
}

template<ScalarType T>
bool SliderN(const char* l, T* p, int n, T min = std::numeric_limits<T>::lowest(), T max = std::numeric_limits<T>::max(), const char* fmt = nullptr, ImGuiSliderFlags flags = 0) {
  return SliderScalar(l, DataTypeEnum<T>, p, n, &min, &max, fmt, flags);
}

template<ScalarType T>
bool InputNumber(const char* l, T* p, T step = 0, T step_fast = 0, const char* fmt = nullptr, ImGuiInputTextFlags flags = 0) {
  return InputScalar(l, DataTypeEnum<T>, p, (step == 0 ? nullptr : &step), (step_fast == 0 ? nullptr : &step_fast), fmt, flags);
}

template<ScalarType T>
bool InputNumberN(const char* l, T* p, int n, T step = 0, T step_fast = 0, const char* fmt = nullptr, ImGuiInputTextFlags flags = 0) {
  return InputScalarN(l, DataTypeEnum<T>, p, n, (step == 0 ? nullptr : &step), (step_fast == 0 ? nullptr : &step_fast), fmt, flags);
}

/* example
 
 static int i;
 static double d;
 ImGui::Slider("i", &i);
 ImGui::Slider("d", &d);
 
 */

} // namespace ImGui

#endif // lab_imgui_ext_hpp
