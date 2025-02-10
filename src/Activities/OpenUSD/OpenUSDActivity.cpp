//
//  OpenUSDActivity.cpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 1/7/24.
//  Copyright © 2024 Nick Porcino. All rights reserved.
//
#include "OpenUSDActivity.hpp"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui.h"
#include "UsdFileDialogModules.hpp"
#include "Lab/App.h"
#include "Lab/CSP.hpp"
#include "Lab/LabFileDialogManager.hpp"
#include "Lab/LabDirectories.h"
#include "Activities/Console/ConsoleActivity.hpp"
#include "HydraActivity.hpp"
#include "UsdCreateActivity.hpp"
#include "UsdOutlinerActivity.hpp"
#include "PropertiesActivity.hpp"
#include "SessionActivity.hpp"
#include "TfDebugActivity.hpp"
#include "Providers/OpenUSD/OpenUSDProvider.hpp"
#include "Providers/OpenUSD/ProfilePrototype.hpp"
#include <pxr/usd/usd/prim.h>

namespace lab {

PXR_NAMESPACE_USING_DIRECTIVE


struct OpenUSDActivity::data {
    data()
        : loadLayerModule(*lab::LabApp::instance()->csp())
        , exportStageModule(*lab::LabApp::instance()->csp())
        , shotTemplateModule(*lab::LabApp::instance()->csp()) {
        }
    ~data() = default;

    std::weak_ptr<ConsoleActivity> console;
    LoadLayerModule loadLayerModule;
    ExportStageModule exportStageModule;
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

    _self->loadLayerModule.Register();
    _self->exportStageModule.Register();
    _self->shotTemplateModule.Register();
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
        if (ImGui::MenuItem("New Stage")) {
            mm->EnqueueTransaction(Transaction{"New Stage", [](){
                auto usd = OpenUSDProvider::instance();
                if (usd)
                    usd->SetEmptyStage();
            }});
        }
        if (ImGui::MenuItem("Load Stage ...")) {
            //_self->engine.test();
            _self->loadLayerModule.LoadStage();
        }
        if (ImGui::MenuItem("Create Shot from Template...")) {
            _self->shotTemplateModule.CreateShotFromTemplate();
        }
        if (ImGui::MenuItem("Export Stage ...")) {
            _self->exportStageModule.emit_event("file_export_request", 0);
        }
        if (ImGui::MenuItem("Add a Sublayer...")) {
            _self->loadLayerModule.InsertSubLayer();
        }
        auto usd = OpenUSDProvider::instance();
        auto stage = usd->Stage();
        if (!stage) {
            // Adjust alpha to make it look disabled
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
        }
        static bool at_hit_point = true;
        if (ImGui::MenuItem("Reference a Layer ...")) {
            _self->loadLayerModule.ReferenceLayer();
        }
        if (ImGui::MenuItem("Instance a Layer ...")) {
            _self->loadLayerModule.InstanceLayer();
        }

        ImGui::Indent(60);
        ImGui::Checkbox("At Hit Point", &at_hit_point);
        ImGui::Unindent();

        if (!stage) {
            ImGui::PopStyleVar();
        }
        ImGui::EndMenu();
    }
    if (ImGui::BeginMenu("Tests")) {
        if (ImGui::MenuItem("Usd: Test Profiles")) {
            testProfiles();
        }

        if (ImGui::MenuItem("Usd: Test Referencing")) {
            mm->EnqueueTransaction(Transaction{"Test referencing", [this]() {
                auto usd = OpenUSDProvider::instance();
                if (usd)
                    usd->TestReferencing();
            }});
        }
        ImGui::EndMenu();
    }

}

} // lab
