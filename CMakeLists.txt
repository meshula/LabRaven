cmake_minimum_required(VERSION 3.26)

# set the project name in a variable
set(PROJECT_NAME raven)

project(${PROJECT_NAME} VERSION 1.0)

set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")

add_executable(${PROJECT_NAME})
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
target_include_directories(${PROJECT_NAME} PUBLIC src)

add_subdirectory("src")
add_subdirectory("ext")

include_directories(
  ${PROJECT_SOURCE_DIR}/ext/nativefiledialog/src/include
)

if(NOT EMSCRIPTEN AND NOT WIN32)
  set(GLFW_BUILD_EXAMPLES OFF)
  set(GLFW_BUILD_TESTS OFF)
  set(GLFW_BUILD_DOCS OFF)
  set(BUILD_SHARED_LIBS OFF)
  add_subdirectory("ext/glfw")
endif()

if(NOT EMSCRIPTEN)
  add_custom_command(
      OUTPUT "${PROJECT_SOURCE_DIR}/fonts/embedded_font.inc"
      COMMAND binary_to_compressed_c -base85 "${PROJECT_SOURCE_DIR}/fonts/mononoki-Regular Nerd Font Complete.ttf" MononokiFont > ${PROJECT_SOURCE_DIR}/fonts/embedded_font.inc
      COMMENT "Embedding font..."
      MAIN_DEPENDENCY "fonts/mononoki-Regular Nerd Font Complete.ttf"
      VERBATIM)
endif()

target_compile_definitions(${PROJECT_NAME}
    PRIVATE BUILT_RESOURCE_PATH=${PROJECT_SOURCE_DIR})

target_link_libraries(${PROJECT_NAME} PUBLIC
    IMGUI
)

if (APPLE)
  find_library(COREFOUNDATION_LIBRARY CoreFoundation)
  find_library(METAL_LIBRARY Metal)
  find_library(METALKIT_LIBRARY MetalKit)
  find_library(QUARTZCORE_LIBRARY QuartzCore)
  target_link_libraries(${PROJECT_NAME} PUBLIC
    glfw
    nativefiledialog
    ${COREFOUNDATION_LIBRARY}
    ${METAL_LIBRARY}
    ${METALKIT_LIBRARY}
    ${QUARTZCORE_LIBRARY}
  )
elseif(WIN32)
  target_link_libraries(${PROJECT_NAME} PUBLIC
    nativefiledialog
    d3d11.lib
    d3dcompiler.lib
    dwmapi.lib
  )
elseif(NOT EMSCRIPTEN)
  target_link_libraries(${PROJECT_NAME} PUBLIC
    glfw
    nativefiledialog
  )
endif()

install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION bin
    RUNTIME DESTINATION bin)
