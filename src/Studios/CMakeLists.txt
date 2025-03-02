
add_subdirectory(Color)
add_subdirectory(Texture)

if (HAVE_OPENUSD)
add_subdirectory(OpenUSD)
add_subdirectory(OpenUSDInsights)
endif()

# if STUDIOS is empty, report that.
get_property(STUDIOS GLOBAL PROPERTY STUDIOS_LIST)
if (NOT STUDIOS)
    message(FATAL_ERROR "Lab: No studios found")
endif()
