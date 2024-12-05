
#get_property(PROVIDERS GLOBAL PROPERTY PROVIDERS_LIST)
#set_property(GLOBAL PROPERTY PROVIDERS_LIST "${PROVIDERS};Metal")

# Platform specific sources
if(APPLE)
    set(METAL_SRCS
        MetalProvider.h MetalProvider.mm
    )

    target_sources(${PROJECT_NAME} PUBLIC ${METAL_SRCS})
endif()
