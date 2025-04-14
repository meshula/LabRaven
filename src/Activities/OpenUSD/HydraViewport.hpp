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

#include "Lab/StudioCore.hpp"
#include "HydraRender.hpp"

 // clang-format off
#include <imgui.h>
#include <imgui_internal.h>
// clang-format on
#include <ImGuizmo.h>
#include <pxr/base/gf/camera.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usdImaging/usdImaging/stageSceneIndex.h>
#include <pxr/usdImaging/usdImaging/sceneIndices.h>

#include "Providers/OpenUSD/sceneindices/gridsceneindex.h"
#include "Providers/OpenUSD/sceneindices/xformfiltersceneindex.h"

#include <imgui.h>
#include <imgui_internal.h>
#include <pxr/imaging/hd/sceneIndex.h>


namespace lab {
class Engine;

/**
 * @file view.h
 * @author Raphael Jouretz (rjouretz.com)
 * @brief View represents an abstract view for the Main Window. Each class
 * inheriting from View can take advantage of convenient methods such as
 * events, flag definition and globals.
 *
 * @copyright Copyright (c) 2023
 */

/**
 * @brief View represents an abstract view for the Main Window. Each class
 * inheriting from View can take advantage of convenient methods such as
 * events, flag definition and globals.
 *
 */
class HydraViewport {
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
         * @brief Get the View Type of the current View object
         *
         * @return the view type
         */
        virtual const std::string GetViewType();

        /**
         * @brief Get the View Label of the current View object
         *
         * @return the view label
         */
        const std::string GetViewLabel();

        /**
         * @brief Update the current ImGui view and set the states for the
         * events
         *
         */
        void Update(const LabViewInteraction& vi);

        /**
         * @brief Check if the current View object is displayed
         *
         * @return true if the view is displayed
         * @return false  otherwise
         */
        bool IsDisplayed();

    protected:        /**
         * @brief Get the inner rectangle object of the current ImGui view
         *
         * @return the rectangle object
         */
        ImRect GetInnerRect();

    private:
        std::string _label;
        bool _wasFocused;
        bool _wasHovered;
        bool _wasDisplayed;
        ImRect _innerRect;
        ImVec2 _prevMousePos;

        /**
         * @brief Called during the update of the view. Allow for custom draw
         *
         */
    virtual void _Draw(const LabViewInteraction& vi);

        /**
         * @brief Called when the view switch from unfocus to focus
         *
         */
        virtual void _FocusInEvent();

        /**
         * @brief Called when the view swich from focus to unfocus
         *
         */
        virtual void _FocusOutEvent();

        /**
         * @brief Called when the mouse enter the view
         *
         */
        virtual void _HoverInEvent();

        /**
         * @brief Called when the mouse exit the view
         *
         */
        virtual void _HoverOutEvent();

        /**
         * @brief Called when a key is press
         *
         * @param key the key that is pressed
         */
        virtual void _KeyPressEvent(ImGuiKey key);

        /**
         * @brief Called when the mouse is pressed
         *
         * @param button the button that is pressed (left, middle, right, ...)
         * @param pos the position where the button is pressed
         */
        virtual void _MousePressEvent(ImGuiMouseButton_ button, ImVec2 pos);

        /**
         * @brief Called when the mouse is released
         *
         * @param button the mouse button that is released (left, middle,
         * right, ...)
         * @param pos the position where the button is released
         */
        virtual void _MouseReleaseEvent(ImGuiMouseButton_ button, ImVec2 pos);

        /**
         * @brief Called when the mouse move
         *
         * @param prevPos the previus position of the mouse
         * @param curPos the current position of the mouse
         */
        virtual void _MouseMoveEvent(ImVec2 prevPos, ImVec2 curPos);

        /**
         * @brief Get the ImGUi Window Flags that will be set to the current
         * view
         *
         * @return The ImGui Window flags
         */
        virtual ImGuiWindowFlags _GetGizmoWindowFlags();

    public:


        /**
         * @brief Destroy the HydraViewport object
         *
         */
        ~HydraViewport();

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

    lab::Engine* _engine;
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
};

} // namespace lab

#endif // ACTIVITIES_OPENUSD_HYDRAVIEWPORT_HPP
