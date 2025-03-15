#---------------------------------------------------
# CMake configuration
#---------------------------------------------------

cmake_minimum_required(VERSION 3.26)

if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR})
  message(
    FATAL_ERROR
      "CMake generation is not allowed within the source directory!")
endif()

cmake_policy(SET CMP0002 NEW)
cmake_policy(SET CMP0076 NEW)
cmake_policy(SET CMP0072 NEW)

set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}/cmake")
include(CheckLanguage)
#include(GroupSourcesByFolder)

#---------------------------------------------------
# Project setup
#---------------------------------------------------

set(PROJECT_NAME raven)
project(${PROJECT_NAME} VERSION 1.0)

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
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")

    enable_language(OBJC)
    enable_language(OBJCXX)
    enable_language(Metal)
    set(CMAKE_OBJC_FLAGS "${CMAKE_OBJC_FLAGS} -fobjc-arc")
    set(CMAKE_OBJCXX_FLAGS "${CMAKE_OBJCXX_FLAGS} -fobjc-arc")
endif()

#---------------------------------------------------
# Windows/MSVC set up
#---------------------------------------------------

if(WIN32)
    target_compile_definitions(${PROJECT_NAME}
        PRIVATE
            NOMINMAX
            _USE_MATH_DEFINES
    )
endif()

#---------------------------------------------------
# Third party libraries
#---------------------------------------------------

add_subdirectory("ext")

#---------------------------------------------------
# Main products
#---------------------------------------------------

add_subdirectory("src")
