
get_property(STUDIOS GLOBAL PROPERTY STUDIO_LIST)
set_property(GLOBAL PROPERTY STUDIO_LIST "${STUDIOS};AI")

set(STUDIO_SRCS
    AIStudio.cpp
    AIStudio.hpp
)

target_sources(${PROJECT_NAME} PUBLIC ${STUDIO_SRCS})
