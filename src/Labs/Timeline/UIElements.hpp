#ifndef UIElements_hpp
#define UIElements_hpp

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include <opentime/rationalTime.h>

void DrawTimecodeRuler(
                       float zebra_factor,
                       uint32_t widget_id,
                       opentime::OPENTIME_VERSION::RationalTime start,
                       opentime::OPENTIME_VERSION::RationalTime end,
                       float frame_rate,
                       float scale,
                       float width,
                       float height);

#endif /* UIElements_hpp */
