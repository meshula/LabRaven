
if(APPLE)
    get_property(PROVIDERS GLOBAL PROPERTY PROVIDERS_LIST)
    set_property(GLOBAL PROPERTY PROVIDERS_LIST "${PROVIDERS};Lab/CoreProviders/Metal")
    set(METAL_SRCS
        MetalProvider.hpp MetalProvider.mm
    )
    target_sources(${PROJECT_NAME} PUBLIC ${METAL_SRCS})
endif()
