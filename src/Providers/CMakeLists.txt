
add_subdirectory(Color)
add_subdirectory(Texture)

if(APPLE)
    add_subdirectory(Metal)
endif()
if(HAVE_OPENUSD)
    add_subdirectory(OpenUSD)
endif()
