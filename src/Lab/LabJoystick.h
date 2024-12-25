#ifndef LAB_JOYSTICK_H
#define LAB_JOYSTICK_H

#include <string>
#include <vector>

struct LabJoystickState {
    bool connected = false;
    float leftStickX = 0.0f;
    float leftStickY = 0.0f;
    float rightStickX = 0.0f;
    float rightStickY = 0.0f;
    float leftTrigger = 0.0f;
    float rightTrigger = 0.0f;
    bool aButton = false;
    bool bButton = false;
    bool xButton = false;
    bool yButton = false;
    bool leftShoulder = false;
    bool rightShoulder = false;
    bool leftStick = false;
    bool rightStick = false;
    bool dpadUp = false;
    bool dpadDown = false;
    bool dpadLeft = false;
    bool dpadRight = false;
    bool startButton = false;
    bool menuButton = false;
};

void LabInitJoystickManager();
void LabShutdownJoystickManager();
std::vector<std::string> LabGetJoystickNames();
void LabSelectJoyStick(int index);
std::string LabCurrentJoystickName();
void LabUpdateJoystickState(LabJoystickState& state);

#endif // LAB_JOYSTICK_H
