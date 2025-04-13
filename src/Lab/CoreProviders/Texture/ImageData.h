#ifndef Providers_Texture_ImageData_h
#define Providers_Texture_ImageData_h

#include <stdint.h>
#include <stddef.h>

typedef enum
{
    LAB_PIXEL_UINT  = 0,
    LAB_PIXEL_HALF  = 1,
    LAB_PIXEL_FLOAT = 2,
    LAB_PIXEL_UINT8 = 3,
    Lab_PIXEL_LAST_TYPE
} LabPixelType_t;

typedef struct {
    uint8_t* data;
    size_t dataSize;
    LabPixelType_t pixelType;
    int channelCount; // 1 for luminance, 3 for RGB, 4 for RGBA
    int width, height;
    int dataWindowMinY, dataWindowMaxY;
} LabImageData_t;
    
#endif //Providers_Texture_ImageData_h
