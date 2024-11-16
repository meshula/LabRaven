
# Get the current list of activities
get_property(ACTIVITIES GLOBAL PROPERTY ACTIVITIES_LIST)
set_property(GLOBAL PROPERTY ACTIVITIES_LIST "${ACTIVITIES};Color")

set(COLOR_SRCS
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

target_sources(${PROJECT_NAME} PUBLIC ${COLOR_SRCS})
#source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Labs/Color" FILES ${COLOR_SRCS})
