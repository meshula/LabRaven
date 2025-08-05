# Get the current list of activities
get_property(ACTIVITIES GLOBAL PROPERTY ACTIVITIES_LIST)
set_property(GLOBAL PROPERTY ACTIVITIES_LIST "${ACTIVITIES};NodeGraph")

target_sources(${PROJECT_NAME} PUBLIC 
    NodeGraphActivity.cpp
    NodeGraphActivity.hpp
)

# Add required include directories for nged
target_include_directories(${PROJECT_NAME} PUBLIC 
    ${CMAKE_SOURCE_DIR}/ext/nged
)
