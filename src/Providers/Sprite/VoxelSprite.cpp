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



// A structure representing a 2D vertex with depth (for perspective correction)
typedef struct {
    float x, y, z;  // z is the depth
    float u, v;     // texture coordinates or other interpolants
} Vertex;

// A helper for linear interpolation
float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

// A helper to compute perspective-correct interpolation weight
float perspective_correct_lerp(float a, float b, float wa, float wb, float t) {
    float one_minus_t = 1.0f - t;
    float inv_w = one_minus_t * wa + t * wb;
    return (one_minus_t * (a / wa) + t * (b / wb)) / inv_w;
}

// Function to rasterize a quad using perspective-correct interpolation
void rasterize_quad(const VoxelSprite* vox, FrameBuffer* fb, int permute[3], Vertex v0, Vertex v1, Vertex v2, Vertex v3) {
    // Assume v0, v1, v2, v3 are in screen space with y already sorted (v0.top, v3.bottom)
    for (int y = (int)v0.y; y <= (int)v3.y; y++) {
        float t0 = (float)(y - v0.y) / (v3.y - v0.y);  // Interpolation factor for left edge
        float t1 = (float)(y - v1.y) / (v2.y - v1.y);  // Interpolation factor for right edge

        Vertex left = {
            .x = lerp(v0.x, v3.x, t0),
            .y = (float) y,
            .z = lerp(v0.z, v3.z, t0),
            .u = perspective_correct_lerp(v0.u, v3.u, 1.0f / v0.z, 1.0f / v3.z, t0),
            .v = perspective_correct_lerp(v0.v, v3.v, 1.0f / v0.z, 1.0f / v3.z, t0)
        };

        Vertex right = {
            .x = lerp(v1.x, v2.x, t1),
            .y = (float) y,
            .z = lerp(v1.z, v2.z, t1),
            .u = perspective_correct_lerp(v1.u, v2.u, 1.0f / v1.z, 1.0f / v2.z, t1),
            .v = perspective_correct_lerp(v1.v, v2.v, 1.0f / v1.z, 1.0f / v2.z, t1)
        };

        // Rasterize from left.x to right.x at y
        for (int x = (int)left.x; x <= (int)right.x; x++) {
            float t = (float)(x - left.x) / (right.x - left.x);

            float z = lerp(left.z, right.z, t);  // Linear interpolation for depth
            float u = perspective_correct_lerp(left.u, right.u, 1.0f / left.z, 1.0f / right.z, t);
            float v = perspective_correct_lerp(left.v, right.v, 1.0f / left.z, 1.0f / right.z, t);

            // Plot pixel (x, y) with interpolated depth, u, and v
            // using permute, u, and v, you can get the color of the voxel from vox:
            int swz[3];
            swz[permute[0]] = u * vox->GetWidth();
            swz[permute[1]] = v * vox->GetHeight();
            swz[permute[2]] = z * vox->GetDepth();
            if (!vox->Voxel(swz[0], swz[1], swz[2]))
                continue;

            fb->ColorX(x, y) = vox->Color(swz[0], swz[1], swz[2]);

            //printf("Pixel (%d, %d): z = %f, u = %f, v = %f\n", x, y, z, u, v);
        }
    }
}

#if 0
int main() {
    Vertex v0 = {100, 50,  1.0f,  0.0f, 0.0f};   // Top-left
    Vertex v1 = {200, 50,  0.5f,  1.0f, 0.0f};   // Top-right
    Vertex v2 = {200, 150, 0.5f, 1.0f, 1.0f};  // Bottom-right
    Vertex v3 = {100, 150, 1.0f, 0.0f, 1.0f};  // Bottom-left

    rasterize_quad(v0, v1, v2, v3);
    return 0;
}
#endif










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

#define SOFTWARE_POLY_RATERIZER 1
#if SOFTWARE_POLY_RATERIZER

    // we're going to use rasterize quad to draw the voxels.
    // we need to compute the 4 corners of the quad in screen space.
    // We'll assume the box is normalized to 1x1x1 and centered at the origin.

    // we'll set up the interpolation based on the permute array above and flp.
    // so first, let's make the 8 corners of the box (as vec4f with 1.0 in the w component)

    // the plane closest to the camera is described by permute. So if it's XY, v0, v1, v2, and v3
    // are the corners of the box in the XY plane. and -v0, -v1, -v2, -v3 are the corners of the
    // box in the -XY plane. We'll use the permutation to figure out which plane is closest to the

    GfVec4f corners[8];
    corners[0] = GfVec4f(-0.5f, -0.5f, -0.5f, 1.f);
    corners[1] = GfVec4f( 0.5f, -0.5f, -0.5f, 1.f);
    corners[2] = GfVec4f( 0.5f,  0.5f, -0.5f, 1.f);
    corners[3] = GfVec4f(-0.5f,  0.5f, -0.5f, 1.f);
    corners[4] = GfVec4f(-0.5f, -0.5f,  0.5f, 1.f);
    corners[5] = GfVec4f( 0.5f, -0.5f,  0.5f, 1.f);
    corners[6] = GfVec4f( 0.5f,  0.5f,  0.5f, 1.f);
    corners[7] = GfVec4f(-0.5f,  0.5f,  0.5f, 1.f);

    // now permute them, ie, swap the components around
    for (int i = 0; i < 8; ++i) {
        GfVec4f c = corners[i];
        GfVec4f pc;
        pc[0] = c[permute[0]];
        pc[1] = c[permute[1]];
        pc[2] = c[permute[2]];
        pc[3] = 1.f;
        corners[i] = pc;
    }

    // now we need to project the corners into screen space using projectionMatrix.
    // we'll also compute the screen space coordinates of the corners.
    GfVec3f screenCorners[8];
    for (int i = 0; i < 8; ++i) {
        GfVec4f projected = corners[i] * projectionMatrix;
        GfVec3f screen(projected[0]/projected[3], projected[1]/projected[3], projected[2]/projected[3]);
        screenCorners[i] = screen;
    }

    // we are going to rasterize the box in 32 slices in the direction of the plane closest to the camera.
    // we'll use the screen space coordinates of the corners to compute the 4 corners of the quad for each slice.
    // we'll use the rasterize_quad function to draw the quad.

    for (int z = 0; z < 32; ++z) {
        float z0 = (z - 0.5f) / 32.f;
        float z1 = (z + 0.5f) / 32.f;

        Vertex v0 = { screenCorners[0][0], screenCorners[0][1], screenCorners[0][2], 0, 0 };
        Vertex v1 = { screenCorners[1][0], screenCorners[1][1], screenCorners[1][2], 1, 0 };
        Vertex v2 = { screenCorners[2][0], screenCorners[2][1], screenCorners[2][2], 1, 1 };
        Vertex v3 = { screenCorners[3][0], screenCorners[3][1], screenCorners[3][2], 0, 1 };

        // we need to teach rasterize_quad how to use Sprite.Color to get the color of a voxel.
        // we'll need to pass the sprite to the function, as well as the permutation and flip.

        rasterize_quad(&sprite, &framebuffer, permute, v0, v1, v2, v3);
    }

#else

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

    #endif
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
