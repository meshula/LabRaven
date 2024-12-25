
# Get the current list of activities
get_property(ACTIVITIES GLOBAL PROPERTY ACTIVITIES_LIST)
set_property(GLOBAL PROPERTY ACTIVITIES_LIST "${ACTIVITIES};Joystick")

target_sources(${PROJECT_NAME} PUBLIC 
    JoystickActivity.cpp
    JoystickActivity.hpp
)
