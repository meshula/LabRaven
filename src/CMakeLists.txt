
target_sources(${PROJECT_NAME}
  PUBLIC
    Lab//App.h Lab/App.cpp
    Lab/AppTheme.h Lab/AppTheme.cpp

    Lab/CSP.hpp Lab/CSP.cpp

    Lab/LabDirectories.h Lab/LabDirectories.cpp
    Lab/LabFileDialogManager.hpp

    Lab/IconsFontaudio.h
    Lab/ImguiExt.hpp Lab/ImguiExt.cpp
    Lab/imgui_stdlib.h Lab/imgui_stdlib.cpp

    Lab/LabJoystick.h

    Lab/Landru.hpp Lab/Landru.cpp
    Lab/LabText.h Lab/LabText_odr.cpp

    Lab/StudioCore.hpp Lab/StudioCore.cpp

    resources/fonts/embedded_font.inc
)

if(APPLE)
    target_sources(${PROJECT_NAME} PUBLIC 
        Lab/main_macos.mm
        Lab/LabFileDialogManager.mm
        Lab/LabJoystick.mm
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
set_property(GLOBAL PROPERTY PROVIDERS_LIST "")
get_property(PROVIDES GLOBAL PROPERTY PROVIDERS_LIST)
set_property(GLOBAL PROPERTY PLUGINS_LIST "")
get_property(PLUGINS GLOBAL PROPERTY PLUGINS_LIST)
set_property(GLOBAL PROPERTY METAL_SHADERS_LIST "")
get_property(METAL_SHADERS GLOBAL PROPERTY METAL_SHADERS_LIST)
set_property(GLOBAL PROPERTY STUDIO_LIST "")
get_property(STUDIOS GLOBAL PROPERTY STUDIO_LIST)

add_subdirectory(Activities)
add_subdirectory(Providers)
add_subdirectory(Studios)

#---------------------------------------------------
get_property(METAL_SHADERS GLOBAL PROPERTY METAL_SHADERS_LIST)
if (NOT METAL_SHADERS OR METAL_SHADERS STREQUAL "")
    message(INFO ": No Metal shaders found")
else()
  message(INFO ": Metal shaders found: ${METAL_SHADERS}")
  target_sources(${PROJECT_NAME} PRIVATE ${METAL_SHADERS})
  set_source_files_properties(${METAL_SHADERS} PROPERTIES LANGUAGE Metal)
endif()

#---------------------------------------------------
# Create RegisterAllModes.cpp
#
# if ACTIVITIES is empty, report that.
get_property(ACTIVITIES GLOBAL PROPERTY ACTIVITIES_LIST)
if (NOT ACTIVITIES)
    message(FATAL_ERROR "Lab: No activities found")
endif()

# if PROVIDERS is empty, report that.
get_property(PROVIDERS GLOBAL PROPERTY PROVIDERS_LIST)
if (NOT PROVIDERS)
    message(FATAL_ERROR "Lab: No providers found")
endif()

# if STUDIOS is empty, report that.
get_property(STUDIOS GLOBAL PROPERTY STUDIOS_LIST)
if (NOT STUDIOS)
    message(FATAL_ERROR "Lab: No studios found")
endif()

foreach(activity ${ACTIVITIES})
    # Trim whitespace from the activity name
    string(STRIP "${activity}" activity)
    
    # Extract the directory and class name
    string(FIND "${activity}" "/" DELIMITER_POS)
    if(DELIMITER_POS GREATER_EQUAL 0)
        string(SUBSTRING "${activity}" 0 ${DELIMITER_POS} DIRECTORY)
        string(SUBSTRING "${activity}" ${DELIMITER_POS} -1 CLASS_NAME)
        string(REPLACE "/" "" CLASS_NAME "${CLASS_NAME}") # Remove any slashes
    else()
        set(DIRECTORY "${activity}")
        set(CLASS_NAME "${activity}")
    endif()

    # Generate include paths
    string(APPEND ACTIVITY_INCLUDES "#include \"Activities/${DIRECTORY}/${CLASS_NAME}Activity.hpp\"\n")

    # Generate activity registration code
    string(APPEND ACTIVITY_REGISTRATION 
           "    orchestrator.RegisterActivity<${CLASS_NAME}Activity>([](){ return std::make_shared<${CLASS_NAME}Activity>(); });\n")
endforeach()

set(PROVIDER_REGISTRATION "")
# List all providers
foreach(provider ${PROVIDERS})
    string(APPEND PROVIDER_REGISTRATION 
           "    orchestrator.RegisterProvider<${provider}Provider>([](){ return ${provider}Provider::instance(); });\n")
endforeach()

set(PROVIDER_INCLUDES "")
foreach(provider ${PROVIDERS})
    string(APPEND PROVIDER_INCLUDES "#include \"Providers/${provider}/${provider}Provider.hpp\"\n")
endforeach()

set(STUDIO_REGISTRATION "")
# List all studios
foreach(studio ${STUDIOS})
    string(APPEND STUDIO_REGISTRATION 
           "    orchestrator.RegisterStudio<${studio}Studio>([](){ return std::make_shared<${studio}Studio>(); });\n")
endforeach()

set(STUDIO_INCLUDES "")
foreach(studio ${STUDIOS})
    string(APPEND STUDIO_INCLUDES "#include \"Studios/${studio}/${studio}Studio.hpp\"\n")
endforeach()

# Write the generated code to a file
file(WRITE "${CMAKE_BINARY_DIR}/RegisterAllActivities.cpp" "
#include \"Lab/StudioCore.hpp\"
${ACTIVITY_INCLUDES}
${PROVIDER_INCLUDES}
${STUDIO_INCLUDES}
using namespace lab;
void RegisterAllActivities(Orchestrator& orchestrator) {
${ACTIVITY_REGISTRATION}
${PROVIDER_REGISTRATION}
${STUDIO_REGISTRATION}
}
")

target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_BINARY_DIR}/RegisterAllActivities.cpp")


# From usdtweak:
# Default ini file is also embedded
set(default_ini_file ${CMAKE_CURRENT_SOURCE_DIR}/ini/imgui.ini)
set(default_ini_header ${CMAKE_CURRENT_SOURCE_DIR}/DefaultImGuiIni.h)
add_custom_command(
    OUTPUT "${default_ini_header}"
    DEPENDS "${default_ini_file}"
    COMMAND "${CMAKE_COMMAND}"
            "-DSOURCE_FILE=${default_ini_file}"
            "-DDESTINATION_FILE=${default_ini_header}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/embed_text.cmake"
    COMMENT "Embedding ${default_ini_file}"
)
# not really necessary, but useful in the IDE
target_sources(${PROJECT_NAME} PRIVATE ${default_ini_header})
