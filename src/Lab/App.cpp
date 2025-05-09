// App.cpp
// Author: Nick Porcino
// Created on 2024-11-07

#include "App.h"
#include "StudioCore.hpp"
#include "LabFileDialogManager.hpp"
#include "AppTheme.h"
#include "CoreProviders/Color/nanocolor.h"
#include "../Providers/Scheme/SchemeProvider.hpp"

#include "imgui.h"
#include "imgui_internal.h" // for DockBuilderGetCentralNode

#include <iostream>
#include <mutex>
#include <time.h>

using lab::FileDialogManager;
using lab::Orchestrator;


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

struct LabApp::data {
    FileDialogManager _fdm;
    Orchestrator _mm;
    CSP_Engine _csp;
    LabViewInteraction _vi;
    int _model_generation = 0;
    bool should_terminate = false;
    bool power_save = true;
    bool reset_window_positions = false;
    int suspend_power_save = 0;
};

LabApp::LabApp() {
    _instance = this;
    _self = new data();
    _self->_csp.run();

    App::Init = [](int argc, char** argv, int width, int height) {
        //MainInit(argc, argv, width, height);
    };
    App::Gui = RunUI;
    App::Cleanup = []() {
        //MainCleanup();
    };
    App::FileDrop = [](int count, const char** paths) {
        //FileDropCallback(count, paths);
    };
    App::IsRunning = AppIsRunning;
    App::PowerSave = GetPowerSave;
    App::SetPowerSave = SetPowerSaveState;
}

LabApp::~LabApp() {
    delete _self;
    _instance = nullptr;
}

LabApp* LabApp::instance() {
    if (!_instance)
        _instance = new LabApp();
    return _instance;
}

bool LabApp::PowerSave() const { return _self->power_save && !_self->suspend_power_save; }
void LabApp::SetPowerSave(bool v) { _self->power_save = v; }
void LabApp::SuspendPowerSave(int frames) { _self->suspend_power_save = frames; }

//static
bool LabApp::GetPowerSave() {
    return _instance->PowerSave();
}

//static
void LabApp::SetPowerSaveState(bool v) {
    _instance->SetPowerSave(v);
}

//static
void LabApp::RunUI() {
    if (_instance->_self->suspend_power_save > 0) {
        --_instance->_self->suspend_power_save;
    }
    _instance->UpdateMainWindow(_instance->FrameTime_ms() * 1.0e-3f,
                                ImGui::IsWindowHovered(), ImGui::IsWindowFocused());
}

//static
bool LabApp::AppIsRunning() {
    return !_instance->_self->should_terminate;
}

void LabApp::ResetWindowPositions() {
    _self->reset_window_positions = true;
}


FileDialogManager* LabApp::fdm() { return &_self->_fdm; }
Orchestrator* LabApp::mm() { return &_self->_mm; }
CSP_Engine* LabApp::csp() { return &_self->_csp; }

// dimensions of the viewport of the main window ~ it is the area not
// occupied by docked panels.
// the value always reflects the most recent invocation of Update
LabViewDimensions LabApp::GetViewportDimensions() const { return _self->_vi.view; }

LabApp* LabApp::_instance = nullptr;

lab::Orchestrator* gOrchestrator() {
    auto i = LabApp::instance();
    if (i)
        return i->mm();
    return nullptr;
}

// Undock all docked windows
void UndockAllWindows()
{
    ImGuiContext& g = *ImGui::GetCurrentContext();
    for (ImGuiWindow* window : g.Windows)
    {
        if (window->DockNode)
            ImGui::DockBuilderDockWindow(window->Name, 0); // Move to no dockspace
    }
}

// Reset docking layout
void ResetLayout()
{
    ImGui::DockBuilderRemoveNode(ImGui::GetID("MainDockspace"));
}

// Render the Window Menu dynamically
void ShowWindowMenu()
{
    if (ImGui::BeginMenu("Window"))
    {
        if (ImGui::MenuItem("Undock All"))
            UndockAllWindows();

        if (ImGui::MenuItem("Reset Layout"))
            ResetLayout();

        ImGui::Separator();

        // Iterate over all windows in ImGui
        ImGuiContext& g = *ImGui::GetCurrentContext();
        for (ImGuiWindow* window : g.Windows)
        {
            if (!(window->Flags & ImGuiWindowFlags_NoTitleBar)) // Ignore hidden/internal windows
            {
                if (ImGui::MenuItem(window->Name)) {
                    window->Hidden = false; // Ensure it's not marked as hidden
                    ImGui::BringWindowToFocusFront(window);
                }
            }
        }

        ImGui::EndMenu();
    }
}


void LabApp::UpdateMainWindow(float dt, bool viewport_hovered, bool viewport_dragging)
{
    auto& mm = _self->_mm;
    mm.ServiceTransactionsAndActivities(dt);
    auto currStudio = mm.CurrentStudio();
    static auto studioNames = mm.StudioNames();
    auto activityNames = mm.ActivityNames();

    if (_self->reset_window_positions) {
        _self->reset_window_positions = false;
    }

    if (ImGui::BeginMainMenuBar()) {
        std::string curr = "Welcome";
        if (currStudio)
            curr = currStudio->Name();
        if (curr == "Empty")
            curr = "Welcome";
        auto menuName = curr + "###St";
        int menuIdx = 0;
        if (ImGui::BeginMenu(menuName.c_str())) {
            for (auto m : studioNames) {
                auto studio = mm.FindStudio(m);
                if (studio) {
                    ptrdiff_t id = (ptrdiff_t) this;
                    id += menuIdx;
                    ImGui::PushID((int) id);
                    bool active = studio->Name() == curr;
                    if (ImGui::MenuItem(m.c_str(), nullptr, active, true)) {
                        if (!active)
                            mm.ActivateStudio(m);
                    }
                    ImGui::PopID();
                }
            }
            if (studioNames.size() > 1)
                ImGui::Separator();
            if (ImGui::MenuItem("Quit")) {
                _self->should_terminate = true;
            }
            ImGui::EndMenu();
        }

        if (activityNames.size() > 0) {
            ptrdiff_t id = (ptrdiff_t) currStudio;
            if (ImGui::BeginMenu("Activities##mmenu")) {
                for (auto m : activityNames) {
                    auto activity = mm.FindActivity(m);
                    if (activity) {
                        bool active = activity->IsActive() && activity->UIVisible();
                        ImGui::PushID((int) id);
                        if (ImGui::MenuItem(activity->Name().c_str(), nullptr, active, true)) {
                            if (active) {
                                mm.DeactivateActivity(m);
                            }
                            else {
                                mm.ActivateActivity(m);
                            }
                        }
                        ImGui::PopID();
                        ++id;
                    }
                }
                ImGui::EndMenu();
            }
        }

        mm.RunMainMenu();

        ShowWindowMenu();

        ImGui::EndMainMenuBar();
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGuiID dockspace_id = lab::DockSpaceOverViewport(viewport,
                                                      //ImGuiDockNodeFlags_NoDockingInCentralNode |
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

    _self->_vi.view = {
        (float) viewport->Size.x, (float) viewport->Size.y,
        view_pos_x, view_pos_y, view_sz_x, view_sz_y };
    _self->_vi.dt = dt;
    mm.RunActivityUIs(_self->_vi);

    static bool was_dragging = false;

    ImVec2 mousePos = io.MousePos;
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 mousePosInWindow = ImVec2(mousePos.x - windowPos.x, mousePos.y - windowPos.y);

    _self->_vi.x = io.DisplayFramebufferScale.x * (mousePosInWindow.x - centralNode->Pos.x);
    _self->_vi.y = io.DisplayFramebufferScale.y * (mousePosInWindow.y - centralNode->Pos.y);

    if (viewport_dragging) {
        _self->_vi.start = !was_dragging;
        _self->_vi.end = false;
        mm.RunViewportDragging(_self->_vi);
    }
    else {
        if (was_dragging) {
            _self->_vi.start = false;
            _self->_vi.end = true;
            mm.RunViewportDragging(_self->_vi);
        }
        else {
            mm.RunViewportHovering(_self->_vi);
        }
    }
    was_dragging = viewport_dragging;
}

// elapsed time since app start
double LabApp::Now_ms() const {
    return ImGui::GetTime() * 1000.0;
}

// time since last frame
double LabApp::FrameTime_ms() const {
    return ImGui::GetIO().DeltaTime * 1000.0;
}


} // lab


App* gApp() {
#if 0
    auto scheme = lab::SchemeProvider::instance();

    // First, define the function
    std::string define_test = "(define (test-function x) (* x 2))";
    scheme->EvalScheme(define_test);

    // Then test it separately
    std::string test_result = scheme->EvalScheme("(if (procedure? test-function) \"Success\" \"Failure\")");
    std::cout << "Test result: " << test_result << std::endl;

    // Try calling the function
    std::string call_result = scheme->EvalScheme("(test-function 21)");
    std::cout << "Function call result: " << call_result << std::endl;

    // Define make-shader
    std::string define_make_shader = "(define (make-shader name inputs uniforms varyings body-fn) (list 'shader name inputs uniforms varyings body-fn))";
    scheme->EvalScheme(define_make_shader);

    // Test if it's defined
    std::string shader_test = scheme->EvalScheme("(if (procedure? make-shader) \"make-shader is defined\" \"make-shader is NOT defined\")");
    std::cout << "make-shader test: " << shader_test << std::endl;

    // Try using it
    std::string use_result = scheme->EvalScheme("(make-shader 'test '() '() '() (lambda () 'dummy))");
    std::cout << "Using make-shader: " << use_result << std::endl;

    scheme->ExerciseShaderTranspiler("/Users/nporcino/dev/labraven/src/Providers/Scheme/shader-system.scm",
    "/Users/nporcino/dev/labraven/src/Providers/Scheme/disney-principled-brdf.scm");
#endif
#if 0
    // create an NcColorSpace for ap1 to ciexyz:
    const NcColorSpace* src = NcGetNamedColorSpace("lin_ap1");
    // create one for ciexyz:
    const NcColorSpace* dst = NcGetNamedColorSpace("identity");
    // compute the bradford thing
    const NcColorSpace* dst2 = NcGetNamedColorSpace("lin_rec709");
    // compute the bradford thing
    NcM33f bradford = NcGetRGBToRGBMatrixBradford(src, dst);
    NcM33f bradford2 = NcGetRGBToRGBMatrixBradford(src, dst2);
#endif

    auto t = time(NULL);

    static std::unique_ptr<App> _app;
    static std::once_flag _app_once_flag;
    std::call_once(_app_once_flag, []() {
        _app.reset(new lab::LabApp());
    });
    return _app.get();
}
