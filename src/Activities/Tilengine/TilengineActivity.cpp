
#include "TilengineActivity.hpp"
#include "Lab/Providers/Tilengine/TilengineProvider.hpp"
#include "Lab/StudioCore.hpp"
#include "Lab/App.h"
#include "Lab/LabDirectories.h"
#include "Tilengine.h"
#include "Lab/CoreProviders/Metal/MetalProvider.hpp"

#include "imgui.h"

extern "C" void ShooterInit(const char* resourceRoot);
extern "C" void ShooterRun();
extern "C" void ShooterRelease();

namespace lab {

struct TilengineData {
};

struct TilengineActivity::data
{
    ~data() {
        if (fb) {
            delete[] fb;
        }
    }
    int fbWidth = 0;
    int fbHeight = 0;
    uint8_t* fb = nullptr;
    int frame = 0;
    int fbTexture = 0;
    bool restorePowerSave = true;
    bool mustInit = true;
};

TilengineActivity::TilengineActivity() : Activity(TilengineActivity::sname()) {
    _self = new data;

    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<TilengineActivity*>(instance)->RunUI(*vi);
    };
}

TilengineActivity::~TilengineActivity() {
    ShooterRelease();
    delete _self;
}

void TilengineActivity::_activate() {
    _self->restorePowerSave = LabApp::instance()->PowerSave();
    LabApp::instance()->SetPowerSave(false);
    if (_self->mustInit) {
        _self->mustInit = false;
        ShooterInit(lab_application_resource_path(nullptr, nullptr));
    }
}

void TilengineActivity::_deactivate() {
    LabApp::instance()->SetPowerSave(_self->restorePowerSave);
}

void TilengineActivity::RunUI(const LabViewInteraction& interaction) {
    int width = TLN_GetWidth();
    int height = TLN_GetHeight();
    if (!_self->fb || _self->fbWidth != width || _self->fbHeight != height) {
        if (_self->fb) {
            delete[] _self->fb;
        }
        _self->fbWidth = width;
        _self->fbHeight = height;
        _self->fb = new uint8_t[width * height * 4];
    }
    TLN_SetRenderTarget(_self->fb, width * 4);
    TLN_UpdateFrame(_self->frame++);
    if (!_self->fbTexture) {
        _self->fbTexture = LabCreateBGRA8Texture(width, height, _self->fb);
    }
    LabUpdateBGRA8Texture(_self->fbTexture, _self->fb);
    void* mtlTexture = LabGetEncodedTexture(_self->fbTexture);
    // set the next window size to twice the size of the texture plus the size of the title bar
    ImGui::SetNextWindowSize(ImVec2(width * 2, height * 2 + 40));
    ImGui::Begin("Tilengine##W");
    ImGui::Image((ImTextureID) mtlTexture, ImVec2(width * 2, height * 2));
    // Check focus and update the global input flag
    bool focussed = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    TilengineProvider::instance()->SetCaptureInput(focussed);
    ShooterRun();
    gInputFocus = false;
    ImGui::End();
}

} // lab
