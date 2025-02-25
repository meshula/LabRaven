
#include "TimelineActivity.hpp"
#include "UIElements.hpp"
#include "imgui.h"

using namespace opentime::OPENTIME_VERSION;

namespace lab {

static TimeRange full_time_range(RationalTime(0, 24), RationalTime(24 * 60 * 2, 24)); // 2 minutes

struct TimelineActivity::data {
    PlayHead playhead = {
        full_time_range,        // start viewing the full timeline
        RationalTime(0, 24),
        1000.0f,    // scale
        true, // snap_to_frames
        true  // draw_pan_zoomer
    };
};

TimelineActivity::TimelineActivity()
: Activity(TimelineActivity::sname()) {
    _self = new TimelineActivity::data;
    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<TimelineActivity*>(instance)->RunUI(*vi);
    };
}

TimelineActivity::~TimelineActivity() {
    delete _self;
}

opentime::OPENTIME_VERSION::RationalTime TimelineActivity::Playhead() const {
    return _self->playhead.playhead;
}

void TimelineActivity::RunUI(const LabViewInteraction&)
{
    const float width = 1000.f;

    ImGui::Begin("Timeline ##mM");

    // remember where the imgui drawing cursor is
    ImVec2 curCursor = ImGui::GetCursorPos();

    DrawTimecodeRuler(&_self->playhead, 0.5f, // zebra_factor
                  (uint32_t) (1 + (ptrdiff_t) _self), // widget_id
                  width, // width
                  100); // height

    curCursor.y += 4 * ImGui::GetTextLineHeightWithSpacing();
    ImGui::SetCursorPos(curCursor);

    DrawTransportControls(&_self->playhead, width, full_time_range);
    ImGui::End();
}

} // lab
