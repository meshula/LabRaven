
get_property(PROVIDERS GLOBAL PROPERTY PROVIDERS_LIST)
set_property(GLOBAL PROPERTY PROVIDERS_LIST "${PROVIDERS};Lab/CoreProviders/Color")

# Platform specific sources
if(APPLE)
    set (PLATFORM_SRCS
        glfwColor.mm
    )
    # todo, windows and linux
endif()

set(COLOR_SRCS
    ${PLATFORM_SRCS}
    ColorProvider.hpp ColorProvider.cpp
    glfwColor.h
    nanocolor.h
    nanocolor.c
    nanocolorUtils.h
    nanocolorUtils.c
    WavelengthToRGB.h
    WavelengthToRGB.c
)

target_sources(${PROJECT_NAME} PUBLIC ${COLOR_SRCS})
