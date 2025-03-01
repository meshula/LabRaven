#ifndef ACTIVITIES_OPENUSD_HYDRAVIEWPORT_HPP
#define ACTIVITIES_OPENUSD_HYDRAVIEWPORT_HPP

/**
 * @file viewport.h
 * @author Raphael Jouretz (rjouretz.com)
 * @brief Viewport view acts as a viewport for the data within the current
 * Model. It allows to visualize the USD data with the current Model using an
 * instance of UsdImagingGLEngine.
 *
 * @copyright Copyright (c) 2023
 *
 */

 // clang-format off
#include <imgui.h>
#include <imgui_internal.h>
// clang-format on
#include <ImGuizmo.h>
#include <pxr/base/gf/camera.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usdImaging/usdImaging/stageSceneIndex.h>
#include <pxr/usdImaging/usdImaging/sceneIndices.h>

#include "engine.h"
#include "sceneindices/gridsceneindex.h"
#include "sceneindices/xformfiltersceneindex.h"
#include "views/view.h"


namespace lab {
/**
 * @brief HydraViewport view acts as a viewport for the data within the current
 * Model. It allows to visualize the USD data with the current Model using an
 * instance of UsdImagingGLEngine.
 *
 */
class HydraViewport : public pxr::View {
    public:
        inline static const std::string VIEW_TYPE = "HydraViewport";

        /**
         * @brief Construct a new HydraViewport object
         *
         * @param model the Model of the new HydraViewport view
         * @param label the ImGui label of the new HydraViewport view
         */
        HydraViewport(const std::string label = VIEW_TYPE);

        /**
         * @brief Destroy the HydraViewport object
         *
         */
        ~HydraViewport();

        /**
         * @brief Override of the View::GetViewType
         *
         */
        const std::string GetViewType() override;

        /**
         * @brief Override of the View::_GetGizmoWindowFlags
         *
         */
        ImGuiWindowFlags _GetGizmoWindowFlags() override;

        PXR_NS::GfVec2d GetNearFar() const { return { _zNear, _zFar }; }
        void SetNearFar(float n, float f) { _zNear = n; _zFar = f; }
        float GetAspect();
        PXR_NS::GfCamera GetGfCamera() const { return _cam; }
        void SetCameraFromGfCamera(const PXR_NS::GfCamera& gfCam);
        PXR_NS::SdfPathVector GetHdSelection();
        void RemoveSceneIndex(pxr::HdSceneIndexBaseRefPtr);
    void SetHdSelection(const PXR_NS::SdfPathVector& spv);
    PXR_NS::HdSceneIndexBaseRefPtr GetFinalSceneIndex();
    void SetStage(PXR_NS::UsdStageRefPtr stage);

    PXR_NS::HdSceneIndexBaseRefPtr GetEditableSceneIndex();
    void SetEditableSceneIndex(PXR_NS::HdSceneIndexBaseRefPtr sceneIndex);
    void SetTime(PXR_NS::UsdTimeCode);


    //--------------------------------------------------------------------------

    void SetHit(PXR_NS::GfVec3f hitPoint, PXR_NS::GfVec3f hitNormal)
    {
        _hitPoint = hitPoint;
        _hitNormal = hitNormal;
        _hitGeneration++;
    }

    int GetHit(PXR_NS::GfVec3f& hitPoint, PXR_NS::GfVec3f& hitNormal)
    {
        hitPoint = _hitPoint;
        hitNormal = _hitNormal;
        return _hitGeneration;
    }

    private:
    class Self;
        std::unique_ptr<Self> _model;

    int _hitGeneration = 0;
    PXR_NS::GfVec3f _hitPoint, _hitNormal;
    
    PXR_NS::SdfPathVector _hdSelection;
    PXR_NS::SdfPath _activeCamera;


    const float _FREE_CAM_FOV = 45.f;

        float _zNear = 0.1f;
        float _zFar = 1.0e4f;

        bool _isAmbientLightEnabled, _isDomeLightEnabled, _isGridEnabled;

        bool _guiInterceptedMouse = false;

        PXR_NS::SdfPath _activeCam;
        PXR_NS::GfCamera _cam;  // cached whenever it's computed for the viewport

    PXR_NS::GfMatrix4d _proj;

        pxr::Engine* _engine;
    PXR_NS::GridSceneIndexRefPtr _gridSceneIndex;
    PXR_NS::XformFilterSceneIndexRefPtr _xformSceneIndex;

    PXR_NS::UsdImagingSceneIndices _sceneIndices;

        ImGuiWindowFlags _gizmoWindowFlags;

        ImGuizmo::OPERATION _curOperation;
        ImGuizmo::MODE _curMode;

        /**
         * @brief Get the width of the HydraViewport
         *
         * @return the width of the viewport
         */
        float _GetViewportWidth();

        /**
         * @brief Get the height of the HydraViewport
         *
         * @return the height of the viewport
         */
        float _GetViewportHeight();

        /**
         * @brief Override of the View::Draw
         *
         */
        void _Draw() override;

        /**
         * @brief Draw the Menu bar of the HydraViewport
         *
         */
        void _DrawMenuBar();

        /**
         * @brief Configure ImGuizmo
         *
         */
        void _ConfigureImGuizmo();

        /**
         * @brief Update the grid within the viewport
         *
         */
        void _UpdateGrid();

        /**
         * @brief Update the USD render
         *
         */
        void _UpdateHydraRender();

        /**
         * @brief Update the transform Guizmo (the 3 axis of a selection)
         *
         */
        void _UpdateTransformGuizmo();

        /**
         * @brief Update the gizmo cube (cube at top right)
         *
         */
        void _UpdateCubeGuizmo();

        /**
         * @brief Update the label of the current Usd Hydra plugin used by the
         * viewport (above the gizmo cube)
         *
         */
        void _UpdatePluginLabel();

        /**
         * @brief Pan the active camera by the mouse position delta
         *
         * @param mouseDeltaPos the mouse position delta for the pan
         */
        void _PanActiveCam(ImVec2 mouseDeltaPos);

        /**
         * @brief Orbit the active camera by the mouse position delta
         *
         * @param mouseDeltaPos the mouse position delta for the orbit
         */
        void _OrbitActiveCam(ImVec2 mouseDeltaPos);

        /**
         * @brief Zoom the active camera by the mouse position delta
         *
         * @param mouseDeltaPos the mouse position delta for the zoom
         */
        void _ZoomActiveCam(ImVec2 mouseDeltaPos);

        /**
         * @brief Zoom the active calera by the scroll whell value
         *
         * @param scrollWheel the scroll wheel value for the zoom
         */
        void _ZoomActiveCam(float scrollWheel);

        /**
         * @brief Set the free camera as the active one
         *
         */
        void _SetFreeCamAsActive();

        /**
         * @brief Set the given camera as the active one
         *
         * @param primPath the path to the camera to set as active
         */
        void _SetActiveCam(pxr::SdfPath primPath);

        /**
         * @brief Update the viewport from the active camera
         *
         */
        void _UpdateViewportFromActiveCam();

        /**
         * @brief Get the view matrix of the viewport
         *
         * @return the view matrix
         */
        pxr::GfMatrix4d _getCurViewMatrix();

        /**
         * @brief Update active camera from viewport
         *
         */
        void _UpdateActiveCamFromViewport();

        /**
         * @brief Update Viewport projection matrix from the active camera
         *
         */
        void _UpdateProjection();

        /**
         * @brief Convert a Hydra Prim to a GfCamera
         *
         * @param prim the Prim to create the camera from
         * @return pxr::GfCamera the camera
         */
        pxr::GfCamera _ToGfCamera(pxr::HdSceneIndexPrim prim);

        /**
         * @brief Focus the active camera and the viewport on the given prim
         *
         * @param primPath the prim to focus on
         */
        void _FocusOnPrim(pxr::SdfPath primPath);

        /**
         * @brief Override of the View::_KeyPressEvent
         *
         */
        void _KeyPressEvent(ImGuiKey key) override;

        /**
         * @brief Override of the View::_MouseMoveEvent
         *
         */
        void _MouseMoveEvent(ImVec2 prevPos, ImVec2 curPos) override;

        /**
         * @brief Override of the View::_MouseReleaseEvent
         *
         */
        void _MouseReleaseEvent(ImGuiMouseButton_ button,
                                ImVec2 mousePos) override;

        /**
         * @brief Override of the View::_HoverInEvent
         *
         */
        void _HoverInEvent() override;

        /**
         * @brief Override of the View::_HoverOutEvent
         *
         */
        void _HoverOutEvent() override;
};

} // namespace lab

#endif // ACTIVITIES_OPENUSD_HYDRAVIEWPORT_HPP
