get_property(PROVIDERS GLOBAL PROPERTY PROVIDERS_LIST)
set_property(GLOBAL PROPERTY PROVIDERS_LIST "${PROVIDERS};OpenUSD")

set(USDTWEAK_SRCS
    ${CMAKE_SOURCE_DIR}/src/Providers/OpenUSD/usdtweakStub.cpp
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/commands/AttributeCommands.cpp
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/commands/EditorCommands.cpp
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/commands/CommandsImpl.h
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/commands/CommandsImpl.cpp
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/commands/CommandStack.h
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/commands/CommandStack.cpp
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/commands/SdfCommandGroup.h
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/commands/SdfCommandGroup.cpp
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/commands/SdfCommandGroupRecorder.h
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/commands/SdfCommandGroupRecorder.cpp
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/commands/SdfLayerInstructions.h
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/commands/SdfLayerInstructions.cpp
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/commands/SdfUndoRedoRecorder.h
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/commands/SdfUndoRedoRecorder.cpp
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/commands/UndoLayerStateDelegate.h
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/commands/UndoLayerStateDelegate.cpp
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/EditorSettings.h
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/EditorSettings.cpp
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/ImGuiHelpers.h
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/ImGuiHelpers.cpp
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/Selection.h
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/Selection.cpp
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/Style.h
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/Style.cpp
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/viewport/Viewport.h
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/viewport/Viewport.cpp
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/viewport/ViewportSettings.h
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/viewport/ViewportSettings.cpp
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/resources/ResourcesLoader.h
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/resources/ResourcesLoader.cpp
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/widgets/StageOutliner.h
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/widgets/StageOutliner.cpp)

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
    ${CMAKE_SOURCE_DIR}/src/Providers/OpenUSD/ProfilePrototype.hpp
    ${CMAKE_SOURCE_DIR}/src/Providers/OpenUSD/ProfilePrototype.cpp
    ${CMAKE_SOURCE_DIR}/src/Providers/OpenUSD/UsdUtils.hpp
    ${CMAKE_SOURCE_DIR}/src/Providers/OpenUSD/UsdUtils.cpp
    ${CMAKE_SOURCE_DIR}/src/Providers/OpenUSD/UsdTemplater.hpp
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
    ${CMAKE_SOURCE_DIR}/ext/ImGuiHydraEditor/src/engine.h
    ${CMAKE_SOURCE_DIR}/ext/ImGuiHydraEditor/src/engine.cpp
    ${CMAKE_SOURCE_DIR}/ext/ImGuiHydraEditor/src/sceneindices/colorfiltersceneindex.h
    ${CMAKE_SOURCE_DIR}/ext/ImGuiHydraEditor/src/sceneindices/colorfiltersceneindex.cpp
    ${CMAKE_SOURCE_DIR}/ext/ImGuiHydraEditor/src/sceneindices/gridsceneindex.h
    ${CMAKE_SOURCE_DIR}/ext/ImGuiHydraEditor/src/sceneindices/gridsceneindex.cpp
    ${CMAKE_SOURCE_DIR}/ext/ImGuiHydraEditor/src/sceneindices/xformfiltersceneindex.h
    ${CMAKE_SOURCE_DIR}/ext/ImGuiHydraEditor/src/sceneindices/xformfiltersceneindex.cpp
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/ext/ImGuiHydraEditor/src
)

target_sources(${PROJECT_NAME} PUBLIC ${OPENUSD_SRCS})

# create a static library to hold usdtweak, and link it to the main project
add_library(usdtweak STATIC ${USDTWEAK_SRCS})
target_include_directories(usdtweak PRIVATE
    ${CMAKE_SOURCE_DIR}/ext/imgui
    ${CMAKE_SOURCE_DIR}/ext/imgui/backends
    ${CMAKE_SOURCE_DIR}/ext/imgui/misc/cpp
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/commands
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/resources
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/viewport
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/widgets
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/3rdparty/fonts
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/3rdparty/iconfontcppheaders
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/3rdparty/wildcards
    ${CMAKE_SOURCE_DIR}/ext/usdtweak/src/3rdparty/fontawesome)

target_link_libraries(usdtweak PRIVATE
    ${PXR_LIBRARIES}
)

target_include_directories(usdtweak PRIVATE
    ${PXR_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src/Lab/imgui
    ${CMAKE_SOURCE_DIR}/src/Lab/imgui/misc/cpp
)

target_compile_definitions(usdtweak PRIVATE
    IMGUI_DEFINE_MATH_OPERATORS
)

set_property(TARGET usdtweak PROPERTY CXX_STANDARD 17)

target_link_libraries(${PROJECT_NAME} PRIVATE usdtweak)

# alias the library to usdtweak::usdtweak
add_library(usdtweak::usdtweak ALIAS usdtweak)
