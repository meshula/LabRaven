#include "Lab/App.h"
#include "Lab/CSP.hpp"
#include "Lab/LabDirectories.h"
#include "Lab/LabFileDialogManager.hpp"
#include "Providers/Animation/AnimationProvider.hpp"

namespace lab {

class LoadSkeletonModule : public CSP_Module {
public:
    LoadSkeletonModule(CSP_Engine& engine)
    : CSP_Module(engine, "LoadSkeletonModule")
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

    void LoadSkeleton() {
        if (pendingFile) {
            std::cerr << "LoadSkeleton: pendingFile is not zero\n";
            return;
        }
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
                const char* dir = lab_pref_for_key("LoadSkeletonDir");
                pendingFile = fdm->RequestOpenFile({"ozz","bvh","gltf","glb","vrm"},
                                                   dir? dir: ".");
                this->emit_event("opening", toInt(Process::Opening));
            }});

        add_process({toInt(Process::Opening), "opening",
            [this]() {
                if (!pendingFile) {
                    this->emit_event("file_error", toInt(Process::Error));
                    return;
                }
                auto fdm = LabApp::instance()->fdm();
                req = fdm->PopOpenedFile(pendingFile);
                switch (req.status) {
                    case FileDialogManager::FileReq::notReady:
                        this->emit_event("opening", toInt(Process::Opening)); // continue polling
                        break;
                    case FileDialogManager::FileReq::expired:
                    case FileDialogManager::FileReq::canceled:
                        this->emit_event("file_open_error", toInt(Process::Error));
                        break;
                    default:
                        this->emit_event("file_open_success", toInt(Process::OpenFile));
                        break;
                }
            }});

        add_process({toInt(Process::Error), "file_open_error",
            [this]() {
                printf("Entering file_error\n");
                std::cout << "Error: Failed to open file. Returning to Ready...\n";
                this->emit_event("idle", toInt(Process::Idle));
            }});

        add_process({toInt(Process::OpenFile), "file_open_success",
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
                this->emit_event("idle", toInt(Process::Idle));
            }});

        add_process({toInt(Process::Idle), "idle", [this]() {
            printf("Entering idle\n");
            pendingFile = 0;
        }});
    }
};

class LoadAnimationModule : public CSP_Module {
public:
    LoadAnimationModule(CSP_Engine& engine)
    : CSP_Module(engine, "LoadAnimationModule")
    {
    }

    enum class Process : int {
        OpenRequest = 300,
        Opening,
        Error,
        OpenFile,
        Idle
    };

    static constexpr int toInt(Process p) { return static_cast<int>(p); }

    void LoadAnimation(const std::string& skeletonName) {
        if (pendingFile) {
            std::cerr << "LoadAnimation: pendingFile is not zero\n";
            return;
        }
        this->skeletonName = skeletonName;
        emit_event("file_open_request", toInt(Process::OpenRequest));
    }

private:
    int pendingFile = 0;
    FileDialogManager::FileReq req;
    std::string skeletonName;

    virtual void initialize_processes() override {
        add_process({toInt(Process::OpenRequest), "file_open_request",
            [this]() {
                printf("Entering file_open_request\n");
                auto fdm = LabApp::instance()->fdm();
                const char* dir = lab_pref_for_key("LoadAnimationDir");
                pendingFile = fdm->RequestOpenFile({"ozz","bvh","gltf","glb","vrm"},
                                                   dir? dir: ".");
                this->emit_event("opening", toInt(Process::Opening));
            }});

        add_process({toInt(Process::Opening), "opening",
            [this]() {
                if (!pendingFile) {
                    this->emit_event("file_error", toInt(Process::Error));
                    return;
                }
                auto fdm = LabApp::instance()->fdm();
                req = fdm->PopOpenedFile(pendingFile);
                switch (req.status) {
                    case FileDialogManager::FileReq::notReady:
                        this->emit_event("opening", toInt(Process::Opening)); // continue polling
                        break;
                    case FileDialogManager::FileReq::expired:
                    case FileDialogManager::FileReq::canceled:
                        this->emit_event("file_open_error", toInt(Process::Error));
                        break;
                    default:
                        this->emit_event("file_open_success", toInt(Process::OpenFile));
                        break;
                }
            }});

        add_process({toInt(Process::Error), "file_open_error",
            [this]() {
                printf("Entering file_error\n");
                std::cout << "Error: Failed to open file. Returning to Ready...\n";
                this->emit_event("idle", toInt(Process::Idle));
            }});

        add_process({toInt(Process::OpenFile), "file_open_success",
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
                this->emit_event("idle", toInt(Process::Idle));
            }});

        add_process({toInt(Process::Idle), "idle", [this]() {
            printf("Entering idle\n");
            pendingFile = 0;
        }});
    }
};

class LoadModelModule : public CSP_Module {
public:
    LoadModelModule(CSP_Engine& engine)
    : CSP_Module(engine, "LoadModelModule")
    {
    }

    enum class Process : int {
        OpenRequest = 400,
        Opening,
        Error,
        OpenFile,
        Idle
    };

    static constexpr int toInt(Process p) { return static_cast<int>(p); }

    void LoadModel(const std::string& skeletonName) {
        if (pendingFile) {
            std::cerr << "LoadModel: pendingFile is not zero\n";
            return;
        }
        this->skeletonName = skeletonName;
        emit_event("file_open_request", toInt(Process::OpenRequest));
    }

private:
    int pendingFile = 0;
    FileDialogManager::FileReq req;
    std::string skeletonName;

    virtual void initialize_processes() override {
        add_process({toInt(Process::OpenRequest), "file_open_request",
            [this]() {
                printf("Entering file_open_request\n");
                auto fdm = LabApp::instance()->fdm();
                const char* dir = lab_pref_for_key("LoadModelDir");
                pendingFile = fdm->RequestOpenFile({"gltf","glb","vrm"},
                                                   dir? dir: ".");
                this->emit_event("opening", toInt(Process::Opening));
            }});

        add_process({toInt(Process::Opening), "opening",
            [this]() {
                if (!pendingFile) {
                    this->emit_event("file_error", toInt(Process::Error));
                    return;
                }
                auto fdm = LabApp::instance()->fdm();
                req = fdm->PopOpenedFile(pendingFile);
                switch (req.status) {
                    case FileDialogManager::FileReq::notReady:
                        this->emit_event("opening", toInt(Process::Opening)); // continue polling
                        break;
                    case FileDialogManager::FileReq::expired:
                    case FileDialogManager::FileReq::canceled:
                        this->emit_event("file_open_error", toInt(Process::Error));
                        break;
                    default:
                        this->emit_event("file_open_success", toInt(Process::OpenFile));
                        break;
                }
            }});

        add_process({toInt(Process::Error), "file_open_error",
            [this]() {
                printf("Entering file_error\n");
                std::cout << "Error: Failed to open file. Returning to Ready...\n";
                this->emit_event("idle", toInt(Process::Idle));
            }});

        add_process({toInt(Process::OpenFile), "file_open_success",
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
                this->emit_event("idle", toInt(Process::Idle));
            }});

        add_process({toInt(Process::Idle), "idle", [this]() {
            printf("Entering idle\n");
            pendingFile = 0;
        }});
    }
};

} // namespace lab
