//
//  ScoutActivity.cpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 2/11/24.
//  Copyright © 2024 Nick Porcino. All rights reserved.
//

#include "ScoutActivity.hpp"
#include "Activities/Timeline/TimelineActivity.hpp"
#include "Activities/OpenUSD/HydraActivity.hpp"
#include "Providers/OpenUSD/OpenUSDProvider.hpp"
#include "Providers/Camera/CameraProvider.hpp"
#include "Providers/Selection/SelectionProvider.hpp"

#include "imgui.h"
#include "Lab/ImguiExt.hpp"
#include <pxr/base/gf/frustum.h>
#include <pxr/base/gf/matrix4d.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/timeCode.h>
#include <pxr/usd/usdGeom/camera.h>
#include <pxr/usd/usdGeom/gprim.h>
#include <pxr/usd/usdGeom/metrics.h>

namespace lab {
    using namespace PXR_NS;

    struct ScoutRecord {
        std::string name;
        UsdTimeCode time;
        UsdPrim camera;
        GfMatrix4d cameraToWorld;
        GfFrustum frustum;
        GfCamera gfCamera;
        CameraProvider::CameraData cameraData;
    };

    struct ScoutActivity::data {
        bool ui_visible = true;
        int selected_scout = -1;
        int selected_camera = -1;
        std::vector<ScoutRecord> scouts;
    };

    ScoutActivity::ScoutActivity() : Activity(ScoutActivity::sname()) {
        _self = new ScoutActivity::data();
        
        activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
            static_cast<ScoutActivity*>(instance)->RunUI(*vi);
        };
        activity.Menu = [](void* instance) {
            static_cast<ScoutActivity*>(instance)->Menu();
        };
    }

    ScoutActivity::~ScoutActivity() {
        delete _self;
    }

    void ScoutActivity::RunUI(const LabViewInteraction& vi) {
        if (!_self->ui_visible)
            return;

        auto mm = Orchestrator::Canonical();
        std::weak_ptr<TimelineActivity> tlact;
        auto timeline = mm->LockActivity(tlact);
        std::weak_ptr<HydraActivity> hact;
        auto hydra = mm->LockActivity(hact);

        auto usd = OpenUSDProvider::instance();
        auto model = usd->Model();
        auto stage = model->GetStage();
        double tcps = stage->GetTimeCodesPerSecond();
        const UsdTimeCode timeCode = timeline->Playhead().to_seconds() * tcps;

        ImGui::Begin("Scout", &_self->ui_visible);

        if (ImGui::Button("Snapshot")) {
            /*                            _           _
                ___ _ __   __ _ _ __  ___| |__   ___ | |_
               / __| '_ \ / _` | '_ \/ __| '_ \ / _ \| __|
               \__ \ | | | (_| | |_) \__ \ | | | (_) | |_
               |___/_| |_|\__,_| .__/|___/_| |_|\___/ \__|
                               |_| */

            ScoutRecord r;
            r.time = timeCode;

            auto cp = CameraProvider::instance();
            r.cameraData = cp->GetLookAt("interactive");
            auto& lookAt = r.cameraData.lookAt;
            GfVec3d at(lookAt.center.x, lookAt.center.y, lookAt.center.z);
            GfVec3d eye(lookAt.eye.x, lookAt.eye.y, lookAt.eye.z);
            GfVec3d up(lookAt.up.x, lookAt.up.y, lookAt.up.z);

            GfMatrix4d view = GfMatrix4d().SetLookAt(eye, at, up);

            GfVec2d nf = hydra->GetNearFar();
            float aspect = hydra->GetAspect();

            GfFrustum frustum;
            frustum.SetViewDistance((eye - at).Normalize());
            frustum.SetPositionAndRotationFromMatrix(view);
            frustum.SetProjectionType(GfFrustum::ProjectionType::Perspective);
            frustum.SetPerspective(180.0 * r.cameraData.HFOV.rad, aspect, nf[0], nf[1]);

            r.cameraToWorld = view;
            r.frustum = frustum;

            float unit_scale = UsdGeomGetStageMetersPerUnit(stage);

            r.cameraData.lookAt.center = { (float) at[0], (float) at[1], (float) at[2] };
            r.gfCamera = hydra->GetGfCamera();

            mm->EnqueueTransaction(
                Transaction{"Create Camera", [this, r, usd, stage]() {
                    ScoutRecord record = r;
                    std::string name = "Scout" + std::to_string(_self->scouts.size());
                    SdfPath cameraPath = usd->CreateCamera(name);
                    UsdPrim cameraPrim = stage->GetPrimAtPath(cameraPath);
                    record.name = cameraPath.GetName();
                    record.camera = cameraPrim;
                    auto cam = UsdGeomCamera(cameraPrim);
                    cam.SetFromCamera(r.gfCamera, r.time);
                    _self->scouts.push_back(record);
                }});
        }

        ImGui::BeginGroup();

        ImVec2 windowSize = ImGui::GetWindowSize();
        ImVec2 pos = ImGui::GetCursorPos();
        
        windowSize.x -= 100;
        if (windowSize.x < 100)
            windowSize.x = 100;
        if (windowSize.x > 250)
            windowSize.x = 250;
        windowSize.y = 0.f;
        if (ImGui::BeginListBox("###ScoutRecordsList", windowSize)) {
            for (size_t n = 0; n < _self->scouts.size(); n++) {
                const bool is_selected = (_self->selected_scout == n);
                if (ImGui::Selectable(_self->scouts[n].name.c_str(), is_selected))
                    _self->selected_scout = (int) n;

                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndListBox();
        }
        
        if (_self->selected_scout >= 0) {
            float itemHeight = ImGui::GetTextLineHeightWithSpacing();
            float ypos = itemHeight * _self->selected_scout;
            pos.x += windowSize.x;
            pos.y += ypos;
            ImGui::SetCursorPos(pos);
            if (ImGui::Button("Go to###scout")) {
                auto cp = CameraProvider::instance();
                auto cameraData = cp->GetLookAt("interactive");

                auto& r = _self->scouts[_self->selected_scout];

                cp->SetHFOV(r.cameraData.HFOV, "interactive");
                cp->SetLookAt(r.cameraData.lookAt, "interactive");

                GfRange1d nearfar = r.frustum.GetNearFar();
                hydra->SetNearFar(nearfar.GetMin(), nearfar.GetMax());
            }
        }
        ImGui::EndGroup();

        /*                                    _ _     _
          ___ __ _ _ __ ___   ___ _ __ __ _  | (_)___| |_
         / __/ _` | '_ ` _ \ / _ \ '__/ _` | | | / __| __|
        | (_| (_| | | | | | |  __/ | | (_| | | | \__ \ |_
         \___\__,_|_| |_| |_|\___|_|  \__,_| |_|_|___/\__|
         */

        ImGui::BeginGroup();
        pos = ImGui::GetCursorPos();
        const std::vector<pxr::UsdPrim>& cameras = usd->GetCameras();
        if (ImGui::BeginListBox("###ScoutCameraList", windowSize)) {
            for (size_t n = 0; n < cameras.size(); n++) {
                const bool is_selected = (_self->selected_camera == n);
                if (ImGui::Selectable(cameras[n].GetName().GetText(), is_selected))
                    _self->selected_camera = (int) n;

                // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
            ImGui::EndListBox();
        }

        if (_self->selected_camera >= 0) {
            float itemHeight = ImGui::GetTextLineHeightWithSpacing();
            float ypos = itemHeight * _self->selected_camera;
            pos.x += windowSize.x;
            pos.y += ypos;
            ImGui::SetCursorPos(pos);
            if (ImGui::Button("Go to###camlist")) {
                auto& prim = cameras[_self->selected_camera];
                auto cam = UsdGeomCamera(prim);
                hydra->SetCameraFromGfCamera(cam.GetCamera(timeCode));
            }

            ImGui::SameLine();
            if (ImGui::Button("Select")) {
                auto selection = SelectionProvider::instance();
                selection->SetSelectionPrims(&(*stage), {cameras[_self->selected_camera]});
            }
        }
        ImGui::EndGroup();

        ImGui::End();
    }

    void ScoutActivity::Menu() {
        if (ImGui::BeginMenu("Modes")) {
            if (ImGui::MenuItem("Scout", nullptr, _self->ui_visible, true)) {
                _self->ui_visible = !_self->ui_visible;
            }
            ImGui::EndMenu();
        }
    }

} // lab
