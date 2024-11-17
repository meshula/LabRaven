cmake_minimum_required(VERSION 3.26)

# set the project name in a variable
set(PROJECT_NAME raven)

project(${PROJECT_NAME} VERSION 1.0)

set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")

if (APPLE)
    set(APPLE_MAKE_BUNDLE ON)
endif()

add_executable(${PROJECT_NAME})
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
target_include_directories(${PROJECT_NAME} PUBLIC src)

# if Mac, set the MACOSX_BUNDLE property
# If a bundle is not created, then when running from xcode, or via
# double-clicking the executable in Finder, the app will launch via the
# terminal.app; under xcode this causes the app to launch twice under the
# current version of glfw. The reasons are unclear. Until this is resolved,
# the MACOSX_BUNDLE property is set to TRUE. If there's a reason to make the
# app a command line program without a bundle, the MACOX_BUNDLE property can
# be set to FALSE.
if (APPLE_MAKE_BUNDLE)
    set(PRODUCT_BUNDLE_IDENTIFIER "com.planetix.${PROJECT_NAME}")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_INFO_PLIST ${PROJECT_SOURCE_DIR}/src/Info.plist
        MACOSX_BUNDLE_GUI_IDENTIFIER "com.${PROJECT_NAME}.app"
        MACOSX_BUNDLE_BUNDLE_NAME ${PROJECT_NAME}
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION}
        MACOSX_BUNDLE_LONG_VERSION_STRING ${PROJECT_VERSION}
        XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${PRODUCT_BUNDLE_IDENTIFIER}"
        #MACOSX_BUNDLE_ICON_FILE ${PROJECT_SOURCE_DIR}/src/icon.icns
    )
endif()

add_subdirectory("src")
add_subdirectory("ext")

include_directories(
  ${PROJECT_SOURCE_DIR}/ext/nativefiledialog/src/include
)

# Organize sources into folders in IDEs nicely!
# function per https://github.com/TheLartians/GroupSourcesByFolder.cmake
function(GroupSourcesByFolder target)
  set(SOURCE_GROUP_DELIMITER "/")
  set(last_dir "")
  set(files "")

  get_target_property(sources ${target} SOURCES)

  foreach(file ${sources})
    file(RELATIVE_PATH relative_file "${PROJECT_SOURCE_DIR}" ${file})
    get_filename_component(dir "${relative_file}" PATH)
    if(NOT "${dir}" STREQUAL "${last_dir}")
      if(files)
        source_group("${last_dir}" FILES ${files})
      endif()
      set(files "")
    endif()
    set(files ${files} ${file})
    set(last_dir "${dir}")
  endforeach()

  if(files)
    source_group("${last_dir}" FILES ${files})
  endif()
endfunction()

GroupSourcesByFolder(${PROJECT_NAME})

set(OTIO_SHARED_LIBS OFF)
set(BUILD_TESTING OFF)
add_subdirectory("ext/opentimelineio")
include_directories(
  ${PROJECT_SOURCE_DIR}/ext/opentimelineio/src
  ${PROJECT_SOURCE_DIR}/ext/opentimelineio/src/deps
  ${PROJECT_SOURCE_DIR}/ext/opentimelineio/src/deps/optional-lite/include
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
  OTIO::opentimelineio
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

# During the installation step, create a directory named plugins in the same
# directory as the executable and copy README.md from the source root into it.
# On MacOS, this will be in the .app bundle.

if (APPLE_MAKE_BUNDLE)
    install(FILES "${PROJECT_SOURCE_DIR}/README.md"
        DESTINATION bin/${PROJECT_NAME}.app/Contents/MacOS/plugins)
else()
    install(FILES "${PROJECT_SOURCE_DIR}/README.md"
        DESTINATION bin/plugins)
endif()
