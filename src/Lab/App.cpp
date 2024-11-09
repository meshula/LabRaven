// App.cpp
// Author: Nick Porcino
// Created on 2024-11-07

#include "App.h"
#include "Modes.hpp"
#include "Lab/LabFileDialogManager.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h" // for DockBuilderGetCentralNode

#include <iostream>
#include <mutex>

using lab::FileDialogManager;
using lab::ModeManager;


using std::vector;
using std::string;
using std::cout;
using std::endl;

namespace lab {

namespace {

// Tips: Use with ImGuiDockNodeFlags_PassthruCentralNode!
// The limitation with this call is that your window won't have a menu bar.
// Even though we could pass window flags, it would also require the user to be 
// able to call BeginMenuBar() somehow meaning we can't Begin/End in a single function.
// But you can also use BeginMainMenuBar(). If you really want a menu bar inside the
// same window as the one hosting the dockspace, you will need to copy this code somewhere and tweak it.

ImGuiID DockSpaceOverViewport(const ImGuiViewport* viewport, ImGuiDockNodeFlags dockspace_flags, const ImGuiWindowClass* window_class)
{
    using namespace ImGui;
    if (viewport == NULL)
        viewport = GetMainViewport();
    
    SetNextWindowPos(viewport->WorkPos);
    SetNextWindowSize(viewport->WorkSize);
    SetNextWindowViewport(viewport->ID);
    
    ImGuiWindowFlags host_window_flags; /* = 0;
    host_window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                         ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking;
    host_window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        host_window_flags |= ImGuiWindowFlags_NoBackground;*/
    
    host_window_flags = ImGuiWindowFlags_NoTitleBar | 
                        ImGuiWindowFlags_NoCollapse |
                        ImGuiWindowFlags_NoBackground |
                        ImGuiDockNodeFlags_PassthruCentralNode;
    
    char label[32];
    ImFormatString(label, IM_ARRAYSIZE(label), "DockSpaceViewport_%08X", viewport->ID);
    
    PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    Begin(label, NULL, host_window_flags);
    PopStyleVar(3);
    ImGuiID dockspace_id = GetID("DockSpace");
    DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags, window_class);
    End();
    
    return dockspace_id;
}

} // anon
} // lab

class LabApp : public App {
    FileDialogManager _fdm;
    ModeManager _mm;
    LabViewInteraction _vi;
    int _model_generation = 0;
    bool _should_terminate = false;

    void UpdateMainWindow(float dt, bool viewport_hovered, bool viewport_dragging);
    static void RunUI() {
        instance->UpdateMainWindow(instance->_vi.dt, 
                        ImGui::IsWindowHovered(), ImGui::IsWindowFocused());
    }

    static bool AppIsRunning() {
        return !instance->_should_terminate;
    }

 public:
    static LabApp* instance;

    LabApp() {
        instance = this;

        Init = [](int argc, char** argv, int width, int height) {
            //MainInit(argc, argv, width, height);
        };
        Gui = RunUI;
        Cleanup = []() {
            //MainCleanup();
        };
        FileDrop = [](int count, const char** paths) {
            //FileDropCallback(count, paths);
        };
        IsRunning = AppIsRunning;
    }

    FileDialogManager* fdm() { return &_fdm; }
    ModeManager* mm() { return &_mm; }

    // dimensions of the viewport of the main window ~ it is the area not
    // occupied by docked panels.
    // the value always reflects the most recent invocation of Update
    LabViewDimensions GetViewportDimensions() const { return _vi.view; }
};

LabApp* LabApp::instance = nullptr;

lab::ModeManager* gModeManager() {
    if (LabApp::instance)
        return LabApp::instance->mm();
    return nullptr;
}

void LabApp::UpdateMainWindow(float dt, bool viewport_hovered, bool viewport_dragging)
{
    auto& mm = _mm;
    mm.UpdateTransactionQueueActivationAndModes();
    auto currMajorMode = mm.CurrentMajorMode();
    static auto majorModeNames = mm.MajorModeNames();
    static auto activityNames = mm.ActivityNames();
        
    if (ImGui::BeginMainMenuBar()) {
        std::string curr = "Welcome";
        if (currMajorMode)
            curr = currMajorMode->Name();
        if (curr == "Empty")
            curr = "Welcome";
        auto menuName = curr + "###MM";
        if (ImGui::BeginMenu(menuName.c_str())) {
            for (auto m : majorModeNames) {
                auto menuName = m + "###MjM";
                auto mode = mm.FindMode(m);
                if (mode) {
                    bool active = mm.CurrentMajorMode()->Name() == m;
                    if (ImGui::MenuItem(menuName.c_str(), nullptr, active, true)) {
                        if (!active)
                            mm.ActivateMajorMode(m);
                    }
                }
            }
            if (majorModeNames.size() > 1)
                ImGui::Separator();
            if (ImGui::MenuItem("Quit")) {
                _should_terminate = true;
            }
            ImGui::EndMenu();
        }

        if (activityNames.size() > 0) {
            if (ImGui::BeginMenu("Modes##mmenu")) {
                for (auto m : activityNames) {
                    auto menuName = m + "###mM";
                    auto mode = mm.FindActivity(m);
                    if (mode) {
                        bool active = mode->IsActive();
                        if (ImGui::MenuItem(menuName.c_str(), nullptr, active, true)) {
                            if (active) {
                                mm.DeactivateActivity(m);
                            }
                            else {
                                mm.ActivateActivity(m);
                            }
                        }
                    }
                }
                ImGui::EndMenu();
            }
        }

        mm.RunMainMenu();
        ImGui::EndMainMenuBar();
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGuiID dockspace_id = lab::DockSpaceOverViewport(viewport, 
                                    ImGuiDockNodeFlags_NoDockingInCentralNode |
                                    ImGuiDockNodeFlags_PassthruCentralNode,
                                    NULL);

    // outline the remaining viewport region
    // https://github.com/ocornut/imgui/issues/5921
    ImGuiDockNode* centralNode = ImGui::DockBuilderGetCentralNode(dockspace_id);
    bool centralNodeHovered = viewport_dragging || viewport_hovered;
    
    // Draw a red rectangle in the central node just for demonstration purposes
    ImGui::GetBackgroundDrawList()->AddRect(   centralNode->Pos,
                                             { centralNode->Pos.x + centralNode->Size.x,
                                               centralNode->Pos.y + centralNode->Size.y },
                                             IM_COL32(0, centralNodeHovered? 100 : 0, 0, 255),
                                             0.f,
                                             ImDrawFlags_None,
                                             3.f);

    ImGuiIO& io = ImGui::GetIO();
    float view_pos_x = centralNode->Pos.x  * io.DisplayFramebufferScale.x;
    float view_pos_y = centralNode->Pos.y  * io.DisplayFramebufferScale.y;
    float view_sz_x =  centralNode->Size.x * io.DisplayFramebufferScale.x;
    float view_sz_y =  centralNode->Size.y * io.DisplayFramebufferScale.y;
    
    /*   _   _ ___
        | | | |_ _|
        | | | || |
        | |_| || |
         \___/|___|  */

    _vi.view = {
        (float) viewport->Size.x, (float) viewport->Size.y,
        view_pos_x, view_pos_y, view_sz_x, view_sz_y };
    _vi.dt = dt;
    mm.RunModeUIs(_vi);

    static bool was_dragging = false;
    
    ImVec2 mousePos = io.MousePos;
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 mousePosInWindow = ImVec2(mousePos.x - windowPos.x, mousePos.y - windowPos.y);

    _vi.x = io.DisplayFramebufferScale.x * (mousePosInWindow.x - centralNode->Pos.x);
    _vi.y = io.DisplayFramebufferScale.y * (mousePosInWindow.y - centralNode->Pos.y);

    if (viewport_dragging) {
        _vi.start = !was_dragging;
        _vi.end = false;
        mm.RunViewportDragging(_vi);
    }
    else {
        if (was_dragging) {
            _vi.start = false;
            _vi.end = true;
            mm.RunViewportDragging(_vi);
        }
        else {
            mm.RunViewportHovering(_vi);
        }
    }
    was_dragging = viewport_dragging;
}

App* gApp() {
    static std::unique_ptr<App> _app;
    static std::once_flag _app_once_flag;
    std::call_once(_app_once_flag, []() {
        _app.reset(new LabApp());
    });
    return _app.get();
}
