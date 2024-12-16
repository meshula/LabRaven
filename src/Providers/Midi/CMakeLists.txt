
get_property(PROVIDERS GLOBAL PROPERTY PROVIDERS_LIST)
set_property(GLOBAL PROPERTY PROVIDERS_LIST "${PROVIDERS};Midi")

set(COLOR_SRCS
    MidiProvider.cpp
    MidiProvider.hpp
)

target_sources(${PROJECT_NAME} PUBLIC ${COLOR_SRCS})
