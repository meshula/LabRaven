
#include "TilengineProvider.hpp"
#include "Tilengine.h"
#include "Lab/LabJoystick.h"
#include "imgui.h"

static bool gInputFocus = false;

namespace lab {

    struct TilengineProvider::data {
        data() = default;
        ~data() = default;
    };
    
    TilengineProvider::TilengineProvider() : Provider(TilengineProvider::sname()) {
        _self = new TilengineProvider::data;
    }

    TilengineProvider::~TilengineProvider() {
        delete _self;
    }

    TilengineProvider* TilengineProvider::instance() {
        if (!_instance) {
            _instance = new TilengineProvider();
        }
        return _instance;
    }

    TilengineProvider* TilengineProvider::_instance = nullptr;

    void TilengineProvider::SetCaptureInput(bool capture) {
        gInputFocus = capture;
    }
} // lab


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
