xxx
# Get the current list of activities
get_property(ACTIVITIES GLOBAL PROPERTY ACTIVITIES_LIST)
set_property(GLOBAL PROPERTY ACTIVITIES_LIST "${ACTIVITIES};Tilengine")

get_property(RESOURCES GLOBAL PROPERTY RESOURCES_LIST)

file(GLOB_RECURSE TILENGINE_RESOURCES ${ROOT_DIR}/ext/Tilengine/samples/assets/tf4/*)
set_property(GLOBAL PROPERTY RESOURCES_LIST "${RESOURCES};${TILENGINE_RESOURCES}")

target_sources(${PROJECT_NAME} PUBLIC 
    TilengineActivity.hpp
    TilengineActivity.cpp
)
