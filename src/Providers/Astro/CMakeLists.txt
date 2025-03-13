


get_property(PROVIDERS GLOBAL PROPERTY PROVIDERS_LIST)
set_property(GLOBAL PROPERTY PROVIDERS_LIST "${PROVIDERS};Astro")

# Platform specific sources
if(APPLE)
    set (PLATFORM_SRCS
        # todo, windows and linux
        LabLocation.mm
    )
endif()

set(Astro_SRCS
    ${PLATFORM_SRCS}
    AstroProvider.cpp
    AstroProvider.hpp
    hosek_wilkie_skyModel/color_info.h 
    hosek_wilkie_skyModel/sky_model_data_ciexyz.h 
    hosek_wilkie_skyModel/sky_model_data_rgb.h 
    hosek_wilkie_skyModel/sky_model_data_spectral.h 
    hosek_wilkie_skyModel/sky_model.cpp 
    hosek_wilkie_skyModel/sky_model.h
    sun-sky/HosekDataXYZ.h
    sun-sky/stb_image_mini.h
    sun-sky/SunSky.cpp
    sun-sky/SunSky.h
    sun-sky/SunSkyTool.cpp
    sun-sky/VL234f.h
    astronomy.c
    astronomy.h
    LabLocation.h
    TimeUtil.hpp
)

target_sources(${PROJECT_NAME} PUBLIC ${Astro_SRCS})
