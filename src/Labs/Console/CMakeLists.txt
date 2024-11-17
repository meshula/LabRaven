set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Labs/Timeline")

# Get the current list of activities
get_property(ACTIVITIES GLOBAL PROPERTY ACTIVITIES_LIST)
set_property(GLOBAL PROPERTY ACTIVITIES_LIST "${ACTIVITIES};Console")

target_sources(${PROJECT_NAME} PUBLIC 
    ConsoleActivity.cpp
    ConsoleActivity.hpp
    imgui_console/imgui_console.h
    imgui_console/imgui_console.cpp
    csys/api.h
    csys/argument_parser.h
    csys/autocomplete.h
    csys/autocomplete.inl
    csys/command.h
    csys/csys.h
    csys/exceptions.h
    csys/history.h
    csys/history.inl
    csys/item.h
    csys/item.inl
    csys/script.h
    csys/script.inl
    csys/string.h
    csys/system.h
    csys/system.inl
)
