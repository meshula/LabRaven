
# Get the current list of activities
get_property(ACTIVITIES GLOBAL PROPERTY ACTIVITIES_LIST)
set_property(GLOBAL PROPERTY ACTIVITIES_LIST "${ACTIVITIES};Plugin")

target_sources(${PROJECT_NAME} PUBLIC 
    PluginActivity.cpp
    PluginActivity.hpp
)
