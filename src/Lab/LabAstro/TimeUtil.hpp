#ifndef LAB_LABASTRO_TIMEUTIL_HPP

#include <stdint.h>

namespace lab {

    struct Milliseconds {
        int64_t ms;
    };

    Milliseconds TimeSinceUnixEpoch();

}
#endif
