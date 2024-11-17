// File: App.h
// Author: Nick Porcino
// Created on 2024-11-07

#ifndef App_h
#define App_h

typedef struct App {
    void (*Init)(int argc, char** argv, int width, int height);
    void (*Gui)();
    void (*Cleanup)();
    void (*FileDrop)(int count, const char** paths);
    bool (*IsRunning)();
} App;

App* gApp();

#ifdef __cplusplus
namespace lab { class Orchestrator; }
lab::Orchestrator* gOrchestrator();
#endif

#endif /* App_h */
