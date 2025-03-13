
#include "TimeUtil.hpp"
#include <chrono>

namespace lab {

    Milliseconds TimeSinceUnixEpoch() {
        auto now = std::chrono::steady_clock::now();
        return { (int64_t) std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count() };
    }

}
