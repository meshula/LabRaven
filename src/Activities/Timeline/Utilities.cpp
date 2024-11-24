
#include "Utilities.hpp"

// C forever <3
std::string Format(const char* format, ...) {
    char buf[1000]; // for this app, this will suffice.
    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    return std::string(buf);
}

std::string TimecodeStringFromTime(opentime::OPENTIME_VERSION::RationalTime time, bool drop_frame_mode) {
    opentime::ErrorStatus error_status;
    auto result = time.to_timecode(time.rate(), 
        drop_frame_mode ? opentime::OPENTIME_VERSION::IsDropFrameRate::InferFromRate 
                        : opentime::OPENTIME_VERSION::IsDropFrameRate::ForceNo, &error_status);
    if (opentime::is_error(error_status)) {
        // non-standard rates can't be turned into timecode
        // so fall back to a time string, which is HH:MM:SS.xxx
        // where xxx is a fraction instead of a :FF frame.
        return time.to_time_string();
    } else {
        return result;
    }
}


std::string FramesStringFromTime(opentime::OPENTIME_VERSION::RationalTime time) {
    return std::to_string(time.to_frames());
}

std::string SecondsStringFromTime(opentime::OPENTIME_VERSION::RationalTime time) {
    return Format("%.5f", time.to_seconds());
}

std::string FormattedStringFromTime(opentime::OPENTIME_VERSION::RationalTime time, uint32_t flags) {
    std::string result;
    if (flags & TIMECODE_RATE) {
        if (result != "")
            result += "\n";
        result += TimecodeStringFromTime(time);
    }
    if (flags & TIMECODE_FRAME) {
        if (result != "")
            result += "\n";
        result += FramesStringFromTime(time) + "f";
    }
    if (flags & TIMECODE_SECONDS) {
        if (result != "")
            result += "\n";
        result += SecondsStringFromTime(time) + "s";
    }
    if (flags & TIMECODE_DISPLAY_RATE) {
        if (result != "")
            result += "\n";
        result += "@" + Format("%.3f", time.rate());
    }
    return result;
}
