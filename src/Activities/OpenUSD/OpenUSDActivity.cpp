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
#include "FileDialogModules.hpp"
#include "Lab/App.h"
#include "Lab/CSP.hpp"
#include "Lab/LabFileDialogManager.hpp"
#include "Lab/LabDirectories.h"
#include "Activities/Console/ConsoleActivity.hpp"
#include "UsdOutlinerActivity.hpp"
#include "HydraActivity.hpp"
#include "PropertiesActivity.hpp"
#include "SessionActivity.hpp"
#include "TfDebugActivity.hpp"
#include "Providers/OpenUSD/OpenUSDProvider.hpp"
#include <pxr/usd/usd/prim.h>

namespace lab {

PXR_NAMESPACE_USING_DIRECTIVE


struct OpenUSDActivity::data {
    CSP_Engine engine;
    data()
        : loadStageModule(engine)
        , exportStageModule(engine)
        , referenceLayerModule(engine)
        , shotTemplateModule(engine) {
            engine.run();
        }
    ~data() = default;

    std::weak_ptr<ConsoleActivity> console;
    LoadStageModule loadStageModule;
    ExportStageModule exportStageModule;
    ReferenceLayerModule referenceLayerModule;
    ShotTemplateModule shotTemplateModule;
};

OpenUSDActivity::OpenUSDActivity() : Activity(OpenUSDActivity::sname()) {
    _self = new OpenUSDActivity::data();

    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<OpenUSDActivity*>(instance)->RunUI(*vi);
    };
    activity.Menu = [](void* instance) {
        static_cast<OpenUSDActivity*>(instance)->Menu();
    };

    _self->loadStageModule.Register();
    _self->exportStageModule.Register();
    _self->referenceLayerModule.Register();
    _self->shotTemplateModule.Register();

    auto orchestrator = Orchestrator::Canonical();
    orchestrator->RegisterActivity<UsdOutlinerActivity>(
        [](){ return std::make_shared<UsdOutlinerActivity>(); });
    orchestrator->RegisterActivity<DebuggerActivity>(
        [](){ return std::make_shared<DebuggerActivity>(); });
    orchestrator->RegisterActivity<HydraActivity>(
        [](){ return std::make_shared<HydraActivity>(); });
    orchestrator->RegisterActivity<PropertiesActivity>(
        [](){ return std::make_shared<PropertiesActivity>(); });
    orchestrator->RegisterActivity<SessionActivity>(
        [](){ return std::make_shared<SessionActivity>(); });
}

OpenUSDActivity::~OpenUSDActivity() {
    delete _self;
}

void OpenUSDActivity::RunUI(const LabViewInteraction&) {
    _self->shotTemplateModule.update();
}

void OpenUSDActivity::Menu() {
    Orchestrator* mm = Orchestrator::Canonical();
    if (ImGui::BeginMenu("Stage")) {
        if (ImGui::MenuItem("Load Stage ...")) {
            _self->engine.test();
            _self->loadStageModule.LoadStage();
        }
        if (ImGui::MenuItem("New Stage")) {
            mm->EnqueueTransaction(Transaction{"New Stage", [](){
                std::shared_ptr<OpenUSDProvider> usd = OpenUSDProvider::instance();
                if (usd)
                    usd->SetEmptyStage();
            }});
        }
        if (ImGui::MenuItem("Create Shot from Template...")) {
            _self->shotTemplateModule.CreateShotFromTemplate();
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
            // Adjust alpha to make it look disabled
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }
        static bool at_hit_point = true;
        if (ImGui::MenuItem("Reference a Layer ...")) {
            _self->referenceLayerModule.instance = false;
            _self->referenceLayerModule.emit_event("layer_reference_request", 0);
        }
        if (ImGui::MenuItem("Instance a Layer ...")) {
            _self->referenceLayerModule.instance = true;
            _self->referenceLayerModule.emit_event("layer_reference_request", 0);
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
