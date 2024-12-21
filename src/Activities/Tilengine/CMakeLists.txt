
# Get the current list of activities
get_property(ACTIVITIES GLOBAL PROPERTY ACTIVITIES_LIST)
set_property(GLOBAL PROPERTY ACTIVITIES_LIST "${ACTIVITIES};Tilengine")

target_sources(${PROJECT_NAME} PUBLIC 
    TilengineActivity.hpp
    TilengineActivity.cpp
)
