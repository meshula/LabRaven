
#include "UsdSessionLayer.hpp"
#include "ImGuiHydraEditor/src/models/model.h"
#include <pxr/imaging/hd/tokens.h>
#include <pxr/usd/usdGeom/camera.h>
#include <pxr/usd/usdGeom/capsule.h>
#include <pxr/usd/usdGeom/cone.h>
#include <pxr/usd/usdGeom/cube.h>
#include <pxr/usd/usdGeom/cylinder.h>
#include <pxr/usd/usdGeom/metrics.h>
#include <pxr/usd/usdGeom/plane.h>
#include <pxr/usd/usdGeom/sphere.h>
#include <pxr/usdImaging/usdImaging/stageSceneIndex.h>
#include <pxr/usdImaging/usdImaging/sceneIndices.h>
#include <fstream>

namespace lab {

using namespace std;
PXR_NAMESPACE_USING_DIRECTIVE

struct UsdSessionLayer::Self {
    Model* _model;
    pxr::SdfLayerRefPtr _rootLayer, _sessionLayer;
    pxr::UsdImagingStageSceneIndexRefPtr _stageSceneIndex;
    pxr::UsdStageRefPtr _stage;
    std::string _lastLoadedText;

    Self(Model* model);
    Self() = delete;


    /**
     * @brief Load a Usd Stage based on the given Usd file path
     *
     * @param usdFilePath a string containing a Usd file path
     */
    void _LoadUsdStage(const string usdFilePath);

    /**
     * @brief Set the model to an empty stage
     *
     */
    void _SetEmptyStage();

    /**
     * @brief Check if USD session layer was updated since the last load
     * (different)
     *
     * @return true if USD session layer is different from the last loaded
     * data
     * @return false otherwise
     */
    bool _IsUsdSessionLayerUpdated();

    /**
     * @brief Load text from the USD session layer of the Model
     *
     */
    void _LoadSessionTextFromModel();

    /**
     * @brief Save the text from the session layer view to the USD session
     * layer of the Model
     *
     */
    void _SaveSessionTextToModel();

    /**
     * @brief Convert the given prim path by an indexed prim path if the
     * given prim path is already used in the Model. Indexing consist of
     * adding a number at the end of the path.
     *
     * @param primPath the given prim path to index if already exists in
     * Model
     * @return string the next index prim path
     */
    string _GetNextAvailableIndexedPath(string primPath);

    /**
     * @brief Create a new prim to the current state
     *
     * @param primType the type of the prim to create
     */
    void _CreatePrim(pxr::TfToken primType);
};


UsdSessionLayer::Self::Self(Model* model)
: _model(model)
{
    UsdImagingCreateSceneIndicesInfo info;
    info.displayUnloadedPrimsWithBounds = false;
    const UsdImagingSceneIndices sceneIndices =
        UsdImagingCreateSceneIndices(info);

    _stageSceneIndex = sceneIndices.stageSceneIndex;
    _model->AddSceneIndexBase(sceneIndices.finalSceneIndex);

    //_SetEmptyStage(); /// @TODO was this in the original code? why wipe out the stage?
}

UsdSessionLayer::~UsdSessionLayer() {}

PXR_NS::SdfLayerRefPtr UsdSessionLayer::GetSessionLayer() {
    return self->_sessionLayer;
}

void UsdSessionLayer::Self::_SetEmptyStage()
{
    _stage = UsdStage::CreateInMemory();
    UsdGeomSetStageUpAxis(_stage, UsdGeomTokens->y);

    _rootLayer = _stage->GetRootLayer();
    _sessionLayer = _stage->GetSessionLayer();

    _stage->SetEditTarget(_sessionLayer);
    _stageSceneIndex->SetStage(_stage);
    _stageSceneIndex->SetTime(UsdTimeCode::Default());

    _model->SetStage(_stage);
}

void UsdSessionLayer::Self::_LoadUsdStage(const string usdFilePath)
{
    if (!ifstream(usdFilePath)) {
        TF_RUNTIME_ERROR(
            "Error: the file does not exist. Empty stage loaded.");
        _SetEmptyStage();
        return;
    }

    _rootLayer = SdfLayer::FindOrOpen(usdFilePath);
    _sessionLayer = SdfLayer::CreateAnonymous();
    _stage = UsdStage::Open(_rootLayer, _sessionLayer);
    _stage->SetEditTarget(_sessionLayer);
    _stageSceneIndex->SetStage(_stage);
    _stageSceneIndex->SetTime(UsdTimeCode::Default());

    _model->SetStage(_stage);
}

string UsdSessionLayer::Self::_GetNextAvailableIndexedPath(string primPath)
{
    UsdPrim prim;
    int i = -1;
    string newPath;
    do {
        i++;
        if (i == 0) newPath = primPath;
        else newPath = primPath + to_string(i);
        prim = _stage->GetPrimAtPath(SdfPath(newPath));
    } while (prim.IsValid());
    return newPath;
}

void UsdSessionLayer::Self::_CreatePrim(TfToken primType)
{
    string primPath = _GetNextAvailableIndexedPath("/" + primType.GetString());

    if (primType == HdPrimTypeTokens->camera) {
        auto cam = UsdGeomCamera::Define(_stage, SdfPath(primPath));
        cam.CreateFocalLengthAttr(VtValue(18.46f));
    }
    else {
        UsdGeomGprim prim;
        if (primType == HdPrimTypeTokens->capsule)
            prim = UsdGeomCapsule::Define(_stage, SdfPath(primPath));
        if (primType == HdPrimTypeTokens->cone)
            prim = UsdGeomCone::Define(_stage, SdfPath(primPath));
        if (primType == HdPrimTypeTokens->cube)
            prim = UsdGeomCube::Define(_stage, SdfPath(primPath));
        if (primType == HdPrimTypeTokens->cylinder)
            prim = UsdGeomCylinder::Define(_stage, SdfPath(primPath));
        if (primType == HdPrimTypeTokens->sphere)
            prim = UsdGeomSphere::Define(_stage, SdfPath(primPath));

        VtVec3fArray extent({{-1, -1, -1}, {1, 1, 1}});
        prim.CreateExtentAttr(VtValue(extent));

        VtVec3fArray color({{.5f, .5f, .5f}});
        prim.CreateDisplayColorAttr(VtValue(color));
    }

    _stageSceneIndex->ApplyPendingUpdates();
}

bool UsdSessionLayer::Self::_IsUsdSessionLayerUpdated()
{
    string layerText;
    _sessionLayer->ExportToString(&layerText);
    return _lastLoadedText != layerText;
}

void UsdSessionLayer::Self::_LoadSessionTextFromModel()
{
    string layerText;
    _sessionLayer->ExportToString(&layerText);
    _lastLoadedText = layerText;
}

void UsdSessionLayer::Self::_SaveSessionTextToModel()
{
    _sessionLayer->ImportFromString(_lastLoadedText.c_str());
}

UsdSessionLayer::UsdSessionLayer(Model* model) : self(new Self(model)) {
}


} // lab
