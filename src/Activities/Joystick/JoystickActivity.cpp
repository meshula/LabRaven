
#include "JoystickActivity.hpp"
#include "Lab/App.h"

#include "Lab/LabJoystick.h"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"

namespace lab {

struct JoystickActivity::data {
    LabJoystickState joystickState;
    bool restorePowerSave = false;
};

JoystickActivity::JoystickActivity() 
: Activity(JoystickActivity::sname())
, _self(new data) {
    LabInitJoystickManager();
    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<JoystickActivity*>(instance)->RunUI(*vi);
    };
}

JoystickActivity::~JoystickActivity() {
    LabShutdownJoystickManager();
    delete _self;
}


void JoystickActivity::_activate() {
    _self->restorePowerSave = LabApp::instance()->PowerSave();
    LabApp::instance()->SetPowerSave(false);
}
void JoystickActivity::_deactivate() {
    LabApp::instance()->SetPowerSave(_self->restorePowerSave);
}

void JoystickActivity::RunUI(const LabViewInteraction&) {
    LabUpdateJoystickState(_self->joystickState);

    // We're going to use drawlists to draw the joystick state.
    // will be drawn above the rect; the analong sticks will be drawn as circles inside the rect,
    // with the position of the joystick indicated by displaying a small circle inside the circle.
    // The buttons will be drawn as small circles with the button name inside, and colored based on
    // whether the button is pressed or not.
    // The dpad will be drawn as a plus shape, with the direction of the dpad indicated by a small circle.
    // The left and right shoulder buttons will be drawn as small circles above the joystick rect.
    // The left and right stick buttons will be drawn as small circles below the joystick rect.
    // The left and right trigger buttons will be drawn as small circles to the left and right of the joystick rect, the color proportional to the amount the trigger is pressed.
    // The joystick rect will be drawn as a rectangle with a border, and the joystick name will be displayed above the rect.
    // The joystick name will be displayed in the top left corner of the window.
    // The A, B, X, and Y buttons will be drawn as small circles to the right of the joystick rect, and the button name will be displayed inside the circle; the color of the circle will be based on whether the button is pressed or not.
    // The start and menu buttons will be drawn as small circles to the right of the A, B, X, and Y buttons, with the button name displayed inside the circle; the color of the circle will be based on whether the button is pressed or not.

    ImGui::Begin("Joystick##mM");
    
    // Add joystick selection dropdown
    std::vector<std::string> joystickNames = LabGetJoystickNames();
    if (!joystickNames.empty()) {
        std::string currentName = LabCurrentJoystickName();
        int currentIndex = 0;
        // Find current joystick index
        for (size_t i = 0; i < joystickNames.size(); i++) {
            if (joystickNames[i] == currentName) {
                currentIndex = static_cast<int>(i);
                break;
            }
        }
        
        // Create combo box
        if (ImGui::Combo("Select Joystick", &currentIndex, 
            [](void* data, int idx, const char** out_text) -> bool {
                auto& names = *static_cast<std::vector<std::string>*>(data);
                if (idx < 0 || idx >= static_cast<int>(names.size())) return false;
                *out_text = names[idx].c_str();
                return true;
            }, 
            &joystickNames, static_cast<int>(joystickNames.size()))) {
            // Selection changed, update active joystick
            LabSelectJoyStick(currentIndex);
        }
        ImGui::Spacing();
    }
    
    const float WINDOW_PADDING = 20.0f;
    const float CIRCLE_RADIUS = 20.0f;
    const float STICK_CIRCLE_RADIUS = 50.0f;
    const float BUTTON_SPACING = 30.0f;
    
    ImDrawList* draw_list = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    
    // Display joystick name and connection status
    std::string name = LabCurrentJoystickName();
    ImGui::Text("%s (%s)", name.c_str(), _self->joystickState.connected ? "Connected" : "Disconnected");
    ImGui::Spacing();
    
    // Calculate base positions
    float centerX = pos.x + ImGui::GetWindowWidth() * 0.5f;
    float centerY = pos.y + ImGui::GetWindowHeight() * 0.5f;
    
    // Draw analog sticks
    // Left stick
    ImVec2 leftStickCenter(centerX - 100, centerY);
    draw_list->AddCircle(leftStickCenter, STICK_CIRCLE_RADIUS, ImGui::GetColorU32(ImVec4(1,1,1,1)));
    ImVec2 leftStickPos(
        leftStickCenter.x + _self->joystickState.leftStickX * STICK_CIRCLE_RADIUS,
        leftStickCenter.y + _self->joystickState.leftStickY * STICK_CIRCLE_RADIUS
    );
    draw_list->AddCircleFilled(leftStickPos, CIRCLE_RADIUS, 
        _self->joystickState.leftStick ? ImGui::GetColorU32(ImVec4(1,0,0,1)) : ImGui::GetColorU32(ImVec4(1,1,1,1)));
    
    // Right stick
    ImVec2 rightStickCenter(centerX + 100, centerY);
    draw_list->AddCircle(rightStickCenter, STICK_CIRCLE_RADIUS, ImGui::GetColorU32(ImVec4(1,1,1,1)));
    ImVec2 rightStickPos(
        rightStickCenter.x + _self->joystickState.rightStickX * STICK_CIRCLE_RADIUS,
        rightStickCenter.y + _self->joystickState.rightStickY * STICK_CIRCLE_RADIUS
    );
    draw_list->AddCircleFilled(rightStickPos, CIRCLE_RADIUS,
        _self->joystickState.rightStick ? ImGui::GetColorU32(ImVec4(1,0,0,1)) : ImGui::GetColorU32(ImVec4(1,1,1,1)));
    
    // Draw ABXY buttons
    ImVec2 buttonCenter(centerX + 200, centerY);
    // B button (bottom)
    draw_list->AddCircleFilled(ImVec2(buttonCenter.x, buttonCenter.y + BUTTON_SPACING), CIRCLE_RADIUS,
        _self->joystickState.bButton ? ImGui::GetColorU32(ImVec4(0,1,0,1)) : ImGui::GetColorU32(ImVec4(0.3f,0.3f,0.3f,1)));
    draw_list->AddText(ImVec2(buttonCenter.x - 5, buttonCenter.y + BUTTON_SPACING - 7), ImGui::GetColorU32(ImVec4(1,1,1,1)), "B");

    // A button (right)
    draw_list->AddCircleFilled(ImVec2(buttonCenter.x + BUTTON_SPACING, buttonCenter.y), CIRCLE_RADIUS,
        _self->joystickState.aButton ? ImGui::GetColorU32(ImVec4(1,0,0,1)) : ImGui::GetColorU32(ImVec4(0.3f,0.3f,0.3f,1)));
    draw_list->AddText(ImVec2(buttonCenter.x + BUTTON_SPACING - 5, buttonCenter.y - 7), ImGui::GetColorU32(ImVec4(1,1,1,1)), "A");

    // Y button (left)
    draw_list->AddCircleFilled(ImVec2(buttonCenter.x - BUTTON_SPACING, buttonCenter.y), CIRCLE_RADIUS,
        _self->joystickState.yButton ? ImGui::GetColorU32(ImVec4(0,0,1,1)) : ImGui::GetColorU32(ImVec4(0.3f,0.3f,0.3f,1)));
    draw_list->AddText(ImVec2(buttonCenter.x - BUTTON_SPACING - 5, buttonCenter.y - 7), ImGui::GetColorU32(ImVec4(1,1,1,1)), "Y");

    // X button (top)
    draw_list->AddCircleFilled(ImVec2(buttonCenter.x, buttonCenter.y - BUTTON_SPACING), CIRCLE_RADIUS,
        _self->joystickState.xButton ? ImGui::GetColorU32(ImVec4(1,1,0,1)) : ImGui::GetColorU32(ImVec4(0.3f,0.3f,0.3f,1)));
    draw_list->AddText(ImVec2(buttonCenter.x - 5, buttonCenter.y - BUTTON_SPACING - 7), ImGui::GetColorU32(ImVec4(1,1,1,1)), "X");

    // Draw D-pad
    ImVec2 dpadCenter(centerX - 200, centerY);
    const float DPAD_SIZE = 15.0f;
    
    // Up
    draw_list->AddRectFilled(
        ImVec2(dpadCenter.x - DPAD_SIZE/2, dpadCenter.y - BUTTON_SPACING - DPAD_SIZE),
        ImVec2(dpadCenter.x + DPAD_SIZE/2, dpadCenter.y - BUTTON_SPACING + DPAD_SIZE),
        _self->joystickState.dpadUp ? ImGui::GetColorU32(ImVec4(1,1,1,1)) : ImGui::GetColorU32(ImVec4(0.3f,0.3f,0.3f,1)));
    
    // Down
    draw_list->AddRectFilled(
        ImVec2(dpadCenter.x - DPAD_SIZE/2, dpadCenter.y + BUTTON_SPACING - DPAD_SIZE),
        ImVec2(dpadCenter.x + DPAD_SIZE/2, dpadCenter.y + BUTTON_SPACING + DPAD_SIZE),
        _self->joystickState.dpadDown ? ImGui::GetColorU32(ImVec4(1,1,1,1)) : ImGui::GetColorU32(ImVec4(0.3f,0.3f,0.3f,1)));
    
    // Left
    draw_list->AddRectFilled(
        ImVec2(dpadCenter.x - BUTTON_SPACING - DPAD_SIZE, dpadCenter.y - DPAD_SIZE/2),
        ImVec2(dpadCenter.x - BUTTON_SPACING + DPAD_SIZE, dpadCenter.y + DPAD_SIZE/2),
        _self->joystickState.dpadLeft ? ImGui::GetColorU32(ImVec4(1,1,1,1)) : ImGui::GetColorU32(ImVec4(0.3f,0.3f,0.3f,1)));
    
    // Right
    draw_list->AddRectFilled(
        ImVec2(dpadCenter.x + BUTTON_SPACING - DPAD_SIZE, dpadCenter.y - DPAD_SIZE/2),
        ImVec2(dpadCenter.x + BUTTON_SPACING + DPAD_SIZE, dpadCenter.y + DPAD_SIZE/2),
        _self->joystickState.dpadRight ? ImGui::GetColorU32(ImVec4(1,1,1,1)) : ImGui::GetColorU32(ImVec4(0.3f,0.3f,0.3f,1)));
    
    // Draw triggers
    float triggerWidth = 100.0f;
    float triggerHeight = 20.0f;
    
    // Left trigger
    ImVec2 leftTriggerPos(pos.x + WINDOW_PADDING, pos.y + WINDOW_PADDING);
    draw_list->AddRectFilled(
        leftTriggerPos,
        ImVec2(leftTriggerPos.x + triggerWidth * _self->joystickState.leftTrigger, leftTriggerPos.y + triggerHeight),
        ImGui::GetColorU32(ImVec4(1,0,0,1)));
    draw_list->AddRect(
        leftTriggerPos,
        ImVec2(leftTriggerPos.x + triggerWidth, leftTriggerPos.y + triggerHeight),
        ImGui::GetColorU32(ImVec4(1,1,1,1)));
    
    // Right trigger
    ImVec2 rightTriggerPos(pos.x + ImGui::GetWindowWidth() - WINDOW_PADDING - triggerWidth, pos.y + WINDOW_PADDING);
    draw_list->AddRectFilled(
        rightTriggerPos,
        ImVec2(rightTriggerPos.x + triggerWidth * _self->joystickState.rightTrigger, rightTriggerPos.y + triggerHeight),
        ImGui::GetColorU32(ImVec4(1,0,0,1)));
    draw_list->AddRect(
        rightTriggerPos,
        ImVec2(rightTriggerPos.x + triggerWidth, rightTriggerPos.y + triggerHeight),
        ImGui::GetColorU32(ImVec4(1,1,1,1)));
    
    // Draw shoulder buttons
    ImVec2 leftShoulderPos(pos.x + WINDOW_PADDING, pos.y + WINDOW_PADDING + triggerHeight + 10);
    ImVec2 rightShoulderPos(pos.x + ImGui::GetWindowWidth() - WINDOW_PADDING - triggerWidth, pos.y + WINDOW_PADDING + triggerHeight + 10);
    
    draw_list->AddCircleFilled(
        ImVec2(leftShoulderPos.x + CIRCLE_RADIUS, leftShoulderPos.y + CIRCLE_RADIUS),
        CIRCLE_RADIUS,
        _self->joystickState.leftShoulder ? ImGui::GetColorU32(ImVec4(1,0,0,1)) : ImGui::GetColorU32(ImVec4(0.3f,0.3f,0.3f,1)));
    
    draw_list->AddCircleFilled(
        ImVec2(rightShoulderPos.x + CIRCLE_RADIUS, rightShoulderPos.y + CIRCLE_RADIUS),
        CIRCLE_RADIUS,
        _self->joystickState.rightShoulder ? ImGui::GetColorU32(ImVec4(1,0,0,1)) : ImGui::GetColorU32(ImVec4(0.3f,0.3f,0.3f,1)));
    
    // Draw menu buttons
    float menuY = pos.y + ImGui::GetWindowHeight() - WINDOW_PADDING - CIRCLE_RADIUS * 4;
    draw_list->AddCircleFilled(
        ImVec2(centerX - 30, menuY),
        CIRCLE_RADIUS,
        _self->joystickState.startButton ? ImGui::GetColorU32(ImVec4(1,0,0,1)) : ImGui::GetColorU32(ImVec4(0.3f,0.3f,0.3f,1)));
    draw_list->AddCircleFilled(
        ImVec2(centerX + 30, menuY),
        CIRCLE_RADIUS,
        _self->joystickState.menuButton ? ImGui::GetColorU32(ImVec4(1,0,0,1)) : ImGui::GetColorU32(ImVec4(0.3f,0.3f,0.3f,1)));
    
    ImGui::End();
}

} // namespace lab
