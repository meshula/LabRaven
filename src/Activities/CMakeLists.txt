
add_subdirectory(App)
add_subdirectory(Color)
add_subdirectory(Console)
add_subdirectory(Joystick)
add_subdirectory(Journal)
add_subdirectory(Plugin)
add_subdirectory(Preferences)
add_subdirectory(Providers)
add_subdirectory(Sprite)
add_subdirectory(Test)
add_subdirectory(Texture)
add_subdirectory(Tilengine)
add_subdirectory(Timeline)

if (HAVE_OPENUSD)
    # strictly speaking the camera mode doesn't really need USD, but in practice
    # it's being used to perform a bunch of camera operations that are useful.
    # If there's another activity in the future that can use a non-usd camera,
    # then we can move the camera activity out of the USD block.
    add_subdirectory(Camera)
    add_subdirectory(OpenUSD)
endif()

# if ACTIVITIES is empty, report that.
get_property(ACTIVITIES GLOBAL PROPERTY ACTIVITIES_LIST)
if (NOT ACTIVITIES)
    message(FATAL_ERROR "Labs: No activities found")
endif()
