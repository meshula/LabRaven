#include "PropertiesActivity.hpp"
#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui.h"
#include "ImGuizmo.h"
#include "Activities/Color/UIElements.hpp"

#include "Activities/OpenUSD/HydraEditor.hpp"
#include "Providers/OpenUSD/OpenUSDProvider.hpp"
#include "Providers/OpenUSD/UsdUtils.hpp"
#include "Providers/Selection/SelectionProvider.hpp"
#include "Providers/Color/nanocolor.h"

#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/matrix4f.h>
#include <pxr/usd/usdGeom/gprim.h>
#include <pxr/usd/usdGeom/metrics.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace lab {

struct PropertiesActivity::data {
    data() = default;
    ~data() = default;

    // hd properties editor
    std::unique_ptr<Editor> editor;

    // usd properties editor
    ImVec4 displayColor = ImVec4(0.5f, 0.5f, 0.5f, 1.f);
    std::string displayColorSpaceName;
    int colorspaceSelectionIndex = -1;

    bool hdPropertyEditorVisible = true;
    bool usdPropertyEditorVisible = true;
};

PropertiesActivity::PropertiesActivity() : Activity(PropertiesActivity::sname()) {
    _self = new PropertiesActivity::data();
    _self->editor = std::unique_ptr<Editor>(
                            new Editor("Hd Properties Editor##A1"));

    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<PropertiesActivity*>(instance)->RunUI(*vi);
    };
    activity.Menu = [](void* instance) {
        static_cast<PropertiesActivity*>(instance)->Menu();
    };
}

PropertiesActivity::~PropertiesActivity() {
    delete _self;
}

void PropertiesActivity::RunUI(const LabViewInteraction&) {
    auto usd = OpenUSDProvider::instance();

    if (_self->hdPropertyEditorVisible) {
        // make a window 800, 600
        if (_self->editor) {
            ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
            _self->editor->Update();
        }
        else {
            ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
            ImGui::Begin("Properties Editor##A2");
            ImGui::Text("No USD stage loaded.");
            ImGui::End();
        }
    }

    if (_self->usdPropertyEditorVisible) {
        // set up an index for the identity colorspace if it hasn't been done yet
        static int cs_ident = -1;
        static const char** nanocolor_names = NcRegisteredColorSpaceNames();
        if (cs_ident == -1) {
            // last one is identity, find it
            _self->colorspaceSelectionIndex = 0;
            const char** names = nanocolor_names;
            while (*names != nullptr) {
                ++_self->colorspaceSelectionIndex;
                ++names;
            }
            cs_ident = _self->colorspaceSelectionIndex - 1;
        }

        auto stage = usd->Stage();
        UsdPrim selectedPrim;
        UsdTimeCode timeCode; // default timecode @TODO wire it up to a TimeProvider
        Orchestrator* mm = Orchestrator::Canonical();

        auto sp = SelectionProvider::instance();
        auto selection = sp->GetSelectionPrims();
        if (selection.size()) {
            selectedPrim = selection[0];
            if (selectedPrim.IsValid()) {
                // stash display colour
                UsdGeomGprim gprim(selectedPrim);
                UsdAttribute displayColorSpaceAttr = gprim.GetDisplayColorAttr();

                VtVec3fArray values;
                bool success = displayColorSpaceAttr.Get(&values, timeCode);
                // if impossible to read, set a default value of [0.5, 0.5, 0.5]
                if (!success)
                    values = VtVec3fArray(1, GfVec3f(0.5f));

                _self->displayColor = (ImVec4){
                    (float)values[0][0], (float)values[0][1], (float)values[0][2], 1.f};

                // stash color space; leave selection of drop down where it was
                if (displayColorSpaceAttr.HasColorSpace()) {
                    TfToken cs = displayColorSpaceAttr.GetColorSpace();
                    _self->displayColorSpaceName = cs.GetString();
                }
            }
        }

        ImGui::SetNextWindowSize(ImVec2(200, 400), ImGuiCond_FirstUseEver);
        ImGui::Begin("Properties");
        if (stage) {
            if (ImGui::CollapsingHeader("Stage attributes")) {
                if (UsdGeomGetStageUpAxis(stage) == UsdGeomTokens->z) {
                    ImGui::TextUnformatted("Z Up");
                }
                else {
                    ImGui::TextUnformatted("Y Up");
                }
                // get units per meter from the stage
                float metersToStage = UsdGeomGetStageMetersPerUnit(stage);
                ImGui::Text("Meters per unit: %f", metersToStage);
                SdfAssetPath path = stage->GetColorConfiguration();

                static std::string stage_cs_path_str;

                stage_cs_path_str = path.GetAssetPath();
                if (stage_cs_path_str.size())
                    ImGui::Text("Color configuration %s", stage_cs_path_str.c_str());
                else
                    ImGui::TextUnformatted("Color configuration is default: lin_rec709");
            }
        }
        
        if (selectedPrim.IsValid()) {
            ImGui::TextUnformatted(selectedPrim.GetName().GetText());
            if (ImGui::CollapsingHeader("Transform attributes")) {
                UsdGeomGprim gprim(selectedPrim);
                
                GfMatrix4d transform = gprim.ComputeLocalToWorldTransform(timeCode);
                GfMatrix4f transformF(transform);
                
                float matrixTranslation[3], matrixRotation[3], matrixScale[3];
                ImGuizmo::DecomposeMatrixToComponents(transformF.data(), matrixTranslation,
                                                    matrixRotation, matrixScale);
                
                ImGui::InputFloat3("Translation", matrixTranslation);
                ImGui::InputFloat3("Rotation", matrixRotation);
                ImGui::InputFloat3("Scale", matrixScale);
                
                ImGuizmo::RecomposeMatrixFromComponents(matrixTranslation, matrixRotation,
                                                        matrixScale, transformF.data());
                
                if (!AreNearlyEquals(transformF, GfMatrix4f(transform))) {
                    mm->EnqueueTransaction(Transaction{"Prop: Set Transform Matrix", [gprim, transformF, timeCode](){
                        SetTransformMatrix(gprim, GfMatrix4d(transformF), timeCode);
                    }});
                }
            }
            
            if (selectedPrim.GetTypeName() == UsdGeomTokens->Camera)
                if (ImGui::CollapsingHeader("Camera attributes")) {
                    for (UsdAttribute attr : selectedPrim.GetAttributes()) {
                        if (attr.GetTypeName() == SdfValueTypeNames->Float) {
                            float value;
                            attr.Get(&value, timeCode);
                            float oldValue(value);
                            ImGui::InputFloat(attr.GetName().GetText(), &value);
                            if (value != oldValue) {
                                mm->EnqueueTransaction(Transaction{"Set Attr Value", [attr, value](){
                                    attr.Set(value);
                                }});
                            }
                        }
                        else if (attr.GetTypeName() == SdfValueTypeNames->Float2) {
                            GfVec2f value;
                            attr.Get(&value, timeCode);
                            GfVec2f oldValue(value);
                            ImGui::InputFloat2(attr.GetName().GetText(), value.data());
                            if (value != oldValue) {
                                mm->EnqueueTransaction(Transaction{"Set Attr Value", [attr, value](){
                                    attr.Set(value);
                                }});
                            }
                        }
                    }
                }
            
            if (selectedPrim.HasAttribute(UsdGeomTokens->primvarsDisplayColor)) {
                if (ImGui::CollapsingHeader("Other attributes")) {
                    // get the display color attr from UsdGeomGprim
                    UsdGeomGprim gprim(selectedPrim);
                    UsdAttribute displayColorAttr = gprim.GetDisplayColorAttr();

                    ImVec4 start = _self->displayColor;
                
                    auto min = ImGui::GetCursorScreenPos();
                    ImGui::BeginGroup();

                    auto pos = ImGui::GetCursorPos();
                    pos.x += 4;
                    pos.y += 6;
                    ImGui::SetCursorPos(pos);
                    ImGui::TextUnformatted("Display Color");
                    ImGui::SameLine();
                    pos = ImGui::GetCursorPos();
                    pos.y -= 3;
                    ImGui::SetCursorPos(pos);
                    ColorChipWithPicker(_self->displayColor, 'd');
                    
                    // set only if values change
                    bool changed = start.x != _self->displayColor.x ||
                                start.y != _self->displayColor.y ||
                                start.z != _self->displayColor.z;

                    if (changed) {
                        VtVec3fArray values;
                        GfVec3f value = { _self->displayColor.x, _self->displayColor.y, _self->displayColor.z };
                        values.push_back(value);
                        mm->EnqueueTransaction(Transaction{"Set Attr Value", [displayColorAttr, values, timeCode]() {
                            displayColorAttr.Set(values, timeCode);
                        }});
                    }
                    
                    static const char* name = nanocolor_names[cs_ident];
                    ImGui::Text("colorspace: %s", _self->displayColorSpaceName.c_str());
                    if (ColorSpacePicker("color space###dcs_picker", &_self->colorspaceSelectionIndex, &name)) {
                        _self->displayColorSpaceName = name;
                        TfToken cs(name);
                        mm->EnqueueTransaction(Transaction{"Set Attr ColorSpace", [displayColorAttr, cs](){
                            displayColorAttr.SetColorSpace(cs);
                        }});
                    }
                    ImGui::EndGroup();
                    
                    auto max = ImGui::GetItemRectMax();
                    ImDrawList* drawList = ImGui::GetWindowDrawList();
                    drawList->AddRect(min, {max.x + 4, max.y + 4}, ImColor(100, 100, 0));
                }
            }
        }
            else {
            ImGui::Text("No USD stage loaded.");
        }
        ImGui::End();
    }
}

void PropertiesActivity::Menu() {
    if (ImGui::BeginMenu("Properties")) {
        // add a check box menu item for the Hd Properties Editor
        if (ImGui::MenuItem("Hd Properties Editor", nullptr, &_self->hdPropertyEditorVisible)) {
            // toggle the visibility of the Hd Properties Editor
            _self->hdPropertyEditorVisible = !_self->hdPropertyEditorVisible;
        }
        if (ImGui::MenuItem("USD Properties Editor", nullptr, &_self->usdPropertyEditorVisible)) {
            // toggle the visibility of the USD Properties Editor
            _self->usdPropertyEditorVisible = !_self->usdPropertyEditorVisible;
        }
        ImGui::EndMenu();
    }
}

} // lab
