
#define IMGUI_DEFINE_MATH_OPERATORS
#include "UIElements.hpp"
#include "TimelineProvider.hpp"
#include "Utilities.hpp"
#include "Lab/AppTheme.h"
#include <opentime/rationalTime.h>
#include <opentime/timeRange.h>
#include "imgui.h"
#include "imgui_internal.h"

using opentime::OPENTIME_VERSION::RationalTime;
using opentime::OPENTIME_VERSION::TimeRange;

void DrawTimecodeRuler(
                       PlayHead* tp,
                       float zebra_factor,
                       uint32_t widget_id,
                       float width,
                       float height)
{
    ImVec2 size(width, height);
    ImVec2 text_offset(7.0f, 5.0f);

    float scale = tp->scale;
    float frame_rate = tp->playhead.rate();
    auto start = tp->playhead_limit.start_time();

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
        const ImVec2 tick_start = ImVec2(p0.x + tick_x, p0.y);// + height) / 2);
        const ImVec2 tick_end = ImVec2(tick_start.x + tick_width, p1.y);

        const ImVec2 vertical_bar_top = ImVec2(p0.x + tick_x, p0.y + text_offset.y);
        const ImVec2 vertical_bar_bot = ImVec2(p0.x + tick_x, p0.y + text_offset.y + height / 2);

        if (!ImGui::IsRectVisible(tick_start, tick_end))
            continue;

        if (seconds_per_tick >= 0.5) {
            // draw thin lines at each tick
            draw_list->AddLine(
                               vertical_bar_top,
                               vertical_bar_bot,
                               tick_color);
        } else {
            // once individual frames are visible, draw dark/light stripes instead
            int frame = tick_time.to_frames();
            const ImVec2 zebra_start = ImVec2(p0.x + tick_x, vertical_bar_top.y);
            const ImVec2 zebra_end = ImVec2(tick_start.x + tick_width, vertical_bar_bot.y + 2);
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

bool DrawTransportControls(PlayHead* tp, float width, TimeRange full_timeline_range) {
    bool moved_playhead = false;

    const auto& playhead_limit = tp->playhead_limit;
    auto start = playhead_limit.start_time();
    auto duration = playhead_limit.duration();
    auto end = playhead_limit.end_time_exclusive();
    auto rate = duration.rate();
    if (tp->playhead.rate() != rate) {
        tp->playhead = tp->playhead.rescaled_to(rate);
        if (tp->snap_to_frames) {
            tp->SnapPlayhead();
        }
    }

    auto start_string = FormattedStringFromTime(start);
    auto playhead_string = FormattedStringFromTime(tp->playhead);
    auto end_string = FormattedStringFromTime(end);

    ImGui::PushID("##TransportControls");
    ImGui::BeginGroup();

    ImGui::Text("%s", start_string.c_str());
    ImGui::SameLine();

    ImGui::SetNextItemWidth(-270);
    float playhead_seconds = tp->playhead.to_seconds();
    if (ImGui::SliderFloat(
                           "##Playhead",
                           &playhead_seconds,
                           playhead_limit.start_time().to_seconds(),
                           playhead_limit.end_time_exclusive().to_seconds(),
                           playhead_string.c_str())) {
                               tp->Seek(playhead_seconds);
                               moved_playhead = true;
                           }

    ImGui::SameLine();
    ImGui::Text("%s", end_string.c_str());

    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    if (ImGui::SliderFloat(
                           "##Zoom",
                           &tp->scale,
                           0.1f,
                           5000.0f,
                           "Zoom",
                           ImGuiSliderFlags_Logarithmic)) {
                               // never go to 0 or less
                               tp->scale = fmax(0.0001f, tp->scale);
                               moved_playhead = true;
                           }

    ImGui::SameLine();
    if (ImGui::Button("Fit")) {
        tp->playhead_limit = full_timeline_range;
        TimeRange r = playhead_limit;
        tp->scale = width / r.duration().to_seconds();
    }

    ImGui::EndGroup();
    ImGui::PopID();

    if (tp->draw_pan_zoomer) {
        //-------------------------------------------------------------------------
        // pan zoomer

        static float s_max_timeline_value = 100.f;
        static float s_pixel_offset = 0.f;

        static double s_time_in = 0.f;
        static double s_time_out = 1.f;

        static double s_time_offset = 0;
        static double s_time_scale = 1;

        static const float TIMELINE_RADIUS = 12;

        ImVec2 upperLeft = ImGui::GetCursorScreenPos();
        ImVec2 lowerRight = ImVec2(upperLeft.x + width, upperLeft.y + 50);

        ImGuiWindow* win = ImGui::GetCurrentWindow();
        const float columnOffset = ImGui::GetColumnOffset(1);
        const float columnWidth = ImGui::GetColumnWidth(1) - GImGui->Style.ScrollbarSize;
        if (columnWidth > 0) {
            const ImU32 pz_inactive_color = ImGui::ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_Button]);
            const ImU32 pz_active_color = ImGui::ColorConvertFloat4ToU32(
                                                                         GImGui->Style.Colors[ImGuiCol_ButtonHovered]);
            const ImU32 color = ImGui::ColorConvertFloat4ToU32(GImGui->Style.Colors[ImGuiCol_Button]);
            const float rounding = GImGui->Style.ScrollbarRounding;
            
            win = ImGui::GetCurrentWindow();
            float startx = upperLeft.x;
            float endy = lowerRight.y;
            ImVec2 tl_start(startx, endy + ImGui::GetTextLineHeightWithSpacing());
            ImVec2 tl_end(
                          startx + columnWidth,
                          endy + 2 * ImGui::GetTextLineHeightWithSpacing());
            
            // this rectangle represents the entire time line
            win->DrawList->AddRectFilled(tl_start, tl_end, color, rounding);
            
            // draw time panzoomer
            
            double time_in = s_time_in;
            double time_out = s_time_out;
            
            float posx[2] = { 0, 0 };
            double values[2] = { time_in, time_out };
            
            bool active = false;
            bool hovered = false;
            bool changed = false;
            ImVec2 cursor_pos = { tl_start.x,
                tl_end.y - ImGui::GetTextLineHeightWithSpacing() };
            
            // draw the end markers and implement dragging them back and forth
            for (int i = 0; i < 2; ++i) {
                ImVec2 pos = cursor_pos;
                pos.x += columnWidth * float(values[i]);
                ImGui::SetCursorScreenPos(pos);
                pos.x += TIMELINE_RADIUS;
                pos.y += TIMELINE_RADIUS;
                posx[i] = pos.x;
                
                ImGui::PushID(i + 99999);
                ImGui::InvisibleButton(
                                       "zoompanner",
                                       ImVec2(2 * TIMELINE_RADIUS, 2 * TIMELINE_RADIUS));
                active = ImGui::IsItemActive();
                if (active || ImGui::IsItemHovered()) {
                    hovered = true;
                }
                if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
                    values[i] += ImGui::GetIO().MouseDelta.x / columnWidth;
                    changed = hovered = true;
                }
                ImGui::PopID();
                
                win->DrawList->AddCircleFilled(
                                               pos,
                                               TIMELINE_RADIUS,
                                               ImGui::IsItemActive() || ImGui::IsItemHovered()
                                               ? pz_active_color
                                               : pz_inactive_color);
            }
            
            if (values[0] > values[1])
                std::swap(values[0], values[1]);
            
            tl_start.x = posx[0];
            tl_start.y += TIMELINE_RADIUS * 0.5f;
            tl_end.x = posx[1];
            tl_end.y = tl_start.y + TIMELINE_RADIUS;
            
            ImGui::PushID(-1);
            ImGui::SetCursorScreenPos(tl_start);
            
            // finally drag the bar between the handles and draw it
            ImVec2 zp = tl_end;
            zp.x -= tl_start.x;
            zp.y -= tl_start.y;
            ImGui::InvisibleButton("zoompanner", zp);
            if (ImGui::IsItemActive() && ImGui::IsMouseDragging(0)) {
                values[0] += ImGui::GetIO().MouseDelta.x / columnWidth;
                values[1] += ImGui::GetIO().MouseDelta.x / columnWidth;
                changed = hovered = true;
            }
            ImGui::PopID();
            
            win->DrawList->AddRectFilled(
                                         tl_start,
                                         tl_end,
                                         ImGui::IsItemActive() || ImGui::IsItemHovered() ? pz_active_color
                                         : pz_inactive_color);
            
            for (int i = 0; i < 2; ++i) {
                if (values[i] < 0)
                    values[i] = 0;
                if (values[i] > 1)
                    values[i] = 1;
            }
            
            time_in = values[0];
            time_out = values[1];
            
            s_time_in = time_in;
            s_time_out = time_out;
            
            ImGui::SetCursorPosY(
                                 ImGui::GetCursorPosY() + 2 * ImGui::GetTextLineHeightWithSpacing());
            
            //---pan zoomer end-----------------------------------------------------
        }
    }

    return moved_playhead;
}

void PlayHead::Seek(double seconds) {
    double lower_limit = playhead_limit.start_time().to_seconds();
    double upper_limit = playhead_limit.end_time_exclusive().to_seconds();
    seconds = fmax(lower_limit, fmin(upper_limit, seconds));
    playhead = RationalTime::from_seconds(seconds, playhead.rate());
    if (snap_to_frames) {
        SnapPlayhead();
    }
}

void PlayHead::SnapPlayhead() {
    playhead = RationalTime::from_frames(
                                        playhead.to_frames(),
                                        playhead.rate());
}
