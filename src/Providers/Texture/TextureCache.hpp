
#ifndef Providers_Texture_hpp
#define Providers_Texture_hpp

#include "ImageData.h"

namespace lab {

class TextureCache {
    struct data;
    data* _self;
    static TextureCache* _instance;

    LabImageData_t* ReadAndCacheEXR(const char* path); // caller does not own the returned pointer
    LabImageData_t* ReadAndCachePFM(const char* path); // caller does not own the returned pointer
    LabImageData_t* ReadAndCacheSTB(const char* path); // caller does not own the returned pointer

public:
    TextureCache();
    ~TextureCache();

    void Add(const char* name, LabImageData_t* image);
    void Erase(const char* name);
    LabImageData_t* Get(const char* name); // caller does not own the returned pointer
    void ExportCache(const char* path);

    LabImageData_t* ReadAndCache(const char* path); // caller does not own the returned pointer

    // on Mac and ios, the returned int can be provided to the MetalProvider to get a texture handle.
    int GetEncodedTexture(LabImageData_t* image);

    static TextureCache* instance();
};

} // lab

#endif // Providers_Texture_hpp