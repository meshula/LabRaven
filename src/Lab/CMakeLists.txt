
#---------------------------------------------------
# LabCore required dependencies
#---------------------------------------------------

#---------------------------------------------------
# add glfw
#---------------------------------------------------

if(NOT EMSCRIPTEN AND NOT WIN32)
  set(GLFW_BUILD_EXAMPLES OFF)
  set(GLFW_BUILD_TESTS OFF)
  set(GLFW_BUILD_DOCS OFF)
  set(BUILD_SHARED_LIBS OFF)
  add_subdirectory("glfw")
endif()

#---------------------------------------------------
# add nativefiledialog as a subdirectory
#---------------------------------------------------

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

#---------------------------------------------------
# add libzmq
#---------------------------------------------------

set(ENABLE_DRAFTS ON CACHE BOOL "" FORCE)
set(WITH_TLS OFF CACHE BOOL "" FORCE)
set(WITH_DOC OFF CACHE BOOL "" FORCE)
set(WITH_PERF_TOOL OFF CACHE BOOL "" FORCE)
set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_STATIC_LIBS ON CACHE BOOL "" FORCE)
set(ENABLE_NO_EXPORT ON CACHE BOOL "" FORCE)
add_subdirectory("libzmq" EXCLUDE_FROM_ALL)

#---------------------------------------------------
# add IMGUI
#---------------------------------------------------

add_library(IMGUI STATIC)
set_property(TARGET IMGUI PROPERTY CXX_STANDARD 14)

# set IMGUI_DIR to root relative path
set(IMGUI_DIR ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
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
  target_link_libraries(IMGUI glfw)
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
  target_include_directories(IMGUI PUBLIC gl3w)
  find_package(OpenGL REQUIRED)
  target_link_libraries(IMGUI OpenGL::GL ${CMAKE_DL_LIBS})
  target_compile_definitions(IMGUI PUBLIC 
        IMGUI_IMPL_OPENGL_LOADER_GL3W)
endif()

target_include_directories(IMGUI SYSTEM
  PUBLIC ${IMGUI_DIR}
  PUBLIC ${IMGUI_DIR}/backends
  PUBLIC ${IMGUI_DIR}/misc/cpp
  PUBLIC ${ROOT_DIR}/src/Lab/glfw/include
  PUBLIC ${ROOT_DIR}/src/Lab/glfw/include/GLFW
  PUBLIC ${IMPLOT_DIR}
  PUBLIC ${IMPLOT3D_DIR}
  PUBLIC ${TEXTEDIT_DIR}
)

# we need to
# #define ImDrawIdx unsigned int

target_compile_definitions(IMGUI PUBLIC 
    "IMGUI_DEFINE_MATH_OPERATORS")

# add an alias library
add_library(ImGui::ImGui ALIAS IMGUI)

#---------------------------------------------------
# add ImGuiColorTextEdit
#---------------------------------------------------

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

#---------------------------------------------------
# create binary to compressed
#---------------------------------------------------

if(NOT EMSCRIPTEN)
  # TODO: Figure out how to tell CMake to build binary_to_compressed_c
  # for the native platform, not cross-compile to WASM/JS with emscripten...
  add_executable(binary_to_compressed_c
      ${IMGUI_DIR}/misc/fonts/binary_to_compressed_c.cpp
  )
  set_property(TARGET binary_to_compressed_c PROPERTY CXX_STANDARD 14)
endif()

#---------------------------------------------------
# LabCore library
#---------------------------------------------------

if (APPLE)
    set (PLATFORM_SRCS
        LabDirectoriesObjC.mm
        LabFileDialogManager.mm
        LabJoystick.mm
    )
endif()

add_library(LabCore
    ${PLATFORM_SRCS}
    App.cpp App.h
    AppTheme.cpp AppTheme.h
    concurrentqueue.hpp
    CSP.cpp CSP.hpp
    IconsFontaudio.h
    imgui_stdlib.cpp imgui_stdlib.h
    ImguiExt.cpp ImguiExt.hpp
    LabDirectories.cpp LabDirectories.h
    LabFileDialogManager.hpp
    LabJoystick.h
    LabText_odr.cpp LabText.h
    Landru.cpp Landru.hpp
    StudioCore.cpp StudioCore.hpp
    tinycolormap.hpp
)

# add Core Providers
add_subdirectory(CoreProviders/Color)

target_include_directories(LabCore PUBLIC
    ${SRC_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/cppzmq
    ${CMAKE_CURRENT_SOURCE_DIR}/libzmq/include)

target_include_directories(LabCore PRIVATE
    nativefiledialog/src/include)

# add HAVE_OPENUSD set to 1 to the compile definitions
if (NOT HAVE_OPENUSD)
  target_compile_definitions(LabCore PRIVATE HAVE_NO_USD)
endif()

set_property(TARGET LabCore PROPERTY CXX_STANDARD 20)
set_property(TARGET LabCore PROPERTY C_STANDARD 11)

target_link_libraries(LabCore PUBLIC ImGui::ImGui)
target_link_libraries(LabCore PRIVATE libzmq-static)
target_link_libraries(LabCore PRIVATE nativefiledialog)
