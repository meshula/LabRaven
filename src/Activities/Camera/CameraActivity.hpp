//
//  CameraActivity.hpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 11/30/23.
//  Copyright © 2023 Nick Porcino. All rights reserved.
//

#ifndef CameraActivity_hpp
#define CameraActivity_hpp

#include "Lab/StudioCore.hpp"
#include "Providers/Camera/LabCamera.h"
#include <pxr/base/gf/camera.h>
#include <pxr/base/gf/frustum.h>
#include <pxr/base/gf/matrix4f.h>

namespace lab {
class AppModel;
class CameraActivity : public Activity
{
    struct data;
    data* _self;

    // activities
    void RunUI(const LabViewInteraction&);
    int  ViewportDragBid(const LabViewInteraction&);
    void ViewportDragging(const LabViewInteraction&);
    void ToolBar();
    void Menu();
    void Render(const LabViewInteraction&);

public:
    explicit CameraActivity();
    virtual ~CameraActivity();
    
    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "Camera"; }

    //--------------------------------------------------------------------------
    PXR_NS::GfMatrix4f GetViewMatrix() const;
    PXR_NS::GfFrustum GetFrustum(const LabViewDimensions&, float znear, float zfar) const;
    void SetViewMatrix(const PXR_NS::GfMatrix4f& m);
    void SetViewMatrix(const PXR_NS::GfMatrix4d& m);
    void SetFrustum(const PXR_NS::GfFrustum);
    void SetPosition(const PXR_NS::GfVec3d pos);
    PXR_NS::GfVec3d GetPosition() const;
    lc_camera& GetCamera() const;
    PXR_NS::GfCamera GetGfCamera() const;
    void FrameSelection();
    void LookAtSelection();
    void LookAt(const PXR_NS::GfVec3d&);
    float ComputeWorldSize(const PXR_NS::GfVec3d& worldPos,
                           float size,
                           const LabViewDimensions&);
    PXR_NS::GfVec3d HitPoint() const;
};

} // lab

#endif /* CameraActivity_hpp */
