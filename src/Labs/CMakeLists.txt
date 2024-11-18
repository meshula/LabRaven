
add_subdirectory(Color)
add_subdirectory(Console)
add_subdirectory(Plugin)
add_subdirectory(Test)
add_subdirectory(Timeline)

# if ACTIVITIES is empty, report that.
get_property(ACTIVITIES GLOBAL PROPERTY ACTIVITIES_LIST)
if (NOT ACTIVITIES)
    message(FATAL_ERROR "Labs: No activities found")
endif()
