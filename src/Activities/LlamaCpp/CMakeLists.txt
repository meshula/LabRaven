
get_property(ACTIVITIES GLOBAL PROPERTY ACTIVITIES_LIST)
set_property(GLOBAL PROPERTY ACTIVITIES_LIST "${ACTIVITIES};LlamaCpp")

set(LLAMACPP_SRCS
    LlamaCppActivity.cpp
    LlamaCppActivity.hpp
)

target_sources(${PROJECT_NAME} PUBLIC ${LLAMACPP_SRCS})

