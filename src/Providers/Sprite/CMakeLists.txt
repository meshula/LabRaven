get_property(PROVIDERS GLOBAL PROPERTY PROVIDERS_LIST)
set_property(GLOBAL PROPERTY PROVIDERS_LIST "${PROVIDERS};Sprite")

set(Sprite_SRCS
    cute_ase_sprite.h
    SpriteProvider.hpp SpriteProvider.cpp
    VoxelSprite.hpp VoxelSprite.cpp
)

target_sources(${PROJECT_NAME} PUBLIC ${Sprite_SRCS})
