#---------------------------------------------------
# CMake configuration
#---------------------------------------------------

cmake_minimum_required(VERSION 3.26)

# USD must come from pxr_DIR, not cmake search cross the environment
set(CMAKE_FIND_USE_SYSTEM_ENVIRONMENT_PATH OFF)

if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR})
  message(
    FATAL_ERROR
      "CMake generation is not allowed within the source directory!")
endif()

cmake_policy(SET CMP0002 NEW)
cmake_policy(SET CMP0076 NEW)
cmake_policy(SET CMP0072 NEW)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}/cmake-scripts")
include(CheckLanguage)
include(MetalShaderSupport)
#include(GroupSourcesByFolder)
set(CMAKE_INSTALL_MESSAGE LAZY)

#---------------------------------------------------
# Project setup
#---------------------------------------------------

set(PROJECT_NAME raven)
project(${PROJECT_NAME} VERSION 1.0)
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

set(ROOT_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
set(EXT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ext")
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

#---------------------------------------------------
# options
#---------------------------------------------------

if (APPLE)
  option(APPLE_MAKE_BUNDLE "Create a macOS bundle" ON)
endif()
option(HAVE_MIDI "Enable MIDI support" ON)
option(HAVE_OPENEXR "Enable OpenEXR support" ON)
option(HAVE_SOUND "Enable sound support" ON)
option(HAVE_OTIO "Enable OpenTimelineIO support" ON)
option(HAVE_IMGUIZMO "Enable ImGuizmo support" ON)
option(HAVE_TILENGINE "Enable Tilengine support" ON)

#---------------------------------------------------

# ---------------------------------------------------
# TBB Diagnostics (before find_package(pxr))
# ---------------------------------------------------

message(STATUS "=== TBB Diagnostics (pre-pxr) ===")

# Check if the imported target already exists
if (TARGET TBB::tbb)
  message(WARNING "TBB::tbb already exists!")
  get_target_property(_inc TBB::tbb INTERFACE_INCLUDE_DIRECTORIES)
  get_target_property(_loc TBB::tbb IMPORTED_LOCATION)
  message(STATUS "  INCLUDE_DIRS: ${_inc}")
  message(STATUS "  LOCATION    : ${_loc}")
else()
  message(STATUS "No TBB::tbb target defined yet.")
endif()

# Check variables CMake might have cached
get_cmake_property(_vars VARIABLES)
foreach(v ${_vars})
  if (v MATCHES "^TBB")
    message(STATUS "  ${v} = ${${v}}")
  endif()
endforeach()

# print the pxr_DIR environment variable
if(DEFINED ENV{pxr_DIR})
  message(STATUS "pxr_DIR environment variable is set to: $ENV{pxr_DIR}")
else()
  fatal(STATUS "pxr_DIR environment variable is not set.")
endif()

#---------------------------------------------------
# OpenUSD support
#---------------------------------------------------
option(HAVE_OPENUSD "Enable OpenUSD support" ON)

if(HAVE_OPENUSD)
    # Prefer the -Dpxr_DIR=… command-line variable; fallback to environment
    if(NOT DEFINED pxr_DIR AND DEFINED ENV{pxr_DIR})
        set(pxr_DIR $ENV{pxr_DIR})
    endif()

    if(NOT DEFINED pxr_DIR)
        message(FATAL_ERROR "OpenUSD support is enabled but pxr_DIR is not set. Use -Dpxr_DIR=<path> or set the environment variable pxr_DIR.")
    endif()

    # Tell USD where OpenSubdiv lives so pxrConfig can find it
    set(OpenSubdiv_DIR "${pxr_DIR}/lib/cmake/OpenSubdiv")
    message(STATUS "Set OpenSubdiv_DIR at ${OpenSubdiv_DIR} for OpenUSD")

    # Prevent pxrConfig from redefining TBB::tbb
    set(PXR_FIND_TBB_IN_CONFIG OFF CACHE BOOL "Prevent pxrConfig from defining TBB::tbb" FORCE)

    # Find USD once
    find_package(pxr REQUIRED)
    message(STATUS "OpenUSD support enabled, using pxr package found at ${pxr_DIR}")

    # USD needs OpenGL
    find_package(OpenGL REQUIRED)
endif()


#---------------------------------------------------
# Apple set up
#---------------------------------------------------

if(APPLE)
  include(MetalShaderSupport)
  include(AppleCodeSigning)
  include(AppleMakeBundle)

  check_language(Metal)
  if(CMAKE_Metal_COMPILER)
    enable_language(Metal)
  endif()
  # Build arm64-only to match USD libraries
  set(CMAKE_OSX_ARCHITECTURES "arm64")

  enable_language(OBJC)
  enable_language(OBJCXX)
  enable_language(Metal)
  set(CMAKE_OBJC_FLAGS "${CMAKE_OBJC_FLAGS} -fobjc-arc")
  set(CMAKE_OBJCXX_FLAGS "${CMAKE_OBJCXX_FLAGS} -fobjc-arc")
endif()


#---------------------------------------------------
# Third party libraries
#---------------------------------------------------

add_subdirectory("ext")

#---------------------------------------------------
# Main products
#---------------------------------------------------

add_subdirectory("src")
