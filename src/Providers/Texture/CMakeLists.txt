
#get_property(PROVIDERS GLOBAL PROPERTY PROVIDERS_LIST)
#set_property(GLOBAL PROPERTY PROVIDERS_LIST "${PROVIDERS};Texturee")

# Platform specific sources
if(APPLE)
    set (PLATFORM_SRCS
        TextureCache.mm
    )
endif()

set(TEXTURE_SRCS
    ${PLATFORM_SRCS}
    TextureCache.cpp TextureCache.hpp
    ImageData.h
    stb_image.h stb_image_write.h stb_image_odr.c
)

target_sources(${PROJECT_NAME} PUBLIC ${TEXTURE_SRCS})
