/* 
    HydraRender.hpp    
*/

#ifndef HYDRARENDER_HPP
#define HYDRARENDER_HPP

/**
 * @file HydraRender.hpp
 * @author Raphael Jouretz (rjouretz.com)
 * @brief Engine is the renderer that renders a stage according to a given
 * renderer plugin.
 *
 * @copyright Copyright (c) 2023
 *
 */



#include <pxr/base/tf/token.h>
#include <pxr/imaging/glf/drawTarget.h>
#include <pxr/imaging/hd/engine.h>
#include <pxr/imaging/hd/pluginRenderDelegateUniqueHandle.h>
#include <pxr/imaging/hd/renderDelegate.h>
#include <pxr/imaging/hd/sceneIndex.h>
#include <pxr/imaging/hdx/taskController.h>
#include <pxr/imaging/hgi/hgi.h>
#include <pxr/imaging/hgiInterop/hgiInterop.h>
#include <pxr/usd/usd/prim.h>

using HgiUniquePtr = std::unique_ptr<class PXR_NS::Hgi>;

namespace lab {


/**
 * @brief Engine is the renderer that renders a stage according to a given
 * renderer plugin.
 *
 */
class Engine {
public:
    /**
     * @brief Construct a new Engine object
     *
     * @param sceneIndex the Scene Index to render
     * @param plugin the renderer plugin that specifies the render
     */
    Engine(PXR_NS::HdSceneIndexBaseRefPtr sceneIndex, PXR_NS::TfToken plugin);
    
    /**
     * @brief Destroy the Engine object
     *
     */
    ~Engine();
    
    PXR_NS::Hgi* GetHgi() const;
    
    /**
     * @brief Get the list of available renderer plugins
     *
     * @return the available renderer plugins
     */
    static PXR_NS::TfTokenVector GetRendererPlugins();
    
    /**
     * @brief Get the default renderer plugin (usually Storm)
     *
     * @return the default renderer plugin
     */
    static PXR_NS::TfToken GetDefaultRendererPlugin();
    
    /**
     * @brief Get the name of a renderer plugin
     *
     * @return the name of a renderer plugin
     */
    std::string GetRendererPluginName(PXR_NS::TfToken plugin);
    
    /**
     * @brief Get the current renderer plugin
     *
     * @return the current renderer plugin
     */
    PXR_NS::TfToken GetCurrentRendererPlugin();
    
    /**
     * @brief Set the matrices of the current camera
     *
     * @param view the view matrix
     * @param proj the projection matrix
     */
    void SetCameraMatrices(PXR_NS::GfMatrix4d view, PXR_NS::GfMatrix4d proj);
    
    /**
     * @brief Set the current selection
     *
     * @param paths a vector of SDF Paths
     */
    void SetSelection(PXR_NS::SdfPathVector paths);
    
    /**
     * @brief Set the render size
     *
     * @param width the width of the render
     * @param height the height of the render
     */
    void SetRenderSize(int width, int height);
    
    /**
     * @brief Render the current state
     */
    void Render();
    
    /**
     * @brief Find the visible USD Prim at the given screen position
     *
     * @param screenPos the position of the screen
     *
     * @return the Sdf Path to the Prim visible at the given screen
     * position
     */
    struct IntersectionResult {
        PXR_NS::SdfPath path;
        PXR_NS::GfVec3f worldSpaceHitPoint;
        PXR_NS::GfVec3f worldSpaceHitNormal;
    };
    IntersectionResult FindIntersection(PXR_NS::GfVec2f screenPos);
    
    /**
     * @brief Get the data from the render buffer
     *
     * @return the data from the render buffer
     */
    void *GetRenderBufferData();
    
    PXR_NS::HdxTaskController* GetHdxTaskController() const;
    
    void RemoveSceneIndex(PXR_NS::HdSceneIndexBaseRefPtr);
    
    
private:
    PXR_NS::UsdStageWeakPtr _stage;
    
#ifdef USD_GLINTEROP
    PXR_NS::GlfDrawTargetRefPtr _drawTarget;
    PXR_NS::HgiInterop _interop;
#endif
    
    PXR_NS::GfMatrix4d _camView, _camProj;
    int _width, _height;
    
    PXR_NS::HgiUniquePtr _hgi;
    PXR_NS::HdDriver _hgiDriver;
    
    PXR_NS::HdEngine _engine;
    PXR_NS::HdPluginRenderDelegateUniqueHandle _renderDelegate;
    PXR_NS::HdRenderIndex *_renderIndex;
    PXR_NS::HdxTaskController *_taskController;
    PXR_NS::HdRprimCollection _collection;
    PXR_NS::HdSceneIndexBaseRefPtr _sceneIndex;
    PXR_NS::SdfPath _taskControllerId;
    
    PXR_NS::HdxSelectionTrackerSharedPtr _selTracker;
    
    PXR_NS::TfToken _curRendererPlugin;
    
    /**
     * @brief Get the render delegate from the given renderer plugin
     *
     * @param plugin the renderer plugin
     *
     * @return the renderer delegate fro, the given renderer plugin
     */
    static PXR_NS::HdPluginRenderDelegateUniqueHandle
    GetRenderDelegateFromPlugin(PXR_NS::TfToken plugin);
    
    /**
     * @brief Initialize the renderer
     */
    void Initialize();
    
    /**
     * @brief Prepare the default lighting
     */
    void PrepareDefaultLighting();
    
    /**
     * @brief Present the last render to the OpenGL context
     */
    void Present();
    
    /**
     * @brief Get the current frustum
     *
     * @return the current frustum
     */
    PXR_NS::GfFrustum GetFrustum();
};


} // namespace lab

#endif // HYDRARENDER_HPP
