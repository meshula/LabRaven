
#include "LabJoystick.h"
#include <string>
#include <vector>
#include <stdio.h>

#import <GameController/GameController.h>

struct JoystickManager {
    std::vector<GCController*> availableControllers;
    int currentControllerIndex = -1;
};

static std::unique_ptr<JoystickManager> joystickManager;

void LabInitJoystickManager() {
    joystickManager = std::make_unique<JoystickManager>();
    NSArray<GCController*>* controllers = GCController.controllers;
    for (GCController* controller in controllers) {
        joystickManager->availableControllers.push_back(controller);
    }
    [[NSNotificationCenter defaultCenter] 
        addObserverForName:GCControllerDidConnectNotification 
                    object:nil 
                     queue:nil 
                usingBlock:^(NSNotification *note) {
                    if (joystickManager) {
                        GCController* controller = note.object;
                        joystickManager->availableControllers.push_back(controller);
                        if (joystickManager->availableControllers.size() == 1) {
                            // if this is the first joystick, connect it automatically.
                            joystickManager->currentControllerIndex = 0;
                            printf("Joystick connected.\n");
                        }
                    }
                }];
    [[NSNotificationCenter defaultCenter] 
        addObserverForName:GCControllerDidDisconnectNotification 
                    object:nil 
                     queue:nil 
                usingBlock:^(NSNotification *note) {
                    if (joystickManager) {
                        GCController* controller = note.object;
                        auto it = std::find(joystickManager->availableControllers.begin(), 
                                            joystickManager->availableControllers.end(), 
                                            controller);
                        if (it != joystickManager->availableControllers.end()) {
                            joystickManager->availableControllers.erase(it);
                            if (joystickManager->currentControllerIndex >=
                                joystickManager->availableControllers.size()) {
                                joystickManager->currentControllerIndex = (int)
                                    joystickManager->availableControllers.size() - 1;
                            }
                        }
                    }
                }];
    LabSelectJoyStick(0);
}

void LabShutdownJoystickManager() {
    //[[NSNotificationCenter defaultCenter] removeObserver:self];
    joystickManager.reset();
}

std::vector<std::string> LabGetJoystickNames() {
    std::vector<std::string> names;
    for (GCController* controller: joystickManager->availableControllers) {
        if (controller.vendorName)
            names.push_back([controller.vendorName UTF8String]);
        else
            names.push_back("Unknown Controller");
    }
    return names;
}

void LabSelectJoyStick(int index) {
    do {
        if (index < 0)
            break;
        if (index >= joystickManager->availableControllers.size())
            break;
        joystickManager->currentControllerIndex = index;
    } while (false);
}

std::string LabCurrentJoystickName() {
    if (joystickManager->currentControllerIndex >= 0 && joystickManager->currentControllerIndex < joystickManager->availableControllers.size()) {
        auto controller = joystickManager->availableControllers[joystickManager->currentControllerIndex];
        if (controller.vendorName)
            return [controller.vendorName UTF8String];
        else
            return "Unknown Controller";
    }
    return "No Controller Selected";
}

void LabUpdateJoystickState(LabJoystickState& state) {
    if (joystickManager->currentControllerIndex >= 0 && joystickManager->currentControllerIndex < joystickManager->availableControllers.size()) {
        GCExtendedGamepad* gamepad = joystickManager->availableControllers[joystickManager->currentControllerIndex].extendedGamepad;
        if (gamepad) {
            state.connected = true;
            state.leftStickX = gamepad.leftThumbstick.xAxis.value;
            state.leftStickY = gamepad.leftThumbstick.yAxis.value;
            state.rightStickX = gamepad.rightThumbstick.xAxis.value;
            state.rightStickY = gamepad.rightThumbstick.yAxis.value;
            state.leftTrigger = gamepad.leftTrigger.value;
            state.rightTrigger = gamepad.rightTrigger.value;
            state.aButton = gamepad.buttonA.isPressed;
            state.bButton = gamepad.buttonB.isPressed;
            state.xButton = gamepad.buttonX.isPressed;
            state.yButton = gamepad.buttonY.isPressed;
            state.leftShoulder = gamepad.leftShoulder.isPressed;
            state.rightShoulder = gamepad.rightShoulder.isPressed;
            state.leftStick = gamepad.leftThumbstickButton.isPressed;
            state.rightStick = gamepad.rightThumbstickButton.isPressed;
            state.dpadUp = gamepad.dpad.up.isPressed;
            state.dpadDown = gamepad.dpad.down.isPressed;
            state.dpadLeft = gamepad.dpad.left.isPressed;
            state.dpadRight = gamepad.dpad.right.isPressed;
            state.menuButton = gamepad.buttonMenu.isPressed;
            if (gamepad.buttonOptions)
                state.startButton = gamepad.buttonOptions.isPressed;
            else
                state.startButton = false;
        }
    }
    else {
        memset(&state, 0, sizeof(LabJoystickState));
    }
}
