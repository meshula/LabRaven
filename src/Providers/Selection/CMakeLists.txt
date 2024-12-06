get_property(PROVIDERS GLOBAL PROPERTY PROVIDERS_LIST)
set_property(GLOBAL PROPERTY PROVIDERS_LIST "${PROVIDERS};Selection")

set(SELECTION_SRCS
    ${CMAKE_SOURCE_DIR}/src/Providers/Selection/SelectionProvider.hpp
    ${CMAKE_SOURCE_DIR}/src/Providers/Selection/SelectionProvider.cpp
)

target_sources(${PROJECT_NAME} PUBLIC ${SELECTION_SRCS})
