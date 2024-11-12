
#define IMGUI_DEFINE_MATH_OPERATORS
#include "TimelineActivity.hpp"
#include "UIElements.hpp"
#include "imgui.h"

using namespace opentime::OPENTIME_VERSION;

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
    static TimeRange full_time_range(RationalTime(0, 24), RationalTime(24 * 60 * 2, 24)); // 2 minutes

    static PlayHead playhead = {
        full_time_range, // start viewing the full timeline
        RationalTime(0, 24),
        1000.0f,    // scale
        true, // snap_to_frames
        true  // draw_pan_zoomer
    };

    const float width = 1000.f;

    ImGui::Begin("Timeline ##mM");

    // remember where the imgui drawing cursor is
    ImVec2 curCursor = ImGui::GetCursorPos();

    DrawTimecodeRuler(&playhead, 0.5f, // zebra_factor
                  1 + (ptrdiff_t) _self, // widget_id
                  width, // width
                  100); // height

    curCursor.y += 4 * ImGui::GetTextLineHeightWithSpacing();
    ImGui::SetCursorPos(curCursor);

    DrawTransportControls(&playhead, width, full_time_range);
    ImGui::End();
}

} // lab
