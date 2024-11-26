
#include "TextureCache.hpp"
#include "openexr-c.h"
#include "stb_image.h"
#include <map>
#include <string>

namespace lab {

struct CacheEntry {
    std::string path;
    std::shared_ptr<LabImageData_t> image;
};

struct TextureCache::data {
    std::map<std::string, CacheEntry> cache;
};

TextureCache* TextureCache::_instance = nullptr;

TextureCache::TextureCache() {
    _self = new TextureCache::data();
    _instance = this;
}

TextureCache* TextureCache::instance() {
    if (!_instance)
        new TextureCache();
    return _instance;
}

TextureCache::~TextureCache() {
    delete _self;
}

void TextureCache::Add(const char* name, std::shared_ptr<LabImageData_t> image) {
    auto i = _self->cache.find(name);
    if (i != _self->cache.end()) {
        i->second = CacheEntry{std::string(name), image};
    }
    else {
        _self->cache[name] = CacheEntry{std::string(name), image};;
    }
}

void TextureCache::Erase(const char* name) {
    auto i = _self->cache.find(name);
    if (i != _self->cache.end()) {
        _self->cache.erase(i);
    }
}

std::shared_ptr<LabImageData_t> TextureCache::Get(const char* name) {
    auto i = _self->cache.find(name);
    if (i != _self->cache.end()) {
        return i->second.image;
    }
    return nullptr;
}

namespace {
    int64_t exr_AssetRead_Func(
            exr_const_context_t ctxt,
            void*    userdata,
            void*    buffer,
            uint64_t sz,
            uint64_t offset,
            exr_stream_error_func_ptr_t error_cb)
    {
        FILE* f = (FILE*) userdata;
        if (!f)
            return -1;
        
        fseek(f, offset, SEEK_SET);
        return fread(buffer, 1, sz, f);
    }
}

std::shared_ptr<LabImageData_t> TextureCache::ReadAndCacheEXR(const char* path) {
    std::shared_ptr<LabImageData_t> imgData = Get(path);
    if (imgData && imgData->channelCount != 0)
        return imgData;

    FILE* f = fopen(path, "rb");
    if (!f)
        return nullptr;

    nanoexr_Reader_t exrReader;
    nanoexr_set_defaults(path, &exrReader);
    const int partIndex = 0;
    const int channelsToRead = 4;
    
    exr_result_t rv = nanoexr_read_header(&exrReader, exr_AssetRead_Func,
                                          nullptr, f,
                                          partIndex);
    
    printf("Mips found: %d\n", exrReader.numMipLevels);

    std::shared_ptr<LabImageData_t> img0 = nullptr;
    for (int mip = 0; mip < exrReader.numMipLevels; ++mip) {
        nanoexr_ImageData_t nimg;
        rv = nanoexr_read_exr(path,
                              exr_AssetRead_Func, f,
                              &nimg, nullptr,
                              channelsToRead,
                              partIndex, mip);
        if (rv != EXR_ERR_SUCCESS) {
            printf("Load EXR failed %s, mip %d\n", path, mip);
        }
        else {
            std::string name = std::string(path) + "_" + std::to_string(mip);
            std::shared_ptr<LabImageData_t> img(new LabImageData_t());
            if (!img0)
                img0 = img;
                        
            img->data = nimg.data;
            img->dataSize = nimg.dataSize;
            img->pixelType = (LabPixelType_t) nimg.pixelType;
            img->channelCount = nimg.channelCount;
            img->width = nimg.width;
            img->height = nimg.height;
            img->dataWindowMinY = nimg.dataWindowMinY;
            img->dataWindowMaxY = nimg.dataWindowMaxY;

            printf("nimg width, height %d %d\n", nimg.width, nimg.height);
            printf("img width, height %d %d\n", img->width, img->height);
            _self->cache[name] = CacheEntry{std::string(path), img};
            printf("nimg width, height %d %d\n", nimg.width, nimg.height);
            printf("img width, height %d %d\n", img->width, img->height);

            printf("Loaded EXR %s, mip %d\n", path, mip);
            printf("mip info: \n");
            printf("  width: %d\n", img->width);
            printf("  height: %d\n", img->height);
            printf("  data size: %zu\n", img->dataSize);
        }
    }
    return img0;
}

std::shared_ptr<LabImageData_t> TextureCache::ReadAndCacheSTB(const char* path) {
    std::shared_ptr<LabImageData_t> imgData = Get(path);
    if (imgData && imgData->channelCount != 0)
        return imgData;

    const int channels = 4; // force RGBA
    int imageFileChannelCount;
    int w, h;
    uint8_t* data = (uint8_t*) stbi_load(path,
                                     &w, &h, &imageFileChannelCount, channels);
    
    if (!data)
        return nullptr;

    std::shared_ptr<LabImageData_t> img(new LabImageData_t());
    img->data = (uint8_t*) data;
    img->dataSize = 4 * w * h;
    img->pixelType = LAB_PIXEL_UINT8;
    img->channelCount = channels;
    img->width = w;
    img->height = h;
    img->dataWindowMinY = 0;
    img->dataWindowMaxY = h - 1;
    _self->cache[std::string(path)] = CacheEntry{std::string(path), img};
    return img;
}

std::shared_ptr<LabImageData_t> TextureCache::ReadAndCachePFM(const char* path) {
    std::shared_ptr<LabImageData_t> imgData = Get(path);
    if (imgData && imgData->channelCount != 0)
        return imgData;

    FILE* f = fopen(path, "rb");
    if (!f)
        return nullptr;

    char magic[3];
    fscanf(f, "%2s\n", magic);
    
    if (magic[0] != 'P')
        return nullptr;
    
    int channels = 1;
    if (magic[1] == 'F')
        channels = 3;
    else if (magic[1] != 'f')
        return nullptr;
    
    uint32_t w, h;
    fscanf(f, "%d %d\n", &w, &h);
    
    // scale is meant to be a multiplier to convert to units
    // such as watts per square meter, but it's ill defined.
    // a negative value means little endian, else big.
    // We are meant to use the absolute value of the scale.
    float scale;
    fscanf(f, "%f\n", &scale);
    if (scale >= 0)
        return nullptr; // bail, not swizzling big to little today
    
    size_t sz = sizeof(float) * w * h * channels;
    float* data = (float*) malloc(sz);
    fread((void*) data, 1, sz, f);
    
    /// @TODO swizzle the buffer if endianness doesn't match the CPU.
    
    fclose(f);

    std::shared_ptr<LabImageData_t> img(new LabImageData_t());
    img->data = (uint8_t*) data;
    img->dataSize = sz;
    img->pixelType = LAB_PIXEL_FLOAT;
    img->channelCount = channels;
    img->width = w;
    img->height = h;
    img->dataWindowMinY = 0;
    img->dataWindowMaxY = h-1;

    _self->cache[std::string(path)] = CacheEntry{std::string(path), img};
    return img;
}

#if 0


nanoexr_ImageData_t TextureActivity::ReadAndCacheAVIF(const char* path) {
    nanoexr_ImageData_t img = Cached(path);
    if (img.channelCount != 0)
        return img;

    // Initialize libavif
    avifImage *image = avifImageCreateEmpty();
    avifDecoder *decoder = avifDecoderCreate();
    avifResult result = avifDecoderReadFile(decoder, image, path);
    if (result != AVIF_RESULT_OK) {
        printf("Error parsing AVIF file: %s\n", avifResultToString(result));
        avifDecoderDestroy(decoder);
        avifImageDestroy(image);
        return img;
    }

    // Convert to sRGB
    avifRGBImage rgb;
    const int bytesPerPixel = 2;
    const int channelCount = 4;
    memset(&rgb, 0, sizeof(rgb));
    avifRGBImageSetDefaults(&rgb, image);
    rgb.format = AVIF_RGB_FORMAT_RGBA; // Choose desired format (RGBA in this case)
    rgb.rowBytes = rgb.width * channelCount * bytesPerPixel;
    rgb.pixels = (uint8_t*) calloc(channelCount * bytesPerPixel * rgb.width * rgb.height, 1);
    rgb.depth = 8 * bytesPerPixel;
    rgb.isFloat = true;
    result = avifImageYUVToRGB(image, &rgb);
    if (result != AVIF_RESULT_OK) {
        printf("Error parsing AVIF file: %s\n", avifResultToString(result));
        avifDecoderDestroy(decoder);
        avifImageDestroy(image);
        return img;
    }

    printf("width: %d, height:%d\n", rgb.width, rgb.height);
    // rgba8 pixels are at rgb.pixels
    img.data = (uint8_t*) rgb.pixels;
    img.dataSize = channelCount * bytesPerPixel * rgb.width * rgb.height;
    if (bytesPerPixel == 1)
        img.pixelType = (exr_pixel_type_t) 3;  // A sentinel to indidate an 8 bit texture
    else
        img.pixelType = EXR_PIXEL_HALF;
    img.channelCount = channelCount;
    img.width = rgb.width;
    img.height = rgb.height;
    img.dataWindowMinY = 0;
    img.dataWindowMaxY = rgb.height - 1;
    _self->path_texture$[std::string(path)] = img;
    _self->texture_names.push_back(path);

    avifDecoderDestroy(decoder);
    avifImageDestroy(image);
    return img;
}


#endif

std::shared_ptr<LabImageData_t> TextureCache::ReadAndCache(const char* path) {
    //if (strstr(path, ".avif") || strstr(path, ".AVIF"))
    //    return ReadAndCacheAVIF(path);
    if (strstr(path, ".exr") || strstr(path, ".EXR"))
        return ReadAndCacheEXR(path);
    if (strstr(path, ".pfm") || strstr(path, ".PFM"))
        return ReadAndCachePFM(path);
    return ReadAndCacheSTB(path);       // fallback to whatever STB provides
}

void TextureCache::ExportCache(const char* path) {
    for (const auto& i : _self->cache) {
        exr_result_t rv;
        std::string path = i.first;
        auto slash = path.rfind('/');
        if (slash == std::string::npos)
            continue;
        path = path + path.substr(slash + 1) + ".exr";

        LabPixelType_t typ = i.second.image->pixelType;
        if (typ != LAB_PIXEL_HALF && typ != LAB_PIXEL_FLOAT)
            continue;

        int componentSize = 2;
        if (i.second.image->pixelType == LAB_PIXEL_FLOAT)
            componentSize = sizeof(float);
        int pixelStride = 4 * componentSize;
        int lineStride = pixelStride * i.second.image->width;

        rv = nanoexr_write_exr(
            path.c_str(),
            nullptr, nullptr,
            i.second.image->width, i.second.image->height, false,
            (exr_pixel_type_t) i.second.image->pixelType,
            i.second.image->data,                     pixelStride, lineStride,
            i.second.image->data + componentSize,     pixelStride, lineStride,
            i.second.image->data + componentSize * 2, pixelStride, lineStride,
            i.second.image->data + componentSize * 3, pixelStride, lineStride);
    }
}

} // lab
