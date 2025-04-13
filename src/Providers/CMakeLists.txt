
add_subdirectory(Assets)
add_subdirectory(Astro)
add_subdirectory(Camera)
add_subdirectory(Animation)
add_subdirectory(Scheme)
add_subdirectory(Selection)
add_subdirectory(Sprite)

if(HAVE_OPENUSD)
    add_subdirectory(OpenUSD)
endif()
if(HAVE_MIDI)
    add_subdirectory(Midi)
endif()
if(HAVE_SOUND)
    add_subdirectory(Sound)
endif()
if(HAVE_TILENGINE)
    add_subdirectory(Tilengine)
endif()

# Add Tilengine include directory to the global includes list
get_property(INCLUDES GLOBAL PROPERTY INCLUDES_LIST)
set_property(GLOBAL PROPERTY INCLUDES_LIST "${INCLUDES};${ROOT_DIR}/ext/Tilengine/include")

# if PROVIDERS is empty, report that.
get_property(PROVIDERS GLOBAL PROPERTY PROVIDERS_LIST)
if (NOT PROVIDERS)
    message(FATAL_ERROR "Labs: No providers found")
endif()
