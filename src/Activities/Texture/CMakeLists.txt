
# Get the current list of activities
get_property(ACTIVITIES GLOBAL PROPERTY ACTIVITIES_LIST)
set_property(GLOBAL PROPERTY ACTIVITIES_LIST "${ACTIVITIES};Texture")

if(APPLE)
    set(PLATFORM_SOURCES
        TextureActivity.mm
    )
endif()

target_sources(${PROJECT_NAME} PUBLIC 
    ${PLATFORM_SOURCES}
    TextureActivity.cpp
    TextureActivity.hpp
    imgui_tex_inspect/imgui_tex_inspect.h 
    imgui_tex_inspect/imgui_tex_inspect.cpp
)

get_property(METAL_SHADERS GLOBAL PROPERTY METAL_SHADERS_LIST)
list(APPEND METAL_SHADERS "${CMAKE_SOURCE_DIR}/src/Activities/Texture/TextureActivity.metal")
set_property(GLOBAL PROPERTY METAL_SHADERS_LIST "${METAL_SHADERS}")
