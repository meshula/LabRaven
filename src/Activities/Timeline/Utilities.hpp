
#ifndef TimelineUtilities_h
#define TimelineUtilities_h

#include <opentime/rationalTime.h>
#include <string>

// Flags for formatting.
//   Timecode rate, seconds, and frame
#define TIMECODE_RATE 1
#define TIMECODE_SECONDS 2
#define TIMECODE_FRAME 4
#define TIMECODE_DISPLAY_RATE 8
#define TIMECODE_ALL (TIMECODE_RATE | TIMECODE_SECONDS | TIMECODE_FRAME | TIMECODE_DISPLAY_RATE)


std::string FormattedStringFromTime(opentime::OPENTIME_VERSION::RationalTime time,
                                    uint32_t flags = TIMECODE_ALL);

std::string TimecodeStringFromTime(opentime::OPENTIME_VERSION::RationalTime time, bool drop_frame_mode = false);
std::string FramesStringFromTime(opentime::OPENTIME_VERSION::RationalTime time);
std::string SecondsStringFromTime(opentime::OPENTIME_VERSION::RationalTime time);

#endif /* TimelineUtilities_h */
