# make an ImGuizmo static library.

# ImGuizmo
project(ImGuizmo LANGUAGES CXX)
add_library(ImGuizmo STATIC
    ImGuizmo.cpp
    ImGuizmo.h)

set_property(TARGET ImGuizmo PROPERTY CXX_STANDARD 17)

# publish this directory as an include directory
target_include_directories(ImGuizmo PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# link to the ImGui library
target_link_libraries(ImGuizmo PUBLIC ImGui::ImGui)

# create an alias to make it easy to use
add_library(ImGuizmo::ImGuizmo ALIAS ImGuizmo)
