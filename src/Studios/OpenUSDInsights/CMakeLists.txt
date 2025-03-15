
get_property(STUDIOS GLOBAL PROPERTY STUDIO_LIST)
set_property(GLOBAL PROPERTY STUDIO_LIST "${STUDIOS};OpenUSDInsights")

set(OPENUSD_SRCS
    OpenUSDInsightsStudio.cpp
    OpenUSDInsightsStudio.hpp
)

target_sources(${PROJECT_NAME} PUBLIC ${OPENUSD_SRCS})
