#ifndef Provider_OpenUSDProvider_hpp
#define Provider_OpenUSDProvider_hpp

#include <memory>
#include <string>
#include <pxr/base/gf/vec3d.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/stage.h>

namespace lab {

class OpenUSDProvider {
    struct Self;
    std::unique_ptr<Self> self;
public:
    OpenUSDProvider();
    ~OpenUSDProvider();

    static std::shared_ptr<OpenUSDProvider> instance();
    static void ReleaseInstance();

    void LoadStage(std::string const& filePath);
    void SaveStage();
    void ExportStage(std::string const& path);
    void ExportSessionLayer(std::string const& path);

    void SetEmptyStage();

    void CreateShotFromTemplate(const std::string& dst, 
                                const std::string& shotname);

    PXR_NS::UsdStageRefPtr Stage() const;
    PXR_NS::SdfLayerRefPtr GetSessionLayer();

    void ReferenceLayer(PXR_NS::UsdStageRefPtr stage,
                                    const std::string& layerFilePath,
                                    PXR_NS::SdfPath& atScope,
                                    bool setTransform, PXR_NS::GfVec3d pos, bool asInstance);

    void TestReferencing();

    std::string GetNextAvailableIndexedPath(std::string primPath);

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
    PXR_NS::SdfPath CreateTestData();
    PXR_NS::SdfPath CreatePrimShape(const std::string& shape, PXR_NS::GfVec3d pos, float radius);
    PXR_NS::SdfPath CreateParGeometry(const std::string& shape, PXR_NS::GfVec3d pos);
    PXR_NS::SdfPath CreateParHeightfield(PXR_NS::GfVec3d pos);


};

} // lab

#endif