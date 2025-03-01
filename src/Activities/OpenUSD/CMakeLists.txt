# Get the current list of activities
get_property(ACTIVITIES GLOBAL PROPERTY ACTIVITIES_LIST)
set_property(GLOBAL PROPERTY ACTIVITIES_LIST 
             "${ACTIVITIES};\
             OpenUSD/OpenUSD;\
             OpenUSD/Hydra;\
             OpenUSD/Layers;\
             OpenUSD/Properties;\
             OpenUSD/Scout;\
             OpenUSD/Session;\
             OpenUSD/TfDebug;\
             OpenUSD/UsdCreate;\
             OpenUSD/UsdOutliner;\
             OpenUSD/Component")

target_sources(${PROJECT_NAME} PUBLIC 
    ComponentActivity.cpp ComponentActivity.hpp
    UsdFileDialogModules.hpp
    OpenUSDActivity.cpp OpenUSDActivity.hpp
    HydraActivity.cpp HydraActivity.hpp
    HydraEditor.cpp HydraEditor.hpp
    HydraViewport.cpp HydraViewport.hpp
    LayersActivity.cpp LayersActivity.hpp
    PropertiesActivity.cpp PropertiesActivity.hpp
    ScoutActivity.cpp ScoutActivity.hpp
    SessionActivity.cpp SessionActivity.hpp
    TfDebugActivity.cpp TfDebugActivity.hpp
    UsdCreateActivity.cpp UsdCreateActivity.hpp
    UsdOutlinerActivity.cpp UsdOutlinerActivity.hpp
    UsdSessionLayer.cpp UsdSessionLayer.hpp
)
