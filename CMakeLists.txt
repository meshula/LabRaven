cmake_minimum_required(VERSION 3.26)

if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR})
  message(
    FATAL_ERROR
      "CMake generation is not allowed within the source directory!")
endif()

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}/cmake")
include(CheckLanguage)
include(MetalShaderSupport)
check_language(Metal)
if(CMAKE_Metal_COMPILER)
    enable_language(Metal)
endif()

#---------------------------------------------------
# Helper function to group sources by folder
#---------------------------------------------------

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

#---------------------------------------------------

# set the project name in a variable
set(PROJECT_NAME raven)

project(${PROJECT_NAME} VERSION 1.0)

#---------------------------------------------------
# options
if (APPLE)
    option(APPLE_MAKE_BUNDLE "Create a macOS bundle" ON)
endif()

option(HAVE_OPENUSD "Enable OpenUSD support" OFF)

if (HAVE_OPENUSD)
    find_package(pxr QUIET)
    if (pxr_FOUND)
        message(STATUS "OpenUSD support enabled, using pxr package found at ${pxr_DIR}")
    else()
        message(FATAL_ERROR "OpenUSD support is enabled, but the pxr package was not found. Please set the Pxr_DIR to point to the root of the USD installation.")
    endif()

    # garch needs OpenGL.
    find_package(OpenGL REQUIRED)
endif()

option(HAVE_MIDI "Enable MIDI support" ON)

#---------------------------------------------------
# add ext/libzmq as a subdirectory, with EXCLUDE_FROM_ALL.
# note: EXCLUDE_FROM_ALL is used to indicate that only referenced components
# should be built.
# options:
# ENABLE_DRAFTS ON
# WITH_TLS OFF ~ requires further investigation on how much burden WSS adds
# WITH_DOC OFF
# WITH_PERF_TOOL OFF
# BUILD_TESTS OFF
# BUILD_SHARED_LIBS OFF
# BUILD_STATIC_LIBS ON
# ENABLE_NO_EXPORT ON
# set the variables before including the subdirectory

set(ENABLE_DRAFTS ON CACHE BOOL "" FORCE)
set(WITH_TLS OFF CACHE BOOL "" FORCE)
set(WITH_DOC OFF CACHE BOOL "" FORCE)
set(WITH_PERF_TOOL OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
set(ENABLE_NO_EXPORT ON CACHE BOOL "" FORCE)
add_subdirectory("ext/libzmq" EXCLUDE_FROM_ALL)

#---------------------------------------------------

if(WIN32)
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE
            NOMINMAX
            _USE_MATH_DEFINES
    )
endif()

# add HAVE_OPENUSD set to 1 to the compile definitions
if (NOT HAVE_OPENUSD)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_NO_USD)
endif()

set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")

if (APPLE)
    enable_language(OBJC)
    enable_language(OBJCXX)
    enable_language(Metal)
    set(CMAKE_OBJC_FLAGS "${CMAKE_OBJC_FLAGS} -fobjc-arc")
    set(CMAKE_OBJCXX_FLAGS "${CMAKE_OBJCXX_FLAGS} -fobjc-arc")
endif()

message(STATUS ": Creating executable ${PROJECT_NAME}")
add_executable(${PROJECT_NAME})
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
target_include_directories(${PROJECT_NAME} PUBLIC 
    src
    ext
    ext/cppzmq
    ext/libzmq/include
    ext/Tilengine/include
    ext/ImGuiHydraEditor/src
    Imath::Imath
    ImGuizmo::ImGuizmo
    ${PXR_INCLUDE_DIRS})

if (APPLE)
    set(PLUGINS_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${PROJECT_NAME}.app/Contents/MacOS/plugins")
    set(RESOURCES_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${PROJECT_NAME}.app/Contents/Resources")
else()
    set(PLUGINS_DIR "${CMAKE_CURRENT_BINARY_DIR}/plugins")
    set(RESOURCES_DIR "${CMAKE_CURRENT_BINARY_DIR}/Resources")
endif()

if (APPLE)
    # enable ARC
    set_target_properties(${PROJECT_NAME} PROPERTIES
        XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC TRUE
    )
endif()

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

set_property(GLOBAL PROPERTY RESOURCES_LIST "")

add_subdirectory("src")
add_subdirectory("ext")

get_property(RESOURCES GLOBAL PROPERTY RESOURCES_LIST)

include_directories(
  ${PROJECT_SOURCE_DIR}/ext/nativefiledialog/src/include
)

GroupSourcesByFolder(${PROJECT_NAME})

add_subdirectory("ext/LabMidi")

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

set(HAVE_OPENGL NO)
if (HAVE_OPENGL)
    find_package(OpenGL REQUIRED) # Note: Hydra has no way to opt out of Metal GL Interop on apple.
    message(STATUS "Found OpenGL: ${OPENGL_LIBRARIES}")
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        USE_GLINTEROP=1
    )
    target_link_libraries(${PROJECT_NAME} PUBLIC
        ${OPENGL_LIBRARIES}
    )
endif()

target_link_libraries(${PROJECT_NAME} PUBLIC
    OTIO::opentimelineio
    ImGui::ImGui
    openexr
    sqlite3
    libzmq-static
    Tilengine
    ImGuizmo::ImGuizmo
    ImGuiColorTextEdit::ImGuiColorTextEdit
    ${PXR_LIBRARIES}
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    IMGUI_DEFINE_MATH_OPERATORS
)

if (HAVE_MIDI)
    target_link_libraries(${PROJECT_NAME} PUBLIC Lab::Midi)
endif()

if (APPLE)
    find_library(COREFOUNDATION_LIBRARY CoreFoundation)
    find_library(METAL_LIBRARY Metal)
    find_library(METALKIT_LIBRARY MetalKit)
    find_library(QUARTZCORE_LIBRARY QuartzCore)
    find_library(GAMECONTROLLER_LIBRARY GameController)
    target_link_libraries(${PROJECT_NAME} PUBLIC
        glfw
        nativefiledialog
        ${COREFOUNDATION_LIBRARY}
        ${GAMECONTROLLER_LIBRARY}
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

#---------------------------------------------------
# Plugin and Resources support
#---------------------------------------------------
# Define paths based on the platform

# Ensure the plugins directory exists
file(MAKE_DIRECTORY ${PLUGINS_DIR})
file(MAKE_DIRECTORY ${RESOURCES_DIR})

# List of files to copy to the plugins directory
set(PLUGIN_FILES
    # "${PROJECT_SOURCE_DIR}/README.md" nb. README is not executable content so it belongs in Resources.
    # Add more files as needed, like config.json, plugin_data.dat
)

# List to keep track of copied files
set(COPIED_FILES)

# Loop over each file in PLUGIN_FILES to copy them
foreach(file ${PLUGIN_FILES})
    # Extract the filename from the path
    get_filename_component(filename "${file}" NAME)
    
    add_custom_command(
        OUTPUT "${PLUGINS_DIR}/${filename}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${file}" "${PLUGINS_DIR}/${filename}"
        DEPENDS "${file}"
        COMMENT "Copying ${filename} to plugins directory"
    )
    list(APPEND COPIED_FILES "${PLUGINS_DIR}/${filename}")
endforeach()

# Loop over each file in RESOURCES to copy them
foreach(file ${RESOURCES})
    # Extract the filename from the path
    get_filename_component(filename "${file}" NAME)
    
    set(dest_file "${RESOURCES_DIR}/${filename}")
    add_custom_command(
        OUTPUT "${RESOURCES_DIR}/${filename}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${file}" "${dest_file}"
        DEPENDS "${file}"
        COMMENT "Copying ${filename} to resources directory"
    )
    list(APPEND COPIED_FILES "${dest_file}")
endforeach()

# Create a custom target that triggers the copy step
add_custom_target(copy_plugins ALL DEPENDS ${COPIED_FILES})

if (APPLE)
    # Set the path to the entitlements file
    set(ENTITLEMENTS_PATH "${CMAKE_SOURCE_DIR}/src/Lab.entitlements")
    set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${ENTITLEMENTS_PATH}")
    
    #set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Sign to Run Locally")
    #set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual")
endif()

# Ensure the plugin build depends on the copy and sign steps
add_dependencies(${PROJECT_NAME} copy_plugins)


get_target_property(DEFINITIONS ${PROJECT_NAME} COMPILE_DEFINITIONS)
message(STATUS "Definitions for ${PROJECT_NAME}: ${DEFINITIONS}")

