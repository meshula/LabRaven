
#include "OpenUSDProvider.hpp"
#include "UsdSessionLayer.hpp"
#include "UsdUtils.hpp"
#include "SpaceFillCurve.hpp"

#include "Lab/App.h"
#include "Lab/LabDirectories.h"

#define PAR_SHAPES_IMPLEMENTATION
#include "par_shapes.h"
#include "par_heman/include/heman.h"

#include "Activities/Console/ConsoleActivity.hpp"
#include "Providers/Color/nanocolor.h"
#include "Providers/Color/nanocolorUtils.h"

#include "ImGuiHydraEditor/src/models/model.h"
#include "ImGuiHydraEditor/src/engine.h"
#include "ImGuiHydraEditor/src/sceneindices/xformfiltersceneindex.h"
#include "ImGuiHydraEditor/src/sceneindices/gridsceneindex.h"

#include <pxr/base/plug/registry.h>
#include <pxr/base/tf/fileUtils.h>
#include <pxr/base/tf/pathUtils.h>
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usdGeom/basisCurves.h>
#include <pxr/usd/usdGeom/camera.h>
#include <pxr/usd/usdGeom/capsule.h>
#include <pxr/usd/usdGeom/cone.h>
#include <pxr/usd/usdGeom/cube.h>
#include <pxr/usd/usdGeom/cylinder.h>
#include <pxr/usd/usdGeom/metrics.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/metrics.h>
#include <pxr/usd/usdGeom/plane.h>
#include <pxr/usd/usdGeom/scope.h>
#include <pxr/usd/usdGeom/sphere.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdGeom/xformable.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>
#include <pxr/usd/usdUtils/usdzPackage.h>


#include <iostream>

namespace lab {

PXR_NAMESPACE_USING_DIRECTIVE

struct OpenUSDProvider::Self {
    // things related to the stage and hydra
    std::unique_ptr<pxr::Model> model;
    std::unique_ptr<UsdSessionLayer> layer;
    std::unique_ptr<Engine> engine;
    pxr::XformFilterSceneIndexRefPtr xformSceneIndex;
    pxr::GridSceneIndexRefPtr gridSceneIndex;

    // things related to the templating utility
    pxr::UsdStageRefPtr templateStage;

    // list of all the schema types, and a string buffer for dear ImGui
    std::set<TfType> schemaTypes;
    std::map<std::string, TfType> primTypes;
    char* primTypesMenuData = nullptr;

    Self() {
        const TfType baseType = TfType::Find<UsdTyped>();
        PlugRegistry::GetAllDerivedTypes(baseType, &schemaTypes);
        TF_FOR_ALL (type, schemaTypes) {
            primTypes[UsdSchemaRegistry::GetInstance().GetSchemaTypeName(*type)] = *type;
        }
        
        if (primTypesMenuData)
            free(primTypesMenuData);
        size_t sz = 0;
        for (auto& i : primTypes) {
            sz += i.first.length() + 1;
        }
        if (sz > 0) {
            // concatenate all the names into a buffer the way
            // dear ImGui wants it.
            primTypesMenuData = (char*) calloc(sz, 1);
            char* curr = primTypesMenuData;
            for (auto& i : primTypes) {
                strcpy(curr, i.first.c_str());
                curr += i.first.length() + 1;
            }
        }
    }

    ~Self() {
        SetEmptyStage();
    }

    void SetEmptyStage() {
        engine.reset();
        layer.reset();
        model.reset();
    }
};

OpenUSDProvider::OpenUSDProvider() : Provider(OpenUSDProvider::sname())
, self(new Self)
{
    provider.Documentation = [](void* instance) -> const char* {
        return "Provides a Stage, a Session Layer, and a Hydra2 Engine";
    };
}

OpenUSDProvider::~OpenUSDProvider()
{
}

void OpenUSDProvider::SetEmptyStage() {
    self->SetEmptyStage();
}

UsdStageRefPtr OpenUSDProvider::Stage() const {
    if (self->model)
        return self->model->GetStage();
    return {};
}


namespace {
    std::shared_ptr<OpenUSDProvider> gInstance;
}
// static
std::shared_ptr<OpenUSDProvider> OpenUSDProvider::instance()
{
    if (!gInstance)
        gInstance.reset(new OpenUSDProvider());
    return gInstance;
}

// static
void OpenUSDProvider::ReleaseInstance()
{
    gInstance.reset();
}


pxr::SdfPath OpenUSDProvider::CreateCamera(const std::string& name) {
    pxr::UsdStageRefPtr stage = Stage();
    if (!stage)
        return {};

    pxr::SdfPath primPath = stage->GetDefaultPrim().GetPath();
    primPath = primPath.AppendChild(TfToken("Cameras"));
    primPath = primPath.AppendChild(TfToken(name));
    string primPathStr = GetNextAvailableIndexedPath(primPath.GetString());
    pxr::SdfPath r(primPathStr);
    pxr::UsdGeomCamera::Define(stage, r);

    auto mm = lab::Orchestrator::Canonical();
    std::weak_ptr<ConsoleActivity> cap;
    auto console = mm->LockActivity(cap);
    std::string msg = "Created Camera: " + r.GetString();
    console->Info(msg);
    return r;
}


pxr::SdfPath OpenUSDProvider::CreateCapsule(PXR_NS::GfVec3d pos)
{
    auto mm = lab::Orchestrator::Canonical();
    pxr::UsdStageRefPtr stage = Stage();
    if (!stage)
        return {};

    pxr::SdfPath primPath = stage->GetDefaultPrim().GetPath();
    primPath = primPath.AppendChild(TfToken("Shapes"));
    primPath = primPath.AppendChild(TfToken("capsule"));
    string primPathStr = GetNextAvailableIndexedPath(primPath.GetString());
    pxr::SdfPath r(primPathStr);
    auto prim = pxr::UsdGeomCapsule::Define(stage, r);

    GfMatrix4d m;
    m.SetTranslate(pos);
    SetTransformMatrix(prim, m, UsdTimeCode::Default());

    std::weak_ptr<ConsoleActivity> cap;
    auto console = mm->LockActivity(cap);
    std::string msg = "Created Capsule: " + r.GetString();
    console->Info(msg);
    return r;
}

pxr::SdfPath OpenUSDProvider::CreateCone(PXR_NS::GfVec3d pos)
{
    auto mm = lab::Orchestrator::Canonical();
    pxr::UsdStageRefPtr stage = Stage();
    if (!stage)
        return {};

    pxr::SdfPath primPath = stage->GetDefaultPrim().GetPath();
    primPath = primPath.AppendChild(TfToken("Shapes"));
    primPath = primPath.AppendChild(TfToken("cone"));
    string primPathStr = GetNextAvailableIndexedPath(primPath.GetString());
    pxr::SdfPath r(primPathStr);
    auto prim = pxr::UsdGeomCone::Define(stage, r);

    GfMatrix4d m;
    m.SetTranslate(pos);
    SetTransformMatrix(prim, m, UsdTimeCode::Default());

    std::weak_ptr<ConsoleActivity> cap;
    auto console = mm->LockActivity(cap);
    std::string msg = "Created Cone: " + r.GetString();
    console->Info(msg);
    return r;
}

pxr::SdfPath OpenUSDProvider::CreateCube(PXR_NS::GfVec3d pos) {
    auto mm = lab::Orchestrator::Canonical();
    pxr::UsdStageRefPtr stage = Stage();
    if (!stage)
        return {};

    pxr::SdfPath primPath = stage->GetDefaultPrim().GetPath();
    primPath = primPath.AppendChild(TfToken("Shapes"));
    primPath = primPath.AppendChild(TfToken("cube"));
    string primPathStr = GetNextAvailableIndexedPath(primPath.GetString());
    pxr::SdfPath r(primPathStr);
    auto prim = pxr::UsdGeomCube::Define(stage, r);

    GfMatrix4d m;
    m.SetTranslate(pos);
    SetTransformMatrix(prim, m, UsdTimeCode::Default());

    std::weak_ptr<ConsoleActivity> cap;
    auto console = mm->LockActivity(cap);
    std::string msg = "Created Cube: " + r.GetString();
    console->Info(msg);
    return r;
}


/*  _   _ _ _ _               _
   | | | (_) | |__   ___ _ __| |_
   | |_| | | | '_ \ / _ \ '__| __|
   |  _  | | | |_) |  __/ |  | |_
   |_| |_|_|_|_.__/ \___|_|   \__| */

void generatePoints(int iterations, std::vector<GfVec3f>& points) {
    typedef uint32_t intcode_t;
    float nf = pow(2, iterations);
    float nrm = nf - 1.f;
    intcode_t n = (intcode_t) nf;
    intcode_t maxCode = n * n * n;
    for (intcode_t code = 0; code < maxCode; code++) {
        i3vec v = toSFCurveCoords<intcode_t>(code, CURVE_HILBERT);
        float xNorm = (float) v.x / nrm - 0.5f;
        float yNorm = (float) v.y / nrm - 0.5f;
        float zNorm = (float) v.z / nrm - 0.5f;
        points.push_back(GfVec3f(xNorm, yNorm, zNorm));
    }
}

PXR_NS::SdfPath OpenUSDProvider::CreateHilbertCurve(int iterations, PXR_NS::GfVec3d pos) {
    std::vector<GfVec3f> points;
    generatePoints(iterations, points);

    auto mm = lab::Orchestrator::Canonical();
    pxr::UsdStageRefPtr stage = Stage();
    if (!stage)
        return {};

    pxr::SdfPath primPath = stage->GetDefaultPrim().GetPath();
    primPath = primPath.AppendChild(TfToken("Shapes"));
    primPath = primPath.AppendChild(TfToken("hilbert_curve"));
    string primPathStr = GetNextAvailableIndexedPath(primPath.GetString());
    pxr::SdfPath r(primPathStr);
    auto prim = pxr::UsdGeomBasisCurves::Define(stage, r);

    // Create points attribute
    pxr::UsdAttribute pointsAttr = prim.GetPointsAttr();
    pxr::VtArray<pxr::GfVec3f> pointsArray;
    for (int i = 0; i < points.size(); i++) {
        pointsArray.push_back(points[i]);
    }
    pointsAttr.Set(pointsArray);

    prim.GetTypeAttr().Set(TfToken("linear"));
    //prim.GetBasisAttr().Set(TfToken("linear"));
    prim.CreateCurveVertexCountsAttr();
    pxr::VtArray<int> counts;
    counts.push_back((int) points.size());
    prim.GetCurveVertexCountsAttr().Set(VtValue(counts));

    // set the widths as follows
    // float[] widths = [.5] (interpolation = "constant")
    pxr::VtArray<float> widths;
    widths.push_back(0.5f);
    prim.GetWidthsAttr().Set(widths);
    prim.GetWidthsAttr().SetMetadata(pxr::UsdGeomTokens->interpolation, pxr::UsdGeomTokens->constant);

    GfMatrix4d m;
    m.SetTranslate(pos);
    SetTransformMatrix(prim, m, UsdTimeCode::Default());

    std::weak_ptr<ConsoleActivity> cap;
    auto console = mm->LockActivity(cap);
    std::string msg = "Created Hilbert Curve: " + r.GetString();
    console->Info(msg);
    return r;
}

pxr::SdfPath OpenUSDProvider::CreateMaterial() {
    auto mm = lab::Orchestrator::Canonical();
    pxr::UsdStageRefPtr stage = Stage();
    if (!stage)
        return {};

    pxr::SdfPath primPath = stage->GetDefaultPrim().GetPath();
    primPath = primPath.AppendChild(TfToken("Materials"));
    primPath = primPath.AppendChild(TfToken("material"));
    string primPathStr = GetNextAvailableIndexedPath(primPath.GetString());
    pxr::SdfPath r(primPathStr);
    pxr::UsdShadeMaterial::Define(stage, r);
    std::weak_ptr<ConsoleActivity> cap;
    auto console = mm->LockActivity(cap);
    std::string msg = "Created Material: " + r.GetString();
    console->Info(msg);
    return r;
}

pxr::SdfPath OpenUSDProvider::CreateTestData() {
    auto mm = lab::Orchestrator::Canonical();
    pxr::UsdStageRefPtr stage = Stage();
    if (!stage)
        return {};

    UsdGeomCube cubeRed = UsdGeomCube::Define(stage, SdfPath("/CubeRed"));

    auto material = pxr::UsdShadeMaterial::Define(stage, SdfPath("/Materials/testMaterial"));
    auto pbrShader = UsdShadeShader::Define(stage, SdfPath("/Materials/PBRShader"));
    pbrShader.CreateIdAttr(VtValue(TfToken("UsdPreviewSurface")));
    pbrShader.CreateInput(TfToken("roughness"), SdfValueTypeNames->Float).Set(0.4f);
    pbrShader.CreateInput(TfToken("metallic"), SdfValueTypeNames->Float).Set(0.0f);
    pbrShader.CreateInput(TfToken("emissiveColor"), SdfValueTypeNames->Color3f).Set(GfVec3f(0.0, 1.0, 0.0));
    material.CreateSurfaceOutput().ConnectToSource(pbrShader.ConnectableAPI(), TfToken("surface"));
    cubeRed.GetPrim().ApplyAPI<UsdShadeMaterialBindingAPI>();
    auto redBinding = UsdShadeMaterialBindingAPI(cubeRed);
    redBinding.Bind(material);
    return SdfPath("/CubeRed");
}

pxr::SdfPath OpenUSDProvider::CreateMacbethChart(const std::string& chipsColorspace,
                                                 PXR_NS::GfVec3d pos) {
    auto mm = lab::Orchestrator::Canonical();
    pxr::UsdStageRefPtr stage = Stage();
    if (!stage)
        return {};

    std::weak_ptr<ConsoleActivity> cap;
    auto console = mm->LockActivity(cap);
    std::string chartName = "Chart_" + chipsColorspace;

    pxr::SdfPath primPath = stage->GetDefaultPrim().GetPath();
    primPath = primPath.AppendChild(TfToken("Shapes"));
    primPath = primPath.AppendChild(TfToken(chartName));
    string primPathStr = GetNextAvailableIndexedPath(primPath.GetString());
    pxr::SdfPath r(primPathStr);
    primPath = r;
    auto prim = pxr::UsdGeomXform::Define(stage, r);

    GfMatrix4d m;
    m.SetTranslate(pos);
    SetTransformMatrix(prim, m, UsdTimeCode::Default());

    pxr::SdfPath materialPath = stage->GetDefaultPrim().GetPath();
    materialPath = materialPath.AppendChild(TfToken("Materials"));
    materialPath = materialPath.AppendChild(TfToken(chartName));
    string materialPathStr = GetNextAvailableIndexedPath(materialPath.GetString());
    pxr::SdfPath rm(materialPathStr);
    materialPath = rm;

//#define USEAP0_CHIPS
#ifdef USEAP0_CHIPS
    NcRGB* chips = NcISO17321ColorChipsAP0();
    const NcColorSpace* source_cs = NcGetNamedColorSpace("lin_ap0");
#else
    NcRGB* chips = NcCheckerColorChipsSRGB();
    const NcColorSpace* source_cs = NcGetNamedColorSpace("lin_rec709");
#endif

    const NcColorSpace* result_cs = NcGetNamedColorSpace(chipsColorspace.c_str());
    const char** chipNames = NcISO17321ColorChipsNames();

    for (int i = 0; i < 24; ++i) {
        std::string n = chipNames[i];
        if (n == "Black") {
            printf("black\n");        }
        std::replace(n.begin(), n.end(), ' ', '_');
        std::replace(n.begin(), n.end(), '.', '_');
        pxr::SdfPath chipPath = primPath.AppendChild(TfToken(n));
        pxr::UsdGeomGprim cube = pxr::UsdGeomCube::Define(stage, chipPath);
        pxr::UsdAttribute color = cube.GetDisplayColorAttr();
        color.SetColorSpace(TfToken(chipsColorspace));
        pxr::VtArray<GfVec3f> array;

        // display color
        NcRGB rgb = NcTransformColor(result_cs, source_cs, chips[i]);
        GfVec3f c(rgb.r, rgb.g, rgb.b);
        array.push_back(c);
        color.Set(array);

        // emissive standard surface
        pxr::SdfPath chipMaterialPath = materialPath.AppendChild(TfToken(n));
        UsdShadeMaterial material = pxr::UsdShadeMaterial::Define(stage, chipMaterialPath);
        pxr::SdfPath chipShaderPath = materialPath.AppendChild(TfToken(n + "Shader"));
        auto pbrShader = UsdShadeShader::Define(stage, chipShaderPath);
        pbrShader.CreateIdAttr(VtValue(TfToken("UsdPreviewSurface")));
        pbrShader.CreateInput(TfToken("diffuseColor"), SdfValueTypeNames->Color3f).Set(GfVec3f(0,0,0));
        pbrShader.CreateInput(TfToken("specularColor"), SdfValueTypeNames->Color3f).Set(GfVec3f(0,0,0));
        pbrShader.CreateInput(TfToken("emissiveColor"), SdfValueTypeNames->Color3f).Set(c);
        material.CreateSurfaceOutput().ConnectToSource(pbrShader.ConnectableAPI(), TfToken("surface"));
        cube.GetPrim().ApplyAPI<UsdShadeMaterialBindingAPI>();
        auto binding = UsdShadeMaterialBindingAPI(cube);
        binding.Bind(material);

        pxr::UsdGeomXformable xformable(cube);
        double x = (i % 6) * 2.0;
        double y = ((23-i) / 6) * 2.0;
        double z = 0;
        xformable.ClearXformOpOrder();

        // usd prim cubes are centered at the origin, so offset the cubes so the chart sits entirely in the positive octant
        xformable.AddTranslateOp().Set(GfVec3d(x + 1.0, y + 1.0, z + 1.0));
    }

    std::string msg = "Created Macbeth Chart: " + r.GetString();
    console->Info(msg);
    return r;
}

pxr::SdfPath OpenUSDProvider::CreateDemoText(const std::string& text, PXR_NS::GfVec3d pos) {
    if (!text.size())
        return {};

    const uint8_t sokol_font_c64[2048] = {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 00
        0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, // 01
        0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, // 02
        0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 03
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, // 04
        0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, // 05
        0xCC, 0xCC, 0x33, 0x33, 0xCC, 0xCC, 0x33, 0x33, // 06
        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, // 07
        0x00, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0x33, 0x33, // 08
        0xCC, 0x99, 0x33, 0x66, 0xCC, 0x99, 0x33, 0x66, // 09
        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, // 0A
        0x18, 0x18, 0x18, 0x1F, 0x1F, 0x18, 0x18, 0x18, // 0B
        0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x0F, 0x0F, // 0C
        0x18, 0x18, 0x18, 0x1F, 0x1F, 0x00, 0x00, 0x00, // 0D
        0x00, 0x00, 0x00, 0xF8, 0xF8, 0x18, 0x18, 0x18, // 0E
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, // 0F
        0x00, 0x00, 0x00, 0x1F, 0x1F, 0x18, 0x18, 0x18, // 10
        0x18, 0x18, 0x18, 0xFF, 0xFF, 0x00, 0x00, 0x00, // 11
        0x00, 0x00, 0x00, 0xFF, 0xFF, 0x18, 0x18, 0x18, // 12
        0x18, 0x18, 0x18, 0xF8, 0xF8, 0x18, 0x18, 0x18, // 13
        0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, // 14
        0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, // 15
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, // 16
        0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 17
        0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, // 18
        0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, // 19
        0x01, 0x03, 0x06, 0x6C, 0x78, 0x70, 0x60, 0x00, // 1A
        0x00, 0x00, 0x00, 0x00, 0xF0, 0xF0, 0xF0, 0xF0, // 1B
        0x0F, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x00, // 1C
        0x18, 0x18, 0x18, 0xF8, 0xF8, 0x00, 0x00, 0x00, // 1D
        0xF0, 0xF0, 0xF0, 0xF0, 0x00, 0x00, 0x00, 0x00, // 1E
        0xF0, 0xF0, 0xF0, 0xF0, 0x0F, 0x0F, 0x0F, 0x0F, // 1F
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // 20
        0x18, 0x18, 0x18, 0x18, 0x00, 0x00, 0x18, 0x00, // 21
        0x66, 0x66, 0x66, 0x00, 0x00, 0x00, 0x00, 0x00, // 22
        0x66, 0x66, 0xFF, 0x66, 0xFF, 0x66, 0x66, 0x00, // 23
        0x18, 0x3E, 0x60, 0x3C, 0x06, 0x7C, 0x18, 0x00, // 24
        0x62, 0x66, 0x0C, 0x18, 0x30, 0x66, 0x46, 0x00, // 25
        0x3C, 0x66, 0x3C, 0x38, 0x67, 0x66, 0x3F, 0x00, // 26
        0x06, 0x0C, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00, // 27
        0x0C, 0x18, 0x30, 0x30, 0x30, 0x18, 0x0C, 0x00, // 28
        0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x18, 0x30, 0x00, // 29
        0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00, // 2A
        0x00, 0x18, 0x18, 0x7E, 0x18, 0x18, 0x00, 0x00, // 2B
        0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x30, // 2C
        0x00, 0x00, 0x00, 0x7E, 0x00, 0x00, 0x00, 0x00, // 2D
        0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00, // 2E
        0x00, 0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x00, // 2F
        0x3C, 0x66, 0x6E, 0x76, 0x66, 0x66, 0x3C, 0x00, // 30
        0x18, 0x18, 0x38, 0x18, 0x18, 0x18, 0x7E, 0x00, // 31
        0x3C, 0x66, 0x06, 0x0C, 0x30, 0x60, 0x7E, 0x00, // 32
        0x3C, 0x66, 0x06, 0x1C, 0x06, 0x66, 0x3C, 0x00, // 33
        0x06, 0x0E, 0x1E, 0x66, 0x7F, 0x06, 0x06, 0x00, // 34
        0x7E, 0x60, 0x7C, 0x06, 0x06, 0x66, 0x3C, 0x00, // 35
        0x3C, 0x66, 0x60, 0x7C, 0x66, 0x66, 0x3C, 0x00, // 36
        0x7E, 0x66, 0x0C, 0x18, 0x18, 0x18, 0x18, 0x00, // 37
        0x3C, 0x66, 0x66, 0x3C, 0x66, 0x66, 0x3C, 0x00, // 38
        0x3C, 0x66, 0x66, 0x3E, 0x06, 0x66, 0x3C, 0x00, // 39
        0x00, 0x00, 0x18, 0x00, 0x00, 0x18, 0x00, 0x00, // 3A
        0x00, 0x00, 0x18, 0x00, 0x00, 0x18, 0x18, 0x30, // 3B
        0x0E, 0x18, 0x30, 0x60, 0x30, 0x18, 0x0E, 0x00, // 3C
        0x00, 0x00, 0x7E, 0x00, 0x7E, 0x00, 0x00, 0x00, // 3D
        0x70, 0x18, 0x0C, 0x06, 0x0C, 0x18, 0x70, 0x00, // 3E
        0x3C, 0x66, 0x06, 0x0C, 0x18, 0x00, 0x18, 0x00, // 3F
        0x3C, 0x66, 0x6E, 0x6E, 0x60, 0x62, 0x3C, 0x00, // 40
        0x18, 0x3C, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00, // 41
        0x7C, 0x66, 0x66, 0x7C, 0x66, 0x66, 0x7C, 0x00, // 42
        0x3C, 0x66, 0x60, 0x60, 0x60, 0x66, 0x3C, 0x00, // 43
        0x78, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0x78, 0x00, // 44
        0x7E, 0x60, 0x60, 0x78, 0x60, 0x60, 0x7E, 0x00, // 45
        0x7E, 0x60, 0x60, 0x78, 0x60, 0x60, 0x60, 0x00, // 46
        0x3C, 0x66, 0x60, 0x6E, 0x66, 0x66, 0x3C, 0x00, // 47
        0x66, 0x66, 0x66, 0x7E, 0x66, 0x66, 0x66, 0x00, // 48
        0x3C, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00, // 49
        0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x6C, 0x38, 0x00, // 4A
        0x66, 0x6C, 0x78, 0x70, 0x78, 0x6C, 0x66, 0x00, // 4B
        0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7E, 0x00, // 4C
        0x63, 0x77, 0x7F, 0x6B, 0x63, 0x63, 0x63, 0x00, // 4D
        0x66, 0x76, 0x7E, 0x7E, 0x6E, 0x66, 0x66, 0x00, // 4E
        0x3C, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00, // 4F
        0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60, 0x60, 0x00, // 50
        0x3C, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x0E, 0x00, // 51
        0x7C, 0x66, 0x66, 0x7C, 0x78, 0x6C, 0x66, 0x00, // 52
        0x3C, 0x66, 0x60, 0x3C, 0x06, 0x66, 0x3C, 0x00, // 53
        0x7E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00, // 54
        0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x00, // 55
        0x66, 0x66, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x00, // 56
        0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00, // 57
        0x66, 0x66, 0x3C, 0x18, 0x3C, 0x66, 0x66, 0x00, // 58
        0x66, 0x66, 0x66, 0x3C, 0x18, 0x18, 0x18, 0x00, // 59
        0x7E, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x7E, 0x00, // 5A
        0x3C, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3C, 0x00, // 5B
        0x0C, 0x12, 0x30, 0x7C, 0x30, 0x62, 0xFC, 0x00, // 5C
        0x3C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x3C, 0x00, // 5D
        0x00, 0x18, 0x3C, 0x7E, 0x18, 0x18, 0x18, 0x18, // 5E
        0x00, 0x10, 0x30, 0x7F, 0x7F, 0x30, 0x10, 0x00, // 5F
        0x3C, 0x66, 0x6E, 0x6E, 0x60, 0x62, 0x3C, 0x00, // 60
        0x00, 0x00, 0x3C, 0x06, 0x3E, 0x66, 0x3E, 0x00, // 61
        0x00, 0x60, 0x60, 0x7C, 0x66, 0x66, 0x7C, 0x00, // 62
        0x00, 0x00, 0x3C, 0x60, 0x60, 0x60, 0x3C, 0x00, // 63
        0x00, 0x06, 0x06, 0x3E, 0x66, 0x66, 0x3E, 0x00, // 64
        0x00, 0x00, 0x3C, 0x66, 0x7E, 0x60, 0x3C, 0x00, // 65
        0x00, 0x0E, 0x18, 0x3E, 0x18, 0x18, 0x18, 0x00, // 66
        0x00, 0x00, 0x3E, 0x66, 0x66, 0x3E, 0x06, 0x7C, // 67
        0x00, 0x60, 0x60, 0x7C, 0x66, 0x66, 0x66, 0x00, // 68
        0x00, 0x18, 0x00, 0x38, 0x18, 0x18, 0x3C, 0x00, // 69
        0x00, 0x06, 0x00, 0x06, 0x06, 0x06, 0x06, 0x3C, // 6A
        0x00, 0x60, 0x60, 0x6C, 0x78, 0x6C, 0x66, 0x00, // 6B
        0x00, 0x38, 0x18, 0x18, 0x18, 0x18, 0x3C, 0x00, // 6C
        0x00, 0x00, 0x66, 0x7F, 0x7F, 0x6B, 0x63, 0x00, // 6D
        0x00, 0x00, 0x7C, 0x66, 0x66, 0x66, 0x66, 0x00, // 6E
        0x00, 0x00, 0x3C, 0x66, 0x66, 0x66, 0x3C, 0x00, // 6F
        0x00, 0x00, 0x7C, 0x66, 0x66, 0x7C, 0x60, 0x60, // 70
        0x00, 0x00, 0x3E, 0x66, 0x66, 0x3E, 0x06, 0x06, // 71
        0x00, 0x00, 0x7C, 0x66, 0x60, 0x60, 0x60, 0x00, // 72
        0x00, 0x00, 0x3E, 0x60, 0x3C, 0x06, 0x7C, 0x00, // 73
        0x00, 0x18, 0x7E, 0x18, 0x18, 0x18, 0x0E, 0x00, // 74
        0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x3E, 0x00, // 75
        0x00, 0x00, 0x66, 0x66, 0x66, 0x3C, 0x18, 0x00, // 76
        0x00, 0x00, 0x63, 0x6B, 0x7F, 0x3E, 0x36, 0x00, // 77
        0x00, 0x00, 0x66, 0x3C, 0x18, 0x3C, 0x66, 0x00, // 78
        0x00, 0x00, 0x66, 0x66, 0x66, 0x3E, 0x0C, 0x78, // 79
        0x00, 0x00, 0x7E, 0x0C, 0x18, 0x30, 0x7E, 0x00, // 7A
        0x3C, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3C, 0x00, // 7B
        0x0C, 0x12, 0x30, 0x7C, 0x30, 0x62, 0xFC, 0x00, // 7C
        0x3C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x3C, 0x00, // 7D
        0x00, 0x18, 0x3C, 0x7E, 0x18, 0x18, 0x18, 0x18, // 7E
        0x00, 0x10, 0x30, 0x7F, 0x7F, 0x30, 0x10, 0x00, // 7F
        0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, // 80
        0x08, 0x1C, 0x3E, 0x7F, 0x7F, 0x1C, 0x3E, 0x00, // 81
        0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, // 82
        0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, // 83
        0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, // 84
        0x00, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, // 85
        0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, 0x00, // 86
        0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, // 87
        0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, // 88
        0x00, 0x00, 0x00, 0xE0, 0xF0, 0x38, 0x18, 0x18, // 89
        0x18, 0x18, 0x1C, 0x0F, 0x07, 0x00, 0x00, 0x00, // 8A
        0x18, 0x18, 0x38, 0xF0, 0xE0, 0x00, 0x00, 0x00, // 8B
        0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xFF, 0xFF, // 8C
        0xC0, 0xE0, 0x70, 0x38, 0x1C, 0x0E, 0x07, 0x03, // 8D
        0x03, 0x07, 0x0E, 0x1C, 0x38, 0x70, 0xE0, 0xC0, // 8E
        0xFF, 0xFF, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, // 8F
        0xFF, 0xFF, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, // 90
        0x00, 0x3C, 0x7E, 0x7E, 0x7E, 0x7E, 0x3C, 0x00, // 91
        0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0x00, // 92
        0x36, 0x7F, 0x7F, 0x7F, 0x3E, 0x1C, 0x08, 0x00, // 93
        0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x60, // 94
        0x00, 0x00, 0x00, 0x07, 0x0F, 0x1C, 0x18, 0x18, // 95
        0xC3, 0xE7, 0x7E, 0x3C, 0x3C, 0x7E, 0xE7, 0xC3, // 96
        0x00, 0x3C, 0x7E, 0x66, 0x66, 0x7E, 0x3C, 0x00, // 97
        0x18, 0x18, 0x66, 0x66, 0x18, 0x18, 0x3C, 0x00, // 98
        0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, 0x06, // 99
        0x08, 0x1C, 0x3E, 0x7F, 0x3E, 0x1C, 0x08, 0x00, // 9A
        0x18, 0x18, 0x18, 0xFF, 0xFF, 0x18, 0x18, 0x18, // 9B
        0xC0, 0xC0, 0x30, 0x30, 0xC0, 0xC0, 0x30, 0x30, // 9C
        0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, // 9D
        0x00, 0x00, 0x03, 0x3E, 0x76, 0x36, 0x36, 0x00, // 9E
        0xFF, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01, // 9F
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // A0
        0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, // A1
        0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, // A2
        0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // A3
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, // A4
        0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, // A5
        0xCC, 0xCC, 0x33, 0x33, 0xCC, 0xCC, 0x33, 0x33, // A6
        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, // A7
        0x00, 0x00, 0x00, 0x00, 0xCC, 0xCC, 0x33, 0x33, // A8
        0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC0, 0x80, // A9
        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0x03, // AA
        0x18, 0x18, 0x18, 0x1F, 0x1F, 0x18, 0x18, 0x18, // AB
        0x00, 0x00, 0x00, 0x00, 0x0F, 0x0F, 0x0F, 0x0F, // AC
        0x18, 0x18, 0x18, 0x1F, 0x1F, 0x00, 0x00, 0x00, // AD
        0x00, 0x00, 0x00, 0xF8, 0xF8, 0x18, 0x18, 0x18, // AE
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, // AF
        0x00, 0x00, 0x00, 0x1F, 0x1F, 0x18, 0x18, 0x18, // B0
        0x18, 0x18, 0x18, 0xFF, 0xFF, 0x00, 0x00, 0x00, // B1
        0x00, 0x00, 0x00, 0xFF, 0xFF, 0x18, 0x18, 0x18, // B2
        0x18, 0x18, 0x18, 0xF8, 0xF8, 0x18, 0x18, 0x18, // B3
        0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, 0xC0, // B4
        0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, 0xE0, // B5
        0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, 0x07, // B6
        0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, // B7
        0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, // B8
        0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, // B9
        0x03, 0x03, 0x03, 0x03, 0x03, 0x03, 0xFF, 0xFF, // BA
        0x00, 0x00, 0x00, 0x00, 0xF0, 0xF0, 0xF0, 0xF0, // BB
        0x0F, 0x0F, 0x0F, 0x0F, 0x00, 0x00, 0x00, 0x00, // BC
        0x18, 0x18, 0x18, 0xF8, 0xF8, 0x00, 0x00, 0x00, // BD
        0xF0, 0xF0, 0xF0, 0xF0, 0x00, 0x00, 0x00, 0x00, // BE
        0xF0, 0xF0, 0xF0, 0xF0, 0x0F, 0x0F, 0x0F, 0x0F, // BF
        0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, // C0
        0xF7, 0xE3, 0xC1, 0x80, 0x80, 0xE3, 0xC1, 0xFF, // C1
        0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7, // C2
        0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, // C3
        0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, // C4
        0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // C5
        0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFF, 0xFF, // C6
        0xCF, 0xCF, 0xCF, 0xCF, 0xCF, 0xCF, 0xCF, 0xCF, // C7
        0xF3, 0xF3, 0xF3, 0xF3, 0xF3, 0xF3, 0xF3, 0xF3, // C8
        0xFF, 0xFF, 0xFF, 0x1F, 0x0F, 0xC7, 0xE7, 0xE7, // C9
        0xE7, 0xE7, 0xE3, 0xF0, 0xF8, 0xFF, 0xFF, 0xFF, // CA
        0xE7, 0xE7, 0xC7, 0x0F, 0x1F, 0xFF, 0xFF, 0xFF, // CB
        0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x00, 0x00, // CC
        0x3F, 0x1F, 0x8F, 0xC7, 0xE3, 0xF1, 0xF8, 0xFC, // CD
        0xFC, 0xF8, 0xF1, 0xE3, 0xC7, 0x8F, 0x1F, 0x3F, // CE
        0x00, 0x00, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, // CF
        0x00, 0x00, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, // D0
        0xFF, 0xC3, 0x81, 0x81, 0x81, 0x81, 0xC3, 0xFF, // D1
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xFF, // D2
        0xC9, 0x80, 0x80, 0x80, 0xC1, 0xE3, 0xF7, 0xFF, // D3
        0x9F, 0x9F, 0x9F, 0x9F, 0x9F, 0x9F, 0x9F, 0x9F, // D4
        0xFF, 0xFF, 0xFF, 0xF8, 0xF0, 0xE3, 0xE7, 0xE7, // D5
        0x3C, 0x18, 0x81, 0xC3, 0xC3, 0x81, 0x18, 0x3C, // D6
        0xFF, 0xC3, 0x81, 0x99, 0x99, 0x81, 0xC3, 0xFF, // D7
        0xE7, 0xE7, 0x99, 0x99, 0xE7, 0xE7, 0xC3, 0xFF, // D8
        0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, 0xF9, // D9
        0xF7, 0xE3, 0xC1, 0x80, 0xC1, 0xE3, 0xF7, 0xFF, // DA
        0xE7, 0xE7, 0xE7, 0x00, 0x00, 0xE7, 0xE7, 0xE7, // DB
        0x3F, 0x3F, 0xCF, 0xCF, 0x3F, 0x3F, 0xCF, 0xCF, // DC
        0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7, 0xE7, // DD
        0xFF, 0xFF, 0xFC, 0xC1, 0x89, 0xC9, 0xC9, 0xFF, // DE
        0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE, // DF
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // E0
        0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, 0x0F, // E1
        0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, 0x00, // E2
        0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // E3
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, // E4
        0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, // E5
        0x33, 0x33, 0xCC, 0xCC, 0x33, 0x33, 0xCC, 0xCC, // E6
        0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, // E7
        0xFF, 0xFF, 0xFF, 0xFF, 0x33, 0x33, 0xCC, 0xCC, // E8
        0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, // E9
        0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, // EA
        0xE7, 0xE7, 0xE7, 0xE0, 0xE0, 0xE7, 0xE7, 0xE7, // EB
        0xFF, 0xFF, 0xFF, 0xFF, 0xF0, 0xF0, 0xF0, 0xF0, // EC
        0xE7, 0xE7, 0xE7, 0xE0, 0xE0, 0xFF, 0xFF, 0xFF, // ED
        0xFF, 0xFF, 0xFF, 0x07, 0x07, 0xE7, 0xE7, 0xE7, // EE
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, // EF
        0xFF, 0xFF, 0xFF, 0xE0, 0xE0, 0xE7, 0xE7, 0xE7, // F0
        0xE7, 0xE7, 0xE7, 0x00, 0x00, 0xFF, 0xFF, 0xFF, // F1
        0xFF, 0xFF, 0xFF, 0x00, 0x00, 0xE7, 0xE7, 0xE7, // F2
        0xE7, 0xE7, 0xE7, 0x07, 0x07, 0xE7, 0xE7, 0xE7, // F3
        0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, 0x3F, // F4
        0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, 0x1F, // F5
        0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, 0xF8, // F6
        0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // F7
        0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, // F8
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00, 0x00, // F9
        0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0xFC, 0x00, 0x00, // FA
        0xFF, 0xFF, 0xFF, 0xFF, 0x0F, 0x0F, 0x0F, 0x0F, // FB
        0xF0, 0xF0, 0xF0, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, // FC
        0xE7, 0xE7, 0xE7, 0x07, 0x07, 0xFF, 0xFF, 0xFF, // FD
        0x0F, 0x0F, 0x0F, 0x0F, 0xFF, 0xFF, 0xFF, 0xFF, // FE
        0x0F, 0x0F, 0x0F, 0x0F, 0xF0, 0xF0, 0xF0, 0xF0, // FF
    };

    /* unpack linear 8x8 bits-per-pixel font data into 2D byte-per-pixel texture
     * data. The font will be unpacked as a 2048x8 texture.
     */

    static uint8_t out_pixels[2048*8];
    static std::once_flag init_font;
    std::call_once(init_font, [sokol_font_c64]() {
        int first_char = 0, last_char = 0xff;
        const uint8_t* ptr = sokol_font_c64;
        for (int chr = first_char; chr <= last_char; chr++) {
            for (int line = 0; line < 8; line++) {
                uint8_t bits = *ptr++;
                for (int x = 0; x < 8; x++) {
                    out_pixels[line*256*8 + chr*8 + x] = ((bits>>(7-x)) & 1) ? 0xFF : 0x00;
                }
            }
        }
    });

    auto mm = lab::Orchestrator::Canonical();
    pxr::UsdStageRefPtr stage = Stage();
    if (!stage)
        return {};

    std::string chartName = "C64Text";

    pxr::SdfPath primPath = stage->GetDefaultPrim().GetPath();
    primPath = primPath.AppendChild(TfToken("Shapes"));
    primPath = primPath.AppendChild(TfToken(chartName));
    string primPathStr = GetNextAvailableIndexedPath(primPath.GetString());
    pxr::SdfPath r(primPathStr);
    primPath = r;
    auto prim = pxr::UsdGeomXform::Define(stage, r);
    GfMatrix4d m;
    m.SetTranslate(pos);
    SetTransformMatrix(prim, m, UsdTimeCode::Default());

    float cursor_x = 0;
    float pixelSz = 1.f/8.f;
    int pixel = 0;
    int len =  (int) text.size();
    for (int i = 0; i <len; ++i, cursor_x += pixelSz * 8.f) {
        uint8_t* char_start = &out_pixels[text[i] * 8];
        for (int y = 0; y < 8; ++y) {
            uint8_t* row_start = char_start + y * 256 * 8;
            for (int x = 0; x < 8; ++x, ++pixel) {
                uint8_t* cp = row_start + x;
                if (!*cp)
                    continue; // empty pixel

                std::string n = "px" + std::to_string(pixel);
                pxr::SdfPath chipPath = primPath.AppendChild(TfToken(n));
                pxr::UsdGeomGprim cube = pxr::UsdGeomCube::Define(stage, chipPath);
                pxr::UsdAttribute color = cube.GetDisplayColorAttr();
                pxr::VtArray<GfVec3f> array;
                NcRGB rgb = {0,0.5f, 0.5f};
                array.push_back({ rgb.r, rgb.g, rgb.b });
                color.Set(array);
                pxr::UsdGeomXformable xformable(cube);
                double pixel_x = cursor_x + pixelSz * (double) x;
                double pixel_y = pixelSz * (double) (8-y);
                double z = 0;
                xformable.ClearXformOpOrder();
                xformable.AddTranslateOp().Set(GfVec3d(pixel_x*2, pixel_y*2, z));
                xformable.AddScaleOp().Set(GfVec3f(pixelSz,pixelSz,pixelSz));
            }
        }
    }

    std::weak_ptr<ConsoleActivity> cap;
    auto console = mm->LockActivity(cap);
    std::string msg = "Created Pixel Text: " + r.GetString();
    console->Info(msg);
    return r;
}

pxr::SdfPath OpenUSDProvider::CreateCylinder(PXR_NS::GfVec3d pos)
{
    auto mm = lab::Orchestrator::Canonical();
    pxr::UsdStageRefPtr stage = Stage();
    if (!stage)
        return {};

    pxr::SdfPath primPath = stage->GetDefaultPrim().GetPath();
    primPath = primPath.AppendChild(TfToken("Shapes"));
    primPath = primPath.AppendChild(TfToken("cylinder"));
    string primPathStr = GetNextAvailableIndexedPath(primPath.GetString());
    pxr::SdfPath r(primPathStr);
    auto prim = pxr::UsdGeomCylinder::Define(stage, r);

    GfMatrix4d m;
    m.SetTranslate(pos);
    SetTransformMatrix(prim, m, UsdTimeCode::Default());

    std::weak_ptr<ConsoleActivity> cap;
    auto console = mm->LockActivity(cap);
    std::string msg = "Created Cylinder: " + r.GetString();
    console->Info(msg);
    return r;
}

PXR_NS::SdfPath OpenUSDProvider::CreateGroundGrid(PXR_NS::GfVec3d pos, int x, int y, float spacing) {
    // Create a grid of cube prims; coloring each with 85% gray or 15% gray
    // material based on the parity of the x and y indices. The cubes will be
    // grouped under a common UsdGeomXformable, which will be tagged as the
    // "GroundGrid" group.
    auto mm = lab::Orchestrator::Canonical();
    pxr::UsdStageRefPtr stage = Stage();
    if (!stage)
        return {};

    pxr::SdfPath primPath = stage->GetDefaultPrim().GetPath();
    primPath = primPath.AppendChild(TfToken("Shapes"));
    primPath = primPath.AppendChild(TfToken("plane"));
    string primPathStr = GetNextAvailableIndexedPath(primPath.GetString());
    pxr::SdfPath r(primPathStr);

    auto prim = pxr::UsdGeomXform::Define(stage, r);
    GfMatrix4d m;
    m.SetTranslate(pos);
    SetTransformMatrix(prim, m, UsdTimeCode::Default());

    // Create a 50% gray material for the cubes
    pxr::SdfPath chipMaterialPath = r.AppendChild(TfToken("Gray1"));
    auto material = pxr::UsdShadeMaterial::Define(stage, chipMaterialPath);
    pxr::SdfPath chipShaderPath = r.AppendChild(TfToken("Gray1Shader"));
    auto pbrShader = UsdShadeShader::Define(stage, chipShaderPath);
    pbrShader.CreateIdAttr(VtValue(TfToken("UsdPreviewSurface")));
    pbrShader.CreateInput(TfToken("diffuseColor"), SdfValueTypeNames->Color3f).Set(GfVec3f(0.18f,0.18f,0.18f));
    pbrShader.CreateInput(TfToken("specularColor"), SdfValueTypeNames->Color3f).Set(GfVec3f(0,0,0));
    //pbrShader.CreateInput(TfToken("emissiveColor"), SdfValueTypeNames->Color3f).Set(c);
    material.CreateSurfaceOutput().ConnectToSource(pbrShader.ConnectableAPI(), TfToken("surface"));

    // Create a 85% gray material for the cubes
    chipMaterialPath = r.AppendChild(TfToken("Gray2"));
    auto material2 = pxr::UsdShadeMaterial::Define(stage, chipMaterialPath);
    chipShaderPath = r.AppendChild(TfToken("Gray2Shader"));
    pbrShader = UsdShadeShader::Define(stage, chipShaderPath);
    pbrShader.CreateIdAttr(VtValue(TfToken("UsdPreviewSurface")));
    pbrShader.CreateInput(TfToken("diffuseColor"), SdfValueTypeNames->Color3f).Set(GfVec3f(0.55f,0.55f,0.55f));
    pbrShader.CreateInput(TfToken("specularColor"), SdfValueTypeNames->Color3f).Set(GfVec3f(0,0,0));
    //pbrShader.CreateInput(TfToken("emissiveColor"), SdfValueTypeNames->Color3f).Set(c);
    material2.CreateSurfaceOutput().ConnectToSource(pbrShader.ConnectableAPI(), TfToken("surface"));

    // Create the cubes
    for (int i = 0; i < x; i++) {
        for (int j = 0; j < y; j++) {
            pxr::SdfPath cubePath = r.AppendChild(TfToken("Cube" + std::to_string(i) + "_" + std::to_string(j)));
            auto cube = pxr::UsdGeomCube::Define(stage, cubePath);
            pxr::UsdGeomXformable xformable(cube);
            cube.ClearXformOpOrder();
            cube.AddTranslateOp().Set(GfVec3d((i - x/2) * spacing, 0, (j - y/2) * spacing));
            cube.AddScaleOp().Set(GfVec3f(spacing * 0.5f, spacing * 0.05f, spacing * 0.5f));
            if ((i + j) % 2 == 0) {
                cube.GetPrim().ApplyAPI<UsdShadeMaterialBindingAPI>();
                auto binding = UsdShadeMaterialBindingAPI(cube);
                binding.Bind(material);
            } else {
                cube.GetPrim().ApplyAPI<UsdShadeMaterialBindingAPI>();
                auto binding = UsdShadeMaterialBindingAPI(cube);
                binding.Bind(material2);
            }
        }
    }

    std::weak_ptr<ConsoleActivity> cap;
    auto console = mm->LockActivity(cap);
    std::string msg = "Created Ground Grid: " + r.GetString();
    console->Info(msg);
    return r;
}

pxr::SdfPath OpenUSDProvider::CreatePlane(PXR_NS::GfVec3d pos)
{
    auto mm = lab::Orchestrator::Canonical();
    pxr::UsdStageRefPtr stage = Stage();
    if (!stage)
        return {};

    pxr::SdfPath primPath = stage->GetDefaultPrim().GetPath();
    primPath = primPath.AppendChild(TfToken("Shapes"));
    primPath = primPath.AppendChild(TfToken("plane"));
    string primPathStr = GetNextAvailableIndexedPath(primPath.GetString());
    pxr::SdfPath r(primPathStr);
    auto prim = pxr::UsdGeomPlane::Define(stage, r);

    GfMatrix4d m;
    m.SetTranslate(pos);
    SetTransformMatrix(prim, m, UsdTimeCode::Default());

    std::weak_ptr<ConsoleActivity> cap;
    auto console = mm->LockActivity(cap);
    std::string msg = "Created Plane: " + r.GetString();
    console->Info(msg);
    return r;
}

pxr::SdfPath OpenUSDProvider::CreateSphere(PXR_NS::GfVec3d pos, float radius)
{
    auto mm = lab::Orchestrator::Canonical();
    pxr::UsdStageRefPtr stage = Stage();
    if (!stage)
        return {};

    pxr::SdfPath primPath = stage->GetDefaultPrim().GetPath();
    primPath = primPath.AppendChild(TfToken("Shapes"));
    primPath = primPath.AppendChild(TfToken("sphere"));
    string primPathStr = GetNextAvailableIndexedPath(primPath.GetString());
    pxr::SdfPath r(primPathStr);
    auto sphere = pxr::UsdGeomSphere::Define(stage, r);

    GfMatrix4d m;
    m.SetTranslate(pos);
    SetTransformMatrix(sphere, m, UsdTimeCode::Default());

    sphere.GetRadiusAttr().Set((double)radius);
    std::weak_ptr<ConsoleActivity> cap;
    auto console = mm->LockActivity(cap);
    std::string msg = "Created Sphere: " + r.GetString();
    console->Info(msg);
    return r;
}


pxr::SdfPath OpenUSDProvider::CreateParHeightfield(PXR_NS::GfVec3d pos) {
    auto mm = lab::Orchestrator::Canonical();
    pxr::UsdStageRefPtr stage = Stage();
    if (!stage)
        return {};

    // Create a reasonable ocean-to-land color gradient.
    static int cp_locations[] = {
        000,  // Dark Blue
        126,  // Light Blue
        127,  // Yellow
        128,  // Dark Green
        160,  // Brown
        200,  // White
        255,  // White
    };

    static heman_color cp_colors[] = {
        0x001070,  // Dark Blue
        0x2C5A7C,  // Light Blue
        0xE0F0A0,  // Yellow
        0x5D943C,  // Dark Green
        0x606011,  // Brown
        0xFFFFFF,  // White
        0xFFFFFF,  // White
    };
    #define COUNT(a) (sizeof(a) / sizeof(a[0]))

    heman_image* grad = heman_color_create_gradient(256, COUNT(cp_colors), cp_locations, cp_colors);

    const int hmapWidth = 1024;
    const int hmapHeight = 1024;

    // Generate an island shape using simplex noise and a distance field.
    heman_image* elevation = heman_generate_island_heightmap(hmapWidth, hmapHeight, rand());
    heman_image* elevationviz = heman_ops_normalize_f32(elevation, -0.5, 0.5);

    // Compute ambient occlusion from the height map.
    heman_image* occ = heman_lighting_compute_occlusion(elevation);

    // Visualize the normal vectors.
    heman_image* normals = heman_lighting_compute_normals(elevation);
    heman_image* normviz = heman_ops_normalize_f32(normals, -1, 1);

    // Create an albedo image.
    heman_image* albedo = heman_color_apply_gradient(elevation, -0.5, 0.5, grad);
    heman_image_destroy(grad);

    // Perform lighting.
    float lightpos[] = {-0.5f, 0.5f, 1.0f};
    heman_image* final = heman_lighting_apply(elevation, albedo, 1, 1, 0.5, lightpos);

    heman_image* frames[] = {0, 0, normviz, albedo, final};
    frames[0] = heman_color_from_grayscale(elevationviz);
    frames[1] = heman_color_from_grayscale(occ);
    heman_image* filmstrip = heman_ops_stitch_horizontal(frames, 5);

    // create a USD prim for the mesh
    pxr::SdfPath primPath = stage->GetDefaultPrim().GetPath();
    primPath = primPath.AppendChild(TfToken("Shapes"));
    primPath = primPath.AppendChild(TfToken("Heightfield"));
    string primPathStr = GetNextAvailableIndexedPath(primPath.GetString());
    pxr::SdfPath r(primPathStr);
    auto prim = pxr::UsdGeomMesh::Define(stage, r);
    GfMatrix4d m;
    m.SetTranslate(pos);
    SetTransformMatrix(prim, m, UsdTimeCode::Default());

    HEMAN_FLOAT* elevData = heman_image_data(elevation);

    // first populate the points attr with the points from the mesh
    pxr::UsdAttribute pointsAttr = prim.GetPointsAttr();
    pxr::VtArray<GfVec3f> points;
    int idx = 0;
    for (int i = 0; i < hmapHeight; ++i) {
        for (int j = 0; j < hmapWidth; ++j) {
            float x = float(i) * 0.1f;
            float y = elevData[idx++] * 20.f;
            float z = float(j) * -0.1f;
            points.push_back(GfVec3f(x, y, z));
        }
    }
    pointsAttr.Set(points);

    // the face vertex counts
    pxr::UsdAttribute faceVertexCountsAttr = prim.GetFaceVertexCountsAttr();
    pxr::VtArray<int> faceVertexCounts;
    for (int i = 0; i < hmapHeight; ++i) {
        for (int j = 0; j < hmapWidth; ++j) {
            faceVertexCounts.push_back(4);
        }
    }
    faceVertexCountsAttr.Set(faceVertexCounts);

    // the face vertex indices
    pxr::UsdAttribute faceVertexIndicesAttr = prim.GetFaceVertexIndicesAttr();
    pxr::VtArray<int> faceVertexIndices;
    for (int i = 0; i < hmapHeight-1; ++i) {
        for (int j = 0; j < hmapWidth-1; ++j) {
            faceVertexIndices.push_back((i+1)*hmapWidth+j);
            faceVertexIndices.push_back((i+1)*hmapWidth+j+1);
            faceVertexIndices.push_back(i*hmapWidth+j+1);
            faceVertexIndices.push_back(i*hmapWidth+j);
        }
    }
    faceVertexIndicesAttr.Set(faceVertexIndices);

    // create the vertex colors
    HEMAN_FLOAT* albedoData = heman_image_data(albedo);
    UsdGeomPrimvar colorPrimVar = prim.GetDisplayColorPrimvar();
    colorPrimVar.SetInterpolation(UsdGeomTokens->varying);

    pxr::VtArray<GfVec3f> colors;
    for (int i = 0; i < hmapWidth * hmapHeight; ++i) {
        colors.push_back(GfVec3f(albedoData[i*3], albedoData[i*3+1], albedoData[i*3+2]));
    }
    pxr::UsdAttribute colorAttr = prim.GetDisplayColorAttr();
    colorAttr.Set(colors);

    // create the normals
    HEMAN_FLOAT* normalData = heman_image_data(normals);
    pxr::UsdAttribute normalsAttr = prim.GetNormalsAttr();
    pxr::VtArray<GfVec3f> normalsArray;
    for (int i = 0; i < hmapWidth * hmapHeight; ++i) {
        normalsArray.push_back(GfVec3f(normalData[i*3], normalData[i*3+1], normalData[i*3+2]));
    }
    normalsAttr.Set(normalsArray);

    // free the mesh
    heman_image_destroy(filmstrip);

    heman_image_destroy(frames[0]);
    heman_image_destroy(frames[1]);
    heman_image_destroy(elevation);
    heman_image_destroy(elevationviz);
    heman_image_destroy(occ);
    heman_image_destroy(normals);
    heman_image_destroy(normviz);
    heman_image_destroy(albedo);
    heman_image_destroy(final);

    std::weak_ptr<ConsoleActivity> cap;
    auto console = mm->LockActivity(cap);
    std::string msg = "Created Par Heightfield: " + r.GetString();
    console->Info(msg);
    return r;
}

pxr::SdfPath OpenUSDProvider::CreatePrimShape(const std::string& shape,
                                              PXR_NS::GfVec3d pos, float radius) {
    auto mm = lab::Orchestrator::Canonical();
    pxr::UsdStageRefPtr stage = Stage();
    if (!stage)
        return {};

    pxr::SdfPath primPath = stage->GetDefaultPrim().GetPath();
    primPath = primPath.AppendChild(TfToken("Shapes"));
    primPath = primPath.AppendChild(TfToken(shape));
    string primPathStr = GetNextAvailableIndexedPath(primPath.GetString());
    pxr::SdfPath r(primPathStr);
    if (shape == "capsule") {
        CreateCapsule(pos);
    }
    else if (shape == "cone") {
        CreateCone(pos);
    }
    else if (shape == "cube") {
        CreateCube(pos);
    }
    else if (shape == "cylinder") {
        CreateCylinder(pos);
    }
    else if (shape == "plane") {
        CreatePlane(pos);
    }
    else if (shape == "sphere") {
        CreateSphere(pos, radius);
    }
    else {
        return {};
    }

    std::weak_ptr<ConsoleActivity> cap;
    auto console = mm->LockActivity(cap);
    std::string msg = "Created " + shape + ": " + r.GetString();
    console->Info(msg);
    return r;
}


void OpenUSDProvider::LoadStage(std::string const& filePath)
{
    if (UsdStage::IsSupportedFile(filePath))
    {
        printf("File format supported: %s\n", filePath.c_str());
    }
    else {
        fprintf(stderr, "%s : File format not supported\n", filePath.c_str());
        return;
    }

    UsdStageRefPtr loadedStage = UsdStage::Open(filePath);
    if (loadedStage) {
        self->model.reset(new Model());
        self->model->SetStage(loadedStage);
        self->layer.reset(new UsdSessionLayer(self->model.get()));
        self->gridSceneIndex = GridSceneIndex::New();
        self->model->AddSceneIndexBase(self->gridSceneIndex);
        auto editableSceneIndex = self->model->GetEditableSceneIndex();
        self->xformSceneIndex = XformFilterSceneIndex::New(editableSceneIndex);
        self->model->SetEditableSceneIndex(self->xformSceneIndex);
        TfToken plugin = Engine::GetDefaultRendererPlugin();
        self->engine.reset(new Engine(self->model->GetFinalSceneIndex(), plugin));
    }
    else {
        self->engine.reset();
        self->layer.reset();
        self->model.reset();
        fprintf(stderr, "Stage was not loaded at %s\n", filePath.c_str());
    }
}

void OpenUSDProvider::SaveStage() {
    if (self->model) {
        auto stage = self->model->GetStage();
        if (stage)
            stage->GetRootLayer()->Save();
    }
}

void OpenUSDProvider::ExportStage(std::string const& path)
{
    if (!self->model) {
        fprintf(stderr, "No stage to export\n");
        return;
    }
    auto stage = self->model->GetStage();
    if (!stage) {
        fprintf(stderr, "No stage to export\n");
        return;
    }

    if (path.size() >= 5 && path.substr(path.size() - 5) == ".usdz") {
        // Store current working directory
        char* cwd_buffer = getcwd(nullptr, 0);
        if (!cwd_buffer) {
            std::cerr << "Failed to get current working directory" << std::endl;
            return;
        }

        #ifdef _WIN32
        const std::string tempdir = "C:\\temp";
        #else
        const std::string tempdir = "/var/tmp";
        #endif

        // Change directory to a temporary
        if (chdir(tempdir.c_str()) != 0) {
            std::cerr << "Failed to change directory to /var/tmp" << std::endl;
            free(cwd_buffer);
            return;
        }

        std::string temp_usdc_path = "contents.usdc";
        stage->GetRootLayer()->Export(temp_usdc_path);
        if (!pxr::UsdUtilsCreateNewUsdzPackage(pxr::SdfAssetPath(temp_usdc_path), "new.usdz")) {
            // report error
        }
        
        // Move the temporary USD file to the specified path
        if (std::rename("new.usdz", path.c_str()) != 0) {
            std::cerr << "Failed to move the temporary file to the specified path" << std::endl;
        }
        
        // Restore the original working directory
        if (chdir(cwd_buffer) != 0) {
            std::cerr << "Failed to restore the original working directory" << std::endl;
        }
        free(cwd_buffer);
    }
    else {
        stage->GetRootLayer()->Export(path);
    }
}

PXR_NS::SdfLayerRefPtr OpenUSDProvider::GetSessionLayer() {
    if (self->layer)
        return self->layer->GetSessionLayer();
    return {};
}

void OpenUSDProvider::ExportSessionLayer(std::string const& path) {
    auto sl = GetSessionLayer();
    if (sl) {
        sl->Export(path);
    }
}

void OpenUSDProvider::CreateShotFromTemplate(const std::string& dst, 
                                            const std::string& shotname) {
    if (!self->templateStage) {
      /*   ___         _          _                  _      _
          / __|__ _ __| |_  ___  | |_ ___ _ __  _ __| |__ _| |_ ___
         | (__/ _` / _| ' \/ -_) |  _/ -_) '  \| '_ \ / _` |  _/ -_)
          \___\__,_\__|_||_\___|  \__\___|_|_|_| .__/_\__,_|\__\___|
                                               |_|*/
        auto resource_path = std::string(lab_application_resource_path(nullptr, nullptr));
        std::string path = resource_path + "/LabSceneTemplate.usda";
        self->templateStage = pxr::UsdStage::Open(path);
        if (!self->templateStage) {
            printf("Failed to open the Scene Template\n");
        }
    }
}

void OpenUSDProvider::TestReferencing() {
    /// create disk strcture
     ///
     std::string testDir = "/var/tmp/TestReferencing";
     if (!TfMakeDirs(testDir, 0700, true)) {
         throw std::runtime_error("Failed to create test dir at " + testDir);
     }
     auto quickBrownFoxFilePath = testDir + "/quickbrownfox.usdc";
     auto lazyDogFilePath = testDir + "/lazydog.usdc";

     /// create stages
     auto qbfStage = UsdStage::CreateNew(quickBrownFoxFilePath);
     if (!qbfStage) {
         throw std::runtime_error("Failed to create new stage at " + quickBrownFoxFilePath);
     }
     auto ldStage = UsdStage::CreateNew(lazyDogFilePath); // "CONFUSE 2" setting path here
     if (!ldStage) {
         throw std::runtime_error("Failed to create new stage at " + lazyDogFilePath);
     }

     /// set up the lazy dog
     ldStage->SetEditTarget(ldStage->GetRootLayer()); // "CONFUSE 3" unncessary, I think
     SdfPath lazy("/Lazy"); // "CONFUSE 1" odd that this takes a string
     UsdGeomScope lazyScope = UsdGeomScope::Define(ldStage, lazy);
     ldStage->SetDefaultPrim(lazyScope.GetPrim());
     SdfPath lazyDogPath = lazy.AppendChild(TfToken("Dog")); // "CONFUSE 1" and this takes a token
     auto lazyDogPrim = ldStage->DefinePrim(lazyDogPath);
     ldStage->Export(lazyDogFilePath); // "CONFUSE 2" Why did I have to specify filepath, this is not a "Save As" scenario

     /// set up the quick brown fox
     qbfStage->SetEditTarget(qbfStage->GetRootLayer()); // "CONFUSE 3" unncessary, I think

     // Quick
     SdfPath quickPath("/Quick"); // "CONFUSE 1" odd that this takes a string
     UsdGeomScope qScope = UsdGeomScope::Define(qbfStage, quickPath);
     qbfStage->SetDefaultPrim(qScope.GetPrim());
     std::cout << "quick: " << qScope.GetPrim().GetPath().GetString() << "\n";

     // Quick/Brown
     SdfPath qbPath = quickPath.AppendChild(TfToken("Brown")); // "CONFUSE 1" and this takes a token
     UsdGeomScope qbScope = UsdGeomScope::Define(qbfStage, qbPath);
     std::cout << "quick brown: " << qbScope.GetPrim().GetPath().GetString() << "\n";

     // Quick/Brown/Fox
     SdfPath qbfPath = qbPath.AppendChild(TfToken("Fox")); // "CONFUSE 1" and this takes a token
     // quick brown fox over the lazy dog
     auto referencingPrim = qbfStage->OverridePrim(qbfPath);
     std::cout << "quick brown fox: " << referencingPrim.GetPrim().GetPath().GetString() << "\n";

     if (!referencingPrim.GetReferences().AddReference(lazyDogFilePath)) {
         throw std::runtime_error("quick brown fox failed to over the lazy dog");
     }

     qbfStage->GetRootLayer()->Export("/var/tmp/TestReferencing/foo.usda");//quickBrownFoxFilePath); // "CONFUSE 2" Why did I have to specify filepath, this is not a "Save As" scenario

     std::string result;
     qbfStage->GetRootLayer()->ExportToString(&result);
     std::cout << "referencing test complete\n" << result << std::endl;
}

namespace {

std::string getLastComponent(const std::string& filePath) {
    // Find the last '/' or '\' to get the file name
    size_t lastSlash = filePath.find_last_of("/\\");
    std::string fileName = (lastSlash != std::string::npos) ? filePath.substr(lastSlash + 1) : filePath;

    // Find the last '.' to get the file extension
    size_t lastDot = fileName.find_last_of('.');

    // If there's a dot and it's not at the beginning (hidden file), remove the extension
    if (lastDot != std::string::npos && lastDot != 0) {
        fileName.erase(lastDot);
    }

    return fileName;
}

void createScopeIfNeeded(UsdStageRefPtr stage, const SdfPath& scopePath) {
    // Check if the scope already exists
    UsdGeomScope existingScope = UsdGeomScope::Define(stage, scopePath);
    if (existingScope) {
        std::cout << "Scope already exists at path: " << scopePath.GetString() << std::endl;
    } else {
        // Create the scope if it doesn't exist
        UsdGeomScope newScope = UsdGeomScope::Define(stage, scopePath);
        if (newScope) {
            std::cout << "Created a new scope at path: " << scopePath.GetString() << std::endl;
        } else {
            std::cerr << "Failed to create the scope at path: " << scopePath.GetString() << std::endl;
        }
    }
}

}
void OpenUSDProvider::ReferenceLayer(PXR_NS::UsdStageRefPtr stage,
                                    const std::string& layerFilePath,
                                    PXR_NS::SdfPath& atScope,
                                    bool setTransform, GfVec3d pos, bool asInstance) {
    auto mm = LabApp::instance()->mm();

    // Get the existing stage or print an error message if not available
    if (!stage) {
        printf("Can't reference %s, no stage created\n", layerFilePath.c_str());
        return;
    }
    
    // Create or access the "Models" scope
    createScopeIfNeeded(stage, atScope);

    // Create a unique path for the referenced layer
    /// @TODO somewhere, Sdf? there's a routine to do this.
    auto name = getLastComponent(layerFilePath);
    std::unordered_set<char> illegalChars {'-', '>', '<', '$', '%', '.'};
    for (char& c : name) {
        if (illegalChars.find(c) != illegalChars.end()) {
            c = '_';
        }
    }

    PXR_NS::SdfPath primPath = atScope.AppendChild(TfToken(name));
    name = GetNextAvailableIndexedPath(primPath.GetString());

    // make an override prim at the new path and add a reference to the layer
    pxr::SdfPath r(name);
    stage->OverridePrim(r).GetReferences().AddReference(layerFilePath);

    // Access the prim at the new path
    auto prim = stage->GetPrimAtPath(r);
    if (asInstance)
        prim.SetInstanceable(asInstance);

    if (setTransform) {
        // Create a UsdGeomXformable for the referenced layer
        auto refXform = PXR_NS::UsdGeomXformable(prim);
        refXform.SetXformOpOrder({});
        GfMatrix4d m;
        m.SetTranslate(pos);
        SetTransformMatrix(refXform, m, UsdTimeCode::Default());

        // Access the metadata for the layer by opening it onto its own stage
        pxr::UsdStageRefPtr referencedStage = pxr::UsdStage::Open(layerFilePath);
        if (!referencedStage) {
            std::cerr << "Failed to open referenced stage at path: " << layerFilePath << std::endl;
            return;
        }

        if (UsdGeomGetStageUpAxis(referencedStage) == UsdGeomTokens->z) {
            // Insert a rotation operation to reorient the layer if needed
            pxr::UsdGeomXformOp rotateOp = refXform.AddRotateXOp(pxr::UsdGeomXformOp::PrecisionFloat);
            rotateOp.Set(-90.0f);  // Rotate 90 degrees about the X axis
            std::cout << "Inserted rotation to make the referenced layer, " << name << " Y-up." << std::endl;
        }
    }
}

string OpenUSDProvider::GetNextAvailableIndexedPath(string primPath) {
    pxr::UsdStageRefPtr stage = Stage();
    pxr::UsdPrim prim;
    int i = -1;
    string newPath;
    do {
        i++;
        if (i == 0) newPath = primPath;
        else newPath = primPath + to_string(i);
        prim = stage->GetPrimAtPath(pxr::SdfPath(newPath));
    } while (prim.IsValid());
    return newPath;
}

} // lab
