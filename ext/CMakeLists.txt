cmake_policy(SET CMP0076 NEW)
cmake_policy(SET CMP0072 NEW)

if(UNIX AND NOT APPLE AND NOT EMSCRIPTEN)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
endif()

####################################

if(NOT EMSCRIPTEN)
  add_library(nativefiledialog STATIC)
  
  set_property(TARGET nativefiledialog PROPERTY CXX_STANDARD 11)
  
  set(SOURCES nativefiledialog/src/nfd_common.c)
  set(INCLUDE_DIRS nativefiledialog/src nativefiledialog/src/include)
  set(LIBRARIES)
  if(APPLE)
      list(APPEND SOURCES nativefiledialog/src/nfd_cocoa.m)
  elseif(WIN32)
      list(APPEND SOURCES nativefiledialog/src/nfd_win.cpp)
  elseif(UNIX)
      list(APPEND SOURCES nativefiledialog/src/nfd_gtk.c)
      list(APPEND INCLUDE_DIRS ${GTK3_INCLUDE_DIRS})
      list(APPEND LIBRARIES ${GTK3_LIBRARIES})
  endif()
  
  target_sources(nativefiledialog PRIVATE ${SOURCES})
  target_include_directories(nativefiledialog PRIVATE ${INCLUDE_DIRS})
  target_link_libraries(nativefiledialog PRIVATE ${LIBRARIES})
endif()

####################################

add_library(sqlite3 STATIC)
target_sources(sqlite3 PRIVATE sqlite3/sqlite3.c)
target_include_directories(sqlite3 PUBLIC sqlite3)

####################################

add_library(openexr STATIC)
target_sources(openexr PRIVATE OpenEXR/openexr-c.h OpenEXR/openexr-c.c)
target_include_directories(openexr PUBLIC OpenEXR)

####################################
add_subdirectory(Tilengine)
add_subdirectory(ImGuizmo)
####################################

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
####################################

add_library(IMGUI STATIC)

set_property(TARGET IMGUI PROPERTY CXX_STANDARD 14)

set(IMGUI_DIR imgui)
set(IMPLOT_DIR implot)
set(IMPLOT3D_DIR implot3d)
set(TEXTEDIT_DIR ImGuiColorTextEdit)

target_sources(IMGUI
  PRIVATE
    # core
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/misc/cpp/imgui_stdlib.cpp

    # add-ons
    ${IMPLOT_DIR}/implot.cpp
    ${IMPLOT_DIR}/implot_demo.cpp
    ${IMPLOT_DIR}/implot_items.cpp
    ${IMPLOT3D_DIR}/implot3d.cpp
    ${IMPLOT3D_DIR}/implot3d_items.cpp
    ${IMPLOT3D_DIR}/implot3d_meshes.cpp
    ${TEXTEDIT_DIR}/TextEditor.cpp

  PUBLIC
    ${IMGUI_DIR}/imgui.h
    ${IMGUI_DIR}/imgui_internal.h
    ${IMPLOT_DIR}/implot.h
    ${IMPLOT3D_DIR}/implot3d.h
    ${IMPLOT3D_DIR}/implot3d_internal.h
    ${TEXTEDIT_DIR}/TextEditor.h
)

if(APPLE)
  target_sources(IMGUI
    PRIVATE
      # backends
      ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
      ${IMGUI_DIR}/backends/imgui_impl_metal.mm
      #gl3w/GL/gl3w.c
  )
elseif(WIN32)
  target_sources(IMGUI
  PRIVATE
    # backends
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
    ${IMGUI_DIR}/backends/imgui_impl_dx11.cpp
)
elseif(EMSCRIPTEN)
  target_sources(IMGUI
    PRIVATE
      # backends
      ${IMGUI_DIR}/backends/imgui_impl_sdl2.cpp
      ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
      #gl3w/GL/gl3w.c
  )
else()
  target_sources(IMGUI
    PRIVATE
      # backends
      ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
      ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
      gl3w/GL/gl3w.c
  )
  target_include_directories(IMGUI 
    PUBLIC gl3w)
  find_package(OpenGL REQUIRED)
  target_link_libraries(IMGUI OpenGL::GL ${CMAKE_DL_LIBS})
  target_compile_definitions(IMGUI PUBLIC 
        IMGUI_IMPL_OPENGL_LOADER_GL3W)
endif()

target_include_directories(IMGUI
  SYSTEM
  PUBLIC ${IMGUI_DIR}
  PUBLIC ${IMGUI_DIR}/backends
  PUBLIC ${IMGUI_DIR}/misc/cpp
  PUBLIC glfw/include
  PUBLIC ${IMPLOT_DIR}
  PUBLIC ${IMPLOT3D_DIR}
  PUBLIC ${TEXTEDIT_DIR}
)

target_compile_definitions(IMGUI PUBLIC 
    IMGUI_DEFINE_MATH_OPERATORS)

# add an alias library
add_library(ImGui::ImGui ALIAS IMGUI)

####################################

# create static library ImGuiTextEditor
add_library(imguicolortextedit STATIC)
set_property(TARGET imguicolortextedit PROPERTY CXX_STANDARD 14)
set(IMGUI_TEXTEDITOR_SRCS 
    ImGuiColorTextEdit/LanguageDefinitions.cpp 
    ImGuiColorTextEdit/TextEditor.h ImGuiColorTextEdit/TextEditor.cpp)
target_sources(imguicolortextedit PRIVATE ${IMGUI_TEXTEDITOR_SRCS})
target_include_directories(imguicolortextedit PUBLIC ImGuiColorTextEdit)

# link ImGuiTextEditor with ImGui
target_link_libraries(imguicolortextedit PUBLIC ImGui::ImGui)

# create alias
add_library(ImGuiColorTextEdit::ImGuiColorTextEdit ALIAS imguicolortextedit)


####################################

if(NOT EMSCRIPTEN)
  # TODO: Figure out how to tell CMake to build binary_to_compressed_c
  # for the native platform, not cross-compile to WASM/JS with emscripten...
  add_executable(binary_to_compressed_c
      ${IMGUI_DIR}/misc/fonts/binary_to_compressed_c.cpp
  )
  set_property(TARGET binary_to_compressed_c PROPERTY CXX_STANDARD 14)
endif()
