set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Labs/Timeline")

# Get the current list of activities
get_property(ACTIVITIES GLOBAL PROPERTY ACTIVITIES_LIST)
set_property(GLOBAL PROPERTY ACTIVITIES_LIST "${ACTIVITIES};Timeline")

target_sources(${PROJECT_NAME} PUBLIC 
    TimelineActivity.hpp
    TimelineActivity.cpp
    TimelineProvider.hpp
)
