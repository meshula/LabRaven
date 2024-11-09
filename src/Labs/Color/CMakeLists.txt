
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER "Labs/Color")

# Get the current list of activities
get_property(ACTIVITIES GLOBAL PROPERTY ACTIVITIES_LIST)
set_property(GLOBAL PROPERTY ACTIVITIES_LIST "${ACTIVITIES};Color")

target_sources(${PROJECT_NAME} PUBLIC 
    ColorActivity.cpp
    ColorActivity.hpp
    nanocolor.h
    nanocolor.c
    nanocolorUtils.h
    nanocolorUtils.c
    UIElements.hpp
    UIElements.cpp
    WavelengthToRGB.h
    WavelengthToRGB.c
)
