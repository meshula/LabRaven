
#include "TilengineActivity.hpp"
#include "Lab/StudioCore.hpp"
#include "Lab/App.h"
#include "Lab/LabJoystick.h"
#include "Lab/LabDirectories.h"
#include "Tilengine.h"
#include "Providers/Metal/MetalProvider.h"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"

extern "C" void ShooterInit(const char* resourceRoot);
extern "C" void ShooterRun();
extern "C" void ShooterRelease();

static bool gInputFocus = false;

extern "C"
bool TLN_GetInput(TLN_Input id) {
    if (!gInputFocus) {
        return false;
    }
    LabJoystickState joy;
    LabUpdateJoystickState(joy);
    if (joy.connected) {
        switch (id) {
            case INPUT_UP:
                if (joy.leftStickY > 0.25f)
                    return true;
                 return joy.dpadUp;
            case INPUT_DOWN:   
                if (joy.leftStickY < -0.25f)
                    return true;
                 return joy.dpadDown;
            case INPUT_LEFT:
                if (joy.leftStickX < -0.25f)
                    return true;
               return joy.dpadLeft;
            case INPUT_RIGHT:
                if (joy.leftStickX > 0.25f)
                    return true;
                return joy.dpadRight;
            case INPUT_BUTTON1: return joy.aButton;
            case INPUT_BUTTON2: return joy.bButton;
            case INPUT_BUTTON3: return joy.xButton;
            case INPUT_BUTTON4: return joy.yButton;
            case INPUT_BUTTON5: return joy.leftShoulder;
            case INPUT_BUTTON6: return joy.rightShoulder;
            case INPUT_START:   return joy.startButton;
            case INPUT_QUIT:    return joy.menuButton;
            default: break;
        }
    }
    switch (id) {
        case INPUT_UP:     return ImGui::IsKeyDown(ImGuiKey_W); // W for up
        case INPUT_DOWN:   return ImGui::IsKeyDown(ImGuiKey_S); // S for down
        case INPUT_LEFT:   return ImGui::IsKeyDown(ImGuiKey_A); // A for left
        case INPUT_RIGHT:  return ImGui::IsKeyDown(ImGuiKey_D); // D for right
        case INPUT_BUTTON1: return ImGui::IsKeyDown(ImGuiKey_Space); // Space for action button 1
        case INPUT_BUTTON2: return ImGui::IsKeyDown(ImGuiKey_LeftShift); // LeftShift for action button 2
        case INPUT_BUTTON3: return ImGui::IsKeyDown(ImGuiKey_E); // E for action button 3
        case INPUT_BUTTON4: return ImGui::IsKeyDown(ImGuiKey_Q); // Q for action button 4
        case INPUT_BUTTON5: return ImGui::IsKeyDown(ImGuiKey_R); // R for action button 5
        case INPUT_BUTTON6: return ImGui::IsKeyDown(ImGuiKey_F); // F for action button 6
        case INPUT_START:   return ImGui::IsKeyDown(ImGuiKey_Enter); // Enter for start
        case INPUT_QUIT:    return ImGui::IsKeyDown(ImGuiKey_Escape); // Escape to quit
        case INPUT_CRT:     return ImGui::IsKeyDown(ImGuiKey_C); // C to toggle CRT
        default: break;
    }
    return false;
}


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
};

TilengineActivity::TilengineActivity() : Activity(TilengineActivity::sname()) {
    _self = new data;

    ShooterInit(lab_application_resource_path(nullptr, nullptr));

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
    gInputFocus = ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows);
    ShooterRun();
    gInputFocus = false;
    ImGui::End();
}

} // lab
