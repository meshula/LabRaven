
#define IMGUI_DEFINE_MATH_OPERATORS
#include "TimelineActivity.hpp"
#include "UIElements.hpp"
#include "imgui.h"

#include <opentimelineio/timeline.h>
namespace otio = opentimelineio::OPENTIMELINEIO_VERSION;

namespace lab {

struct TimelineActivity::data {
};

TimelineActivity::TimelineActivity()
: Activity() {
    _self = new TimelineActivity::data;
    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<TimelineActivity*>(instance)->RunUI(*vi);
    };
}

TimelineActivity::~TimelineActivity() {
    delete _self;
}

void TimelineActivity::RunUI(const LabViewInteraction&)
{

/*
void DrawTimecodeRuler(
                       float zebra_factor,
                       uint32_t widget_id,
                       otio::RationalTime start,
                       otio::RationalTime end,
                       float frame_rate,
                       float scale,
                       float width,
                       float height);
*/

    ImGui::Begin("Timeline ##mM");
    DrawTimecodeRuler(0.5f, // zebra_factor
                  1 + (ptrdiff_t) _self, // widget_id
                  otio::RationalTime(0, 24), // start
                  otio::RationalTime(24 * 60 * 2, 24), // end (2 minutes)
                  24, // frame_rate
                  100, // scale
                  1000, // width
                  100); // height
    ImGui::End();
}

} // lab
