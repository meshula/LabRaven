
get_property(STUDIOS GLOBAL PROPERTY STUDIO_LIST)
set_property(GLOBAL PROPERTY STUDIO_LIST "${STUDIOS};OpenUSD")

set(OPENUSD_SRCS
    OpenUSDStudio.cpp
    OpenUSDStudio.hpp
)

target_sources(${PROJECT_NAME} PUBLIC ${OPENUSD_SRCS})
