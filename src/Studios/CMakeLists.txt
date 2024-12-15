
add_subdirectory(Texture)

# if STUDIOS is empty, report that.
get_property(STUDIOS GLOBAL PROPERTY STUDIOS_LIST)
if (NOT STUDIOS)
    message(FATAL_ERROR "Lab: No studios found")
endif()
