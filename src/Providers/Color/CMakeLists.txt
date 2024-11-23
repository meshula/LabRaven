
# Platforrm specific sources
if(APPLE)
    set (PLATFORM_SRCS
        glfwColor.mm
    )
    # todo, windows and linux
endif()

set(COLOR_SRCS
    ${PLATFORM_SRCS}
    glfwColor.h
    nanocolor.h
    nanocolor.c
    nanocolorUtils.h
    nanocolorUtils.c
    WavelengthToRGB.h
    WavelengthToRGB.c
)

target_sources(${PROJECT_NAME} PUBLIC ${COLOR_SRCS})
