get_property(PROVIDERS GLOBAL PROPERTY PROVIDERS_LIST)
set_property(GLOBAL PROPERTY PROVIDERS_LIST "${PROVIDERS};Assets")

set(Assets_SRCS
    ${CMAKE_SOURCE_DIR}/src/Providers/Assets/AssetsProvider.hpp
    ${CMAKE_SOURCE_DIR}/src/Providers/Assets/AssetsProvider.cpp
)

target_sources(${PROJECT_NAME} PUBLIC ${Assets_SRCS})
