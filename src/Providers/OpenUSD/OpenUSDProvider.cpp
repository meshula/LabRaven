
#include "OpenUSDProvider.hpp"
#include "UsdSessionLayer.hpp"

#include "ImGuiHydraEditor/src/models/model.h"
#include "ImGuiHydraEditor/src/engine.h"
#include "ImGuiHydraEditor/src/sceneindices/xformfiltersceneindex.h"

#include <pxr/usd/usdUtils/usdzPackage.h>

#include <iostream>

namespace lab {

PXR_NAMESPACE_USING_DIRECTIVE

struct OpenUSDProvider::Self {
    std::unique_ptr<pxr::Model> model;
    std::unique_ptr<UsdSessionLayer> layer;
    std::unique_ptr<Engine> engine;
    pxr::XformFilterSceneIndexRefPtr xformSceneIndex;

    ~Self() {
        engine.reset();
        layer.reset();
        model.reset();
    }
};

OpenUSDProvider::OpenUSDProvider()
: self(new Self)
{
}

OpenUSDProvider::~OpenUSDProvider()
{
}

void OpenUSDProvider::LoadStage(std::string const& filePath)
{
    if (UsdStage::IsSupportedFile(filePath))
    {
        printf("File format supported: %s\n", filePath.c_str());
    }
    else {
        fprintf(stderr, "%s : File format not supported\n", filePath.c_str());
        return;
    }

    UsdStageRefPtr loadedStage = UsdStage::Open(filePath);
    if (loadedStage) {
        self->model.reset(new Model());
        self->model->SetStage(loadedStage);
        self->layer.reset(new UsdSessionLayer(self->model.get()));
        auto editableSceneIndex = self->model->GetEditableSceneIndex();
        self->xformSceneIndex = XformFilterSceneIndex::New(editableSceneIndex);
        self->model->SetEditableSceneIndex(self->xformSceneIndex);
        TfToken plugin = Engine::GetDefaultRendererPlugin();
        self->engine.reset(new Engine(self->model->GetFinalSceneIndex(), plugin));
    }
    else {
        self->engine.reset();
        self->layer.reset();
        self->model.reset();
        fprintf(stderr, "Stage was not loaded at %s\n", filePath.c_str());
    }
}

void OpenUSDProvider::SaveStage() {
    if (self->model) {
        auto stage = self->model->GetStage();
        if (stage)
            stage->GetRootLayer()->Save();
    }
}

void OpenUSDProvider::ExportStage(std::string const& path)
{
    if (!self->model) {
        fprintf(stderr, "No stage to export\n");
        return;
    }
    auto stage = self->model->GetStage();
    if (!stage) {
        fprintf(stderr, "No stage to export\n");
        return;
    }

    if (path.size() >= 5 && path.substr(path.size() - 5) == ".usdz") {
        // Store current working directory
        char* cwd_buffer = getcwd(nullptr, 0);
        if (!cwd_buffer) {
            std::cerr << "Failed to get current working directory" << std::endl;
            return;
        }

        #ifdef _WIN32
        const std::string tempdir = "C:\\temp";
        #else
        const std::string tempdir = "/var/tmp";
        #endif

        // Change directory to a temporary
        if (chdir(tempdir.c_str()) != 0) {
            std::cerr << "Failed to change directory to /var/tmp" << std::endl;
            free(cwd_buffer);
            return;
        }

        std::string temp_usdc_path = "contents.usdc";
        stage->GetRootLayer()->Export(temp_usdc_path);
        if (!pxr::UsdUtilsCreateNewUsdzPackage(pxr::SdfAssetPath(temp_usdc_path), "new.usdz")) {
            // report error
        }
        
        // Move the temporary USD file to the specified path
        if (std::rename("new.usdz", path.c_str()) != 0) {
            std::cerr << "Failed to move the temporary file to the specified path" << std::endl;
        }
        
        // Restore the original working directory
        if (chdir(cwd_buffer) != 0) {
            std::cerr << "Failed to restore the original working directory" << std::endl;
        }
        free(cwd_buffer);
    }
    else {
        stage->GetRootLayer()->Export(path);
    }
}

} // lab
