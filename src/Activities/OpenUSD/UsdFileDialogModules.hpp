#include "Activities/Console/ConsoleActivity.hpp"
#include "Lab/App.h"
#include "Lab/CSP.hpp"
#include "Lab/LabDirectories.h"
#include "Lab/LabFileDialogManager.hpp"
#include "Providers/OpenUSD/OpenUSDProvider.hpp"

namespace lab {

PXR_NAMESPACE_USING_DIRECTIVE

class LoadLayerModule : public CSP_Module {
    CSP_Process OpenRequest, Opening, Error, OpenFile, Idle;
    enum class LoadType {
        InsertSubLayer,
        LoadStage,
        InstanceLayer,
        ReferenceLayer
    };
    LoadType loadType = LoadType::LoadStage;
    int pendingFile = 0;
    FileDialogManager::FileReq req;

public:
    LoadLayerModule(CSP_Engine& engine)
    : CSP_Module(engine, "LoadLayerModule")
    , OpenRequest("OpenRequest",
                  [this]() {
                      printf("Entering file_open_request\n");
                      auto fdm = LabApp::instance()->fdm();
                      const char* dir = lab_pref_for_key("LoadStageDir");
                      pendingFile = fdm->RequestOpenFile({"usd","usda","usdc","usdz"},
                                                         dir? dir: ".");
                      this->emit_event(Opening);
                  })
    , Opening("Opening",
              [this]() {
                  if (!pendingFile) {
                      this->emit_event(Error);
                      return;
                  }
                  auto fdm = LabApp::instance()->fdm();
                  req = fdm->PopOpenedFile(pendingFile);
                  switch (req.status) {
                      case FileDialogManager::FileReq::notReady:
                          this->emit_event(Opening); // continue polling
                          break;
                      case FileDialogManager::FileReq::expired:
                      case FileDialogManager::FileReq::canceled:
                          this->emit_event(Error);
                          break;
                      default:
                          this->emit_event(OpenFile);
                          break;
                  }
              })
    , Error("Error",
            [this]() {
                printf("Entering file_error\n");
                std::cout << "Error: Failed to open file. Returning to Ready...\n";
                this->emit_event(Idle);
            })
    , OpenFile("OpenFile",
               [this]() {
                   printf("Entering file_open_success\n");
                   auto usd = OpenUSDProvider::instance();
                   if (usd) {
                       switch (loadType) {
                           case LoadType::InsertSubLayer: {
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
                               break;
                           }
                           case LoadType::LoadStage: {
                               usd->LoadStage(req.path);
                               // get the directory of path and save it as the default
                               // directory for the next time the user loads a stage
                               std::string dir = req.path.substr(0, req.path.find_last_of("/\\"));
                               lab_set_pref_for_key("LoadStageDir", dir.c_str());
                               break;
                           }

                           case LoadType::InstanceLayer: {
                               auto stage = usd->Stage();
                               if (stage) {
                                   auto mm = LabApp::instance()->mm();
                                   //auto cameraMode = mm->LockActivity(_self->cameraMode);
                                   pxr::GfVec3d pos(0,0,0);// = cameraMode->HitPoint();
                                   mm->EnqueueTransaction(Transaction{"Reference a layer", [this, usd, pos, stage]() {
                                       std::string path(req.path);
                                       usd->CreateDefaultPrimIfNeeded("/Lab");
                                       auto defaultPrim = stage->GetDefaultPrim();
                                       if (!defaultPrim) {
                                           std::cerr << "No default prim on stage\n";
                                       }
                                       else {
                                           pxr::SdfPath primPath = defaultPrim.GetPath();
                                           primPath = primPath.AppendChild(TfToken("Models"));
                                           const bool instanceLayer = true;
                                           usd->ReferenceLayer(stage, path, primPath, true, pos, instanceLayer);
                                           // get the directory of path and save it as the default
                                           // directory for the next time the user loads a stage
                                           std::string dir = path.substr(0, path.find_last_of("/\\"));
                                           lab_set_pref_for_key("LoadStageDir", dir.c_str());
                                       }
                                   }});
                               }
                               break;
                           }

                           case LoadType::ReferenceLayer: {
                               auto stage = usd->Stage();
                               if (stage) {
                                   auto mm = LabApp::instance()->mm();
                                   //auto cameraMode = mm->LockActivity(_self->cameraMode);
                                   pxr::GfVec3d pos(0,0,0);// = cameraMode->HitPoint();
                                   mm->EnqueueTransaction(Transaction{"Reference a layer", [this, usd, pos, stage]() {
                                       std::string path(req.path);
                                       usd->CreateDefaultPrimIfNeeded("/Lab");
                                       auto defaultPrim = stage->GetDefaultPrim();
                                       if (!defaultPrim) {
                                           std::cerr << "No default prim on stage\n";
                                       }
                                       else {
                                           pxr::SdfPath primPath = defaultPrim.GetPath();
                                           primPath = primPath.AppendChild(TfToken("Models"));
                                           const bool instanceLayer = false;
                                           usd->ReferenceLayer(stage, path, primPath, true, pos, instanceLayer);
                                           // get the directory of path and save it as the default
                                           // directory for the next time the user loads a stage
                                           std::string dir = path.substr(0, path.find_last_of("/\\"));
                                           lab_set_pref_for_key("LoadStageDir", dir.c_str());
                                       }
                                   }});
                               }
                               break;
                           }
                       } // switch
                   }
               this->emit_event(Idle);
           })
    , Idle("Idle",
           [this]() {
               printf("Entering idle\n");
               pendingFile = 0;
           })
    {
        add_process(OpenRequest);
        add_process(Opening);
        add_process(Error);
        add_process(OpenFile);
        add_process(Idle);
    }

    void LoadStage() {
        if (pendingFile) {
            std::cerr << "LoadStage: pendingFile is not zero\n";
            return;
        }
        loadType = LoadType::LoadStage;
        emit_event(OpenRequest);
    }

    void InsertSubLayer() {
        if (pendingFile) {
            std::cerr << "InsertSubLayer: pendingFile is not zero\n";
            return;
        }
        loadType = LoadType::InsertSubLayer;
        emit_event(OpenRequest);
    }

    void InstanceLayer() {
        if (pendingFile) {
            std::cerr << "InstanceLayer: pendingFile is not zero\n";
            return;
        }
        loadType = LoadType::InstanceLayer;
        emit_event(OpenRequest);
    }

    void ReferenceLayer() {
        if (pendingFile) {
            std::cerr << "ReferenceLayer: pendingFile is not zero\n";
            return;
        }
        loadType = LoadType::ReferenceLayer;
        emit_event(OpenRequest);
    }
};

class ExportStageModule : public CSP_Module {
    CSP_Process ExportRequest, Saving, Error, ExportFile, Idle;
    int pendingFile = 0;
    FileDialogManager::FileReq req;
public:
    ExportStageModule(CSP_Engine& engine)
    : CSP_Module(engine, "ExportStageModule")
    , ExportRequest("ExportRequest",
                    [this]() {
                                 printf("Entering file_export_request\n");
                                 auto fdm = LabApp::instance()->fdm();
                                 const char* dir = lab_pref_for_key("LoadStageDir");
                                 pendingFile = fdm->RequestSaveFile(
                                                                    {"usd","usda","usdc","usdz"},
                                                                      dir? dir: ".");
                                 this->emit_event(Saving);
                             })
    , Saving("Saving",
             [this]() {
                         auto fdm = LabApp::instance()->fdm();
                         req = fdm->PopOpenedFile(pendingFile);
                         switch (req.status) {
                             case FileDialogManager::FileReq::notReady:
                                 this->emit_event(Saving); // continue polling
                                 break;
                             case FileDialogManager::FileReq::expired:
                             case FileDialogManager::FileReq::canceled:
                                 this->emit_event(Error);
                                 break;
                             default:
                                 this->emit_event(ExportFile);
                                 break;
                         }
                      })
    , Error("Error",
            [this]() {
                printf("Entering file_error\n");
                std::cout << "Error: Failed to save file. Returning to idle...\n";
                this->emit_event(Idle);
            })
    , ExportFile("ExportFile",
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
                             this->emit_event(Idle);
                          })
    , Idle("Idle",
           []() {
               printf("Entering idle\n");
           })
    {
        add_process(ExportRequest);
        add_process(Saving);
        add_process(Error);
        add_process(ExportFile);
        add_process(Idle);
    }

    void ExportCurrentStage() { emit_event(ExportRequest); }
};

class ShotTemplateModule : public lab::CSP_Module {
    CSP_Process NameRequest, PickFolder, Error, CreateShot, Idle;

    int pendingDir = 0;
    FileDialogManager::FileReq req;
    std::string shotName;
    bool showNameDialog = false;
    static constexpr char popupName[] = "Create Shot from Template";

public:
    ShotTemplateModule(lab::CSP_Engine& engine)
    : CSP_Module(engine, "ShotTemplateModule")
    , NameRequest("NameRequest",
                  [this]() {
                      printf("Entering name_request\n");
                      showNameDialog = true;
                      // Stay in this state until the dialog is completed
                      this->emit_event(NameRequest);
                  })
    , PickFolder("PickFolder",
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
                             this->emit_event(PickFolder); // continue polling
                             break;
                         case FileDialogManager::FileReq::expired:
                         case FileDialogManager::FileReq::canceled:
                             this->emit_event(Error);
                             break;
                         default:
                             this->emit_event(CreateShot);
                             break;
                     }
                 })
    , Error("Error",
            [this]() {
                printf("Entering folder_error\n");
                std::cout << "Error: Failed to pick folder. Returning to idle...\n";
                pendingDir = 0;
                this->emit_event(Idle);
            })
    , CreateShot("CreateShot",
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
                     this->emit_event(Idle);
                 })
    , Idle("Idle",
           []() {
               printf("Entering idle\n");
           })
    {
    }

    void CreateShotFromTemplate() {
        emit_event(NameRequest);
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
                    this->emit_event(PickFolder);
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel###Shot", ImVec2(120, 0))) {
                    ImGui::CloseCurrentPopup();
                    showNameDialog = false;
                    this->emit_event(Idle);
                }
                ImGui::EndPopup();
            }
        }
    }
};

} // namespace lab
