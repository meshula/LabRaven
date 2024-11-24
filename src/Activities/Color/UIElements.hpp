
#ifndef UIElements_hpp
#define UIElements_hpp

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"

void ColorChipWithPicker(ImVec4& color, char id);
bool ColorSpacePicker(const char* label, int* cs, const char** csName);

#endif /* UIElements_hpp */
