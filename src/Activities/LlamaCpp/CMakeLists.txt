add_library(LlamaCppActivity STATIC
    LlamaCppActivity.cpp
    LlamaCppActivity.hpp
)

# Set C++ standard to match LabCore (required for USD headers)
set_property(TARGET LlamaCppActivity PROPERTY CXX_STANDARD 20)

target_link_libraries(LlamaCppActivity
    Lab
)

# Platform-specific network libraries
if(WIN32)
    target_link_libraries(LlamaCppActivity ws2_32)
endif()

target_include_directories(LlamaCppActivity PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Register the activity for auto-registration
set_property(TARGET LlamaCppActivity PROPERTY ACTIVITY_CLASS "LlamaCppActivity")
