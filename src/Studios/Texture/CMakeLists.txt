
get_property(STUDIOS GLOBAL PROPERTY STUDIO_LIST)
set_property(GLOBAL PROPERTY STUDIO_LIST "${STUDIOS};Texture")

set(TEXTURE_SRCS
    TextureStudio.cpp
    TextureStudio.hpp
)

target_sources(${PROJECT_NAME} PUBLIC ${TEXTURE_SRCS})
