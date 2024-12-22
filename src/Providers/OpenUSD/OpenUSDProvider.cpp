
#include "OpenUSDProvider.hpp"
#include "UsdSessionLayer.hpp"
#include "UsdUtils.hpp"
#include "SpaceFillCurve.hpp"
#include "CreateDemoText.hpp"

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
#include <pxr/usd/usd/debugCodes.h>
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
    int stage_generation = 0;

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
        engine.reset();
        layer.reset();
        model.reset();
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
    PXR_NS::TfDebug::SetDebugSymbolsByName("USD_STAGE_LIFETIMES", true);
    self->SetEmptyStage();
    LoadStage("");
    auto stage = Stage();
    pxr::SdfPath primPath = stage->GetDefaultPrim().GetPath();
    CreateCube({0,0,0});
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
    return CreateC64DemoText(*this, text, pos);
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
    UsdStageRefPtr stage;
    if (filePath.length()) {
        if (UsdStage::IsSupportedFile(filePath))
        {
            printf("File format supported: %s\n", filePath.c_str());
        }
        else {
            fprintf(stderr, "%s : File format not supported\n", filePath.c_str());
            return;
        }
        stage = UsdStage::Open(filePath, UsdStage::LoadAll);
    }
    else {
        stage = pxr::UsdStage::CreateInMemory();
        if (stage) {
            UsdGeomSetStageUpAxis(stage, pxr::UsdGeomTokens->y);

            auto rootLayer = stage->GetRootLayer();

            UsdGeomScope labScope = UsdGeomScope::Define(stage, SdfPath("/Lab"));
            stage->SetDefaultPrim(labScope.GetPrim());
            stage->SetEditTarget(rootLayer);
        }
    }
    if (stage) {
        self->stage_generation++;
        constexpr bool testLoad = false;
        if (testLoad) {
            auto pseudoRoot = stage->GetPseudoRoot();
            printf("Pseudo root path: %s\n", pseudoRoot.GetPath().GetString().c_str());
            for (auto const& c : pseudoRoot.GetChildren())
            {
                printf("\tChild path: %s\n", c.GetPath().GetString().c_str());
            }
        }

        self->model.reset(new Model());
        self->model->SetStage(stage);
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
