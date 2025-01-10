#include "VoxelSprite.hpp"

#include <vector>
#include <array>
#include <algorithm>
#include <cmath>

#include <pxr/base/gf/vec2f.h>
#include <pxr/base/gf/vec4f.h>

#define OGT_VOX_IMPLEMENTATION
#include "ogt_vox.h"
PXR_NAMESPACE_USING_DIRECTIVE

bool VoxelSprite::LoadMagicavoxelFile(const uint8_t* data, size_t size) {
    // Parse the .vox file using ogt_vox library
    const ogt_vox_scene* scene = ogt_vox_read_scene(data, (uint32_t) size);
    if (!scene || scene->num_models == 0) {
        return false;
    }

    // Get the first model from the scene
    const ogt_vox_model* model = scene->models[0];
    if (!model) {
        ogt_vox_destroy_scene(scene);
        return false;
    }

    // Update dimensions
    width = model->size_x;
    height = model->size_y;
    depth = model->size_z;

    // Resize voxels and colors vectors
    size_t total_size = width * height * depth;
    voxels.resize(total_size);
    colors.resize(total_size);

    // Copy voxel data and convert colors
    for (size_t i = 0; i < total_size; i++) {
        uint8_t color_index = model->voxel_data[i];
        voxels[i] = color_index;
        if (color_index > 0) {
            // Get color from palette and convert to GfVec3f
            const ogt_vox_rgba& color = scene->palette.color[color_index];
            colors[i] = GfVec3f(
                color.r / 255.0f,
                color.g / 255.0f,
                color.b / 255.0f
            );
        } else {
            colors[i] = GfVec3f(0.0f);
        }
    }

    // Clean up
    ogt_vox_destroy_scene(scene);
    return true;
}

void VoxelRender::MarchVoxels(const VoxelSprite& sprite, const GfMatrix4f& projectionMatrix) {

    GfVec4f xvec(1,0,0,1);
    GfVec4f yvec(0,1,0,1);
    GfVec4f zvec(0,0,1,1);
    GfVec4f xvecp = xvec * projectionMatrix;
    GfVec4f yvecp = xvec * projectionMatrix;
    GfVec4f zvecp = zvec * projectionMatrix;

    GfVec4f eye(0,0,0,1);
    double dotx = GfDot(eye, xvecp);
    double doty = GfDot(eye, yvecp);
    double dotz = GfDot(eye, zvecp);

    int main = 0;
    if (abs(dotx) >= abs(doty) && abs(dotx) >= abs(dotz)) {
        main = dotx >= 0 ? 3 : 0;
    }
    else if (abs(doty) >= abs(dotx) && abs(doty) >= abs(dotz)) {
        main = doty >= 0 ? 4 : 1;
    }
    else {
        main = dotz >= 0 ? 5 : 2;
    }

    int row = 0;
    int col = 1;
    int lyr = 2;
    int flp = 0;
    int vw = 0;
    int vh = 0;
    int vl = 0;
    switch (main) {
        case 0: // XY
            row = 0; col = 1; lyr = 2; flp = 0;
            vw = sprite.GetWidth(); vh = sprite.GetHeight(); vl = sprite.GetDepth();
            break;
        case 1: // XZ
            row = 0; col = 2; lyr = 1; flp = 0;
            vw = sprite.GetWidth(); vh = sprite.GetDepth(); vl = sprite.GetHeight();
            break;
        case 2: // YZ
            row = 1; col = 2; lyr = 0; flp = 0;
            vw = sprite.GetHeight(); vh = sprite.GetDepth(); vl = sprite.GetWidth();
            break;
        case 3: // -XY
            row = 0; col = 1; lyr = 2; flp = 1;
            vw = sprite.GetWidth(); vh = sprite.GetHeight(); vl = sprite.GetDepth();
            break;
        case 4: // -XZ
            row = 0; col = 2; lyr = 1; flp = 1;
            vw = sprite.GetWidth(); vh = sprite.GetDepth(); vl = sprite.GetHeight();
            break;
        case 5: // -YZ
            row = 1; col = 2; lyr = 0; flp = 1;
            vw = sprite.GetHeight(); vh = sprite.GetDepth(); vl = sprite.GetWidth();
            break;
    }

    int permute[3] = { row, col, lyr };

    float scale = vw;
    if (vh > scale) { scale = vh; }
    if (vl > scale) { scale = vl; }
    scale = 1.f / scale;

    int width = framebuffer.GetWidth();
    int height = framebuffer.GetHeight();

    float minDepth = std::numeric_limits<float>::infinity();
    //GfVec3f bestColor(0.0f);

    for (int z = 0; z < vl; ++z) {
        for (int y = 0; y < vh; ++y) {
            for (int x = 0; x < vw; ++x) {
                int swz[3];
                swz[permute[0]] = x;
                swz[permute[1]] = y;
                swz[permute[2]] = z;
                if (!sprite.Voxel(swz[0], swz[1], swz[2]))
                    continue;

                // center the object and push it back
                GfVec4f voxelPosition(swz[0] + 0.5f, swz[1] + 0.5f, swz[2] + 0.5f, 1.f);
                voxelPosition *= 2.f * scale;
                voxelPosition -= GfVec4f(scale, scale, scale, 0.f);
                voxelPosition[3] = 1.f;

                GfVec4f projected = voxelPosition * projectionMatrix;
                GfVec2f screen(projected[0]/projected[3], projected[1]/projected[3]);

                int pixelX = static_cast<int>(screen[0]*width + width/2);
                int pixelY = static_cast<int>(screen[1]*height + height/2);
                float depthValue = projected[2]/projected[3];

                if (pixelX >= 0 && pixelX < width && pixelY >= 0 && pixelY < height) {
                    //minDepth = depthValue;
                    auto color = sprite.Color(swz[0], swz[1], swz[2]);
                    //if (minDepth < framebuffer.Depth(screenX, screenY)) {
                    framebuffer.ColorX(pixelX, pixelY) = color;
                        //framebuffer.Depth(screenX, screenY) = minDepth;
                    //}
                }
            }
        }
    }
}

void VoxelRender::ShadeVoxels(const VoxelSprite& sprite, const GfVec3f& lightDirection) {
    return;
    int width = framebuffer.GetWidth();
    int height = framebuffer.GetHeight();

    // Normalize the light direction for correct dot product computation.
    GfVec3f normalizedLight = lightDirection.GetNormalized();

    // Precompute the plane normals for color contributions.
    const GfVec3f normalX(1.0f, 0.0f, 0.0f);
    const GfVec3f normalY(0.0f, 1.0f, 0.0f);
    const GfVec3f normalZ(0.0f, 0.0f, 1.0f);

    // Iterate over the framebuffer pixels.
    for (int screenY = 0; screenY < height; ++screenY) {
        for (int screenX = 0; screenX < width; ++screenX) {
            float depthValue = framebuffer.Depth(screenX, screenY);
            if (depthValue == std::numeric_limits<float>::infinity()) {
                continue;  // Skip empty pixels.
            }

            for (int z = 0; z < sprite.GetDepth(); ++z) {
                for (int y = 0; y < sprite.GetHeight(); ++y) {
                    for (int x = 0; x < sprite.GetWidth(); ++x) {
                        if (!sprite.Voxel(x, y, z)) continue;

                        GfVec3f voxelPosition(x + 0.5f, y + 0.5f, z + 0.5f);
                        float voxelDepth = voxelPosition[2];  // Assuming depth is along Z-axis.

                        if (voxelDepth == depthValue) {
                            GfVec3f voxelColor = sprite.Color(x, y, z);

                            // Compute lighting contributions.
                            float lightX = std::max(0.0f, GfDot(normalizedLight, normalX)) * voxelColor[0];
                            float lightY = std::max(0.0f, GfDot(normalizedLight, normalY)) * voxelColor[1];
                            float lightZ = std::max(0.0f, GfDot(normalizedLight, normalZ)) * voxelColor[2];

                            // Update color buffers.
                            framebuffer.ColorX(screenX, screenY) += lightX * voxelColor;
                            framebuffer.ColorY(screenX, screenY) += lightY * voxelColor;
                            framebuffer.ColorZ(screenX, screenY) += lightZ * voxelColor;
                        }
                    }
                }
            }
        }
    }
}

void VoxelRender::ResolveVoxels() {
    return;
    int width = framebuffer.GetWidth();
    int height = framebuffer.GetHeight();

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            GfVec3f colorX = framebuffer.ColorX(x, y);
            GfVec3f colorY = framebuffer.ColorY(x, y);
            GfVec3f colorZ = framebuffer.ColorZ(x, y);

            GfVec3f finalColor = (colorX + colorY + colorZ) / 3.0f;
            framebuffer.ResolveColor(x, y) = finalColor;
        }
    }
}

void VoxelRender::Render(const VoxelSprite& sprite, const GfMatrix4f& projectionMatrix, const GfVec3f& lightDirection) {
    framebuffer.Clear();
    MarchVoxels(sprite, projectionMatrix);
    ShadeVoxels(sprite, lightDirection);
    ResolveVoxels();
}
