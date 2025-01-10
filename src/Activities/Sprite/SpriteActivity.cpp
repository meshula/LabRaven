
#include "SpriteActivity.hpp"
#include "Lab/App.h"
#include "Lab/StudioCore.hpp"
#include "Lab/LabFileDialogManager.hpp"
#include "Providers/Sprite/VoxelSprite.hpp"
#include <pxr/base/gf/frustum.h>
#include <stdio.h>
#include "imgui.h"
#include <pxr/base/gf/vec3f.h>

#include "Providers/Metal/MetalProvider.h"

namespace lab {

struct SpriteActivity::data {
};

SpriteActivity::SpriteActivity() 
: Activity(SpriteActivity::sname())
, _self(new data) {
    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<SpriteActivity*>(instance)->RunUI(*vi);
    };
}

SpriteActivity::~SpriteActivity() {
    delete _self;
}

void SpriteActivity::RunUI(const LabViewInteraction&) {
    typedef PXR_NS::GfVec3f GfVec3f;
    ImGui::Begin("Sprite##mM");
    static int width = 0, height = 0, depth = 0;

    static int texture = 0;

    if (ImGui::Button("Load")) {
        do {
            std::string dir;
            std::string path;
            int id = LabApp::instance()->fdm()->RequestOpenFile({"vox"}, dir);
            do {
                FileDialogManager::FileReq r = LabApp::instance()->fdm()->PopOpenedFile(id);
                if (r.status == FileDialogManager::FileReq::canceled ||
                    r.status == FileDialogManager::FileReq::expired) {
                    break;
                }
                if (r.status == FileDialogManager::FileReq::ready) {
                    path = r.path;
                    break;
                }
                sleep(1);
            } while (true);

            if (path.size()) {
                // file size
                FILE* file = fopen(path.c_str(), "rb");
                fseek(file, 0, SEEK_END);
                long size = ftell(file);

                // read file
                uint8_t* data = new uint8_t[size];
                fseek(file, 0, SEEK_SET);
                fread(data, 1, size, file);
                fclose(file);

                auto sprite = new VoxelSprite(16, 16, 16);
                sprite->LoadMagicavoxelFile(data, size);
                width = sprite->GetWidth();
                height = sprite->GetHeight();
                depth = sprite->GetDepth();

                const int rw = 64, rh = 64;
                VoxelRender render(rw, rh);
                pxr::GfMatrix4d proj;
                pxr::GfFrustum frustum;
                frustum.SetProjectionType(pxr::GfFrustum::ProjectionType::Perspective);
                double targetAspect = 1.0;//double(vd.ww) / double(vd.wh);
                double fov = (180.0 * 0.5f)/M_PI;
                float znear = 0.01f;
                float zfar = 100000.f;
                frustum.SetPerspective(fov, targetAspect, znear, zfar);
                pxr::GfVec3f lightDir(0, 0, 1);
                proj = frustum.ComputeProjectionMatrix();
                pxr::GfMatrix4f projf(proj);
                render.Render(*sprite, projf, lightDir);

                static uint8_t copyPixels[rw*rh*4];
                int count = 2;
                GfVec3f* pixels = render.Pixels();
                float* pixelsf = (float*) pixels;
                for (int i = 0; i < rw * rh; ++i) {
                    float r = pixelsf[i*3];
                    float g = pixelsf[i*3+1];
                    float b = pixelsf[i*3+2];
                    copyPixels[i*4] = (uint8_t) (r * 255.f);
                    copyPixels[i*4+1] = (uint8_t) (g * 255.f);
                    copyPixels[i*4+2] = (uint8_t) (b * 255.f);
                    copyPixels[i*4+3] = (r+g+b) > 0 ? 255 : 0;
                }
                if (!texture) {
                    texture = LabCreateRGBA8Texture(rw, rh, copyPixels);
                }
                else {
                    LabUpdateRGBA8Texture(texture, copyPixels);
                }
            }
            break;
        } while (false);
    }
    // display w, h, d
    ImGui::Text("Width: %d", width);
    ImGui::Text("Height: %d", height);
    ImGui::Text("Depth: %d", depth);

    if (texture) {
        void* t = LabGetEncodedTexture(texture);
        ImGui::Image((ImTextureID) t, ImVec2(128,128));
    }

    ImGui::End();
}

} // namespace lab
