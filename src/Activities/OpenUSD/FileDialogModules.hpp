
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
        emit_event("file_open_request", toInt(Process::OpenRequest));
    }

private:
    int pendingFile = 0;
    FileDialogManager::FileReq req;

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
                         std::shared_ptr<OpenUSDProvider> usd = OpenUSDProvider::instance();
                         if (usd) {
                             usd->LoadStage(req.path);
                             // get the directory of path and save it as the default
                             // directory for the next time the user loads a stage
                             std::string dir = req.path.substr(0, req.path.find_last_of("/\\"));
                             lab_set_pref_for_key("LoadStageDir", dir.c_str());
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

    void ReferenceLayer() {
        emit_event("file_open_request", toInt(Process::OpenRequest));
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
                this->emit_event("idle", toInt(Process::Idle));
                     }});

        add_process({toInt(Process::Idle), "idle", []() {
            printf("Entering idle\n");
        }});
    }
};


} // namespace lab
