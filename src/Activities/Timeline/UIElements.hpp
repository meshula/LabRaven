#ifndef UIElements_hpp
#define UIElements_hpp

#include "imgui.h"
#include <opentime/rationalTime.h>
#include <opentime/timeRange.h>

struct PlayHead {
    opentime::OPENTIME_VERSION::TimeRange playhead_limit;
    opentime::OPENTIME_VERSION::RationalTime playhead;
    float scale = 1.0f;
    bool snap_to_frames = true;
    bool draw_pan_zoomer = true;

    void Seek(double seconds);
    void SnapPlayhead();
};

bool DrawTransportControls(
                           PlayHead* tp, float width, 
                           opentime::OPENTIME_VERSION::TimeRange full_timeline_range);

void DrawTimecodeRuler(
                       PlayHead* tp,
                       float zebra_factor,
                       uint32_t widget_id,
                       float width,
                       float height);

#endif /* UIElements_hpp */
