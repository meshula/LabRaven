
add_subdirectory(Color)
add_subdirectory(Console)
add_subdirectory(Journal)
add_subdirectory(Plugin)
add_subdirectory(Preferences)
add_subdirectory(Providers)
add_subdirectory(Test)
add_subdirectory(Texture)
add_subdirectory(Tilengine)
add_subdirectory(Timeline)

if (HAVE_OPENUSD)
    add_subdirectory(OpenUSD)
endif()

# if ACTIVITIES is empty, report that.
get_property(ACTIVITIES GLOBAL PROPERTY ACTIVITIES_LIST)
if (NOT ACTIVITIES)
    message(FATAL_ERROR "Labs: No activities found")
endif()
