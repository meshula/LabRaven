//
//  OpenUSDActivity.cpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 1/7/24.
//  Copyright © 2024 Nick Porcino. All rights reserved.
//
#include "OpenUSDActivity.hpp"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "Lab/App.h"
#include "Lab/CSP.hpp"
#include "Lab/LabFileDialogManager.hpp"
#include "Lab/LabDirectories.h"
#include "Activities/Console/ConsoleActivity.hpp"
#include "Providers/OpenUSD/OpenUSDProvider.hpp"
#include <pxr/usd/usd/prim.h>

namespace lab {

PXR_NAMESPACE_USING_DIRECTIVE

class LoadStageModule : public CSP_Module {
public:
    LoadStageModule(CSP_Engine& engine)
        : CSP_Module(engine, "LoadStageModule") {
        initialize_states();
    }

private:
    int pendingFile = 0;
    FileDialogManager::FileReq req;

    void initialize_states() {
        processes.push_back({"OpenRequest", "file_open_request",
                             [this](CSP_Module& module, int) {
                                 printf("Entering file_open_request\n");
                                 auto fdm = LabApp::instance()->fdm();
                                 const char* dir = lab_pref_for_key("LoadStageDir");
                                 pendingFile = fdm->RequestOpenFile(
                                                                    {"usd","usda","usdc","usdz"},
                                                                      dir? dir: ".");
                                 module.emit_event("opening", 0);
                             }});

        processes.push_back({"Opening", "opening",
                            [this](CSP_Module& module, int) {
                                printf("Entering opening\n");
                                auto fdm = LabApp::instance()->fdm();
                                req = fdm->PopOpenedFile(pendingFile);
                                switch (req.status) {
                                    case FileDialogManager::FileReq::notReady:
                                        module.emit_event("opening", 0); // continue polling
                                        break;
                                    case FileDialogManager::FileReq::expired:
                                    case FileDialogManager::FileReq::canceled:
                                        module.emit_event("file_error", 0);
                                        break;
                                    default:
                                        module.emit_event("file_open_success", 0);
                                        break;
                                }
                             }});

        processes.push_back({"Error", "file_error",
                             [](CSP_Module& module, int) {
                                 printf("Entering file_error\n");
                                 std::cout << "Error: Failed to open file. Returning to Ready...\n";
                                 module.emit_event("idle", 0);
                             }});

        processes.push_back({"OpenFile", "file_success",
                             [this](CSP_Module& module, int) {
                                 printf("Entering file_success\n");
                                 std::shared_ptr<OpenUSDProvider> usd = OpenUSDProvider::instance();
                                 if (usd) {
                                     usd->LoadStage(req.path);
                                     // get the directory of path and save it as the default
                                     // directory for the next time the user loads a stage
                                     std::string dir = req.path.substr(0, req.path.find_last_of("/\\"));
                                     lab_set_pref_for_key("LoadStageDir", dir.c_str());
                                 }
                                 module.emit_event("idle", 0);
                             }});

        processes.push_back({"Idle", "idle", [](CSP_Module&, int) {
            printf("Entering idle\n");
        }});
    }
};


class ReferenceLayerModule : public CSP_Module {
public:
    ReferenceLayerModule(CSP_Engine& engine)
        : CSP_Module(engine, "ReferenceLayerModule") {
        initialize_states();
    }

    bool instance = false;

private:
    int pendingFile = 0;
    FileDialogManager::FileReq req;

    void initialize_states() {
        processes.push_back({"OpenRequest", "layer_reference_request",
                             [this](CSP_Module& module, int) {
                                 printf("Entering layer_reference_request\n");
                                 auto fdm = LabApp::instance()->fdm();
                                 const char* dir = lab_pref_for_key("LoadStageDir");
                                 pendingFile = fdm->RequestOpenFile(
                                                                    {"usd","usda","usdc","usdz"},
                                                                      dir? dir: ".");
                                 module.emit_event("opening", 0);
                             }});

        processes.push_back({"Opening", "opening",
                            [this](CSP_Module& module, int) {
                                printf("Entering opening\n");
                                auto fdm = LabApp::instance()->fdm();
                                req = fdm->PopOpenedFile(pendingFile);
                                switch (req.status) {
                                    case FileDialogManager::FileReq::notReady:
                                        module.emit_event("opening", 0); // continue polling
                                        break;
                                    case FileDialogManager::FileReq::expired:
                                    case FileDialogManager::FileReq::canceled:
                                        module.emit_event("file_error", 0);
                                        break;
                                    default:
                                        module.emit_event("file_open_success", 0);
                                        break;
                                }
                             }});

        processes.push_back({"Error", "file_error",
                             [](CSP_Module& module, int) {
                                 printf("Entering file_error\n");
                                 std::cout << "Error: Failed to open file. Returning to Ready...\n";
                                 module.emit_event("idle", 0);
                             }});

        processes.push_back({"OpenFile", "file_success",
                             [this](CSP_Module& module, int) {
                                 printf("Entering file_success\n");
                                 std::shared_ptr<OpenUSDProvider> usd = OpenUSDProvider::instance();
                                 if (usd) {
                                     auto stage = usd->Stage();
                                     if (stage) {
                                         auto mm = LabApp::instance()->mm();
                                         //auto cameraMode = mm->LockActivity(_self->cameraMode);
                                         pxr::GfVec3d pos(0,0,0);// = cameraMode->HitPoint();
                                         mm->EnqueueTransaction(Transaction{"Reference a layer", [this, usd, pos, stage](){
                                             std::string path(req.path);
                                             pxr::SdfPath primPath = stage->GetDefaultPrim().GetPath();
                                             primPath = primPath.AppendChild(TfToken("Models"));
                                             usd->ReferenceLayer(stage, path, primPath, true, pos, instance);
                                             // get the directory of path and save it as the default
                                             // directory for the next time the user loads a stage
                                             std::string dir = path.substr(0, path.find_last_of("/\\"));
                                             lab_set_pref_for_key("LoadStageDir", dir.c_str());
                                         }});
                                     }
                                 }
                                 module.emit_event("idle", 0);
                             }});

        processes.push_back({"Idle", "idle", [](CSP_Module&, int) {
                                printf("Entering idle\n");
                            }});
    }
};

class ExportStageModule : public CSP_Module {
public:
    ExportStageModule(CSP_Engine& engine)
        : CSP_Module(engine, "ExportStageModule") {
        initialize_states();
    }

private:
    int pendingFile = 0;
    FileDialogManager::FileReq req;

    void initialize_states() {
        processes.push_back({"Export Request", "file_export_request",
                             [this](CSP_Module& module, int) {
                                 printf("Entering file_export_request\n");
                                 auto fdm = LabApp::instance()->fdm();
                                 const char* dir = lab_pref_for_key("LoadStageDir");
                                 pendingFile = fdm->RequestSaveFile(
                                                                    {"usd","usda","usdc","usdz"},
                                                                      dir? dir: ".");
                                 module.emit_event("saving", 0);
                             }});

        processes.push_back({"Saving", "saving",
                            [this](CSP_Module& module, int) {
                                printf("Entering saving\n");
                                auto fdm = LabApp::instance()->fdm();
                                req = fdm->PopOpenedFile(pendingFile);
                                switch (req.status) {
                                    case FileDialogManager::FileReq::notReady:
                                        module.emit_event("opening", 0); // continue polling
                                        break;
                                    case FileDialogManager::FileReq::expired:
                                    case FileDialogManager::FileReq::canceled:
                                        module.emit_event("file_error", 0);
                                        break;
                                    default:
                                        module.emit_event("file_success", 0);
                                        break;
                                }
                             }});

        processes.push_back({"Error", "file_error",
                             [](CSP_Module& module, int) {
                                 printf("Entering file_error\n");
                                 std::cout << "Error: Failed to save file. Returning to idle...\n";
                                 module.emit_event("idle", 0);
                             }});

        processes.push_back({"ExportFile", "file_success",
                             [this](CSP_Module& module, int) {
                                 printf("Entering file_success\n");
                                 std::shared_ptr<OpenUSDProvider> usd = OpenUSDProvider::instance();
                                 if (usd) {
                                     std::string path(req.path);
                                     auto mm = LabApp::instance()->mm();
                                     std::weak_ptr<ConsoleActivity> cap;
                                     auto console = mm->LockActivity(cap);
                                     std::string msg = "Exporting stage: " + path;
                                     console->Info(msg);
                                     usd->ExportStage(path);

                                     // get the directory of path and save it as the default
                                     // directory for the next time the user loads a stage
                                     std::string dir = path.substr(0, path.find_last_of("/\\"));
                                     lab_set_pref_for_key("LoadStageDir", dir.c_str());

                                     // create a string of the form yyyymmdd-hhmmss
                                     // to append to the session file name
                                     time_t rawtime;
                                     struct tm* timeinfo;
                                     char buffer[80];
                                     time(&rawtime);
                                     timeinfo = localtime(&rawtime);
                                     strftime(buffer, sizeof(buffer), "%Y%m%d-%H%M%S", timeinfo);
                                     std::string str(buffer);

                                     // create a new path by stripping of the extension,
                                     // adding -session, then set the extension to usda.
                                     std::string sessionPath = path.substr(0, path.find_last_of("."));
                                     sessionPath += "-session-" + str + ".usda";
                                     usd->ExportSessionLayer(sessionPath);
                                 }
                                 module.emit_event("idle", 0);
                             }});

        processes.push_back({"Idle", "idle", [](CSP_Module&, int) {
            printf("Entering idle\n");
        }});
    }
};


struct OpenUSDActivity::data {
    CSP_Engine engine;
    data()
        : loadStageModule(engine)
        , exportStageModule(engine)
        , referenceLayerModule(engine) {}
    ~data() = default;

    bool run_shot_template_ui = false;
    bool run_pick_template_folder = false;
    int pending_template_dir = 0;
    std::string constructShotName;
    std::weak_ptr<ConsoleActivity> console;
    LoadStageModule loadStageModule;
    ExportStageModule exportStageModule;
    ReferenceLayerModule referenceLayerModule;
};

OpenUSDActivity::OpenUSDActivity() : Activity() {
    _self = new OpenUSDActivity::data();

    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<OpenUSDActivity*>(instance)->RunUI(*vi);
    };
    activity.Menu = [](void* instance) {
        static_cast<OpenUSDActivity*>(instance)->Menu();
    };
}

OpenUSDActivity::~OpenUSDActivity() {
    delete _self;
}

void OpenUSDActivity::RunUI(const LabViewInteraction&) {
    auto fdm = LabApp::instance()->fdm();
    if (_self->run_pick_template_folder) {
        if (!_self->pending_template_dir) {
            const char* dir = lab_pref_for_key("LoadStageDir");
            _self->pending_template_dir = fdm->RequestPickFolder(dir? dir: ".");
        }
        else {
            do {
                auto req = fdm->PopOpenedFile(_self->pending_template_dir);
                if (req.status == FileDialogManager::FileReq::notReady)
                    break;

                if (req.status == FileDialogManager::FileReq::expired ||
                    req.status == FileDialogManager::FileReq::canceled) {
                    _self->pending_template_dir = 0;
                    _self->run_pick_template_folder = false;
                    _self->run_shot_template_ui = false;
                    break;
                }
                std::string path(req.path.c_str());
                auto mm = LabApp::instance()->mm();
                auto console = mm->LockActivity(_self->console);
                std::string sn = _self->constructShotName;
                mm->EnqueueTransaction(Transaction{"Create Shot from Template", [this, mm, path, sn](){
                    auto console = mm->LockActivity(_self->console);
                    console->Info("Creating Shot from Template");

                    std::shared_ptr<OpenUSDProvider> usd = OpenUSDProvider::instance();
                    if (usd)
                        usd->CreateShotFromTemplate(path, sn);
                }});

                _self->pending_template_dir = 0;
                _self->run_pick_template_folder = false;
                _self->run_shot_template_ui = false;
            } while(false);
        }
    }
    else if (_self->run_shot_template_ui) {
        // pop open a dialog box asking for the shot name.
        // if the OK button is selected then continue by asking
        // for the destination directory.

        const char* popupName = "Create Shot from Template";

        ImGui::OpenPopup(popupName);
        if (ImGui::BeginPopupModal(popupName, NULL,
                                   ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize |
                                   ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
            static char shotName[128] = "ShotName";
            ImGui::InputText("Shot Name", shotName, IM_ARRAYSIZE(shotName));
            if (ImGui::Button("OK###Shot", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
                _self->constructShotName.assign(shotName);
                _self->run_shot_template_ui = false;
                _self->run_pick_template_folder = true;
            }
            ImGui::SameLine();
            if (ImGui::Button("Cancel###Shot", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
                _self->run_shot_template_ui = false;
            }
            ImGui::EndPopup();
        }
    }
}

void OpenUSDActivity::Menu() {
    Orchestrator* mm = Orchestrator::Canonical();
    if (ImGui::BeginMenu("Stage")) {
        if (ImGui::MenuItem("Load Stage ...")) {
            _self->loadStageModule.emit_event("file_open_request", 0);
        }
        if (ImGui::MenuItem("New Stage")) {
            mm->EnqueueTransaction(Transaction{"New Stage", [](){
                std::shared_ptr<OpenUSDProvider> usd = OpenUSDProvider::instance();
                if (usd)
                    usd->SetEmptyStage();
            }});
        }
        if (ImGui::MenuItem("Create Shot from Template...")) {
            _self->run_shot_template_ui = true;
        }
        if (ImGui::MenuItem("Export Stage ...")) {
            _self->exportStageModule.emit_event("file_export_request", 0);
        }
        if (ImGui::MenuItem("Test referencing")) {
            mm->EnqueueTransaction(Transaction{"Test referencing", [this]() {
                std::shared_ptr<OpenUSDProvider> usd = OpenUSDProvider::instance();
                if (usd)
                    usd->TestReferencing();
            }});
        }
        std::shared_ptr<OpenUSDProvider> usd = OpenUSDProvider::instance();
        auto stage = usd->Stage();
        if (!stage) {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f); // Adjust alpha to make it look disabled
        }
        static bool at_hit_point = true;
        if (ImGui::MenuItem("Reference a Layer ...")) {
            _self->referenceLayerModule.instance = false;
            _self->referenceLayerModule.emit_event("layer_reference_requeset", 0);
        }
        if (ImGui::MenuItem("Instance a Layer ...")) {
            _self->referenceLayerModule.instance = true;
            _self->referenceLayerModule.emit_event("layer_reference_requeset", 0);
        }

        ImGui::Indent(60);
        ImGui::Checkbox("At Hit Point", &at_hit_point);
        ImGui::Unindent();

        if (!stage) {
            ImGui::PopStyleVar();
        }
        ImGui::EndMenu();
    }
}

} // lab
