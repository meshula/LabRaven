get_property(PROVIDERS GLOBAL PROPERTY PROVIDERS_LIST)
set_property(GLOBAL PROPERTY PROVIDERS_LIST "${PROVIDERS};Camera")

set(CAMERA_PROVIDER_SRCS
    CameraProvider.cpp CameraProvider.hpp
    LabCamera.cpp LabCamera.h
)
target_sources(${PROJECT_NAME} PUBLIC ${CAMERA_PROVIDER_SRCS})
