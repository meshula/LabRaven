#include "Activities/Console/ConsoleActivity.hpp"
#include "Lab/App.h"
#include "Lab/CSP.hpp"
#include "Lab/LabDirectories.h"
#include "Lab/LabFileDialogManager.hpp"
#include "Providers/OpenUSD/OpenUSDProvider.hpp"

namespace lab {

PXR_NAMESPACE_USING_DIRECTIVE


class LoadStageModule : public CSP_Module {
public:
    LoadStageModule(CSP_Engine& engine)
    : CSP_Module(engine, "LoadStageModule")
    {
    }

    enum class Process : int {
        OpenRequest = 200,
        Opening,
        Error,
        OpenFile,
        Idle
    };

    static constexpr int toInt(Process p) { return static_cast<int>(p); }

    void LoadStage() {
        insertSubLayer = false;
        emit_event("file_open_request", toInt(Process::OpenRequest));
    }

    void InsertSubLayer() {
        insertSubLayer = true;
        emit_event("file_open_request", toInt(Process::OpenRequest));
    }

private:
    int pendingFile = 0;
    FileDialogManager::FileReq req;
    bool insertSubLayer = false;

    virtual void initialize_processes() override {
        add_process({toInt(Process::OpenRequest), "file_open_request",
                     [this]() {
                         printf("Entering file_open_request\n");
                         auto fdm = LabApp::instance()->fdm();
                         const char* dir = lab_pref_for_key("LoadStageDir");
                         pendingFile = fdm->RequestOpenFile(
                                                            {"usd","usda","usdc","usdz"},
                                                              dir? dir: ".");
                         this->emit_event("opening", toInt(Process::Opening));
                     }});

        add_process({toInt(Process::Opening), "opening",
            [this]() {
                        auto fdm = LabApp::instance()->fdm();
                        req = fdm->PopOpenedFile(pendingFile);
                        switch (req.status) {
                            case FileDialogManager::FileReq::notReady:
                                this->emit_event("opening", toInt(Process::Opening)); // continue polling
                                break;
                            case FileDialogManager::FileReq::expired:
                            case FileDialogManager::FileReq::canceled:
                                this->emit_event("file_error", toInt(Process::Error));
                                break;
                            default:
                                this->emit_event("file_open_success", toInt(Process::OpenFile));
                                break;
                        }
                     }});

        add_process({toInt(Process::Error), "file_error",
                     [this]() {
                         printf("Entering file_error\n");
                         std::cout << "Error: Failed to open file. Returning to Ready...\n";
                         this->emit_event("idle", toInt(Process::Idle));
                     }});

        add_process({toInt(Process::OpenFile), "file_success",
            [this]() {
                         printf("Entering file_success\n");
                         auto usd = OpenUSDProvider::instance();
                         if (usd) {
                             if (insertSubLayer) {
                                 auto stage = usd->Stage();
                                 auto rootLayer = stage->GetRootLayer();
                                 if (!rootLayer) {
                                     std::cerr << "Failed to retrieve root layer." << std::endl;
                                 }
                                 else {
                                     // insert this USDZ as the top subLayer
                                     rootLayer->InsertSubLayerPath(req.path, 0);

                                     // get the directory of path and save it as the default
                                     // directory for the next time the user loads a stage
                                     std::string dir = req.path.substr(0, req.path.find_last_of("/\\"));
                                     lab_set_pref_for_key("LoadStageDir", dir.c_str());
                                 }
                             }
                             else {
                                 usd->LoadStage(req.path);
                                 // get the directory of path and save it as the default
                                 // directory for the next time the user loads a stage
                                 std::string dir = req.path.substr(0, req.path.find_last_of("/\\"));
                                 lab_set_pref_for_key("LoadStageDir", dir.c_str());
                             }
                         }
                this->emit_event("idle", toInt(Process::Idle));
                     }});

        add_process({toInt(Process::Idle), "idle", []() {
            printf("Entering idle\n");
        }});
    }
};


class ReferenceLayerModule : public CSP_Module {
public:
    ReferenceLayerModule(CSP_Engine& engine)
    : CSP_Module(engine, "ReferenceLayerModule")
    {
    }

    bool instance = false;

    enum class Process : int {
        OpenRequest = 300,
        Opening,
        Error,
        OpenFile,
        Idle
    };

    static constexpr int toInt(Process p) { return static_cast<int>(p); }

    void InstanceLayer() {
        instance = true;
        emit_event("layer_reference_request", toInt(Process::OpenRequest));
    }
    void ReferenceLayer() {
        instance = false;
        emit_event("layer_reference_request", toInt(Process::OpenRequest));
    }

private:
    int pendingFile = 0;
    FileDialogManager::FileReq req;

    virtual void initialize_processes() override {
        add_process({toInt(Process::OpenRequest), "layer_reference_request",
            [this]() {
                         printf("Entering layer_reference_request\n");
                         auto fdm = LabApp::instance()->fdm();
                         const char* dir = lab_pref_for_key("LoadStageDir");
                         pendingFile = fdm->RequestOpenFile(
                                                            {"usd","usda","usdc","usdz"},
                                                              dir? dir: ".");
                this->emit_event("opening", toInt(Process::Opening));
                     }});

        add_process({toInt(Process::Opening), "opening",
            [this]() {
                        printf("Entering opening\n");
                        auto fdm = LabApp::instance()->fdm();
                        req = fdm->PopOpenedFile(pendingFile);
                        switch (req.status) {
                            case FileDialogManager::FileReq::notReady:
                                this->emit_event("opening", toInt(Process::Opening)); // continue polling
                                break;
                            case FileDialogManager::FileReq::expired:
                            case FileDialogManager::FileReq::canceled:
                                this->emit_event("file_error", toInt(Process::Error));
                                break;
                            default:
                                this->emit_event("file_open_success", toInt(Process::OpenFile));
                                break;
                        }
                     }});

        add_process({toInt(Process::Error), "file_error",
                     [this]() {
                         printf("Entering file_error\n");
                         std::cout << "Error: Failed to open file. Returning to Ready...\n";
                         this->emit_event("idle", toInt(Process::Idle));
                     }});

        add_process({toInt(Process::OpenFile), "file_success",
            [this]() {
                         printf("Entering file_success\n");
                         auto usd = OpenUSDProvider::instance();
                         if (usd) {
                             auto stage = usd->Stage();
                             if (stage) {
                                 auto mm = LabApp::instance()->mm();
                                 //auto cameraMode = mm->LockActivity(_self->cameraMode);
                                 pxr::GfVec3d pos(0,0,0);// = cameraMode->HitPoint();
                                 mm->EnqueueTransaction(Transaction{"Reference a layer", [this, usd, pos, stage](){
                                     std::string path(req.path);
                                     usd->CreateDefaultPrimIfNeeded("/Lab");
                                     auto defaultPrim = stage->GetDefaultPrim();
                                     if (!defaultPrim) {
                                         std::cerr << "No default prim on stage\n";
                                     }
                                     else {
                                         pxr::SdfPath primPath = defaultPrim.GetPath();
                                         primPath = primPath.AppendChild(TfToken("Models"));
                                         usd->ReferenceLayer(stage, path, primPath, true, pos, instance);
                                         // get the directory of path and save it as the default
                                         // directory for the next time the user loads a stage
                                         std::string dir = path.substr(0, path.find_last_of("/\\"));
                                         lab_set_pref_for_key("LoadStageDir", dir.c_str());
                                     }
                                 }});
                             }
                         }
                this->emit_event("idle", toInt(Process::Idle));
                     }});

        add_process({toInt(Process::Idle), "idle", []() {
                        printf("Entering idle\n");
                    }});
    }
};

class ExportStageModule : public CSP_Module {
public:
    ExportStageModule(CSP_Engine& engine)
        : CSP_Module(engine, "ExportStageModule")
    {
    }

    enum class Process : int {
        ExportRequest = 400,
        Saving,
        Error,
        ExportFile,
        Idle
    };

    static constexpr int toInt(Process p) { return static_cast<int>(p); }


private:
    int pendingFile = 0;
    FileDialogManager::FileReq req;

    virtual void initialize_processes() override {
        add_process({toInt(Process::ExportRequest), "file_export_request",
            [this]() {
                         printf("Entering file_export_request\n");
                         auto fdm = LabApp::instance()->fdm();
                         const char* dir = lab_pref_for_key("LoadStageDir");
                         pendingFile = fdm->RequestSaveFile(
                                                            {"usd","usda","usdc","usdz"},
                                                              dir? dir: ".");
                this->emit_event("saving", toInt(Process::Saving));
                     }});

        add_process({toInt(Process::Saving), "saving",
            [this]() {
                        auto fdm = LabApp::instance()->fdm();
                        req = fdm->PopOpenedFile(pendingFile);
                        switch (req.status) {
                            case FileDialogManager::FileReq::notReady:
                                this->emit_event("opening", toInt(Process::Saving)); // continue polling
                                break;
                            case FileDialogManager::FileReq::expired:
                            case FileDialogManager::FileReq::canceled:
                                this->emit_event("file_error", toInt(Process::Error));
                                break;
                            default:
                                this->emit_event("file_success", toInt(Process::ExportFile));
                                break;
                        }
                     }});

        add_process({toInt(Process::Error), "file_error",
                     [this]() {
                         printf("Entering file_error\n");
                         std::cout << "Error: Failed to save file. Returning to idle...\n";
                         this->emit_event("idle", toInt(Process::Idle));
                     }});

        add_process({toInt(Process::ExportFile), "file_success",
            [this]() {
                         printf("Entering file_success\n");
                         auto usd = OpenUSDProvider::instance();
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
                this->emit_event("idle", toInt(Process::Idle));
                     }});

        add_process({toInt(Process::Idle), "idle", []() {
            printf("Entering idle\n");
        }});
    }
};

class ShotTemplateModule : public CSP_Module {
public:
    ShotTemplateModule(CSP_Engine& engine)
    : CSP_Module(engine, "ShotTemplateModule")
    {
    }

    enum class Process : int {
        NameRequest = 500,
        PickFolder,
        Error,
        CreateShot,
        Idle
    };

    static constexpr int toInt(Process p) { return static_cast<int>(p); }

    void CreateShotFromTemplate() {
        emit_event("name_request", toInt(Process::NameRequest));
    }

private:
    int pendingDir = 0;
    FileDialogManager::FileReq req;
    std::string shotName;
    bool showNameDialog = false;
    static constexpr char popupName[] = "Create Shot from Template";

    virtual void initialize_processes() override {
        add_process({toInt(Process::NameRequest), "name_request",
            [this]() {
                printf("Entering name_request\n");
                showNameDialog = true;
                // Stay in this state until the dialog is completed
                this->emit_event("name_request", toInt(Process::NameRequest));
            }});

        add_process({toInt(Process::PickFolder), "pick_folder",
            [this]() {
                printf("Entering pick_folder\n");
                if (!pendingDir) {
                    auto fdm = LabApp::instance()->fdm();
                    const char* dir = lab_pref_for_key("LoadStageDir");
                    pendingDir = fdm->RequestPickFolder(dir ? dir : ".");
                }
                auto fdm = LabApp::instance()->fdm();
                req = fdm->PopOpenedFile(pendingDir);
                switch (req.status) {
                    case FileDialogManager::FileReq::notReady:
                        this->emit_event("pick_folder", toInt(Process::PickFolder)); // continue polling
                        break;
                    case FileDialogManager::FileReq::expired:
                    case FileDialogManager::FileReq::canceled:
                        this->emit_event("folder_error", toInt(Process::Error));
                        break;
                    default:
                        this->emit_event("folder_success", toInt(Process::CreateShot));
                        break;
                }
            }});

        add_process({toInt(Process::Error), "folder_error",
            [this]() {
                printf("Entering folder_error\n");
                std::cout << "Error: Failed to pick folder. Returning to idle...\n";
                pendingDir = 0;
                this->emit_event("idle", toInt(Process::Idle));
            }});

        add_process({toInt(Process::CreateShot), "folder_success",
            [this]() {
                printf("Entering folder_success\n");
                std::string path(req.path);
                auto mm = LabApp::instance()->mm();
                std::weak_ptr<ConsoleActivity> console;
                auto con = mm->LockActivity(console);
                std::string sn = shotName;
                mm->EnqueueTransaction(Transaction{"Create Shot from Template", [mm, path, sn](){
                    std::weak_ptr<ConsoleActivity> console;
                    auto con = mm->LockActivity(console);
                    con->Info("Creating Shot from Template");

                    auto usd = OpenUSDProvider::instance();
                    if (usd)
                        usd->CreateShotFromTemplate(path, sn);
                }});
                pendingDir = 0;
                this->emit_event("idle", toInt(Process::Idle));
            }});

        add_process({toInt(Process::Idle), "idle", []() {
            printf("Entering idle\n");
        }});
    }

public:
    void update() {
        if (showNameDialog) {
            ImGui::OpenPopup(popupName);
            if (ImGui::BeginPopupModal(popupName, NULL,
                                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize |
                                     ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize)) {
                static char shotNameBuffer[128] = "ShotName";
                ImGui::InputText("Shot Name", shotNameBuffer, IM_ARRAYSIZE(shotNameBuffer));
                if (ImGui::Button("OK###Shot", ImVec2(120, 0))) {
                    ImGui::CloseCurrentPopup();
                    shotName = shotNameBuffer;
                    showNameDialog = false;
                    this->emit_event("pick_folder", toInt(Process::PickFolder));
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel###Shot", ImVec2(120, 0))) {
                    ImGui::CloseCurrentPopup();
                    showNameDialog = false;
                    this->emit_event("idle", toInt(Process::Idle));
                }
                ImGui::EndPopup();
            }
        }
    }
};

} // namespace lab
