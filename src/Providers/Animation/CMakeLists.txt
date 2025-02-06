get_property(PROVIDERS GLOBAL PROPERTY PROVIDERS_LIST)
set_property(GLOBAL PROPERTY PROVIDERS_LIST "${PROVIDERS};Animation")

set(ANIMATION_SRCS
    AnimationProvider.hpp
    AnimationProvider.cpp
)

target_sources(${PROJECT_NAME} PUBLIC ${ANIMATION_SRCS})

# Link with ozz-animation libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ozz_animation
    ozz_animation_offline
    ozz_base
)

# Add include directories for ozz-animation
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/ext/ozz-animation/include
    ${CMAKE_SOURCE_DIR}/ext/ozz-animation/include/ozz
)
