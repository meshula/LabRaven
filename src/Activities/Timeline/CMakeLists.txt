
# Get the current list of activities
get_property(ACTIVITIES GLOBAL PROPERTY ACTIVITIES_LIST)
set_property(GLOBAL PROPERTY ACTIVITIES_LIST "${ACTIVITIES};Timeline")

# Add Tilengine include directory to the global includes list
get_property(INCLUDES GLOBAL PROPERTY INCLUDES_LIST)
set_property(GLOBAL PROPERTY INCLUDES_LIST "${INCLUDES};${ROOT_DIR}/ext/opentimelineio/src")

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
