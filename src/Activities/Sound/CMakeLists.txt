
# Get the current list of activities
get_property(ACTIVITIES GLOBAL PROPERTY ACTIVITIES_LIST)
set_property(GLOBAL PROPERTY ACTIVITIES_LIST "${ACTIVITIES};Sound")

target_sources(${PROJECT_NAME} PUBLIC
    AudioContextManager.cpp AudioContextManager.hpp
    SoundActivity.cpp SoundActivity.hpp
    PlaySoundFIle.cpp PlaySoundFile.hpp
)
