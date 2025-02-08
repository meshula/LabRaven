# Get the current list of activities
get_property(ACTIVITIES GLOBAL PROPERTY ACTIVITIES_LIST)
set_property(GLOBAL PROPERTY ACTIVITIES_LIST "${ACTIVITIES};Animation")

target_sources(${PROJECT_NAME} PUBLIC 
    AnimationActivity.cpp
    AnimationActivity.hpp
)

# Add required include directories
target_include_directories(${PROJECT_NAME} PUBLIC 
    ${CMAKE_SOURCE_DIR}/ext/implot3d
    ${CMAKE_SOURCE_DIR}/ext/ozz-animation/include
)
