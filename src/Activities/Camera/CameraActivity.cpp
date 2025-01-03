//
//  CameraActivity.cpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 11/30/23.
//  Copyright © 2023 Nick Porcino. All rights reserved.
//

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "CameraActivity.hpp"
#include "imgui.h"
#include "imgui-knobs.hpp"
#include "LabCamera.h"
#include "LabCameraImGui.h"
#include "ImGuizmo.h"
#include "LabGizmo.hpp"
#include "Providers/OpenUSD/UsdUtils.hpp"
#include "Providers/OpenUSD/OpenUSDProvider.hpp"
#include <pxr/base/gf/matrix3f.h>
#include <pxr/usd/usdGeom/metrics.h>

namespace lab {

using namespace PXR_NS;

struct CameraActivity::data {
    bool navigator_visible = true;
    LCNav_Panel* navigator_panel;
    lc_interaction* interactive_controller = nullptr;
    lc_interaction* ttl_controller = nullptr;
    float yaw_input = 0.f;
    float pitch_input = 0.f;
    float dolly_input = 0.f;
    float crane_input = 0.f;
    float pan_input = 0.f;
    lc_i_Mode drag_mode = lc_i_ModePanTilt;
    
    float initial_mouse_x = 0.f;
    float initial_mouse_y = 0.f;
    
    int drag_bid = 1;
    int check_hydra_pick = 0;
    int hitGeneration = 0;
    bool drag_became_select = false;
    bool hitPointValid = false;
    bool show_tumble_box = false;
    PXR_NS::GfVec3d hit_point = {0,0,0};
    
    lc_camera camera;
};

int CameraActivity::ViewportDragBid(const LabViewInteraction&) {
    return _self->drag_bid;
}

CameraActivity::CameraActivity() : Activity(CameraActivity::sname()) {
    _self = new CameraActivity::data;
    _self->navigator_panel = create_navigator_panel();
    _self->interactive_controller = lc_i_create_interactive_controller();
    lc_i_set_speed(_self->interactive_controller, 0.01f, 0.005f);
    _self->ttl_controller = lc_i_create_interactive_controller();
    lc_camera_set_defaults(&_self->camera);
    _self->camera.mount.transform.position = {0, 5, 50};
    lc_v3f local_up = { 0, 1, 0 };
    lc_v3f orbit = { 0, 0.5f, 0 };
    lc_i_set_orbit_center_constraint(_self->interactive_controller, orbit);
    lc_mount_look_at(&_self->camera.mount, _self->camera.mount.transform.position, orbit, local_up);
    
    activity.Render = [](void* instance, const LabViewInteraction* vi) {
        static_cast<CameraActivity*>(instance)->Render(*vi);
    };
    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<CameraActivity*>(instance)->RunUI(*vi);
    };
    activity.ViewportDragBid = [](void* instance, const LabViewInteraction* vi) -> int {
        return static_cast<CameraActivity*>(instance)->ViewportDragBid(*vi);
    };
    activity.ViewportDragging = [](void* instance, const LabViewInteraction* vi) {
        static_cast<CameraActivity*>(instance)->ViewportDragging(*vi);
    };
    activity.ToolBar = [](void* instance) {
        static_cast<CameraActivity*>(instance)->ToolBar();
    };
    activity.Menu = [](void* instance) {
        static_cast<CameraActivity*>(instance)->Menu();
    };
}

CameraActivity::~CameraActivity() {
    release_navigator_panel(_self->navigator_panel);
    _self->navigator_panel = nullptr;
    delete _self;
}

lc_camera& CameraActivity::GetCamera() const { return _self->camera; }

PXR_NS::GfCamera CameraActivity::GetGfCamera() const {
    auto usd = OpenUSDProvider::instance();
    GfCamera ret;
    ret.SetTransform(GfMatrix4d(GetViewMatrix()));
    ret.SetProjection(GfCamera::Perspective);

    // camera units are 1/10 stage units, the value 1/10 is in the 
    // constant GfCamera::APERTURE_UNIT.
    // we know how to convert stage units to meters, so we can convert
    // millimeters to camera units by dividng by the APERTURE_UNIT, and then dividing
    // by the stage to meters conversion factor.

    auto stage = usd->Stage();
    float metersToStage = UsdGeomGetStageMetersPerUnit(stage);

    float apertureWidth = _self->camera.sensor.aperture.y.mm * 0.001f / metersToStage / GfCamera::APERTURE_UNIT;
    float apertureHeight = _self->camera.sensor.aperture.x.mm * 0.001f / metersToStage / GfCamera::APERTURE_UNIT;
    ret.SetHorizontalAperture(apertureWidth);
    ret.SetVerticalAperture(apertureHeight);

    // similarly for focal length, except using GfCamera::FOCAL_LENGTH_UNIT
    float focalLengthM = _self->camera.optics.focal_length.mm * 0.001f;
    float focalLengthStage = focalLengthM / metersToStage;
    ret.SetFocalLength(focalLengthStage / GfCamera::FOCAL_LENGTH_UNIT);

    ret.SetFStop(_self->camera.optics.fStop);
    ret.SetFocusDistance(_self->camera.optics.focus_distance.m * metersToStage);

    return ret;
}


PXR_NS::GfMatrix4f CameraActivity::GetViewMatrix() const {
    auto m = lc_mount_gl_view_transform(&_self->camera.mount);
    GfMatrix4f ret = reinterpret_cast<const float(*)[4]>(&m);
    return ret;
}

PXR_NS::GfFrustum CameraActivity::GetFrustum(const LabViewDimensions& vd, float znear, float zfar) const {
    GfFrustum frustum;
    frustum.SetPositionAndRotationFromMatrix(GfMatrix4d(GetViewMatrix()));
    frustum.SetProjectionType(GfFrustum::ProjectionType::Perspective);
    double targetAspect = double(vd.ww) / double(vd.wh);
    float filmbackWidthMM = GetCamera().sensor.aperture.y.mm;
    double hFOVInRadians = 2.0 * atan(0.5 * filmbackWidthMM /
                                      GetCamera().optics.focal_length.mm);
    double fov = (180.0 * hFOVInRadians)/M_PI;
    frustum.SetPerspective(fov, targetAspect, znear, zfar);
    return frustum;
}

void CameraActivity::SetViewMatrix(const GfMatrix4f& m) {
    GfMatrix4f o = m.GetOrthonormalized();
    GfVec3f t = o.ExtractTranslation();
    _self->camera.mount.transform.position = {t[0], t[1], t[2]};
    GfQuatf q = o.ExtractRotationQuat();
    const GfVec3f i = q.GetImaginary();
    _self->camera.mount.transform.orientation = {i[0], i[1], i[2], q.GetReal()};
}

PXR_NS::GfVec3d CameraActivity::GetPosition() const {
    return {
        _self->camera.mount.transform.position.x,
        _self->camera.mount.transform.position.y,
        _self->camera.mount.transform.position.z
    };
}

void CameraActivity::SetPosition(const PXR_NS::GfVec3d t) {
    _self->camera.mount.transform.position = {(float)t[0], (float)t[1], (float)t[2]};
}


void CameraActivity::SetViewMatrix(const GfMatrix4d& m) {
    GfMatrix4d o = m.GetOrthonormalized();
    GfVec3d t = o.ExtractTranslation();
    _self->camera.mount.transform.position = {(float) t[0], (float)t[1], (float) t[2]};
    GfQuatd q = o.ExtractRotationQuat();
    const GfVec3d i = q.GetImaginary();
    _self->camera.mount.transform.orientation = {(float) i[0], (float) i[1], (float) i[2], (float) q.GetReal()};
}

void CameraActivity::SetFrustum(const GfFrustum f) {
    // translate the parameters in the frustum to the camera
    double fov, aspect, near, far;
    if (f.GetPerspective(&fov, &aspect, &near, &far)) {
        // it is a perspective frustum
        float fov_rad = (float) (fov * 2 * M_PI) / 360.f;
        lc_millimeters focal_length = lc_sensor_focal_length_from_vertical_FOV(
                                            &_self->camera.sensor, lc_radians{fov_rad});
        _self->camera.optics.focal_length = focal_length;
    }
}

void CameraActivity::RunUI(const LabViewInteraction& vi)
{
    activate_navigator_panel(true);
    if (fabsf(_self->yaw_input) > 0.f || fabsf(_self->pitch_input) > 0.f) {
        float trackball_size = 32.f;
        _self->drag_mode = lc_i_ModeTurnTableOrbit;
        lc_i_sync_constraints(_self->ttl_controller, _self->interactive_controller);
        InteractionToken tok = lc_i_begin_interaction(_self->interactive_controller,
                                                      {trackball_size, trackball_size});

        const float roll = 0.f;
        lc_i_single_stick_interaction(
              _self->interactive_controller,
              &_self->camera,
              tok,
              lc_i_ModeTurnTableOrbit,
              {_self->yaw_input * 2.0f, _self->pitch_input * 2.0f},
              {roll},
              vi.dt);
        lc_i_end_interaction(_self->interactive_controller, tok);
    }
    
    if (fabsf(_self->dolly_input) > 0.f) {
        float trackball_size = 32.f;
        _self->drag_mode = lc_i_ModeDolly;
        lc_i_sync_constraints(_self->ttl_controller, _self->interactive_controller);
        InteractionToken tok = lc_i_begin_interaction(_self->interactive_controller,
                                                      {trackball_size, trackball_size});

        float sn = _self->dolly_input > 0.f ? 1.f : -1.f;
        float in = fabsf(_self->dolly_input);
        if (in > 0.5f) {
            // faster
            in = (in - 0.5f) * 2.f + in;
        }
        if (in > 1.f) {
            // faster still
            in = (in - 1.f) * 2.f + in;
        }
        in *= 10.f * sn;
        
        const float roll = 0.f;
        lc_i_single_stick_interaction(
              _self->interactive_controller,
              &_self->camera,
              tok,
              lc_i_ModeDolly,
              {0, in},
              {roll},
              vi.dt);
        lc_i_end_interaction(_self->interactive_controller, tok);
    }
    
    if (fabsf(_self->crane_input) > 0.f) {
        _self->drag_mode = lc_i_ModeCrane;
        lc_i_set_fixed_orbit_center(_self->interactive_controller, false);

        float trackball_size = 32.f;
        lc_i_sync_constraints(_self->ttl_controller, _self->interactive_controller);
        InteractionToken tok = lc_i_begin_interaction(_self->interactive_controller,
                                                      {trackball_size, trackball_size});

        float sn = _self->crane_input > 0.f ? 1.f : -1.f;
        float in = fabsf(_self->crane_input);
        if (in > 0.5f) {
            // faster
            in = (in - 0.5f) * 2.f + in;
        }
        if (in > 1.f) {
            // faster still
            in = (in - 1.f) * 2.f + in;
        }
        in *= 10.f * sn;
        
        const float roll = 0.f;
        lc_i_single_stick_interaction(
              _self->interactive_controller,
              &_self->camera,
              tok,
              lc_i_ModeCrane,
              {0, in},
              {roll},
              vi.dt);
        lc_i_end_interaction(_self->interactive_controller, tok);
    }
    
    if (fabsf(_self->pan_input) > 0.f) {
        _self->drag_mode = lc_i_ModePanTilt;
        lc_i_set_fixed_orbit_center(_self->interactive_controller, false);

        float trackball_size = 32.f;
        lc_i_sync_constraints(_self->ttl_controller, _self->interactive_controller);
        InteractionToken tok = lc_i_begin_interaction(_self->interactive_controller,
                                                      {trackball_size, trackball_size});

        float sn = _self->pan_input > 0.f ? 1.f : -1.f;
        float in = fabsf(_self->pan_input);
        if (in > 0.5f) {
            // faster
            in = (in - 0.5f) * 2.f + in;
        }
        if (in > 1.f) {
            // faster still
            in = (in - 1.f) * 2.f + in;
        }
        in *= 10.f * sn;
        
        const float roll = 0.f;
        lc_i_single_stick_interaction(
              _self->interactive_controller,
              &_self->camera,
              tok,
              lc_i_ModePanTilt,
              {in, 0},
              {roll},
              vi.dt);
        lc_i_end_interaction(_self->interactive_controller, tok);
    }
    
    if (_self->show_tumble_box)
    {
        // create a fixed size window that can't move or be resized at the top of the screen
        // and force it to be the topmost window
        const float windowX = vi.view.wx * 0.5 + 5;
        const float windowY = vi.view.wy * 0.5 + 5;
        const float windowW = 200;
        const float windowH = 150;
        ImGui::SetNextWindowPos({windowX, windowY});
        ImGui::SetNextWindowSize({windowW, windowH});
        
        // the window should have no border, background, resize controls, and be immovable
        ImGui::SetNextWindowPos({windowX, windowY});
        ImGui::SetNextWindowSize({windowW, windowH});
        ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0);
        ImGui::Begin("Camera Controls", nullptr,
                     ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoTitleBar |
                     ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoCollapse);
        
        lc_m44f m = lc_rt_matrix(&_self->camera.mount.transform);
        lc_m44f m0 = m;
        lc_v3f orbit = lc_i_orbit_center_constraint(_self->interactive_controller);
        lc_v3f pos = _self->camera.mount.transform.position;
        lc_v3f pole = { pos.x - orbit.x, pos.y - orbit.y, pos.z - orbit.z };
        float orbit_distance = sqrtf(pole.x*pole.x + pole.y*pole.y + pole.z*pole.z);

        ImGuizmo::SetDrawlist();
        ImGuizmo::ViewManipulate(
                                 (float*)&m, // view matrix
                                 orbit_distance,        // camera distance constraint
                                 ImVec2(windowX, windowY), // position
                                 ImVec2(100, 100), IM_COL32_BLACK_TRANS);
        float matrixTranslation[3], matrixRotation[3], matrixScale[3];
        ImGuizmo::DecomposeMatrixToComponents((float*)&m,
                                              matrixTranslation,
                                              matrixRotation,
                                              matrixScale);

        // update the camera if the matrix changed.
        float* pm = &m.x.x;
        float* pm0 = &m0.x.x;
        for (int i = 0; i < 16; ++i) {
            if (pm[i] != pm0[i]) {
                tinygizmo::rigid_transform tx;
                tx.set_orientation((float*) &m);
                _self->camera.mount.transform.position = { matrixTranslation[0], matrixTranslation[1], matrixTranslation[2]};
                _self->camera.mount.transform.orientation = {
                    tx.orientation.x, tx.orientation.y, tx.orientation.z, tx.orientation.w };
                break;
            }
        }
        
        ImGui::End();
        ImGui::PopStyleVar(1);
    }
    
    if (_self->navigator_visible)
        run_navigator_panel(_self->navigator_panel, &_self->camera, vi.dt);
}

void CameraActivity::ViewportDragging(const LabViewInteraction& vi) {
    /*--------------------------------------------------------------------------
     On entry to these routine, decide what do according to the following
     state machine.  The star state is always active.
     ---------------------------------------------------------------------------
     [ * ]
          ---( end == true ) ---> [ Not dragging ]
     
     [ Not dragging ]
          ---{ start == true }---> [ raycast ]
     
     [ end ]
          ---> [ Not dragging ]
     
     [ raycast ]
          ---( hydra check incremented }---> [ decide ]
     
     [ decide ]
          if (hit && TTL pick enabled) ---> [ select ]
          if (hit && TTL hit enabled)  ---> [ hit ]
          ---> [ dragging ]
     
     [ select ]
          ---> [ end ]
     
     [ hit ]
          ---> [ dragging ]
     
     [ dragging ]
          ---> [ dragging ]
     -------------------------------------------------------------------------*/

    enum class CameraDraggingStates {
        NotDragging,
        Raycast,
        Decide,
        Select,
        Hit,
        Dragging
    };

    static CameraDraggingStates state = CameraDraggingStates::NotDragging;
    lc_i_Phase phase = lc_i_PhaseContinue;

    // first check the star condition
    if (vi.end) {
        state = CameraDraggingStates::NotDragging;
        _self->drag_bid = 1;
    }

    // the states themselves are implemented as a bunch of individual lambdas,
    // and the machine is executed by running a while loop until one of the
    // states returns false.
    
    auto not_dragging = [&]() -> bool {
        if (vi.start) {
            state = CameraDraggingStates::Raycast;
            return true;
        }
        return false;
    };
    
    auto raycast = [&]() -> bool {
        auto usd = OpenUSDProvider::instance();
        auto model = usd->Model();
        PXR_NS::GfVec3f point;
        PXR_NS::GfVec3f normal;
        _self->check_hydra_pick = model->GetHit(point, normal); // find out the current hit, we're going to wait for the next
        state = CameraDraggingStates::Decide;
        return true; // continue to next state
    };

    auto decide = [&]() -> bool {
        _self->hitPointValid = false;
        auto usd = OpenUSDProvider::instance();
        auto model = usd->Model();
        PXR_NS::GfVec3f point;
        PXR_NS::GfVec3f normal;
        int hitGeneration = model->GetHit(point, normal);
        auto selectedPrims = model->GetSelection();
        PXR_NS::SdfPath prim;
        if (selectedPrims.size() > 0)
            selectedPrims[0];
        bool got_hit = !prim.IsEmpty();
        if (hitGeneration > _self->check_hydra_pick) {

            /// @TODO expose the "hit enabled" option
            constexpr bool selectionHitEnabled = true; //selection->HitEnabled()
            if (selectionHitEnabled && got_hit) {
                state = CameraDraggingStates::Hit;
                _self->hit_point = point;
                _self->hitPointValid = true;
            }
            /// @TODO expose the "ttl enabled" option
            constexpr bool selectionTTLEnabled = true; // selection->TTLEnabled()
            if (selectionTTLEnabled) {
                state = CameraDraggingStates::Select;
                if (hitGeneration > _self->hitGeneration && got_hit) {
                    _self->drag_bid = 0;
                    _self->hitGeneration = hitGeneration;
                    _self->drag_became_select = true;
                    bool selection_changed = false;
                    if (selectedPrims.size() > 0 && selectedPrims[0] == prim) {
                        state = CameraDraggingStates::Dragging;
                    }
                    else {
                        selectedPrims.clear();
                        selectedPrims.push_back(prim);
                        model->SetSelection(selectedPrims);
                        return false; // break out
                    }
                }
            }
            state = CameraDraggingStates::Dragging;
            phase = lc_i_PhaseStart;
            _self->initial_mouse_x = vi.x;
            _self->initial_mouse_y = vi.y;
            _self->check_hydra_pick = 0;
            _self->drag_became_select = false;

            if (true) {//_self->hitPointValid) {
                lc_v3f camera_forward = lc_mount_forward(&_self->camera.mount);
                lc_v3f hit_point = { (float) _self->hit_point[0],
                                     (float) _self->hit_point[1],
                                     (float) _self->hit_point[2] };
                lc_v3f plane_normal = camera_forward;
                lc_v3f plane_point = hit_point;

                // compute the intersection of the ray with the plane
                lc_hit_result intersects = lc_camera_hit_test(
                                                    &_self->camera,
                                                    (lc_v2f) {vi.x, vi.y},
                                                    (lc_v2f) {vi.view.ww, vi.view.wh},
                                                    plane_point, plane_normal);
                if (intersects.hit) {
                    _self->hit_point = { intersects.point.x, intersects.point.y, intersects.point.z };
                }
            }

            return true; // continue in Dragging mode
        }
        return false; // nothing to do this time, try again next frame
    };

    auto select = [&]() -> bool {
        state = CameraDraggingStates::NotDragging;
        return false;
    };

    auto hit = [&]() -> bool {
        state = CameraDraggingStates::Dragging;
        phase = lc_i_PhaseStart;
        return false;
    };

    auto dragging = [&]() -> bool {
        state = CameraDraggingStates::Dragging;
        const float roll = 0.f;
        
        //printf("%f %f %f %f\n", mousex, mousex, _self->initial_mouse_x, _self->initial_mouse_y);
        
        // there's an error in the controller so compensate here until I've got time to work out the math again
        float mousex_aspected = ((vi.x - vi.view.ww * 0.5f) *
                                 vi.view.ww/vi.view.wh) + vi.view.ww * 0.5f;

        lc_i_sync_constraints(_self->ttl_controller, _self->interactive_controller);
        InteractionToken tok = lc_i_begin_interaction(_self->ttl_controller,
                                                      {vi.view.ww, vi.view.wh});
        lc_i_constrained_ttl_interaction(_self->ttl_controller,
                                        &_self->camera,
                                        tok, phase, _self->drag_mode,
                                        { mousex_aspected, vi.y },
                                        { (float) _self->hit_point[0],
                                          (float) _self->hit_point[1],
                                          (float) _self->hit_point[2] },
                                        lc_radians{roll},
                                        vi.dt);
        lc_i_end_interaction(_self->ttl_controller, tok);
        phase = lc_i_PhaseContinue;
        return false;
    };

    while (true) {
        switch (state) {
            case CameraDraggingStates::NotDragging:
                if (!not_dragging()) return;
                break;
            case CameraDraggingStates::Raycast:
                if (!raycast()) return;
                break;
            case CameraDraggingStates::Decide:
                if (!decide()) return;
                break;
            case CameraDraggingStates::Select:
                if (!select()) return;
                break;
            case CameraDraggingStates::Hit:
                if (!hit()) return;
                break;
            case CameraDraggingStates::Dragging:
                if (!dragging()) return;
                break;
        }
    }    
}

void CameraActivity::Render(const LabViewInteraction& vi) {
#ifdef HAVE_HYDRA_IMM
    static TriMeshPNC scribble;
#endif

    auto& d = vi.view;

    auto usd = OpenUSDProvider::instance();
    auto model = usd->Model();
    PXR_NS::GfVec3f point;
    PXR_NS::GfVec3f normal;
    int gen = model->GetHit(point, normal);
    GfRotation rot({0,1,0}, normal);
    GfMatrix3f m(rot);
    m = m.GetInverse();

#ifdef HAVE_HYDRA_IMM
    // create a scribble mesh centered at _self->hitPoint
    if (_self->hitGeneration > 0) {
        float sz = ComputeWorldSize(_self->hit_point, 50, d);

        scribble.vertices.clear();
        scribble.triangles.clear();

#if 1
        // create a cone whose point is at _self->hitPoint, and oriented in the
        // direction of _self->hitNormal. The cone has eight faces.
        // the cone angle is 30 degrees.
        int n = 8;
        float angle = 0;
        float angle_step = 2 * M_PI / n;
        for (uint32_t i = 0; i < n; ++i) {
            float x0 = cosf(angle) * sz * 0.5f;
            float y0 = sinf(angle) * sz * 0.5f;
            float x1 = cosf(angle + angle_step) * sz * 0.5f;
            float y1 = sinf(angle + angle_step) * sz * 0.5f;

            scribble.vertices.push_back({{0, 0, 0},    {0,0,1}, {1, 0, 0, 1}});
            scribble.vertices.push_back({{x0, sz, y0}, {0,0,1}, {1, 0, 0, 1}});
            scribble.vertices.push_back({{x1, sz, y1}, {0,0,1}, {1, 0, 0, 1}});
            scribble.triangles.push_back({(i * 3) + 0, (i * 3) + 1, (i * 3) + 2});
            angle += angle_step;
        }

        // rotate the cone along the normal
        GfRotation({0,1,0}, {normal});
        for (auto& v : scribble.vertices) {
            GfVec3f p = {v.position.x, v.position.y, v.position.z};
            p = m * p;
            v.position = {p[0], p[1], p[2]};
        }

        // compute the normals
        for (uint32_t i = 0; i < n*3; i += 3) {
            GfVec3f v0 = {scribble.vertices[i+0].position.x,
                          scribble.vertices[i+0].position.y,
                          scribble.vertices[i+0].position.z};
            GfVec3f v1 = {scribble.vertices[i+1].position.x,
                          scribble.vertices[i+1].position.y,
                          scribble.vertices[i+1].position.z};
            GfVec3f v2 = {scribble.vertices[i+2].position.x,
                          scribble.vertices[i+2].position.y,
                          scribble.vertices[i+2].position.z};
            GfVec3f n = (v1 - v0) ^ (v2 - v0);
            n.Normalize();
            scribble.vertices[i+0].normal = {n[0], n[1], n[2]};
            scribble.vertices[i+1].normal = {n[0], n[1], n[2]};
            scribble.vertices[i+2].normal = {n[0], n[1], n[2]};
        }

#if 0
        // compute the normals
        for (uint32_t i = 0; i < scribble.triangles.size(); ++i) {
            auto& t = scribble.triangles[i];
            GfVec3f v0 = {scribble.vertices[t[0]].position.x, scribble.vertices[t[0]].position.y, scribble.vertices[t[0]].position.z};
            GfVec3f v1 = {scribble.vertices[t[1]].position.x, scribble.vertices[t[1]].position.y, scribble.vertices[t[1]].position.z};
            GfVec3f v2 = {scribble.vertices[t[2]].position.x, scribble.vertices[t[2]].position.y, scribble.vertices[t[2]].position.z};
            GfVec3f n = (v1 - v0) ^ (v2 - v0);
            n.Normalize();
            scribble.vertices[t[0]].normal = {n[0], n[1], n[2]};
            scribble.vertices[t[1]].normal = {n[0], n[1], n[2]};
            scribble.vertices[t[2]].normal = {n[0], n[1], n[2]};
        }
#endif
        // add _self->hitPoint to all vertices
        for (auto& v : scribble.vertices) {
            v.position.x += _self->hit_point[0];
            v.position.y += _self->hit_point[1];
            v.position.z += _self->hit_point[2];
        }

        hydra->ImmMesh(&scribble, HydraActivity::StencilPass);

#else
        scribble.vertices.push_back({{ -sz, sz, 0.f }, { 0.f, 0.f, 1.f }, { 1.f, 0.f, 0.f, 1.f }});
        scribble.vertices.push_back({{ -sz,-sz, 0.f }, { 0.f, 0.f, 1.f }, { 1.f, 0.f, 0.f, 1.f }});
        scribble.vertices.push_back({{  sz,-sz, 0.f }, { 0.f, 0.f, 1.f }, { 1.f, 0.f, 0.f, 1.f }});
        scribble.vertices.push_back({{  sz, sz, 0.f }, { 0.f, 0.f, 1.f }, { 1.f, 0.f, 0.f, 1.f }});
        // add _self->hitPoint to all vertices
        for (auto& v : scribble.vertices) {
            v.position.x += _self->hit_point[0];
            v.position.y += _self->hit_point[1];
            v.position.z += _self->hit_point[2];
        }
        // add random values between -sz*0.25 and sz*0.25 to all vertices
        for (auto& v : scribble.vertices) {
            v.position.x += (rand() / (float)RAND_MAX - 0.5f) * sz * 0.5f;
            v.position.y += (rand() / (float)RAND_MAX - 0.5f) * sz * 0.5f;
            v.position.z += (rand() / (float)RAND_MAX - 0.5f) * sz * 0.5f;
        }
        scribble.triangles.push_back({0, 1, 2});
        scribble.triangles.push_back({2, 3, 0});
        hydra->ImmMesh(&scribble);
#endif
    }
#endif // HAVE_HYDRA_IMM
}

void CameraActivity::ToolBar() {
    auto min = ImGui::GetCursorScreenPos();
    auto cp = ImGui::GetCursorPos();

    ImGui::SetCursorPos({cp.x + 8, cp.y + 8});
    ImGui::BeginGroup();

    ImGui::BeginGroup();
    if (ImGui::Button("Navigator", ImVec2(0, 37))) {
        activate_navigator_panel(!_self->navigator_visible);
    }
    
    static const char* lensesStr[] = {
        "8", "16", "24", "35", "50", "80", "100", "120", "200", "300", nullptr };
    
    static int selected_fl = 4; // 50
    ImGui::PushItemWidth(75);
    if (ImGui::BeginCombo("###focal-combo", lensesStr[selected_fl]))
    {
        int n = 0;
        if (lensesStr[n] != nullptr) {
            do {
                bool is_selected = (selected_fl == n);
                if (ImGui::Selectable(lensesStr[n], is_selected)) {
                    selected_fl = n;
                    _self->camera.optics.focal_length = { std::stof(lensesStr[n]) };
                }
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
                ++n;
            } while (lensesStr[n] != nullptr);
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    ImGui::EndGroup();
    
    ImGui::SameLine();
    
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    //---------------------------
    static float orbit = 0;
    if (ImGuiKnobs::Knob("orbit", &orbit, -M_PI, M_PI, true)) {
        _self->drag_mode = lc_i_ModeTurnTableOrbit;
    }
    _self->yaw_input = orbit;
    if (_self->drag_mode == lc_i_ModeTurnTableOrbit) {
        auto szmn = ImGui::GetItemRectMin();
        auto szmx = ImGui::GetItemRectMax();
        szmn.y = szmx.y - 16;
        szmx.y += 2;
        drawList->AddRect(szmn, szmx, ImColor(100, 100, 0));
    }
    //---------------------------
    static float pitch = 0;
    ImGui::SameLine();
    if (ImGuiKnobs::Knob("pitch", &pitch, -M_PI, M_PI, true)) {
        _self->drag_mode = lc_i_ModeTurnTableOrbit;
    }
    _self->pitch_input = pitch;
    //---------------------------
    static float dolly = 0;
    ImGui::SameLine();
    if (ImGuiKnobs::Knob("dolly", &dolly, -1, 1, true)) {
        _self->drag_mode = lc_i_ModeDolly;
    }
    _self->dolly_input = dolly;
    if (_self->drag_mode == lc_i_ModeDolly) {
        auto szmn = ImGui::GetItemRectMin();
        auto szmx = ImGui::GetItemRectMax();
        szmn.y = szmx.y - 16;
        szmx.y += 2;
        drawList->AddRect(szmn, szmx, ImColor(100, 100, 0));
    }
    //---------------------------
    static float crane = 0;
    ImGui::SameLine();
    if (ImGuiKnobs::Knob("crane", &crane, -1, 1, true)) {
        _self->drag_mode = lc_i_ModeCrane;
    }
    _self->crane_input = crane;
    if (_self->drag_mode == lc_i_ModeCrane) {
        auto szmn = ImGui::GetItemRectMin();
        auto szmx = ImGui::GetItemRectMax();
        szmn.y = szmx.y - 16;
        szmx.y += 2;
        drawList->AddRect(szmn, szmx, ImColor(100, 100, 0));
    }
    //---------------------------
    static float pan = 0;
    ImGui::SameLine();
    if (ImGuiKnobs::Knob("pan", &pan, -1, 1, true)) {
        _self->drag_mode = lc_i_ModePanTilt;
    }
    _self->pan_input = pan;
    if (_self->drag_mode == lc_i_ModePanTilt) {
        auto szmn = ImGui::GetItemRectMin();
        auto szmx = ImGui::GetItemRectMax();
        szmn.y = szmx.y - 16;
        szmx.y += 2;
        drawList->AddRect(szmn, szmx, ImColor(100, 100, 0));
    }
    //---------------------------

    auto usd = OpenUSDProvider::instance();
    auto model = usd->Model();
    auto selection = model->GetSelection();

    PXR_NS::SdfPath prim;
    bool have_selection = selection.size() > 0;
    if (_self->hitGeneration > 0) {
        if (ImGui::Button("Look at hit")) {
            LookAt(_self->hit_point);
        }
        if (have_selection)
            ImGui::SameLine();
    }
    
    if (have_selection) {
        if (ImGui::Button("Look at selection")) {
            LookAtSelection();
        }
        ImGui::SameLine();
        if (ImGui::Button("Frame selection")) {
            FrameSelection();
        }
    }
    ImGui::Checkbox("tumbler", &_self->show_tumble_box);

    ImGui::EndGroup();

    auto max = ImGui::GetItemRectMax();
    drawList->AddRect(min, {max.x + 4, max.y + 4}, ImColor(100, 100, 0));

    ImGui::SameLine();
    cp = ImGui::GetCursorPos();
    ImGui::SetCursorPos({cp.x+4, cp.y});
}

PXR_NS::GfVec3d CameraActivity::HitPoint() const {
    if (_self->hitGeneration > 0)
        return _self->hit_point;
    return {0,0,0};
}


void CameraActivity::Menu() {
    if (ImGui::BeginMenu("Selection")) {
        if (ImGui::MenuItem("Frame Selection")) {
            FrameSelection();
        }
        if (ImGui::MenuItem("Look at Selection")) {
            LookAtSelection();
        }
        ImGui::EndMenu();
    }
}

void CameraActivity::FrameSelection() {
    lc_i_sync_constraints(_self->ttl_controller, _self->interactive_controller);

    auto usd = OpenUSDProvider::instance();
    auto model = usd->Model();
    auto selection = model->GetSelection();
    auto bbox = ComputeWorldBounds(model->GetStage(), {}, selection);

    GfVec3d center = bbox.ComputeCentroid();
    if (bbox.GetVolume() > 0) {
        auto bboxRange = bbox.ComputeAlignedRange();
        auto rect = bboxRange.GetMax() - bboxRange.GetMin();
        float selectionSize = std::max(rect[0], rect[1]) * 2; // This reset the selection size
        lc_radians fov = lc_camera_horizontal_FOV(&_self->camera);
        auto lengthToFit = selectionSize * 0.5;
        float dist = lengthToFit / atan(fov.rad * 0.5);
        lc_v3f p = _self->camera.mount.transform.position;
        GfVec3d gp = { p.x, p.y, p.z };
        lc_v3f o = lc_i_orbit_center_constraint(_self->interactive_controller);
        GfVec3d go = { o.x, o.y, o.z };
        gp -= go;
        gp.Normalize();
        gp *= dist;
        gp += go;
        o = {(float)center[0], (float) center[1], (float) center[2]};
        _self->hit_point = center;
        lc_mount_look_at(&_self->camera.mount,
                         {(float) gp[0], (float) gp[1], (float) gp[2]},
                         o,
                         {0.f, 1.f, 0.f});
        lc_i_set_orbit_center_constraint(_self->interactive_controller, o);
    }
    else {
        // just do a look at
        lc_v3f o = {(float)center[0], (float) center[1], (float) center[2]};
        _self->hit_point = center;
        lc_mount_look_at(&_self->camera.mount,
                         _self->camera.mount.transform.position,
                         o,
                         {0.f, 1.f, 0.f});
        lc_i_set_orbit_center_constraint(_self->interactive_controller, o);
    }
}

void CameraActivity::LookAt(const PXR_NS::GfVec3d& center) {
    lc_i_sync_constraints(_self->ttl_controller, _self->interactive_controller);
    lc_v3f o = {(float)center[0], (float) center[1], (float) center[2]};
    _self->hit_point = center;
    lc_mount_look_at(&_self->camera.mount,
                     _self->camera.mount.transform.position,
                     o,
                     {0.f, 1.f, 0.f});
    lc_i_set_orbit_center_constraint(_self->interactive_controller, o);
}

void CameraActivity::LookAtSelection() {
    lc_i_sync_constraints(_self->ttl_controller, _self->interactive_controller);

    auto usd = OpenUSDProvider::instance();
    auto model = usd->Model();
    auto selection = model->GetSelection();
    auto box = ComputeWorldBounds(model->GetStage(), {}, selection);

    GfVec3d center = box.ComputeCentroid();
    _self->hit_point = center;
    lc_v3f o = {(float)center[0], (float) center[1], (float) center[2]};
    lc_mount_look_at(&_self->camera.mount,
                     _self->camera.mount.transform.position,
                     o,
                     {0.f, 1.f, 0.f});
    lc_i_set_orbit_center_constraint(_self->interactive_controller, o);
}

float CameraActivity::ComputeWorldSize(const PXR_NS::GfVec3d& worldPos,
                                       float pixel_scale,
                                       const LabViewDimensions& dimensions) {
    // given a world position and the size an object at that position
    // would appear on screen, compute the size of the object in world space
    // that would appear the same size on screen

    PXR_NS::GfVec3d cameraPos = {
        _self->camera.mount.transform.position.x,
        _self->camera.mount.transform.position.y,
        _self->camera.mount.transform.position.z,
    };
    
    float distance = (worldPos - cameraPos).GetLength();
    float yfov = lc_camera_vertical_FOV(&_self->camera).rad;
    float viewHeightPixels = dimensions.wh;
    float desiredScreenSize = pixel_scale;
    float worldSize = desiredScreenSize * distance * tanf(yfov * 0.5f) * 2.f / viewHeightPixels;
    return worldSize;
}


} //lab

