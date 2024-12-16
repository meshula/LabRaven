# Get the current list of activities
get_property(ACTIVITIES GLOBAL PROPERTY ACTIVITIES_LIST)
set_property(GLOBAL PROPERTY ACTIVITIES_LIST 
             "${ACTIVITIES};OpenUSD")

target_sources(${PROJECT_NAME} PUBLIC 
    OpenUSDActivity.cpp OpenUSDActivity.hpp
    UsdOutlinerActivity.cpp UsdOutlinerActivity.hpp
    TfDebugActivity.cpp TfDebugActivity.hpp
)
