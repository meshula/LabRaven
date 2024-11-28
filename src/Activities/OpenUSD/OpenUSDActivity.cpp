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

namespace lab {

struct OpenUSDActivity::data {
    bool run_shot_template_ui = false;
};

OpenUSDActivity::OpenUSDActivity() : Activity() {
    _self = new OpenUSDActivity::data;
    
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
    if (_self->run_shot_template_ui) {
        auto mm = ApplicationEngine::app()->mm();

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
                _self->run_shot_template_ui = false;

                nfdchar_t* outPath = nullptr;
                const char* dir = lab_pref_for_key("LoadStageDir");
                nfdresult_t result = NFD_PickFolder(dir? dir : ".", &outPath);
                if (result == NFD_OKAY) {
                    std::string path(outPath);
                    auto console = mm->LockActivity(_self->console);
                    std::string sn(shotName);
                    enqueue(Transaction{"Create Shot from Template", [this, mm, path, sn](){
                        auto console = mm->LockActivity(_self->console);
                        console->Info("Creating Shot from Template");
                        
                        CreateShotFromTemplate(path, sn);
                    }});
                }
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
    auto mm = ApplicationEngine::app()->mm();
    if (ImGui::BeginMenu("Stage")) {
        if (ImGui::MenuItem("Load Stage ...")) {
            enqueue(Transaction{"Load Stage", [this](){
                nfdchar_t* outPath = nullptr;
                const char* dir = lab_pref_for_key("LoadStageDir");
                nfdresult_t result = NFD_OpenDialog("usd,usda,usdc,usdz", dir? dir : ".", &outPath);
                if (result == NFD_OKAY) {
                    std::string path(outPath);
                    LoadStage(path);
                    // get the directory of path and save it as the default
                    // directory for the next time the user loads a stage
                    std::string dir = path.substr(0, path.find_last_of("/\\"));
                    lab_set_pref_for_key("LoadStageDir", dir.c_str());
                }
            }});
        }
        if (ImGui::MenuItem("New Stage")) {
            enqueue(Transaction{"New Stage", [this](){
                SetEmptyStage();
            }});
        }
        if (ImGui::MenuItem("Create Shot from Template...")) {
            _self->run_shot_template_ui = true;
        }
        if (ImGui::MenuItem("Export Stage ...")) {
            enqueue(Transaction{"Export Stage", [this, mm]() {
                nfdchar_t* outPath = nullptr;
                const char* dir = lab_pref_for_key("LoadStageDir");
                nfdresult_t result = NFD_SaveDialog("usda,usdc,usdz", dir? dir : ".", &outPath);
                if (result == NFD_OKAY) {
                    std::string path(outPath);
                    auto console = mm->LockActivity(_self->console);
                    std::string msg = "Exporting stage: " + path;
                    console->Info(msg);
                    _self->scene.export_stage(path);

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
                    _self->sessionLayer->Export(sessionPath);
                }
            }});
        }
        if (ImGui::MenuItem("Test referencing")) {
            enqueue(Transaction{"Test referencing", [this]() {
                TestReferencing();
            }});
        }
        auto stage = _self->scene.stage();
        if (!stage) {
            ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f); // Adjust alpha to make it look disabled
        }
        static bool at_hit_point = true;
        if (ImGui::MenuItem("Reference a Layer ...")) {
            if (stage) {
                auto cameraMode = mm->LockActivity(_self->cameraMode);
                GfVec3d pos = cameraMode->HitPoint();
                enqueue(Transaction{"Reference a layer", [this, pos, stage](){
                    nfdchar_t *outPath = nullptr;
                    const char* dir = lab_pref_for_key("LoadStageDir");
                    nfdresult_t result = NFD_OpenDialog("usd,usda,usdc,usdz",
                                                        dir? dir: ".", &outPath);
                    if (result == NFD_OKAY) {
                        std::string path(outPath);
                        pxr::SdfPath primPath = stage->GetDefaultPrim().GetPath();
                        primPath = primPath.AppendChild(TfToken("Models"));
                        ReferenceLayer(stage, path, primPath, true, pos, false);
                        // get the directory of path and save it as the default
                        // directory for the next time the user loads a stage
                        std::string dir = path.substr(0, path.find_last_of("/\\"));
                        lab_set_pref_for_key("LoadStageDir", dir.c_str());
                    }
                }});
            }
        }
        if (ImGui::MenuItem("Instance a Layer ...")) {
            if (stage) {
                auto cameraMode = mm->LockActivity(_self->cameraMode);
                GfVec3d pos = cameraMode->HitPoint();
                enqueue(Transaction{"Instance a layer", [this, pos, stage](){
                    nfdchar_t *outPath = nullptr;
                    const char* dir = lab_pref_for_key("LoadStageDir");
                    nfdresult_t result = NFD_OpenDialog("usd,usda,usdc,usdz",
                                                        dir? dir: ".", &outPath);
                    if (result == NFD_OKAY) {
                        std::string path(outPath);
                        pxr::SdfPath primPath = stage->GetDefaultPrim().GetPath();
                        primPath = primPath.AppendChild(TfToken("Models"));
                        ReferenceLayer(stage, path, primPath, true, pos, true);
                        // get the directory of path and save it as the default
                        // directory for the next time the user loads a stage
                        std::string dir = path.substr(0, path.find_last_of("/\\"));
                        lab_set_pref_for_key("LoadStageDir", dir.c_str());
                    }
                }});
            }
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
