//
//  CreateActivity.cpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 12/1/23.
//  Copyright © 2023 Nick Porcino. All rights reserved.
//

#include "UsdCreateActivity.hpp"

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include <pxr/usd/sdf/types.h>
#include <pxr/usd/usd/attribute.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/schemaRegistry.h>
#include <pxr/usd/usdGeom/basisCurves.h>
#include <pxr/usd/usdGeom/camera.h>
#include <pxr/usd/usdGeom/capsule.h>
#include <pxr/usd/usdGeom/cone.h>
#include <pxr/usd/usdGeom/cube.h>
#include <pxr/usd/usdGeom/cylinder.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/metrics.h>
#include <pxr/usd/usdGeom/plane.h>
#include <pxr/usd/usdGeom/scope.h>
#include <pxr/usd/usdGeom/sphere.h>
#include <pxr/usd/usdGeom/xform.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>
#include <pxr/base/plug/registry.h>

#include "Lab/LabDirectories.h"
#include "Activities/Console/ConsoleActivity.hpp"
#include "Providers/OpenUSD/OpenUSDProvider.hpp"
#include "Providers/OpenUSD/UsdUtils.hpp"
#include "Providers/OpenUSD/par_heman/include/heman.h"
#include "Providers/Color/nanocolorUtils.h"
#include "Providers/OpenUSD/par_shapes.h"

#include <string>

using namespace std;

namespace lab {

PXR_NAMESPACE_USING_DIRECTIVE

struct CreateActivity::data {
    bool runCreateColorChecker = false;
    bool runCreateUsdPrim = false;
    bool runCreateCard = false;
    bool runCreateUsdPrimShape = false;
    bool runCreateParGeometry = false;
    bool runCreateGroundGrid = false;
    bool runCreateParHeightfield = false;
    bool runCreateHilbertCurve = false;

    // a vector of schemas for which a prim can be created
    std::map<std::string, TfType> primTypes;
    std::string selectedPrimType;
    char* primTypesMenuData = nullptr;

    void UpdatePrimTypesList() {
        // https://groups.google.com/g/usd-interest/c/q8asqMYuyeg/m/sRhFTIEfCAAJ
        static std::once_flag called_once;
        std::call_once(called_once, [&]() {
            const TfType baseType = TfType::Find<UsdTyped>();
            std::set<TfType> schemaTypes;
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
        });
    }
};

CreateActivity::CreateActivity()
: Activity(CreateActivity::sname()) {
    _self = new CreateActivity::data;
    
    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<CreateActivity*>(instance)->RunUI(*vi);
    };
    activity.Menu = [](void* instance) {
        static_cast<CreateActivity*>(instance)->Menu();
    };
}

CreateActivity::~CreateActivity() {
    delete _self;
}

void CreateActivity::Menu() {
    auto usd = OpenUSDProvider::instance();
    if (!usd)
        return;

    pxr::UsdStageRefPtr stage = usd->Stage();
    if (!stage)
        return;

    if (ImGui::BeginMenu("Create")) {
        if (ImGui::MenuItem("Camera")) {
            Orchestrator* mm = Orchestrator::Canonical();
            mm->EnqueueTransaction(Transaction{"Create Camera", [this](){
                auto usd = OpenUSDProvider::instance();
                if (usd)
                    usd->CreateCamera("camera");
            }});
        }
        
        ImGui::Separator();
        if (ImGui::MenuItem("USD Prim Shape...")) {
            _self->runCreateUsdPrimShape = true;
        }
        if (ImGui::MenuItem("Usd Prim...")) {
            _self->runCreateUsdPrim = true;
        }
        if (ImGui::MenuItem("Card...")) {
            _self->runCreateCard = true;
        }
        if (ImGui::MenuItem("Color Checker...")) {
            _self->runCreateColorChecker = true;
        }
        if (ImGui::MenuItem("Ground Grid...")) {
            _self->runCreateGroundGrid = true;
        }
        if (ImGui::MenuItem("Par Geometry...")) {
            _self->runCreateParGeometry = true;
        }
        if (ImGui::MenuItem("Par Heightfield...")) {
            _self->runCreateParHeightfield = true;
        }
        if (ImGui::MenuItem("Hilbert Curve...")) {
            _self->runCreateHilbertCurve = true;
        }

        static char buffer[128] = "Greetz & Lulz 2 otri";
        if (ImGui::MenuItem("DemoText")) {
            //auto cameraMode = mm->LockActivity(_self->cameraMode);
            //GfVec3d pos = cameraMode->HitPoint();
            GfVec3d pos(0,0,0);
            Orchestrator* mm = Orchestrator::Canonical();
            mm->EnqueueTransaction(Transaction{
                "Create Demo Text", [this, pos]() {
                    auto usd = OpenUSDProvider::instance();
                    if (usd)
                        usd->CreateDemoText(buffer, pos);
            }});
        }
        ImGui::Indent(20);
        ImGui::SetNextItemWidth(150);
        ImGui::InputText("##DemoTextInput", buffer, IM_ARRAYSIZE(buffer));
        ImGui::Unindent(20);

        if (ImGui::MenuItem("Material")) {
            Orchestrator* mm = Orchestrator::Canonical();
            mm->EnqueueTransaction(Transaction {
                "Create Material", [this]() {
                auto usd = OpenUSDProvider::instance();
                if (usd)
                    usd->CreateMaterial();
            }});
        }
        if (ImGui::MenuItem("Test Data")) {
            Orchestrator* mm = Orchestrator::Canonical();
            mm->EnqueueTransaction(Transaction {
                "Create Test Data", [this]() {
                auto usd = OpenUSDProvider::instance();
                if (usd)
                    usd->CreateTestData();
            }});
        }
        ImGui::EndMenu();
    }
}

void CreateActivity::RunUI(const LabViewInteraction&) {
    auto usd = OpenUSDProvider::instance();
    if (!usd)
        return;

    pxr::UsdStageRefPtr stage = usd->Stage();
    if (!stage)
        return;

    if (_self->runCreateColorChecker) {
        const char* popupName = "Create Color Checker";
        ImGui::OpenPopup(popupName);
        if (ImGui::BeginPopupModal(popupName, nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
        {
            static int selected = 0;
            ImGui::Combo("Color Space", &selected, "sRGB\0Linear Rec709\0Linear DisplayP3\0Linear AP0\0");
            if (ImGui::Button("Create")) {
                std::string colorSpace;
                switch (selected) {
                    case 0: colorSpace = "srgb_texture"; break;
                    case 1: colorSpace = "lin_rec709"; break;
                    case 2: colorSpace = "lin_displayp3"; break;
                    case 3: colorSpace = "lin_ap0"; break;
                }
                //auto stageMode = mm->LockActivity(_self->stage);
                //auto cameraMode = mm->LockActivity(_self->cameraMode);
                GfVec3d pos(0,0,0); // = cameraMode->HitPoint();
                Orchestrator* mm = Orchestrator::Canonical();
                mm->EnqueueTransaction(Transaction{"Create Color Checker", [this, colorSpace, pos](){
                    auto usd = OpenUSDProvider::instance();
                    if (usd)
                        usd->CreateMacbethChart(colorSpace, pos);
                }});
                ImGui::CloseCurrentPopup();
                _self->runCreateColorChecker = false;
            }
            ImGui::EndPopup();
        }
    }
    else if (_self->runCreateUsdPrimShape) {
        const char* popupName = "Create USD Prim Shape";
        ImGui::OpenPopup(popupName);
        if (ImGui::BeginPopupModal(popupName, nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
        {
            static int selected = 0;
            ImGui::Combo("Shape", &selected, "Capsule\0Cone\0Cube\0Cylinder\0Plane\0Sphere\0");

            // if the shape is a sphere, allow the user to input the radius
            static float sphere_radius = 1.f;
            if (selected == 5) {
                ImGui::InputFloat("Radius###csrad", &sphere_radius);
            }

            if (ImGui::Button("Create")) {
                std::string shape;
                switch (selected) {
                    case 0: shape = "capsule"; break;
                    case 1: shape = "cone"; break;
                    case 2: shape = "cube"; break;
                    case 3: shape = "cylinder"; break;
                    case 4: shape = "plane"; break;
                    case 5: shape = "sphere"; break;
                }
                //auto stageMode = mm->LockActivity(_self->stage);
                //auto cameraMode = mm->LockActivity(_self->cameraMode);
                GfVec3d pos(0,0,0);// = cameraMode->HitPoint();

                Orchestrator* mm = Orchestrator::Canonical();
                mm->EnqueueTransaction(Transaction{"Create USD Prim Shape", [this, shape, pos](){
                    auto usd = OpenUSDProvider::instance();
                    if (usd)
                        usd->CreatePrimShape(shape, pos, sphere_radius);
                }});
                ImGui::CloseCurrentPopup();
                _self->runCreateUsdPrimShape = false;
            }
            ImGui::EndPopup();
        }
    }
    else if (_self->runCreateCard) {
        _self->runCreateCard = false;
#if 0
        nfdchar_t* outPath = nullptr;
        const char* dir = lab_pref_for_key("LoadTextureDir");
        nfdresult_t result = NFD_OpenDialog("exr,jpg,jpeg,png,avif,hdr", dir? dir : ".", &outPath);
        if (result == NFD_OKAY) {
            std::string path(outPath);
            // get the directory of path and save it as the default
            // directory for the next time the user loads a stage
            std::string dir = path.substr(0, path.find_last_of("/\\"));
            lab_set_pref_for_key("LoadTextureDir", dir.c_str());
            auto console = mm->LockActivity(_self->console);
            //auto cameraMode = mm->LockActivity(_self->cameraMode);
            GfVec3d pos(0,0,0); // = cameraMode->HitPoint();
            Orchestrator* mm = Orchestrator::Canonical();
            mm->EnqueueTransaction(Transaction{"Create Card", [this, mm, path, pos]() {
                auto mm = lab::Orchestrator::Canonical();
                std::weak_ptr<ConsoleActivity> cap;
                auto console = mm->LockActivity(cap);
                console->Info("Creating Card");

                auto usd = OpenUSDProvider::instance();
                if (!usd)
                    return;

                pxr::UsdStageRefPtr stage = usd->Stage();
                if (!stage)
                    return;

                pxr::SdfPath primPath = stage->GetDefaultPrim().GetPath();
                primPath = primPath.AppendChild(TfToken("Shapes"));
                primPath = primPath.AppendChild(TfToken("Card"));
                string primPathStr = usd->GetNextAvailableIndexedPath(primPath.GetString());
                auto templateMode = mm->LockActivity(_self->templateMode);
                templateMode->create_card(stage, primPathStr, path);
            }});
        }
#endif
    }
    else if (_self->runCreateUsdPrim) {
        _self->UpdatePrimTypesList();
        const char* popupName = "Create USD Prim";
        ImGui::OpenPopup(popupName);
        if (ImGui::BeginPopupModal(popupName, nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
        {
            static int selected = 0;
            ImGui::Combo("Prims", &selected, _self->primTypesMenuData);
            if (ImGui::Button("Create")) {
                _self->runCreateUsdPrim = false;
                char* curr = _self->primTypesMenuData;
                for (int i = 0; i < selected; ++i) {
                    if (i == selected) {
                        break;
                    }
                    curr += strlen(curr) + 1;
                }
                //auto stageMode = mm->LockActivity(_self->stage);
                //auto cameraMode = mm->LockActivity(_self->cameraMode);
                GfVec3d pos(0,0,0); // = cameraMode->HitPoint();
                Orchestrator* mm = Orchestrator::Canonical();
                mm->EnqueueTransaction(Transaction{"Create USD Prim", [this, mm, curr, pos](){
                    auto usd = OpenUSDProvider::instance();
                    if (!usd)
                        return;

                    pxr::UsdStageRefPtr stage = usd->Stage();
                    if (!stage)
                        return;

                    pxr::SdfPath primPath = stage->GetDefaultPrim().GetPath();
                    primPath = primPath.AppendChild(TfToken("Shapes"));
                    primPath = primPath.AppendChild(TfToken(curr));
                    string primPathStr = usd->GetNextAvailableIndexedPath(primPath.GetString());
                    pxr::SdfPath r(primPathStr);
                    auto prim = stage->DefinePrim(r, TfToken(curr));
                    
                    // if the prim is transformable, move it to the desired position
                    auto xform = UsdGeomXformable::Get(stage, r);
                    if (xform) {
                        GfMatrix4d m;
                        m.SetTranslate(pos);
                        SetTransformMatrix(xform, m, UsdTimeCode::Default());
                    }
                }});
            }
            ImGui::EndPopup();
        }
    }
    else if (_self->runCreateGroundGrid) {
        const char* popupName = "Create Ground Grid";
        ImGui::OpenPopup(popupName);
        if (ImGui::BeginPopupModal(popupName, nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
        {
            static int x = 10;
            static int y = 10;
            static float spacing = 1.f;
            ImGui::InputInt("X", &x);
            ImGui::InputInt("Y", &y);
            ImGui::InputFloat("Spacing (m)", &spacing);
            if (ImGui::Button("Create")) {
                do {
                    //auto cameraMode = mm->LockActivity(_self->cameraMode);
                    GfVec3d pos(0,0,0);// = cameraMode->HitPoint();

                    auto usd = OpenUSDProvider::instance();
                    if (!usd)
                       break;

                    pxr::UsdStageRefPtr stage = usd->Stage();
                    if (!stage)
                        break;

                    float capturedSpacing = spacing / PXR_NS::UsdGeomGetStageMetersPerUnit(stage);
                    int capturedX = x;
                    int capturedY = y;
                    Orchestrator* mm = Orchestrator::Canonical();
                    mm->EnqueueTransaction(Transaction {
                        "Create Ground Grid",
                        [this, pos,
                         capturedX, capturedY, capturedSpacing]() {
                        auto usd = OpenUSDProvider::instance();
                        if (usd)
                            usd->CreateGroundGrid(pos, capturedX, capturedY, capturedSpacing);
                    }});
                    ImGui::CloseCurrentPopup();
                } while (false);
                _self->runCreateGroundGrid = false;
            }
            ImGui::EndPopup();
        }
    }
    else if (_self->runCreateParGeometry) {
        const char* popupName = "Create Par Geometry";
        ImGui::OpenPopup(popupName);
        if (ImGui::BeginPopupModal(popupName, nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
        {
            static int selected = 0;
            //ImGui::Combo("Geometry", &selected, "Cylinder\0Cone\0Disk\0Torus\0Sphere\0Subdivided Sphere\0Klein Bottle\0Trefoil Knot\0Hemisphere\0Plane\0Icosahedron\0Dodecahedron\0Octahedron\0Tetrahedron\0Cube\0Disk\0Empty\0Rock\0L-System\0");
            ImGui::Combo("Geometry", &selected, "Cylinder\0Cone\0Disk\0Torus\0Sphere\0Subdivided Sphere\0Klein Bottle\0Trefoil Knot\0Hemisphere\0Plane\0Icosahedron\0Dodecahedron\0Octahedron\0Tetrahedron\0Cube\0Disk\0Empty\0Rock\0");
            if (ImGui::Button("Create")) {
                //auto cameraMode = mm->LockActivity(_self->cameraMode);
                GfVec3d pos(0,0,0);// = cameraMode->HitPoint();
                std::string shape;
                switch (selected) {
                    case 0: shape = "Cylinder"; break;
                    case 1: shape = "Cone"; break;
                    case 2: shape = "Disk"; break;
                    case 3: shape = "Torus"; break;
                    case 4: shape = "Sphere"; break;
                    case 5: shape = "Subdivided_Sphere"; break;
                    case 6: shape = "Klein_Bottle"; break;
                    case 7: shape = "Trefoil_Knot"; break;
                    case 8: shape = "Hemisphere"; break;
                    case 9: shape = "Plane"; break;
                    case 10: shape = "Icosahedron"; break;
                    case 11: shape = "Dodecahedron"; break;
                    case 12: shape = "Octahedron"; break;
                    case 13: shape = "Tetrahedron"; break;
                    case 14: shape = "Cube"; break;
                    case 15: shape = "Disk"; break;
                    case 16: shape = "Empty"; break;
                    case 17: shape = "Rock"; break;
                    //case 18: shape = "L_System"; break;
                }
                Orchestrator* mm = Orchestrator::Canonical();
                mm->EnqueueTransaction(Transaction {
                    "Create Par Geometry",
                    [this, shape, pos]() {
                    auto usd = OpenUSDProvider::instance();
                    if (usd)
                        usd->CreateParGeometry(shape, pos);
                }});
                ImGui::CloseCurrentPopup();
                _self->runCreateParGeometry = false;
            }
            ImGui::EndPopup();
        }
    }
    else if (_self->runCreateParHeightfield) {
        const char* popupName = "Create Par Heightfield";
        ImGui::OpenPopup(popupName);
        if (ImGui::BeginPopupModal(popupName, nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
        {
            static int selected = 0;
            ImGui::Combo("Heightfield", &selected, "Hills\0Hills2\0Hills3\0Hills4\0Hills5\0Hills6\0Hills7\0Hills8\0Hills9\0Hills10\0Hills11\0Hills12\0Hills13\0Hills14\0Hills15\0Hills16\0Hills17\0Hills18\0Hills19\0Hills20\0");
            if (ImGui::Button("Create")) {
                //auto cameraMode = mm->LockActivity(_self->cameraMode);
                GfVec3d pos(0,0,0);// = cameraMode->HitPoint();
                Orchestrator* mm = Orchestrator::Canonical();
                mm->EnqueueTransaction(Transaction {
                    "Create Par Heightfield", [this, pos]() {
                    auto usd = OpenUSDProvider::instance();
                    if (usd)
                        usd->CreateParHeightfield(pos);
                }});
                ImGui::CloseCurrentPopup();
                _self->runCreateParHeightfield = false;
            }
            ImGui::EndPopup();
        }
    }
    else if (_self->runCreateHilbertCurve) {
        // ask the user for the number of iterations
        const char* popupName = "Create Hilbert Curve";
        ImGui::OpenPopup(popupName);
        if (ImGui::BeginPopupModal(popupName, nullptr, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize))
        {
            static int iterations = 1;
            ImGui::InputInt("Iterations", &iterations);
            if (ImGui::Button("Create")) {
                //auto cameraMode = mm->LockActivity(_self->cameraMode);
                GfVec3d pos(0,0,0); // = cameraMode->HitPoint();
                int captureIterations = iterations;
                Orchestrator* mm = Orchestrator::Canonical();
                mm->EnqueueTransaction(Transaction{
                    "Create Hilbert Curve", [this, pos, captureIterations]() {
                        auto usd = OpenUSDProvider::instance();
                        if (usd)
                            usd->CreateHilbertCurve(captureIterations, pos);
                }});
                ImGui::CloseCurrentPopup();
                _self->runCreateHilbertCurve = false;
            }
            ImGui::EndPopup();
        }
    }
}

} //lab
