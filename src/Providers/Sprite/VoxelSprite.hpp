#ifndef PROVIDERS_SPRITE_VOXEL
#define PROVIDERS_SPRITE_VOXEL

#ifndef HAVE_NO_USD

#include <vector>
#include <array>
#include <algorithm>
#include <cmath>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/gf/matrix4f.h>

// FrameBuffer class to store depth and color buffers.
class FrameBuffer {
public:
    typedef PXR_NS::GfVec3f GfVec3f;
    FrameBuffer(int width, int height)
        : width(width), height(height), 
          depthBuffer(width * height, std::numeric_limits<float>::infinity()),
          colorX(width * height, GfVec3f(0.0f)),
          colorY(width * height, GfVec3f(0.0f)),
          colorZ(width * height, GfVec3f(0.0f)),
          resolveBuffer(width * height, GfVec3f(0.0f)) {}

    void Clear() {
        std::fill(depthBuffer.begin(), depthBuffer.end(), std::numeric_limits<float>::infinity());
        std::fill(colorX.begin(), colorX.end(), GfVec3f(0.0f));
        std::fill(colorY.begin(), colorY.end(), GfVec3f(0.0f));
        std::fill(colorZ.begin(), colorZ.end(), GfVec3f(0.0f));
        std::fill(resolveBuffer.begin(), resolveBuffer.end(), GfVec3f(0.0f));
    }

    int GetWidth() const { return width; }
    int GetHeight() const { return height; }

    float& Depth(int x, int y) { return depthBuffer[y * width + x]; }
    GfVec3f& ColorX(int x, int y) { return colorX[y * width + x]; }
    GfVec3f& ColorY(int x, int y) { return colorY[y * width + x]; }
    GfVec3f& ColorZ(int x, int y) { return colorZ[y * width + x]; }
    GfVec3f& ResolveColor(int x, int y) { return resolveBuffer[y * width + x]; }

private:
    int width, height;
    std::vector<float> depthBuffer;
    std::vector<GfVec3f> colorX, colorY, colorZ;
    std::vector<GfVec3f> resolveBuffer;
};

// VoxelSprite class to represent voxel data.
class VoxelSprite {
public:
    typedef PXR_NS::GfVec3f GfVec3f;
    VoxelSprite(int width, int height, int depth)
        : width(width), height(height), depth(depth), 
          voxels(width * height * depth, false), colors(width * height * depth, GfVec3f(1.0f)) {}

    // at the moment, the value of a voxel is either zero or something.
    // if the source of data was the LoadMagicavoxelFile function, then the value
    // is a palette index.
    uint8_t& Voxel(int x, int y, int z) { return voxels[z * width * height + y * width + x]; }
    uint8_t  Voxel(int x, int y, int z) const { return voxels[z * width * height + y * width + x]; }
    GfVec3f& Color(int x, int y, int z) { return colors[z * width * height + y * width + x]; }
    GfVec3f Color(int x, int y, int z) const { return colors[z * width * height + y * width + x]; }

    int GetWidth() const { return width; }
    int GetHeight() const { return height; }
    int GetDepth() const { return depth; }

    bool LoadMagicavoxelFile(const uint8_t* data, size_t size);

private:
    int width, height, depth;
    std::vector<uint8_t> voxels;
    std::vector<GfVec3f> colors;
};

// VoxelRender class to manage rendering.
class VoxelRender {
public:
    typedef PXR_NS::GfMatrix4f GfMatrix4f;
    typedef PXR_NS::GfVec3f GfVec3f;

    VoxelRender(int width, int height)
        : framebuffer(width, height) {}

    void Render(const VoxelSprite& sprite, const GfMatrix4f& projectionMatrix, const GfVec3f& lightDirection);

    GfVec3f* Pixels() { return &framebuffer.ColorX(0,0); }

private:
    FrameBuffer framebuffer;
    void MarchVoxels(const VoxelSprite& sprite, const GfMatrix4f& projectionMatrix);
    void ShadeVoxels(const VoxelSprite& sprite, const GfVec3f& lightDirection);
    void ResolveVoxels();
};

#endif
#endif
