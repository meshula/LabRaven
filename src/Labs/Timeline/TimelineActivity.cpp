
#include "TimelineActivity.hpp"

namespace lab {

struct TimelineActivity::data {
};

TimelineActivity::TimelineActivity()
: Activity() {
    _self = new TimelineActivity::data;
}

TimelineActivity::~TimelineActivity() {
    delete _self;
}

} // lab
