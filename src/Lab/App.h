// File: App.h
// Author: Nick Porcino
// Created on 2024-11-07

#ifndef App_h
#define App_h

#include "Lab/StudioCore.hpp"

typedef struct App {
    void (*Init)(int argc, char** argv, int width, int height);
    void (*Gui)();
    void (*Cleanup)();
    void (*FileDrop)(int count, const char** paths);
    bool (*IsRunning)();
} App;

App* gApp();

#ifdef __cplusplus
namespace lab {
class Orchestrator;
class FileDialogManager;

Orchestrator* gOrchestrator();


class LabApp : public App {
    struct data;
    data* _self = nullptr;
    
    void UpdateMainWindow(float dt, bool viewport_hovered, bool viewport_dragging);
    static void RunUI();
    static bool AppIsRunning();
    
    static LabApp* _instance;
public:
    
    LabApp();
    ~LabApp();
    
    static LabApp* instance();
    
    FileDialogManager* fdm();
    Orchestrator* mm();
    
    // dimensions of the viewport of the main window ~ it is the area not
    // occupied by docked panels.
    // the value always reflects the most recent invocation of Update
    LabViewDimensions GetViewportDimensions() const;
};

} // lab
#endif

#endif /* App_h */
