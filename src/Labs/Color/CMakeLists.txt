
# Get the current list of activities
get_property(ACTIVITIES GLOBAL PROPERTY ACTIVITIES_LIST)
set_property(GLOBAL PROPERTY ACTIVITIES_LIST "${ACTIVITIES};Color")

set(COLOR_SRCS
    ColorActivity.cpp
    ColorActivity.hpp
    UIElements.hpp
    UIElements.cpp
)

target_sources(${PROJECT_NAME} PUBLIC ${COLOR_SRCS})
