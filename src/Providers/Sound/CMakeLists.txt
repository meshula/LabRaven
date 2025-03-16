
get_property(PROVIDERS GLOBAL PROPERTY PROVIDERS_LIST)
set_property(GLOBAL PROPERTY PROVIDERS_LIST "${PROVIDERS};Sound")

set(Sound_SRCS
    SoundProvider.cpp SoundProvider.hpp)

target_sources(${PROJECT_NAME} PUBLIC ${Sound_SRCS})

# Get the current RESOURCES_LIST
get_property(EXISTING_RESOURCES GLOBAL PROPERTY RESOURCES_LIST)

# Add HRTF resources
file(GLOB HRTF_RESOURCES "${ROOT_DIR}/ext/LabSound/assets/hrtf/*.wav")
list(APPEND SOUND_RESOURCES "PUSH_DEST" "hrtf" ${HRTF_RESOURCES} "POP_DEST")

# Add samples resources
file(GLOB SAMPLES_RESOURCES "${ROOT_DIR}/ext/LabSound/assets/samples/*.*")
list(APPEND SOUND_RESOURCES "PUSH_DEST" "samples" ${SAMPLES_RESOURCES} "POP_DEST")

# Add cello_pluck resources
file(GLOB CELLO_RESOURCES "${ROOT_DIR}/ext/LabSound/assets/samples/cello_pluck/*")
list(APPEND SOUND_RESOURCES "PUSH_DEST" "samples/cello_pluck" ${CELLO_RESOURCES} "POP_DEST")

# Add impulse resources
file(GLOB IMPULSE_RESOURCES "${ROOT_DIR}/ext/LabSound/assets/impulse/*")
list(APPEND SOUND_RESOURCES "PUSH_DEST" "impulse" ${IMPULSE_RESOURCES} "POP_DEST")

# Update the global RESOURCES_LIST property with our sound resources
set_property(GLOBAL PROPERTY RESOURCES_LIST "${EXISTING_RESOURCES};${SOUND_RESOURCES}")
