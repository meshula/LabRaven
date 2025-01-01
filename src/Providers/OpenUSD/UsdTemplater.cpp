
#include "UsdTemplater.hpp"
#include "Lab/LabDirectories.h"
#include "Lab/LabText.h"
#include <pxr/base/tf/fileUtils.h>
#include <pxr/base/tf/pathUtils.h>
#include <pxr/base/tf/span.h>
#include "pxr/imaging/hio/image.h"
#include <pxr/usd/kind/registry.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/copyUtils.h>
#include <pxr/usd/usd/modelAPI.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usdGeom/primvarsAPI.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>
#include <iostream>
#include <map>
#include <set>

PXR_NAMESPACE_USING_DIRECTIVE
using std::vector;
using std::string;

struct UsdTemplater::data {
    pxr::UsdStageRefPtr templateStage;

    void CopyMetadata(const SdfSpecHandle& dest, const UsdMetadataValueMap& metadata)
    {
        // Copy each key/value into the Sdf spec.
        TfErrorMark m;
        vector<string> msgs;
        for (auto const& tokVal : metadata) {
            dest->SetInfo(tokVal.first, tokVal.second);
            if (!m.IsClean()) {
                msgs.clear();
                for (auto i = m.GetBegin(); i != m.GetEnd(); ++i) {
                    msgs.push_back(i->GetCommentary());
                }
                m.Clear();
                TF_WARN("Failed copying metadata: %s", TfStringJoin(msgs).c_str());
            }
        }
    }

    std::string ExpandString(const std::string& str_, const TemplateData& td) {
        // find "${" within str
        std::string str = str_;
        size_t start = 0;
        while (start < str.size()) {
            size_t pos = str.find("${", start);
            if (pos == std::string::npos) {
                break;
            }
            size_t end = str.find("}", pos);
            if (end == std::string::npos) {
                break;
            }
            std::string key = str.substr(pos + 2, end - pos - 2);
            auto i = td.dict.find(key);
            if (i != td.dict.end()) {
                std::string value;
                if (i->second.IsHolding<std::string>()) {
                    value = i->second.Get<std::string>();
                }
                else {
                    value = TfStringify(i->second);
                }
                str.replace(pos, end - pos + 1, value);
                start = pos + value.size();
            }
            else {
                start = end + 1;
            }
        }
        return str;
    }

    std::vector<VtValue> RunSexpr(const std::string& name, TfSpan<VtValue> arg,
                                    const TemplateData& td) {
        std::vector<VtValue> ret;
        if (name == "add") {
            if (arg.size() != 2) {
                printf("add requires two arguments of the same type\n");
                return {};
            }
            if (arg[0].IsHolding<int>() && arg[1].IsHolding<int>()) {
                ret.push_back(VtValue(arg[0].Get<int>() + arg[1].Get<int>()));
            }
            else if (arg[0].IsHolding<float>() && arg[1].IsHolding<float>()) {
                ret.push_back(VtValue(arg[0].Get<float>() + arg[1].Get<float>()));
            }
            else {
                printf("add requires two arguments of the same type\n");
            }
        }
        else if (name == "sub") {
            if (arg.size() != 2) {
                printf("sub requires two arguments of the same type\n");
                return {};
            }
            if (arg[0].IsHolding<int>() && arg[1].IsHolding<int>()) {
                ret.push_back(VtValue(arg[0].Get<int>() - arg[1].Get<int>()));
            }
            else if (arg[0].IsHolding<float>() && arg[1].IsHolding<float>()) {
                ret.push_back(VtValue(arg[0].Get<float>() - arg[1].Get<float>()));
            }
            else {
                printf("sub requires two arguments of the same type\n");
            }
        }
        else if (name == "mul") {
            if (arg.size() != 2) {
                printf("mul requires two arguments of the same type\n");
                return {};
            }
            if (arg[0].IsHolding<int>() && arg[1].IsHolding<int>()) {
                ret.push_back(VtValue(arg[0].Get<int>() * arg[1].Get<int>()));
            }
            else if (arg[0].IsHolding<float>() && arg[1].IsHolding<float>()) {
                ret.push_back(VtValue(arg[0].Get<float>() * arg[1].Get<float>()));
            }
            else {
                printf("mul requires two arguments of the same type\n");
            }
        }
        else if (name == "div") {
            if (arg.size() != 2) {
                printf("div requires two arguments of the same type\n");
                return {};
            }
            if (arg[0].IsHolding<int>() && arg[1].IsHolding<int>()) {
                ret.push_back(VtValue(arg[0].Get<int>() / arg[1].Get<int>()));
            }
            else if (arg[0].IsHolding<float>() && arg[1].IsHolding<float>()) {
                ret.push_back(VtValue(arg[0].Get<float>() / arg[1].Get<float>()));
            }
            else {
                printf("div requires two arguments of the same type\n");
            }
        }
        else if (name == "mod") {
            if (arg.size() != 2) {
                printf("mod requires two arguments of the same type\n");
                return {};
            }
            if (arg[0].IsHolding<int>() && arg[1].IsHolding<int>()) {
                ret.push_back(VtValue(arg[0].Get<int>() % arg[1].Get<int>()));
            }
            else if (arg[0].IsHolding<float>() && arg[1].IsHolding<float>()) {
                ret.push_back(VtValue(fmod(arg[0].Get<float>(), arg[1].Get<float>())));
            }
            else {
                printf("mod requires two arguments of the same type\n");
            }
        }
        else if (name == "eq") {
            if (arg.size() != 2) {
                printf("eq requires two arguments of the same type\n");
                return {};
            }
            if (arg[0].IsHolding<int>() && arg[1].IsHolding<int>()) {
                ret.push_back(VtValue(arg[0].Get<int>() == arg[1].Get<int>()));
            }
            else if (arg[0].IsHolding<float>() && arg[1].IsHolding<float>()) {
                ret.push_back(VtValue(arg[0].Get<float>() == arg[1].Get<float>()));
            }
            else if (arg[0].IsHolding<std::string>() && arg[1].IsHolding<std::string>()) {
                ret.push_back(VtValue(arg[0].Get<std::string>() == arg[1].Get<std::string>()));
            }
            else {
                printf("eq requires two arguments of the same type\n");
            }
        }
        else if (name == "ne") {
            if (arg.size() != 2) {
                printf("ne requires two arguments of the same type\n");
                return {};
            }
            if (arg[0].IsHolding<int>() && arg[1].IsHolding<int>()) {
                ret.push_back(VtValue(arg[0].Get<int>() != arg[1].Get<int>()));
            }
            else if (arg[0].IsHolding<float>() && arg[1].IsHolding<float>()) {
                ret.push_back(VtValue(arg[0].Get<float>() != arg[1].Get<float>()));
            }
            else if (arg[0].IsHolding<std::string>() && arg[1].IsHolding<std::string>()) {
                ret.push_back(VtValue(arg[0].Get<std::string>() != arg[1].Get<std::string>()));
            }
            else {
                printf("ne requires two arguments of the same type\n");
            }
        }
        else if (name == "gt") {
            if (arg.size() != 2) {
                printf("gt requires two numeric arguments of the same type\n");
                return {};
            }
            if (arg[0].IsHolding<int>() && arg[1].IsHolding<int>()) {
                ret.push_back(VtValue(arg[0].Get<int>() > arg[1].Get<int>()));
            }
            else if (arg[0].IsHolding<float>() && arg[1].IsHolding<float>()) {
                ret.push_back(VtValue(arg[0].Get<float>() > arg[1].Get<float>()));
            }
            else {
                printf("gt requires two numeric arguments of the same type\n");
            }
        }
        else if (name == "lt") {
            if (arg.size() != 2) {
                printf("lt requires two numeric arguments of the same type\n");
                return {};
            }
            if (arg[0].IsHolding<int>() && arg[1].IsHolding<int>()) {
                ret.push_back(VtValue(arg[0].Get<int>() < arg[1].Get<int>()));
            }
            else if (arg[0].IsHolding<float>() && arg[1].IsHolding<float>()) {
                ret.push_back(VtValue(arg[0].Get<float>() < arg[1].Get<float>()));
            }
            else {
                printf("lt requires two numeric arguments of the same type\n");
            }
        }
        else if (name == "ge") {
            if (arg.size() != 2) {
                printf("ge requires two numeric arguments of the same type\n");
                return {};
            }
            if (arg[0].IsHolding<int>() && arg[1].IsHolding<int>()) {
                ret.push_back(VtValue(arg[0].Get<int>() >= arg[1].Get<int>()));
            }
            else if (arg[0].IsHolding<float>() && arg[1].IsHolding<float>()) {
                ret.push_back(VtValue(arg[0].Get<float>() >= arg[1].Get<float>()));
            }
            else {
                printf("ge requires two numeric arguments of the same type\n");
            }
        }
        else if (name == "le") {
            if (arg.size() != 2) {
                printf("le requires two numeric arguments of the same type\n");
                return {};
            }
            if (arg[0].IsHolding<int>() && arg[1].IsHolding<int>()) {
                ret.push_back(VtValue(arg[0].Get<int>() <= arg[1].Get<int>()));
            }
            else if (arg[0].IsHolding<float>() && arg[1].IsHolding<float>()) {
                ret.push_back(VtValue(arg[0].Get<float>() <= arg[1].Get<float>()));
            }
            else {
                printf("le requires two numeric arguments of the same type\n");
            }
        }
        else if (name == "print") {
            for (auto& i : arg) {
                if (i.IsHolding<int>()) {
                    printf("%d ", i.Get<int>());
                }
                else if (i.IsHolding<float>()) {
                    printf("%f ", i.Get<float>());
                }
                else if (i.IsHolding<std::string>()) {
                    printf("%s ", i.Get<std::string>().c_str());
                }
            }
        }
        else if (name == "connectMaterial") {
            // "(connectMaterial ${SCOPE}/cardMaterial ${SCOPE}/cardMaterial/PBRShader)",
            if (arg.size() != 2) {
                printf("connectMaterial requires two arguments\n");
                return {};
            }
            if (arg[0].IsHolding<std::string>() && arg[1].IsHolding<std::string>()) {
                std::string materialName = ExpandString(arg[0].Get<std::string>(), td);
                std::string shaderName = ExpandString(arg[1].Get<std::string>(), td);
                printf("Connecting %s to %s\n", materialName.c_str(), shaderName.c_str());
                // the python code to this is
                //             # material.CreateSurfaceOutput().ConnectToSource(pbrShader.ConnectableAPI(), "surface")
                // first, get the material prim
                try {
                    UsdPrim materialPrim = td.stage->GetPrimAtPath(SdfPath(materialName));
                    if (!materialPrim) {
                        printf("Could not find material prim: %s\n", materialName.c_str());
                        return {};
                    }
                    // then, get the shader prim
                    UsdPrim shaderPrim = td.stage->GetPrimAtPath(SdfPath(shaderName));
                    if (!shaderPrim) {
                        printf("Could not find shader prim: %s\n", shaderName.c_str());
                        return {};
                    }
                    // get the material's surface output
                    UsdShadeMaterial material(materialPrim);
                    UsdShadeOutput surfaceOutput = material.CreateSurfaceOutput();
                    // get the shader's connectable API
                    UsdShadeConnectableAPI connectableAPI(shaderPrim);
                    // connect the material's surface output to the shader's surface input
                    surfaceOutput.ConnectToSource(connectableAPI, TfToken("surface"));
                }
                catch (const std::exception& e) {
                    printf("Error connecting material: %s\n", e.what());
                }
            }
            else {
                printf("connectMaterial requires two string arguments\n");
            }
        }
        else if (name == "bindMaterial") {
            // (bindMaterial ${SCOPE}/cardMesh ${SCOPE}/cardMaterial})"
            // corresponds to
            // billboard.GetPrim().ApplyAPI(UsdShade.MaterialBindingAPI)
            // UsdShade.MaterialBindingAPI(billboard).Bind(material)
            if (arg.size() != 2) {
                printf("bindMaterial requires two arguments\n");
                return {};
            }
            if (arg[0].IsHolding<std::string>() && arg[1].IsHolding<std::string>()) {
                printf("Binding %s to %s\n", arg[0].Get<std::string>().c_str(), arg[1].Get<std::string>().c_str());
                try {
                    std::string meshName = ExpandString(arg[0].Get<std::string>(), td);
                    std::string materialName = ExpandString(arg[1].Get<std::string>(), td);
                    UsdPrim meshPrim = td.stage->GetPrimAtPath(SdfPath(meshName));
                    if (!meshPrim) {
                        printf("Could not find mesh prim: %s\n", meshName.c_str());
                        return {};
                    }
                    UsdPrim materialPrim = td.stage->GetPrimAtPath(SdfPath(materialName));
                    if (!materialPrim) {
                        printf("Could not find material prim: %s\n", materialName.c_str());
                        return {};
                    }
                    meshPrim.ApplyAPI<UsdShadeMaterialBindingAPI>();
                    auto binding = UsdShadeMaterialBindingAPI(meshPrim);
                    UsdShadeMaterial material(materialPrim);
                    binding.Bind(material);
                }
                catch (const std::exception& e) {
                    printf("Error binding material: %s\n", e.what());
                }
            }
            else {
                printf("bindMaterial requires two string arguments\n");
            }
        }
        else if (name == "connect") {
            // "(connect ${SCOPE}/cardMaterial/PBRShader diffuseColor ${SCOPE}/cardMaterial/diffuseTexture rgb f3)"
            // corresponds to
            // pbrShader.CreateInput("diffuseColor", Sdf.ValueTypeNames.Color3f).ConnectToSource(diffuseTextureSampler.ConnectableAPI(), 'rgb')
            // which results in this, in the usd file:
            // color3f inputs:diffuseColor.connect = </Card/cardMaterial/diffuseTexture.outputs:rgb>
            if (arg.size() != 5) {
                printf("connect requires five arguments\n");
                return {};
            }
            if (arg[0].IsHolding<std::string>() && arg[1].IsHolding<std::string>() &&
                arg[2].IsHolding<std::string>() && arg[3].IsHolding<std::string>() &&
                arg[4].IsHolding<std::string>()) {
                printf("Connecting %s.%s to %s.%s\n",
                        arg[0].Get<std::string>().c_str(), arg[1].Get<std::string>().c_str(),
                        arg[2].Get<std::string>().c_str(), arg[3].Get<std::string>().c_str());
                try {
                    std::string shaderName = ExpandString(arg[0].Get<std::string>(), td);
                    std::string shaderInputName = ExpandString(arg[1].Get<std::string>(), td);
                    std::string textureName = ExpandString(arg[2].Get<std::string>(), td);
                    std::string textureOutputName = ExpandString(arg[3].Get<std::string>(), td);
                    UsdPrim shaderPrim = td.stage->GetPrimAtPath(SdfPath(shaderName));
                    if (!shaderPrim) {
                        printf("Could not find shader prim: %s\n", shaderName.c_str());
                        return {};
                    }
                    UsdShadeConnectableAPI connectableAPI(shaderPrim);
                    SdfValueTypeName type;
                    std::string connectionType = arg[4].Get<std::string>();
                    if (connectionType == "c3") {
                        type = SdfValueTypeNames->Color3f;
                    }
                    else if (connectionType == "f2") {
                        type = SdfValueTypeNames->Float2;
                    }
                    else if (connectionType == "f3") {
                        type = SdfValueTypeNames->Float3;
                    }
                    else if (connectionType == "f4") {
                        type = SdfValueTypeNames->Float4;
                    }
                    else if (connectionType == "token") {
                        type = SdfValueTypeNames->Token;
                    }
                    else {
                        printf("Unknown type: %s\n", arg[4].Get<std::string>().c_str());
                        return {};
                    }
                    UsdShadeInput shaderInput = connectableAPI.CreateInput(TfToken(shaderInputName), type);
                    if (!shaderInput) {
                        printf("Could not find shader input: %s\n", shaderInputName.c_str());
                        return {};
                    }
                    UsdPrim texturePrim = td.stage->GetPrimAtPath(SdfPath(textureName));
                    if (!texturePrim) {
                        printf("Could not find texture prim: %s\n", textureName.c_str());
                        return {};
                    }
                    UsdShadeConnectableAPI textureConnectableAPI(texturePrim);
                    UsdShadeOutput textureOutput = textureConnectableAPI.GetOutput(TfToken(textureOutputName));
                    if (!textureOutput) {
                        UsdShadeInput textureInput = textureConnectableAPI.GetInput(TfToken(textureOutputName));
                        if (!textureInput) {
                            printf("Could not find texture output: %s\n", textureOutputName.c_str());
                            return {};
                        }
                        shaderInput.ConnectToSource(textureInput);
                    }
                    else
                        shaderInput.ConnectToSource(textureOutput);
                }
                catch (const std::exception& e) {
                    printf("Error connecting shader: %s\n", e.what());
                }
            }
            else {
                printf("connect requires five string arguments\n");
            }
        }
        else if (name == "setShaderInput") {
            // "(setShaderInput ${SCOPE}/cardMaterial/diffuseTexture file ${IMAGE_FILE})"
            // corresponds to
            // diffuseTextureSampler.CreateInput('file', Sdf.ValueTypeNames.Asset).Set("USDLogoLrg.png")
            // which results in this, in the usd file:
            // asset inputs:file = @USDLogoLrg_png@
            if (arg.size() != 3) {
                printf("setShaderInput requires three arguments\n");
                return {};
            }
            if (arg[0].IsHolding<std::string>() && arg[1].IsHolding<std::string>() &&
                arg[2].IsHolding<std::string>()) {
                printf("Setting %s.%s to %s\n",
                        arg[0].Get<std::string>().c_str(), arg[1].Get<std::string>().c_str(),
                        arg[2].Get<std::string>().c_str());
                try {
                    std::string shaderName = ExpandString(arg[0].Get<std::string>(), td);
                    std::string shaderInputName = ExpandString(arg[1].Get<std::string>(), td);
                    std::string value = ExpandString(arg[2].Get<std::string>(), td);
                    UsdPrim shaderPrim = td.stage->GetPrimAtPath(SdfPath(shaderName));
                    if (!shaderPrim) {
                        printf("Could not find shader prim: %s\n", shaderName.c_str());
                        return {};
                    }
                    UsdShadeConnectableAPI connectableAPI(shaderPrim);
                    SdfValueTypeName type = SdfValueTypeNames->String;
                    UsdShadeInput shaderInput = connectableAPI.CreateInput(TfToken(shaderInputName), type);
                    if (!shaderInput) {
                        printf("Could not find shader input: %s\n", shaderInputName.c_str());
                        return {};
                    }
                    shaderInput.Set(value);
                }
                catch (const std::exception& e) {
                    printf("Error setting shader input: %s\n", e.what());
                }
            }
            else {
                printf("setShaderInput requires three string arguments\n");
            }
        }
        else if (name == "computeExtent") {
            // "(computeExtent ${SCOPE}/cardMesh)"
            if (arg.size() != 1) {
                printf("computeExtent requires one argument\n");
                return {};
            }
            if (arg[0].IsHolding<std::string>()) {
                printf("Computing extent for %s\n", arg[0].Get<std::string>().c_str());
                try {
                    std::string meshName = ExpandString(arg[0].Get<std::string>(), td);
                    UsdPrim meshPrim = td.stage->GetPrimAtPath(SdfPath(meshName));
                    if (!meshPrim) {
                        printf("Could not find mesh prim: %s\n", meshName.c_str());
                        return {};
                    }
                    UsdGeomMesh mesh(meshPrim);
                    VtVec3fArray extent;

                    // fetch the points attribute and compute the extent
                    UsdAttribute pointsAttr = mesh.GetPointsAttr();
                    if (pointsAttr) {
                        VtVec3fArray points;
                        pointsAttr.Get(&points);
                        GfRange3f range;
                        for (auto& p : points) {
                            range.UnionWith(p);
                        }
                        extent.push_back(range.GetMin());
                        extent.push_back(range.GetMax());
                    }

                    // put the extent into the extent property on the mesh
                    UsdAttribute extentAttr = mesh.CreateExtentAttr();
                    extentAttr.Set(extent);
                }
                catch (const std::exception& e) {
                    printf("Error computing extent: %s\n", e.what());
                }
            }
            else {
                printf("computeExtent requires one string argument\n");
            }
        }
        else if (name == "createImageQuad") {
            // "(createImageQuad ${SCOPE}/cardMesh ${IMAGE_FILE})"
            if (arg.size() != 2) {
                printf("createImageQuad requires two arguments: mesh, image\n");
                return {};
            }
            if (arg[0].IsHolding<std::string>() && arg[1].IsHolding<std::string>()) {
                printf("Creating image quad for %s\n", arg[0].Get<std::string>().c_str());
                try {
                    std::string meshName = ExpandString(arg[0].Get<std::string>(), td);
                    std::string imageFile = ExpandString(arg[1].Get<std::string>(), td);
                    UsdPrim meshPrim = td.stage->GetPrimAtPath(SdfPath(meshName));
                    if (!meshPrim) {
                        printf("Could not find mesh prim: %s\n", meshName.c_str());
                        return {};
                    }

                    HioImageSharedPtr image = HioImage::OpenForReading(imageFile);
                    if (!image) {
                        printf("Could not open image file: %s\n", imageFile.c_str());
                        return {};
                    }

                    float w = image->GetWidth();
                    float h = image->GetHeight();
                    float w2 = w * 0.5f;
                    float h2 = h * 0.5f;

                    UsdGeomMesh mesh(meshPrim);
                    // create the points attribute
                    VtVec3fArray points;
                    points.push_back(GfVec3f(-w2, -h2, 0));
                    points.push_back(GfVec3f( w2, -h2, 0));
                    points.push_back(GfVec3f( w2,  h2, 0));
                    points.push_back(GfVec3f(-w2,  h2, 0));
                    UsdAttribute pointsAttr = mesh.CreatePointsAttr();
                    pointsAttr.Set(points);
                    // create the extent attribute
                    VtVec3fArray extent;
                    extent.push_back(GfVec3f(-w2, -h2, 0));
                    extent.push_back(GfVec3f( w2,  h2, 0));
                    UsdAttribute extentAttr = mesh.CreateExtentAttr();
                    extentAttr.Set(extent);
                    // create the st primvar
                    //st = UsdGeom.PrimvarsAPI(billboard).CreatePrimvar("st",
                    //                Sdf.ValueTypeNames.TexCoord2fArray,
                    //               UsdGeom.Tokens.varying)
                    UsdGeomPrimvarsAPI primvarAPI(mesh);
                    UsdGeomPrimvar primvar = primvarAPI.CreatePrimvar(TfToken("st"),
                                                                        SdfValueTypeNames->TexCoord2fArray,
                                                                        UsdGeomTokens->varying);
                    VtVec2fArray st;
                    st.push_back(GfVec2f(0, 0));
                    st.push_back(GfVec2f(1, 0));
                    st.push_back(GfVec2f(1, 1));
                    st.push_back(GfVec2f(0, 1));
                    primvar.Set(st);
                }
                catch (const std::exception& e) {
                    printf("Error creating mesh for image: %s\n", e.what());
                }
            }
        }
        else {
            printf("Unknown function %s\n", name.c_str());
        }
        return ret;
    }

    bool RunRecipe(tsParsedSexpr_t* curr, const TemplateData& td) {
        if (!curr || curr->token != tsSexprPushList) {
            return false;
        }
        using std::string;
        using std::vector;
        struct Atom {
            string name;
            size_t bottom;
        };
        std::vector<Atom> atom;
        std::vector<VtValue> stack;
        while (curr) {
            switch (curr->token) {
            case tsSexprAtom: {
                std::string name(curr->str.curr, curr->str.sz);
                // if name starts with ${ and ends with }, it's a variable
                if (name.size() > 3 && name[0] == '$' && name[1] == '{' && name[name.size() - 1] == '}') {
                    name = name.substr(2, name.size() - 3);
                    auto i = td.dict.find(name);
                    if (i != td.dict.end()) {
                        // if the VtValue is holding a string, set name to that string,
                        // otherwise push the value onto the stack.
                        if (i->second.IsHolding<std::string>()) {
                            name = i->second.Get<std::string>();
                        }
                        else {
                            stack.push_back(i->second);
                            break;
                        }
                    }
                }
                if (atom.back().name.empty()) {
                    atom.back().name = name;
                }
                else {
                    // if an atom doesn't follow a push list, its name is a string
                    stack.push_back(VtValue(name));
                }
            }
            break;

            case tsSexprPushList:
                atom.push_back(Atom());
                atom.back().bottom = stack.size();
                break;

            case tsSexprPopList: {
                if (!atom.size()) {
                    printf("unexpected pop list token\n");
                    return false;
                }
                if (atom.back().name.empty()) {
                    printf("expected atom before pop list\n");
                    return false;
                }
                TfSpan<VtValue> arg(stack.data() + atom.back().bottom, stack.size() - atom.back().bottom);
                std::vector<VtValue> ret = RunSexpr(atom.back().name, arg, td);
                stack.resize(atom.back().bottom);
                stack.insert(stack.end(), ret.begin(), ret.end());
                atom.pop_back();
            }
            break;

            case tsSexprInteger:
                stack.push_back(VtValue((int) curr->i));
                break;

            case tsSexprFloat:
                stack.push_back(VtValue((float) curr->f));
                break;

            case tsSexprString:
                // if name starts with ${ and ends with }, it's a variable
                if (curr->str.sz > 3 && curr->str.curr[0] == '$' && curr->str.curr[1] == '{' && curr->str.curr[curr->str.sz - 1] == '}') {
                    std::string name(curr->str.curr + 2, curr->str.sz - 3);
                    auto i = td.dict.find(name);
                    if (i != td.dict.end()) {
                        stack.push_back(i->second);
                            break;
                    }
                }
                stack.push_back(VtValue(std::string(curr->str.curr, curr->str.sz)));
                break;
            }
            curr = curr->next;
        }
        return true;
    }
};

UsdTemplater::UsdTemplater()
: self(new data()) {
    auto resource_path = std::string(lab_application_resource_path(nullptr, nullptr));
    std::string path = resource_path + "/LabSceneTemplate.usda";
    self->templateStage = pxr::UsdStage::Open(path);
    if (!self->templateStage) {
        printf("Failed to open the Scene Template\n");
    }
}

UsdTemplater::~UsdTemplater() {
    delete self;
}

PXR_NS::UsdPrim UsdTemplater::GetTemplatePrim(const std::string& path) {
    if (!self->templateStage)
        return {};

    return
        self->templateStage->GetPrimAtPath(pxr::SdfPath(path));
}

PXR_NS::UsdMetadataValueMap UsdTemplater::GetTemplateStageMetadata() {
    if (!self->templateStage)
        return {};

    return self->templateStage->GetPseudoRoot().GetAllAuthoredMetadata();
}

void UsdTemplater::InstantiateTemplate(int recursionLevel,
                            TemplateData& templateData,
                            UsdPrim templateRoot,
                            UsdPrim templateAnchor) {

    if (!templateRoot) {
        printf("No root provided for instantiation\n");
        return;
    }

    if (!self->templateStage) {
        printf("No template file\n");
        return;
    }

    std::string templateRootPath = templateRoot.GetPath().GetString();
    if (templateData.builtLayers.find(templateRootPath) != templateData.builtLayers.end()) {
        // early out if we've been here before
        return;
    }

    templateData.builtLayers.insert(templateRootPath);

    /*  ___     _  __ _ _ _   _____               _      _
        | __|  _| |/ _(_) | | |_   _|__ _ __  _ __| |__ _| |_ ___
        | _| || | |  _| | | |   | |/ -_) '  \| '_ \ / _` |  _/ -_)
        |_| \_,_|_|_| |_|_|_|   |_|\___|_|_|_| .__/_\__,_|\__\___|
                                            |_|*/

    printf("Recursing from %s\n", templateRootPath.c_str());
    UsdPrimRange range(templateRoot);

    printf("\n------\n");
    UsdPrimSiblingRange children = templateRoot.GetChildren();
    for (auto i : templateRoot.GetChildren()) {
        printf("%s\n", i.GetPath().GetText());
    }
    printf("------\n");

    for (auto it = range.begin(); it != range.end(); ++it) {
        UsdPrim templatePrim = *it;
        UsdPrim newPrim;

        struct Work {
            bool isLayer = false;
            VtArray<std::string> subLayers;
            VtArray<std::string> recipe;
            VtDictionary newCustomData;
            std::string docs;
            std::string subDirectory;

            void ParseCustomData(const VtDictionary& customData) {
                for (auto i : customData) {
                    const std::string& key = i.first;
                    if (key == "templateSubLayers") {
                        subLayers = i.second.Get<VtArray<std::string>>();
                    }
                    else if (key == "templateKind") {
                        auto kind = i.second.Get<std::string>();
                        if (kind == "layer") {
                            isLayer = true;
                        }
                    }
                    else if (key == "templateDirectory") {
                        subDirectory = i.second.Get<std::string>();
                    }
                    else if (key == "recipe") {
                        recipe = i.second.Get<VtArray<std::string>>();
                    }
                    else {
                        newCustomData.insert(i);
                    }
                }
            }
        };

        Work work;

        // fetch customData
        work.ParseCustomData(templatePrim.GetCustomData());
        work.docs = templatePrim.GetDocumentation();

        if (work.isLayer) {
            // for all the sublayers, change the names from foo.usda to foo_usda.
            // Then, find the corresponding prim at the templatePrim's path + the new name.
            // Recurse on that prim.
            // Do this first because they are going to be sublayered into the new stage.

            if (work.subLayers.size() == 0) {
                printf("{\n%s has no sublayers\n", templatePrim.GetPath().GetText());
            }
            else {
                printf("{\nprocessing %d sublayers for %s\n", (int) work.subLayers.size(), templatePrim.GetPath().GetText());
            }

            for (auto& subLayer : work.subLayers) {
                auto newSubLayer = subLayer;
                newSubLayer.replace(newSubLayer.find(".usda"), 5, "_usda");

                printf("~~~ %s ~~~~\n", templatePrim.GetPath().GetText());

                SdfPath path = templatePrim.GetPath().AppendPath(SdfPath(newSubLayer));
                printf("    %s     \n", path.GetText());
                auto subLayerPrim = self->templateStage->GetPrimAtPath(path);
                if (subLayerPrim) {
                    InstantiateTemplate(recursionLevel + 1, templateData,
                                              subLayerPrim, templateAnchor);
                }
                else {
                    printf("Could not find template prim: %s\n", path.GetText());
                }
            }

            // create a new stage for the layer
            // if the prim name ends in _usda, strip it off
            std::string primName = templatePrim.GetName().GetString();
            if (primName.find("_usda") != std::string::npos) {
                primName = primName.substr(0, primName.size() - 5);
            }
            std::string layerPath = templateData.rootDir + "/";
            if (work.subDirectory.length() > 0) {
                layerPath += work.subDirectory + "/";
            }
            TfMakeDirs(layerPath, 0700, true);

            layerPath += primName + ".usda";
            auto shotSubStage = UsdStage::CreateNew(layerPath);
            // copy the meta data
            SdfSpecHandle subshotRootSpec = shotSubStage->GetRootLayer()->GetPseudoRoot();
            self->CopyMetadata(subshotRootSpec, templateData.commonMetaData);

            // define a root prim, with a group kind; make it the root
            UsdPrim rootPrim = shotSubStage->DefinePrim(SdfPath("/Lab"));
            UsdModelAPI(rootPrim).SetKind(KindTokens->group);
            shotSubStage->SetDefaultPrim(rootPrim);
            rootPrim.SetDocumentation(work.docs);

            // add the sublayers
            auto layerPaths = shotSubStage->GetRootLayer()->GetSubLayerPaths();
            for (auto& subLayer : work.subLayers) {
                std::string layerName = (work.subDirectory.length() > 0)? work.subDirectory + "/" : "";
                layerName += subLayer;
                layerPaths.push_back(layerName);
            }

            for (auto& recipe : work.recipe) {
                tsParsedSexpr_t* sexpr = tsParsedSexpr_New();
                tsStrView_t recipeView = { recipe.c_str(), recipe.size() };
                tsStrView_t end = tsStrViewParseSexpr(&recipeView, sexpr, 0);
                if (end.sz > 0) {
                    printf("Error parsing recipe: %s\n", recipe.c_str());
                }
                else {
                    // the first node is an empty root atom.
                    if (sexpr && sexpr->next)
                        self->RunRecipe(sexpr->next, templateData);
                }
            }

            printf("\n}Saving %s\n", layerPath.c_str());

            // Export the stage to the new directory
            shotSubStage->Save();
        }
        else if (templateData.stage) {
            auto it = templateData.dict.find("SCOPE");
            if (it != templateData.dict.end()) {
                // the templateAnchor is the prim that the template was instantiated from.
                // scopes within the template are relative to the templateAnchor.
                // So, remove the templateAnchor path from the templatePrim path,
                // yielding the relative path of the scope we are currently copying.
                // Then, append the scope to the new prim path, and define the prim.
                std::string scope = it->second.Get<std::string>();
                std::string relativePath = templatePrim.GetPath().GetString();
                std::string anchorPath = templateAnchor.GetPath().GetString();
                if (relativePath.find(anchorPath) == 0) {
                    relativePath = relativePath.substr(anchorPath.size());
                }
                SdfPath newPath = SdfPath(scope);
                if (relativePath.size())
                    newPath = newPath.AppendPath(SdfPath(relativePath));
                newPrim = templateData.stage->DefinePrim(newPath);
                printf("~~Defining prim: %s\n", newPrim.GetPath().GetText());
            }
            else {
                // if SCOPE is not defined, then copy the prim at the same
                // path as the template prim.
                newPrim = templateData.stage->DefinePrim(templatePrim.GetPath());
            }
        }

        if (newPrim) {
            // Copy the metadata from the template prim to the new prim
            UsdMetadataValueMap primMetadata = templatePrim.GetAllAuthoredMetadata();

            // get the SdfSpecHandle for the newPrim
            SdfSpecHandle newPrimSpec = templateData.stage->GetRootLayer()->GetPrimAtPath(newPrim.GetPath());
            self->CopyMetadata(newPrimSpec, primMetadata);

            newPrim.SetCustomData(work.newCustomData);

            // iterate through the properties of the template prim
            // and copy each property to the new prim.
            auto props = templatePrim.GetProperties();
            for (auto prop : props) {
                if (prop.Is<UsdAttribute>()) {
                    UsdAttribute attr = prop.As<UsdAttribute>();
                    if (attr.IsAuthored()) {
                        UsdAttribute newAttr = newPrim.GetAttribute(attr.GetName());
                        if (newAttr) {
                            newAttr = attr;
                        }
                    }
                }
                else if (prop.Is<UsdRelationship>()) {
                    UsdRelationship rel = prop.As<UsdRelationship>();
                    if (rel.IsAuthored()) {
                        UsdRelationship newRel = newPrim.GetRelationship(rel.GetName());
                        if (newRel) {
                            SdfPathVector targets;
                            rel.GetTargets(&targets);
                            newRel.SetTargets(targets);
                        }
                    }
                }
            }

            // Get the SdfLayer for the source prim
            SdfLayerHandle sourceLayer = templatePrim.GetStage()->GetEditTarget().GetLayer();

            // Get the SdfLayer for the copied prim
            SdfLayerHandle targetLayer = templateData.stage->GetEditTarget().GetLayer();

            // Copy properties from source prim to copied prim using SdfLayer.
            // This copies the guts of the prim...
            SdfCopySpec(sourceLayer, templatePrim.GetPath(), targetLayer, newPrim.GetPath());
            printf("Copied %s to %s\n", templatePrim.GetPath().GetText(), newPrim.GetPath().GetText());

            for (auto& recipe : work.recipe) {
                tsParsedSexpr_t* sexpr = tsParsedSexpr_New();
                tsStrView_t recipeView = { recipe.c_str(), recipe.size() };
                tsStrView_t end = tsStrViewParseSexpr(&recipeView, sexpr, 0);
                if (end.sz > 0) {
                    printf("Error parsing recipe: %s\n", recipe.c_str());
                }
                else {
                    // the first node is an empty root atom.
                    if (sexpr && sexpr->next)
                        self->RunRecipe(sexpr->next, templateData);
                }
            }
        }
    }
}
