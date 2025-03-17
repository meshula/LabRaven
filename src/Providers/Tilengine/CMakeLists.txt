
get_property(PROVIDERS GLOBAL PROPERTY PROVIDERS_LIST)
set_property(GLOBAL PROPERTY PROVIDERS_LIST "${PROVIDERS};Tilengine")

set(SAMPLE_SRCS 
    ${PROJECT_SOURCE_DIR}/ext/Tilengine/samples/Shooter.c
    ${PROJECT_SOURCE_DIR}/ext/Tilengine/samples/Actor.c
    ${PROJECT_SOURCE_DIR}/ext/Tilengine/samples/Boss.c
    ${PROJECT_SOURCE_DIR}/ext/Tilengine/samples/Enemy.c
    ${PROJECT_SOURCE_DIR}/ext/Tilengine/samples/Explosion.c
    ${PROJECT_SOURCE_DIR}/ext/Tilengine/samples/Ship.c
    ${PROJECT_SOURCE_DIR}/ext/Tilengine/samples/Sin.c
)

set(Tilengine_SRCS
    TilengineProvider.hpp TilengineProvider.cpp
)

set(Tilengine_resources
    ${PROJECT_SOURCE_DIR}/ext/Tilengine/samples/assets/tf4/FireLeo.png
    ${PROJECT_SOURCE_DIR}/ext/Tilengine/samples/assets/tf4/FireLeo.txt
    ${PROJECT_SOURCE_DIR}/ext/Tilengine/samples/assets/tf4/HellArm.png
    ${PROJECT_SOURCE_DIR}/ext/Tilengine/samples/assets/tf4/HellArm.txt
    ${PROJECT_SOURCE_DIR}/ext/Tilengine/samples/assets/tf4/TF4_bg1.png
    ${PROJECT_SOURCE_DIR}/ext/Tilengine/samples/assets/tf4/TF4_bg1.tmx
    ${PROJECT_SOURCE_DIR}/ext/Tilengine/samples/assets/tf4/TF4_bg1.tsx
    ${PROJECT_SOURCE_DIR}/ext/Tilengine/samples/assets/tf4/TF4_fg1.png
    ${PROJECT_SOURCE_DIR}/ext/Tilengine/samples/assets/tf4/TF4_fg1.tmx
    ${PROJECT_SOURCE_DIR}/ext/Tilengine/samples/assets/tf4/TF4_fg1.tsx
)

# add the resources to the global list
get_property(RESOURCES GLOBAL PROPERTY RESOURCES_LIST)
set_property(GLOBAL PROPERTY RESOURCES_LIST "${RESOURCES};${Tilengine_resources}")

target_sources(${PROJECT_NAME} PUBLIC ${Tilengine_SRCS} ${SAMPLE_SRCS})
