get_property(PROVIDERS GLOBAL PROPERTY PROVIDERS_LIST)
set_property(GLOBAL PROPERTY PROVIDERS_LIST "${PROVIDERS};Animation")

set(ANIMATION_SRCS
    LabBVH.hpp
    LabBVH.cpp
    AnimationProvider.hpp
    AnimationProvider.cpp
)

target_sources(${PROJECT_NAME} PUBLIC ${ANIMATION_SRCS})

# Add include directories for cgltf and cgltf_vrm
target_include_directories(${PROJECT_NAME} PRIVATE 
    ${CMAKE_SOURCE_DIR}/ext
    ${CMAKE_SOURCE_DIR}/ext/cgltf
    ${CMAKE_SOURCE_DIR}/ext/cgltf_vrm
)

# Link required libraries
target_link_libraries(${PROJECT_NAME} PRIVATE 
    ozz_animation_offline 
    ozz_animation 
    ozz_base
    cgltf
    cgltf_vrm
)
