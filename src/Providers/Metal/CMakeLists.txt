
# Platforrm specific sources
if(APPLE)
    set(METAL_SRCS
        MetalProvider.h MetalProvider.mm
    )

    target_sources(${PROJECT_NAME} PUBLIC ${METAL_SRCS})
endif()
