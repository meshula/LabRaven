//
//  HydraActivity.hpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 1/7/24.
//  Copyright © 2024 Nick Porcino. All rights reserved.
//

#ifndef HydraActivity_hpp
#define HydraActivity_hpp

#include "Lab/StudioCore.hpp"
#include <pxr/base/gf/camera.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/usdImaging/usdImaging/stageSceneIndex.h>

namespace lab {
class HydraActivity : public Activity
{
    struct data;
    data* _self;

    // activities
    void RunUI(const LabViewInteraction&);
    void Menu();

public:
    HydraActivity();
    virtual ~HydraActivity();
    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "Hydra"; }

    void SetNearFar(float znear, float zfar);
    PXR_NS::GfVec2d GetNearFar() const;
    float GetAspect() const;
    PXR_NS::GfCamera GetGfCamera() const;
    void SetCameraFromGfCamera(const PXR_NS::GfCamera& gfCam);

    PXR_NS::SdfPathVector GetHdSelection();
    void SetHdSelection(const PXR_NS::SdfPathVector&);
    void RemoveSceneIndex(PXR_NS::HdSceneIndexBaseRefPtr);
    int GetHit(PXR_NS::GfVec3f& hitPoint, PXR_NS::GfVec3f& hitNormal);
    PXR_NS::HdSceneIndexBaseRefPtr GetFinalSceneIndex();
    void SetStage(PXR_NS::UsdStageRefPtr);
    void SetTime(PXR_NS::UsdTimeCode);
    PXR_NS::HdSceneIndexBaseRefPtr GetEditableSceneIndex();
    void SetEditableSceneIndex(PXR_NS::HdSceneIndexBaseRefPtr sceneIndex);
};
}

#endif /* HydraActivity_hpp */
