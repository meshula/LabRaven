
set(OPENUSD_SRCS
    ${CMAKE_SOURCE_DIR}/src/Providers/OpenUSD/OpenUSDProvider.hpp
    ${CMAKE_SOURCE_DIR}/src/Providers/OpenUSD/OpenUSDProvider.cpp
    ${CMAKE_SOURCE_DIR}/src/Providers/OpenUSD/UsdSessionLayer.hpp
    ${CMAKE_SOURCE_DIR}/src/Providers/OpenUSD/UsdSessionLayer.cpp
    ${CMAKE_SOURCE_DIR}/ext/ImGuiHydraEditor/src/engine.h
    ${CMAKE_SOURCE_DIR}/ext/ImGuiHydraEditor/src/engine.cpp
    ${CMAKE_SOURCE_DIR}/ext/ImGuiHydraEditor/src/models/model.h
    ${CMAKE_SOURCE_DIR}/ext/ImGuiHydraEditor/src/models/model.cpp
    ${CMAKE_SOURCE_DIR}/ext/ImGuiHydraEditor/src/sceneindices/colorfiltersceneindex.h
    ${CMAKE_SOURCE_DIR}/ext/ImGuiHydraEditor/src/sceneindices/colorfiltersceneindex.cpp
    ${CMAKE_SOURCE_DIR}/ext/ImGuiHydraEditor/src/sceneindices/gridsceneindex.h
    ${CMAKE_SOURCE_DIR}/ext/ImGuiHydraEditor/src/sceneindices/gridsceneindex.cpp
    ${CMAKE_SOURCE_DIR}/ext/ImGuiHydraEditor/src/sceneindices/xformfiltersceneindex.h
    ${CMAKE_SOURCE_DIR}/ext/ImGuiHydraEditor/src/sceneindices/xformfiltersceneindex.cpp
)

target_sources(${PROJECT_NAME} PUBLIC ${OPENUSD_SRCS})
