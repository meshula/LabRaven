
#include "TextureCache.hpp"
#include "Lab/CoreProviders/Metal/MetalProvider.hpp"

#include <map>

namespace lab {

static std::map<std::shared_ptr<LabImageData_t>, int> g_textureMap;

int TextureCache::GetEncodedTexture(std::shared_ptr<LabImageData_t> image) {
    if (image == nullptr) {
        return -1;
    }

    auto it = g_textureMap.find(image);
    if (it != g_textureMap.end()) {
        return it->second;
    }

    LabMetalProvider* provider = [LabMetalProvider sharedInstance];
    if (provider == nil) {
        return -1;
    }

    switch (image->pixelType) {
        case LAB_PIXEL_UINT8:
            if (image->channelCount == 4) {
                int texture = [provider CreateRGBA8Texture:image->width height:image->height rgba_pixels:image->data];
                g_textureMap[image] = texture;
                return texture;
            }
            return -1;

        case LAB_PIXEL_HALF:
            if (image->channelCount == 4) {
                int texture = [provider CreateRGBAf16Texture:image->width height:image->height rgba_pixels:image->data];
                g_textureMap[image] = texture;
                return texture;
            }
            return -1;

        case LAB_PIXEL_FLOAT:
            if (image->channelCount == 1) {
                int texture = [provider CreateYf32Texture:image->width height:image->height rgba_pixels:image->data];
                g_textureMap[image] = texture;
                return texture;
            }
            else if (image->channelCount == 4) {
                int texture = [provider CreateRGBAf32Texture:image->width height:image->height rgba_pixels:image->data];
                g_textureMap[image] = texture;
                return texture;
            }
            return -1;

        default:
            return -1;
    }
}

} // namespace lab
