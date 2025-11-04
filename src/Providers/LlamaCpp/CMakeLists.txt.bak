# LlamaCpp Provider - Llama.cpp server integration

get_property(PROVIDERS GLOBAL PROPERTY PROVIDERS_LIST)
set_property(GLOBAL PROPERTY PROVIDERS_LIST "${PROVIDERS};LlamaCpp")

# Find libcurl
find_package(CURL REQUIRED)

set(LLAMACPP_PROVIDER_SRCS
    LlamaCppProvider.cpp
    LlamaCppProvider.hpp
    CurlClient.cpp
    CurlClient.hpp
)

target_sources(${PROJECT_NAME} PUBLIC ${LLAMACPP_PROVIDER_SRCS})
target_link_libraries(${PROJECT_NAME} PUBLIC CURL::libcurl)