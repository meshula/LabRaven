
add_subdirectory(Assets)
add_subdirectory(Color)
add_subdirectory(Animation)
add_subdirectory(Selection)
add_subdirectory(Sprite)
add_subdirectory(Texture)
add_subdirectory(Tilengine)

if(APPLE)
    add_subdirectory(Metal)
endif()
if(HAVE_OPENUSD)
    add_subdirectory(OpenUSD)
endif()
if(HAVE_MIDI)
    add_subdirectory(Midi)
endif()

# if PROVIDERS is empty, report that.
get_property(PROVIDERS GLOBAL PROPERTY PROVIDERS_LIST)
if (NOT PROVIDERS)
    message(FATAL_ERROR "Labs: No providers found")
endif()
