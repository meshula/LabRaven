#ifndef Provider_OpenUSDProvider_hpp
#define Provider_OpenUSDProvider_hpp

#include "Lab/StudioCore.hpp"
#include <memory>
#include <string>
#include <vector>
#include <pxr/base/gf/vec3d.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/stage.h>

namespace lab {

class OpenUSDProvider : public Provider {
    struct Self;
    std::unique_ptr<Self> self;

public:
    OpenUSDProvider();
    ~OpenUSDProvider();

    virtual const std::string Name() const override { return sname(); }
    static constexpr const char* sname() { return "OpenUSD"; }

    static OpenUSDProvider* instance();
    static void ReleaseInstance();

    int StageGeneration() const;
    void SetEmptyStage();
    void LoadStage(std::string const& filePath);
    void SaveStage();
    void ExportStage(std::string const& path);
    void ExportSessionLayer(std::string const& path);

    const std::vector<pxr::UsdPrim>& GetCameras();

    void CreateShotFromTemplate(const std::string& dst, 
                                const std::string& shotname);

    void CreateCard(const std::string& scope,
                    const std::string& filePath);

    PXR_NS::UsdStageRefPtr Stage() const;
    void SetStage(PXR_NS::UsdStageRefPtr stage);
    PXR_NS::SdfLayerRefPtr GetSessionLayer();

    void ReferenceLayer(PXR_NS::UsdStageRefPtr stage,
                                    const std::string& layerFilePath,
                                    PXR_NS::SdfPath& atScope,
                                    bool setTransform, PXR_NS::GfVec3d pos, bool asInstance);

    void TestReferencing();

    std::string GetNextAvailableIndexedPath(std::string const& primPath);

    void CreateDefaultPrimIfNeeded(std::string const& primPath);

    PXR_NS::SdfPath CreateCamera(const std::string& name);
    PXR_NS::SdfPath CreateCapsule(PXR_NS::GfVec3d pos);
    PXR_NS::SdfPath CreateCone(PXR_NS::GfVec3d pos);
    PXR_NS::SdfPath CreateCube(PXR_NS::GfVec3d pos);
    PXR_NS::SdfPath CreateCylinder(PXR_NS::GfVec3d pos);
    PXR_NS::SdfPath CreateMaterial();
    PXR_NS::SdfPath CreateGroundGrid(PXR_NS::GfVec3d pos, int x, int y, float spacing);
    PXR_NS::SdfPath CreatePlane(PXR_NS::GfVec3d pos);
    PXR_NS::SdfPath CreateSphere(PXR_NS::GfVec3d pos, float radius);
    PXR_NS::SdfPath CreateHilbertCurve(int iterations, PXR_NS::GfVec3d pos);
    PXR_NS::SdfPath CreateMacbethChart(const std::string& csName, PXR_NS::GfVec3d pos);
    PXR_NS::SdfPath CreateDemoText(const std::string& text, PXR_NS::GfVec3d pos);
    PXR_NS::SdfPath CreatePrimShape(const std::string& shape, PXR_NS::GfVec3d pos, float radius);
    PXR_NS::SdfPath CreateParGeometry(const std::string& shape, PXR_NS::GfVec3d pos);
    PXR_NS::SdfPath CreateParHeightfield(PXR_NS::GfVec3d pos);
    PXR_NS::SdfPath CreateTestData();
};

} // lab

#endif
