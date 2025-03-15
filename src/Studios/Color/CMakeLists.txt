
get_property(STUDIOS GLOBAL PROPERTY STUDIO_LIST)
set_property(GLOBAL PROPERTY STUDIO_LIST "${STUDIOS};Color")

set(STUDIO_SRCS
    ColorStudio.cpp
    ColorStudio.hpp
)

target_sources(${PROJECT_NAME} PUBLIC ${STUDIO_SRCS})
