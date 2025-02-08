get_property(PROVIDERS GLOBAL PROPERTY PROVIDERS_LIST)
set_property(GLOBAL PROPERTY PROVIDERS_LIST "${PROVIDERS};Animation")

set(ANIMATION_SRCS
    LabBVH.hpp
    LabBVH.cpp
    AnimationProvider.hpp
    AnimationProvider.cpp
)

target_sources(${PROJECT_NAME} PUBLIC ${ANIMATION_SRCS})

# Link ozz-animation libraries
target_link_libraries(${PROJECT_NAME} PRIVATE ozz_animation_offline ozz_animation ozz_base)
