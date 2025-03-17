#---------------------------------------------------
# Create the project executable
#---------------------------------------------------

message(STATUS ": Creating executable ${PROJECT_NAME}")
add_executable(${PROJECT_NAME})
set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)
target_compile_definitions(${PROJECT_NAME}
    PRIVATE BUILT_RESOURCE_PATH=${PROJECT_SOURCE_DIR})

if (APPLE)
    set(PLUGINS_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${PROJECT_NAME}.app/Contents/MacOS/plugins")
    set(RESOURCES_DIR "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${PROJECT_NAME}.app/Contents/Resources")
else()
    set(PLUGINS_DIR "${CMAKE_CURRENT_BINARY_DIR}/plugins")
    set(RESOURCES_DIR "${CMAKE_CURRENT_BINARY_DIR}/Resources")
endif()
  
# add HAVE_OPENUSD set to 1 to the compile definitions
if (NOT HAVE_OPENUSD)
    target_compile_definitions(${PROJECT_NAME} PRIVATE HAVE_NO_USD)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
    IMGUI_DEFINE_MATH_OPERATORS
)

install(TARGETS ${PROJECT_NAME}
    BUNDLE DESTINATION bin
    RUNTIME DESTINATION bin)

if (APPLE)
    # Set the path to the entitlements file
    set(ENTITLEMENTS_PATH "${CMAKE_SOURCE_DIR}/src/Lab.entitlements")
    set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${ENTITLEMENTS_PATH}")
  
    #set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Sign to Run Locally")
    #set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_STYLE "Manual")
  
    # Apply consistent code signing to the main application
    apply_apple_code_signing(${PROJECT_NAME})
  endif()
  
#---------------------------------------------------
# Generate resources and add them to the target
#---------------------------------------------------

if(NOT EMSCRIPTEN)
  add_custom_command(
    OUTPUT "${PROJECT_SOURCE_DIR}/fonts/embedded_font.inc"
    COMMAND binary_to_compressed_c -base85 "${PROJECT_SOURCE_DIR}/fonts/mononoki-Regular Nerd Font Complete.ttf" MononokiFont > ${PROJECT_SOURCE_DIR}/fonts/embedded_font.inc
    COMMENT "Embedding font..."
    MAIN_DEPENDENCY "fonts/mononoki-Regular Nerd Font Complete.ttf"
    VERBATIM)
endif()

target_sources(${PROJECT_NAME}
  PUBLIC
    resources/fonts/embedded_font.inc
)

# From usdtweak:
# Default ini file is also embedded
set(default_ini_file ${ROOT_DIR}/src/imgui.ini)
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

#---------------------------------------------------
# Add sources and includes
#---------------------------------------------------

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    #ext
    Lab/cppzmq
    Lab/libzmq/include
    #${ROOT_DIR}/ext/Tilengine/include
    #${ROOT_DIR}/ext/ImGuiHydraEditor/src
    #Imath::Imath
    #${ROOT_DIR}/ext/opentimelineio/src
    #ImGuizmo::ImGuizmo
    #${PXR_INCLUDE_DIRS}
)

if(APPLE)
    target_sources(${PROJECT_NAME} PUBLIC 
        Lab/main_macos.mm
    )

    # enable ARC
    set_target_properties(${PROJECT_NAME} PROPERTIES
          XCODE_ATTRIBUTE_CLANG_ENABLE_OBJC_ARC TRUE)

    # if there's a reason to run as a command line program without a bundle, set
    # APPLE_MAKE_BUNDLE to FALSE
    if (APPLE_MAKE_BUNDLE)
    # if there's an icon file:
    # apple_make_bundle(${PROJECT_NAME} "planetix" "${PROJECT_SOURCE_DIR}/src/Info.plist" "${PROJECT_SOURCE_DIR}/src/icon.icns")
    apple_make_bundle(${PROJECT_NAME} "planetix" "${PROJECT_SOURCE_DIR}/src/Info.plist")
    endif()
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

#---------------------------------------------------
# Set up the generators
#---------------------------------------------------

set_property(              GLOBAL PROPERTY ACTIVITIES_LIST "")
set_property(              GLOBAL PROPERTY METAL_SHADERS_LIST "")
set_property(              GLOBAL PROPERTY PLUGINS_LIST "")
set_property(              GLOBAL PROPERTY PROVIDERS_LIST "")
set_property(              GLOBAL PROPERTY RESOURCES_LIST "")
set_property(              GLOBAL PROPERTY STUDIO_LIST "")

#---------------------------------------------------
# Allow every component to add its sources to the generators
#---------------------------------------------------

add_subdirectory(Lab)
#add_subdirectory(Activities)
#add_subdirectory(Providers)
#add_subdirectory(Studios)

get_property(ACTIVITIES    GLOBAL PROPERTY ACTIVITIES_LIST)
get_property(METAL_SHADERS GLOBAL PROPERTY METAL_SHADERS_LIST)
get_property(PLUGINS       GLOBAL PROPERTY PLUGINS_LIST)
get_property(PROVIDERS     GLOBAL PROPERTY PROVIDERS_LIST)
get_property(RESOURCES     GLOBAL PROPERTY RESOURCES_LIST)
get_property(STUDIOS       GLOBAL PROPERTY STUDIO_LIST)

#---------------------------------------------------
# Add Activities
#---------------------------------------------------

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

#---------------------------------------------------
# Add Providers
#---------------------------------------------------

set(PROVIDER_REGISTRATION "")
set(PROVIDER_INCLUDES "")
foreach(provider ${PROVIDERS})
    # Trim whitespace from the provider name
    string(STRIP "${provider}" provider)
    
    # Extract the directory and class name
    string(FIND "${provider}" "/" DELIMITER_POS)
    if(DELIMITER_POS GREATER_EQUAL 0)
        get_filename_component(DIRECTORY "${provider}" DIRECTORY)
        get_filename_component(CLASS "${provider}" NAME)
        string(APPEND PROVIDER_INCLUDES "#include \"${DIRECTORY}/${CLASS}/${CLASS}Provider.hpp\"\n")
        string(APPEND PROVIDER_REGISTRATION 
            "    orchestrator.RegisterProvider<${CLASS}Provider>([](){ return ${CLASS}Provider::instance(); });\n")
    else()
        set(DIRECTORY "${provider}")
        set(CLASS_NAME "${provider}")
        string(APPEND PROVIDER_INCLUDES "#include \"Providers/${DIRECTORY}/${CLASS_NAME}Provider.hpp\"\n")
        string(APPEND PROVIDER_REGISTRATION 
            "    orchestrator.RegisterProvider<${provider}Provider>([](){ return ${provider}Provider::instance(); });\n")
    endif()
endforeach()

#---------------------------------------------------
# Add Studios
#---------------------------------------------------

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

#---------------------------------------------------
# Add Metal shaders
#---------------------------------------------------

if(APPLE)
    get_property(METAL_SHADERS GLOBAL PROPERTY METAL_SHADERS_LIST)
    if (NOT METAL_SHADERS OR METAL_SHADERS STREQUAL "")
        message(INFO ": No Metal shaders found")
    else()
    message(INFO ": Metal shaders found: ${METAL_SHADERS}")
    target_sources(${PROJECT_NAME} PRIVATE ${METAL_SHADERS})
    set_source_files_properties(${METAL_SHADERS} PROPERTIES LANGUAGE Metal)
    endif()
endif()

#---------------------------------------------------
# Generate the registration code
#---------------------------------------------------

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

#---------------------------------------------------
# Link libraries
#---------------------------------------------------

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

if(HAVE_SOUND)
    target_link_libraries(${PROJECT_NAME} PUBLIC 
        LabSound::LabSound
        LabSoundRtAudio::LabSoundRtAudio)

    if(APPLE)
        target_link_libraries(${PROJECT_NAME} PUBLIC
            "-framework AudioToolbox"
            "-framework AudioUnit"
            "-framework Accelerate"
            "-framework CoreAudio"
        )
    endif()
endif()

if(HAVE_OTIO)
    target_link_libraries(${PROJECT_NAME} PUBLIC OTIO::opentimelineio)
endif()
if (HAVE_IMGUIZMO)
    target_link_libraries(${PROJECT_NAME} PUBLIC ImGuizmo::ImGuizmo)
endif()
if (HAVE_OPENEXR)
    target_link_libraries(${PROJECT_NAME} PUBLIC openexr)
endif()
if (HAVE_TILENGINE)
    target_link_libraries(${PROJECT_NAME} PUBLIC Tilengine)
endif()
target_link_libraries(${PROJECT_NAME} PUBLIC
    LabCore
    sqlite3
    libzmq-static
    ImGuiColorTextEdit::ImGuiColorTextEdit
    ${PXR_LIBRARIES}
)

if (HAVE_MIDI)
  target_link_libraries(${PROJECT_NAME} PUBLIC Lab::Midi)
endif()

if (APPLE)
  target_link_libraries(${PROJECT_NAME} PUBLIC
        glfw
        "-framework Cocoa"
        "-framework CoreFoundation"
        "-framework CoreLocation"
        "-framework GameController"
        "-framework Metal"
        "-framework MetalKit"
        "-framework QuartzCore"
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

# Process the RESOURCES list which may contain special markers for subdirectories
set(current_dest_subdir "")
# Loop over each item in RESOURCES
foreach(item ${RESOURCES})
  if(item STREQUAL "PUSH_DEST")
    # Next item is the destination subdirectory
    set(next_is_dest TRUE)
  elseif(next_is_dest)
    set(current_dest_subdir "${item}")
    set(next_is_dest FALSE)
  elseif(item STREQUAL "POP_DEST")
    set(current_dest_subdir "")
  else()
    # This is a regular file to copy
    # Extract the filename from the path
    get_filename_component(filename "${item}" NAME)
    
    # Set destination based on current_dest_subdir
    if(current_dest_subdir)
      # Create the destination directory if it doesn't exist
      file(MAKE_DIRECTORY "${RESOURCES_DIR}/${current_dest_subdir}")
      set(dest_file "${RESOURCES_DIR}/${current_dest_subdir}/${filename}")
    else()
      set(dest_file "${RESOURCES_DIR}/${filename}")
    endif()
    
    add_custom_command(
          OUTPUT "${dest_file}"
          COMMAND ${CMAKE_COMMAND} -E copy_if_different "${item}" "${dest_file}"
          DEPENDS "${item}"
          COMMENT "Copying ${filename} to resources directory${current_dest_subdir}"
      )
    list(APPEND COPIED_FILES "${dest_file}")
  endif()
endforeach()

# Create a custom target that triggers the copy step
add_custom_target(copy_plugins ALL DEPENDS ${COPIED_FILES})

# Ensure the plugin build depends on the copy and sign steps
add_dependencies(${PROJECT_NAME} copy_plugins)

#---------------------------------------------------
# Group sources by folder
#---------------------------------------------------

#include(${CMAKE_SOURCE_DIR}/cmake/GroupSourcesByFolder.cmake)
#GroupSourcesByFolder(${PROJECT_NAME})
