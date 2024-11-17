
# Get the current list of activities
get_property(ACTIVITIES GLOBAL PROPERTY ACTIVITIES_LIST)
set_property(GLOBAL PROPERTY ACTIVITIES_LIST "${ACTIVITIES};Timeline")

target_sources(${PROJECT_NAME} PUBLIC 
    OpenTimelineIOProvider.cpp
    TimelineActivity.hpp
    TimelineActivity.cpp
    TimelineProvider.hpp
    Utilities.hpp
    Utilities.cpp
    UIElements.hpp
    UIElements.cpp
)
