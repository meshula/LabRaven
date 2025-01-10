
#ifndef UIElements_hpp
#define UIElements_hpp

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui.h"

void ColorChipWithPicker(ImVec4& color, char id);
bool ColorSpacePicker(const char* label, int* cs, const char** csName);

#endif /* UIElements_hpp */
