
# Get the current list of activities
get_property(ACTIVITIES GLOBAL PROPERTY ACTIVITIES_LIST)
set_property(GLOBAL PROPERTY ACTIVITIES_LIST "${ACTIVITIES};Camera")

set(CAMERA_SRCS
    CameraActivity.cpp CameraActivity.hpp
    LabCameraImGui.cpp LabCameraImGui.h
    imgui-knobs.cpp imgui-knobs.hpp
    LabGizmo.cpp LabGizmo.hpp
)

target_sources(${PROJECT_NAME} PUBLIC ${CAMERA_SRCS})
