
target_sources(${PROJECT_NAME}
  PUBLIC
    Lab//App.h
    Lab/App.cpp
    Lab/AppTheme.h
    Lab/AppTheme.cpp

    Lab/LabDirectories.h
    Lab/LabDirectories.cpp
    Lab/LabFileDialogManager.hpp

    Lab/IconsFontaudio.h
    Lab/ImguiExt.hpp
    Lab/ImguiExt.cpp
    Lab/imgui_stdlib.h
    Lab/imgui_stdlib.cpp

    Lab/Landru.hpp
    Lab/Landru.cpp
    Lab/LabText.h
    Lab/LabText_odr.cpp

    Lab/StudioCore.hpp
    Lab/StudioCore.cpp

    resources/fonts/embedded_font.inc
)

# add HAVE_NO_USD preprocessor directive
if (NOT USD_FOUND)
  target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_NO_USD)
endif()

if(APPLE)
  target_sources(${PROJECT_NAME} PUBLIC 
    Lab/main_macos.mm
    Lab/LabFileDialogManager.mm
  )
elseif(WIN32)
  target_sources(${PROJECT_NAME} PUBLIC Lab/main_win32.cpp)
  target_compile_definitions(${PROJECT_NAME} PRIVATE -DUNICODE -D_UNICODE)
elseif(EMSCRIPTEN)
  target_sources(${PROJECT_NAME} PUBLIC Lab/main_emscripten.cpp)
  set(LIBS ${CMAKE_DL_LIBS} SDL2)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -s USE_SDL=2 -s DISABLE_EXCEPTION_CATCHING=1")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -s WASM=1 -s NO_EXIT_RUNTIME=0 -s ASSERTIONS=1 -s FETCH -s USE_PTHREADS=1 -s ALLOW_MEMORY_GROWTH=1 -s EXPORTED_RUNTIME_METHODS=ccall,cwrap -Wl,--shared-memory,--no-check-features --shell-file ${CMAKE_CURRENT_LIST_DIR}/shell_minimal.html")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DIMGUI_DISABLE_FILE_FUNCTIONS")
  set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX .html)
  target_link_libraries(${PROJECT_NAME} PUBLIC ${LIBS})
else()
  target_sources(${PROJECT_NAME} PUBLIC Lab/main_glfw.cpp)
endif()

# Set up to track the activities, for generating the registration code,
# and the plugins, for the signing system.
set_property(GLOBAL PROPERTY ACTIVITIES_LIST "")
get_property(ACTIVITIES GLOBAL PROPERTY ACTIVITIES_LIST)
set_property(GLOBAL PROPERTY PLUGINS_LIST "")
get_property(PLUGINS GLOBAL PROPERTY PLUGINS_LIST)

add_subdirectory(Labs)
add_subdirectory(Providers)

#---------------------------------------------------
# Create RegisterAllModes.cpp
#
# if ACTIVITIES is empty, report that.
get_property(ACTIVITIES GLOBAL PROPERTY ACTIVITIES_LIST)
if (NOT ACTIVITIES)
    message(FATAL_ERROR "Lab: No activities found")
endif()

set(ACTIVITY_INCLUDES "")
foreach(activity ${ACTIVITIES})
    string(APPEND ACTIVITY_INCLUDES "#include \"Labs/${activity}/${activity}Activity.hpp\"\n")
endforeach()

set(ACTIVITY_REGISTRATION "")
# List all activities
foreach(activity ${ACTIVITIES})
    string(APPEND ACTIVITY_REGISTRATION 
           "    orchestrator.RegisterActivity<${activity}Activity>([](){ return std::make_shared<${activity}Activity>(); });\n")
endforeach()

# Write the generated code to a file
file(WRITE "${CMAKE_BINARY_DIR}/RegisterAllActivities.cpp" "
#include \"Lab/StudioCore.hpp\"
${ACTIVITY_INCLUDES}
using namespace lab;
void RegisterAllActivities(Orchestrator& orchestrator) {
${ACTIVITY_REGISTRATION}
}
")

target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_BINARY_DIR}/RegisterAllActivities.cpp")
