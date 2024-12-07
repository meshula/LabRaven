get_property(PROVIDERS GLOBAL PROPERTY PROVIDERS_LIST)
set_property(GLOBAL PROPERTY PROVIDERS_LIST "${PROVIDERS};Sprite")

set(Sprite_SRCS
${CMAKE_SOURCE_DIR}/src/Providers/Sprite/cute_ase_sprite.h
${CMAKE_SOURCE_DIR}/src/Providers/Sprite/SpriteProvider.hpp
${CMAKE_SOURCE_DIR}/src/Providers/Sprite/SpriteProvider.cpp
)

target_sources(${PROJECT_NAME} PUBLIC ${Sprite_SRCS})
