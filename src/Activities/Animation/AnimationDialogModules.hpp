#include "Lab/App.h"
#include "Lab/CSP.hpp"
#include "Lab/LabDirectories.h"
#include "Lab/LabFileDialogManager.hpp"
#include "Providers/Animation/AnimationProvider.hpp"

namespace lab {

class LoadSkeletonModule : public CSP_Module {
    CSP_Process OpenRequest, Opening, Error, OpenFile, Idle;
    int pendingFile = 0;
    FileDialogManager::FileReq req;

public:
    LoadSkeletonModule(CSP_Engine& engine)
    : CSP_Module(engine, "LoadSkeletonModule")
    , OpenRequest("OpenRequest",
        [this]() {
            printf("Entering file_open_request\n");
            auto fdm = LabApp::instance()->fdm();
            const char* dir = lab_pref_for_key("LoadSkeletonDir");
            pendingFile = fdm->RequestOpenFile({"ozz","bvh","gltf","glb","vrm"},
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
                   auto animation = AnimationProvider::instance();
                   if (animation) {
                       // Extract name from filename
                       std::string path(req.path);
                       std::string filename = path.substr(path.find_last_of("/\\") + 1);
                       std::string name = filename.substr(0, filename.find_last_of('.'));

                       // Get file extension
                       std::string ext = filename.substr(filename.find_last_of('.') + 1);
                       bool success = false;

                       if (ext == "ozz") {
                           success = animation->LoadSkeleton(name.c_str(), req.path.c_str());
                       } else if (ext == "bvh") {
                           success = animation->LoadSkeletonFromBVH(name.c_str(), req.path.c_str());
                       } else if (ext == "gltf" || ext == "glb") {
                           success = animation->LoadSkeletonFromGLTF(name.c_str(), req.path.c_str());
                       } else if (ext == "vrm") {
                           success = animation->LoadSkeletonFromVRM(name.c_str(), req.path.c_str());
                       }

                       if (success) {
                           // Save directory for next time
                           std::string dir = path.substr(0, path.find_last_of("/\\"));
                           lab_set_pref_for_key("LoadSkeletonDir", dir.c_str());
                       }
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

    void LoadSkeleton() {
        if (pendingFile) {
            std::cerr << "LoadSkeleton: pendingFile is not zero\n";
            return;
        }
        emit_event(OpenRequest);
    }
};

class LoadAnimationModule : public CSP_Module {
    CSP_Process OpenRequest, Opening, Error, OpenFile, Idle;
    int pendingFile = 0;
    FileDialogManager::FileReq req;
    std::string skeletonName;

public:
    LoadAnimationModule(CSP_Engine& engine)
    : CSP_Module(engine, "LoadAnimationModule")
    , OpenRequest("OpenRequest",
                  [this]() {
                      printf("Entering file_open_request\n");
                      auto fdm = LabApp::instance()->fdm();
                      const char* dir = lab_pref_for_key("LoadAnimationDir");
                      pendingFile = fdm->RequestOpenFile({"ozz","bvh","gltf","glb","vrm"},
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
                   auto animation = AnimationProvider::instance();
                   if (animation) {
                       // Extract name from filename
                       std::string path(req.path);
                       std::string filename = path.substr(path.find_last_of("/\\") + 1);
                       std::string name = filename.substr(0, filename.find_last_of('.'));

                       // Get file extension
                       std::string ext = filename.substr(filename.find_last_of('.') + 1);
                       bool success = false;

                       if (ext == "ozz") {
                           success = animation->LoadAnimation(name.c_str(), req.path.c_str());
                       } else if (ext == "bvh") {
                           success = animation->LoadAnimationFromBVH(name.c_str(), req.path.c_str(), skeletonName.c_str());
                       } else if (ext == "gltf" || ext == "glb") {
                           success = animation->LoadAnimationFromGLTF(name.c_str(), req.path.c_str(), skeletonName.c_str());
                       } else if (ext == "vrm") {
                           success = animation->LoadAnimationFromVRM(name.c_str(), req.path.c_str(), skeletonName.c_str());
                       }

                       if (success) {
                           // Save directory for next time
                           std::string dir = path.substr(0, path.find_last_of("/\\"));
                           lab_set_pref_for_key("LoadAnimationDir", dir.c_str());
                       }
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

    void LoadAnimation(const std::string& skeletonName) {
        if (pendingFile) {
            std::cerr << "LoadAnimation: pendingFile is not zero\n";
            return;
        }
        this->skeletonName = skeletonName;
        emit_event(OpenRequest);
    }
};

class LoadModelModule : public CSP_Module {
    CSP_Process OpenRequest, Opening, Error, OpenFile, Idle;
    int pendingFile = 0;
    FileDialogManager::FileReq req;
    std::string skeletonName;

public:
    LoadModelModule(CSP_Engine& engine)
    : CSP_Module(engine, "LoadModelModule")
    , OpenRequest("OpenRequest",
                  [this]() {
                      printf("Entering file_open_request\n");
                      auto fdm = LabApp::instance()->fdm();
                      const char* dir = lab_pref_for_key("LoadModelDir");
                      pendingFile = fdm->RequestOpenFile({"gltf","glb","vrm"},
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
                   auto animation = AnimationProvider::instance();
                   if (animation) {
                       // Extract name from filename
                       std::string path(req.path);
                       std::string filename = path.substr(path.find_last_of("/\\") + 1);
                       std::string name = filename.substr(0, filename.find_last_of('.'));

                       bool success = animation->LoadModelFromGLTF(name.c_str(), req.path.c_str(), skeletonName.c_str());

                       if (success) {
                           // Save directory for next time
                           std::string dir = path.substr(0, path.find_last_of("/\\"));
                           lab_set_pref_for_key("LoadModelDir", dir.c_str());
                       }
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

    void LoadModel(const std::string& skeletonName) {
        if (pendingFile) {
            std::cerr << "LoadModel: pendingFile is not zero\n";
            return;
        }
        this->skeletonName = skeletonName;
        emit_event(OpenRequest);
    }
};

} // namespace lab
