# This file is used to build third-party libraries that are not available as CMake packages
# These libraries are built as static libraries and linked to the main project

if(UNIX AND NOT APPLE AND NOT EMSCRIPTEN)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
endif()

#---------------------------------------------------

add_library(sqlite3 STATIC)
target_sources(sqlite3 PRIVATE sqlite3/sqlite3.c)
target_include_directories(sqlite3 PUBLIC sqlite3)

#---------------------------------------------------

if(HAVE_OPENEXR)
    add_library(openexr STATIC)
    target_sources(openexr PRIVATE OpenEXR/openexr-c.h OpenEXR/openexr-c.c)
    target_include_directories(openexr PUBLIC OpenEXR)
    target_compile_definitions(openexr PUBLIC HAVE_OPENEXR)
endif()

#---------------------------------------------------

add_library(cgltf INTERFACE)
target_include_directories(cgltf INTERFACE cgltf)

#---------------------------------------------------

add_library(cgltf_vrm INTERFACE)
target_include_directories(cgltf_vrm INTERFACE cgltf_vrm)

#---------------------------------------------------

if(HAVE_TILENGINE)
    add_subdirectory(Tilengine)
endif()
if(HAVE_IMGUIZMO)
    add_subdirectory(ImGuizmo)
endif()

#---------------------------------------------------

if(HAVE_OTIO)
    # set up imath first
    set(OTIO_FIND_IMATH OFF)
    set(OTIO_BUILD_EXAMPLES OFF)
    set(OTIO_SHARED_LIBS OFF)
    set(BUILD_TESTING OFF)
    set(OTIO_DEPENDENCIES_INSTALL OFF)
    set(OTIO_INSTALL_PYTHON_MODULES OFF)
    set(OTIO_INSTALL_COMMANDLINE_TOOLS OFF)
    set(OTIO_INSTALL_CONTRIB OFF)
    add_subdirectory("opentimelineio")
endif()

#---------------------------------------------------

if(HAVE_SOUND)
    add_subdirectory(LabSound)
endif()

#---------------------------------------------------

if (HAVE_MIDI)
    # Prevent duplicate uninstall targets
    set(RTMIDI_TARGETNAME_UNINSTALL ON CACHE INTERNAL "rtmidi-uninstall")

    # don't build the rtmidi tests, RTMIDI_BUILD_TESTING
    set(RTMIDI_BUILD_TESTING OFF CACHE BOOL "Build rtmidi tests" FORCE)
    add_subdirectory("LabMidi")
endif()

#---------------------------------------------------

# Configure ozz-animation options - disable everything except core library
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build ozz as shared libraries" FORCE)
set(ozz_build_tools OFF CACHE BOOL "Build tools" FORCE)
set(ozz_build_fbx OFF CACHE BOOL "Build Fbx pipeline" FORCE)
set(ozz_build_gltf OFF CACHE BOOL "Build glTF importer" FORCE)
set(ozz_build_data OFF CACHE BOOL "Build data on code change" FORCE)
set(ozz_build_samples OFF CACHE BOOL "Build samples" FORCE)
set(ozz_build_howtos OFF CACHE BOOL "Build howtos" FORCE)
set(ozz_build_tests OFF CACHE BOOL "Build unit tests" FORCE)
set(ozz_build_simd_ref OFF CACHE BOOL "Force SIMD math reference implementation" FORCE)
set(ozz_build_postfix OFF CACHE BOOL "Use per config postfix name" FORCE)
set(ozz_build_msvc_rt_dll OFF CACHE BOOL "Select msvc DLL runtime library" FORCE)

add_subdirectory(ozz-animation EXCLUDE_FROM_ALL)
