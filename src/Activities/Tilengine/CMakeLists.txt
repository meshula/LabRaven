
# Get the current list of activities
get_property(ACTIVITIES GLOBAL PROPERTY ACTIVITIES_LIST)
set_property(GLOBAL PROPERTY ACTIVITIES_LIST "${ACTIVITIES};Tilengine")

# Add Tilengine include directory to the global includes list
get_property(INCLUDES GLOBAL PROPERTY INCLUDES_LIST)
set_property(GLOBAL PROPERTY INCLUDES_LIST "${INCLUDES};${ROOT_DIR}/ext/Tilengine/include")

# Add Tilengine resources
get_property(RESOURCES GLOBAL PROPERTY RESOURCES_LIST)
file(GLOB_RECURSE TILENGINE_RESOURCES ${ROOT_DIR}/ext/Tilengine/samples/assets/tf4/*)
set_property(GLOBAL PROPERTY RESOURCES_LIST "${RESOURCES};${TILENGINE_RESOURCES}")

target_sources(${PROJECT_NAME} PUBLIC 
    TilengineActivity.hpp
    TilengineActivity.cpp
)
