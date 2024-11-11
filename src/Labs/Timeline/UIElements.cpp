
#include "UIElements.hpp"
#include "Utilities.hpp"
#include "Lab/AppTheme.h"

using opentime::OPENTIME_VERSION::RationalTime;

void DrawTimecodeRuler(
                       float zebra_factor,
                       uint32_t widget_id,
                       RationalTime start,
                       RationalTime end,
                       float frame_rate,
                       float scale,
                       float width,
                       float height)
{
    ImVec2 size(width, height);
    ImVec2 text_offset(7.0f, 5.0f);

    auto old_pos = ImGui::GetCursorPos();
    ImGui::PushID(widget_id);
    ImGui::BeginGroup();

    ImGui::Dummy(size);
    const ImVec2 p0 = ImGui::GetItemRectMin();
    const ImVec2 p1 = ImGui::GetItemRectMax();
    ImGui::SetItemAllowOverlap();
    if (!ImGui::IsRectVisible(p0, p1)) {
        ImGui::EndGroup();
        ImGui::PopID();
        ImGui::SetCursorPos(old_pos);
        return;
    }

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    auto fill_color = appTheme.colors[AppThemeCol_Background];
    auto tick_color = appTheme.colors[AppThemeCol_TickMajor];
    // auto tick2_color = appTheme.colors[AppThemeCol_TickMinor];
    auto tick_label_color = appTheme.colors[AppThemeCol_Label];
    auto zebra_color_dark = ImColor(0, 0, 0, 255.0 * zebra_factor);
    auto zebra_color_light = ImColor(255, 255, 255, 255.0 * zebra_factor);

    // background
    // draw_list->AddRectFilled(p0, p1, fill_color);

    // draw every frame?
    // Note: "width" implies pixels, but "duration" implies time.
    double single_frame_width = scale / frame_rate;
    double tick_width = single_frame_width;
    double min_tick_width = 15;
    if (tick_width < min_tick_width) {
        // every second?
        tick_width = scale;
        if (tick_width < min_tick_width) {
            // every minute?
            tick_width = scale * 60.0f;
            if (tick_width < min_tick_width) {
                // every hour
                tick_width = scale * 60.0f * 60.0f;
            }
        }
    }

    // tick marks - roughly every N pixels
    double pixels_per_second = scale;
    double seconds_per_tick = tick_width / pixels_per_second;
    // ticks must use frame_rate, and must have an integer value
    // so that the tick labels align to the human expectation of "frames"
    int tick_duration_in_frames = ceil(seconds_per_tick / frame_rate);
    int tick_count = ceil(width / tick_width);
    // start_floor_time and tick_offset_x adjust the display for cases where
    // the item's start_time is not on a whole frame boundary.
    auto start_floor_time = RationalTime(floor(start.value()), start.rate());
    auto tick_offset = (start - start_floor_time).rescaled_to(frame_rate);
    double tick_offset_x = tick_offset.to_seconds() * scale;
    // last_label_end_x tracks the tail of the last label, so we can prevent
    // overlap
    double last_label_end_x = p0.x - text_offset.x * 2;
    for (int tick_index = 0; tick_index < tick_count; tick_index++) {
        auto tick_time = start.rescaled_to(frame_rate)
        + RationalTime(
                             tick_index * tick_duration_in_frames,
                             frame_rate)
        - tick_offset;

        double tick_x = tick_index * tick_width - tick_offset_x;
        const ImVec2 tick_start = ImVec2(p0.x + tick_x, p0.y + height / 2);
        const ImVec2 tick_end = ImVec2(tick_start.x + tick_width, p1.y);

        if (!ImGui::IsRectVisible(tick_start, tick_end))
            continue;

        if (seconds_per_tick >= 0.5) {
            // draw thin lines at each tick
            draw_list->AddLine(
                               tick_start,
                               ImVec2(tick_start.x, tick_end.y),
                               tick_color);
        } else {
            // once individual frames are visible, draw dark/light stripes instead
            int frame = tick_time.to_frames();
            const ImVec2 zebra_start = ImVec2(p0.x + tick_x, p0.y);
            const ImVec2 zebra_end = ImVec2(tick_start.x + tick_width, p1.y);
            draw_list->AddRectFilled(
                                     zebra_start,
                                     zebra_end,
                                     (frame & 1) ? zebra_color_dark : zebra_color_light);
        }

        const ImVec2 tick_label_pos = ImVec2(p0.x + tick_x + text_offset.x, p0.y + text_offset.y);
        // only draw a label when there's room for it
        if (tick_label_pos.x > last_label_end_x + text_offset.x) {
            std::string tick_label = FormattedStringFromTime(tick_time);
            auto label_size = ImGui::CalcTextSize(tick_label.c_str());
            draw_list->AddText(
                               tick_label_pos,
                               tick_label_color,
                               tick_label.c_str());
            // advance last_label_end_x so nothing will overlap with the one we just
            // drew
            last_label_end_x = tick_label_pos.x + label_size.x;
        }
    }

    // For debugging, this is very helpful...
    // ImGui::SetTooltip("tick_width = %f\nseconds_per_tick =
    // %f\npixels_per_second = %f", tick_width, seconds_per_tick,
    // pixels_per_second);

    ImGui::EndGroup();
    ImGui::PopID();
    ImGui::SetCursorPos(old_pos);
}
