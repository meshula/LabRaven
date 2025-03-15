
get_property(PROVIDERS GLOBAL PROPERTY PROVIDERS_LIST)
set_property(GLOBAL PROPERTY PROVIDERS_LIST "${PROVIDERS};Scheme")

set(Scheme_SRCS
    s7.c s7.h
    s7-extensions.h s7-extensions.cpp
    SchemeProvider.cpp SchemeProvider.hpp
    ShaderTranspiler.cpp ShaderTranspiler.hpp
)

target_sources(${PROJECT_NAME} PUBLIC ${Scheme_SRCS})

set(SCHEME_RESOURCES
    shader-system.scm)

foreach(resource ${SCHEME_RESOURCES})
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${resource} ${CMAKE_BINARY_DIR}/${resource} COPYONLY)
endforeach()
