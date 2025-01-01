get_property(PROVIDERS GLOBAL PROPERTY PROVIDERS_LIST)
set_property(GLOBAL PROPERTY PROVIDERS_LIST "${PROVIDERS};OpenUSD")

set(OPENUSD_SRCS
    ${CMAKE_SOURCE_DIR}/src/Providers/OpenUSD/CreateDemoText.hpp
    ${CMAKE_SOURCE_DIR}/src/Providers/OpenUSD/CreateDemoText.cpp
    ${CMAKE_SOURCE_DIR}/src/Providers/OpenUSD/SpaceFillCurve.hpp
    ${CMAKE_SOURCE_DIR}/src/Providers/OpenUSD/OpenUSDProvider.hpp
    ${CMAKE_SOURCE_DIR}/src/Providers/OpenUSD/OpenUSDProvider.cpp
    ${CMAKE_SOURCE_DIR}/src/Providers/OpenUSD/UsdCreate.hpp
    ${CMAKE_SOURCE_DIR}/src/Providers/OpenUSD/UsdCreate.cpp
    ${CMAKE_SOURCE_DIR}/src/Providers/OpenUSD/UsdTemplater.hpp
    ${CMAKE_SOURCE_DIR}/src/Providers/OpenUSD/UsdTemplater.cpp
    ${CMAKE_SOURCE_DIR}/src/Providers/OpenUSD/UsdUtils.hpp
    ${CMAKE_SOURCE_DIR}/src/Providers/OpenUSD/UsdUtils.cpp
    ${CMAKE_SOURCE_DIR}/src/Providers/OpenUSD/UsdTemplater.hpp
    ${CMAKE_SOURCE_DIR}/ext/ImGuiHydraEditor/src/engine.h
    ${CMAKE_SOURCE_DIR}/ext/ImGuiHydraEditor/src/engine.cpp
    ${CMAKE_SOURCE_DIR}/ext/ImGuiHydraEditor/src/views/editor.h
    ${CMAKE_SOURCE_DIR}/ext/ImGuiHydraEditor/src/views/editor.cpp
    ${CMAKE_SOURCE_DIR}/ext/ImGuiHydraEditor/src/views/usdsessionlayer.h
    ${CMAKE_SOURCE_DIR}/ext/ImGuiHydraEditor/src/views/usdsessionlayer.cpp
    ${CMAKE_SOURCE_DIR}/ext/ImGuiHydraEditor/src/views/view.h
    ${CMAKE_SOURCE_DIR}/ext/ImGuiHydraEditor/src/views/view.cpp
    ${CMAKE_SOURCE_DIR}/ext/ImGuiHydraEditor/src/views/viewport.h
    ${CMAKE_SOURCE_DIR}/ext/ImGuiHydraEditor/src/views/viewport.cpp
    ${CMAKE_SOURCE_DIR}/ext/ImGuiHydraEditor/src/models/model.h
    ${CMAKE_SOURCE_DIR}/ext/ImGuiHydraEditor/src/models/model.cpp
    ${CMAKE_SOURCE_DIR}/ext/ImGuiHydraEditor/src/sceneindices/colorfiltersceneindex.h
    ${CMAKE_SOURCE_DIR}/ext/ImGuiHydraEditor/src/sceneindices/colorfiltersceneindex.cpp
    ${CMAKE_SOURCE_DIR}/ext/ImGuiHydraEditor/src/sceneindices/gridsceneindex.h
    ${CMAKE_SOURCE_DIR}/ext/ImGuiHydraEditor/src/sceneindices/gridsceneindex.cpp
    ${CMAKE_SOURCE_DIR}/ext/ImGuiHydraEditor/src/sceneindices/xformfiltersceneindex.h
    ${CMAKE_SOURCE_DIR}/ext/ImGuiHydraEditor/src/sceneindices/xformfiltersceneindex.cpp
    ${CMAKE_SOURCE_DIR}/src/Providers/OpenUSD/par_heman/src/color.c
    ${CMAKE_SOURCE_DIR}/src/Providers/OpenUSD/par_heman/src/draw.c
    ${CMAKE_SOURCE_DIR}/src/Providers/OpenUSD/par_heman/src/distance.c
    ${CMAKE_SOURCE_DIR}/src/Providers/OpenUSD/par_heman/src/gaussian.c
    ${CMAKE_SOURCE_DIR}/src/Providers/OpenUSD/par_heman/src/generate.c
    ${CMAKE_SOURCE_DIR}/src/Providers/OpenUSD/par_heman/src/image.c
    ${CMAKE_SOURCE_DIR}/src/Providers/OpenUSD/par_heman/src/lighting.c
    ${CMAKE_SOURCE_DIR}/src/Providers/OpenUSD/par_heman/src/noise.c
    ${CMAKE_SOURCE_DIR}/src/Providers/OpenUSD/par_heman/src/ops.c
    ${CMAKE_SOURCE_DIR}/src/Providers/OpenUSD/par_heman/src/points.c
    ${CMAKE_SOURCE_DIR}/src/Providers/OpenUSD/par_heman/kazmath/utility.c
    ${CMAKE_SOURCE_DIR}/src/Providers/OpenUSD/par_heman/kazmath/ray3.c
    ${CMAKE_SOURCE_DIR}/src/Providers/OpenUSD/par_heman/kazmath/vec2.c
    ${CMAKE_SOURCE_DIR}/src/Providers/OpenUSD/par_heman/kazmath/vec3.c
    ${CMAKE_SOURCE_DIR}/src/Providers/OpenUSD/par_heman/kazmath/vec4.c
)

target_sources(${PROJECT_NAME} PUBLIC ${OPENUSD_SRCS})
