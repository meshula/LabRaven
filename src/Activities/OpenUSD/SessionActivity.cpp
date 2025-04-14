#include "SessionActivity.hpp"

#include "Providers/OpenUSD/OpenUSDProvider.hpp"

#include "imgui.h"
#include "imgui_internal.h"

#include <TextEditor.h>

#include <pxr/imaging/hd/tokens.h>
#include <pxr/usd/usdGeom/camera.h>
#include <pxr/usd/usdGeom/capsule.h>
#include <pxr/usd/usdGeom/cone.h>
#include <pxr/usd/usdGeom/cube.h>
#include <pxr/usd/usdGeom/cylinder.h>
#include <pxr/usd/usdGeom/metrics.h>
#include <pxr/usd/usdGeom/plane.h>
#include <pxr/usd/usdGeom/sphere.h>
#include <pxr/usdImaging/usdImaging/sceneIndices.h>

#include <fstream>
#include <string>

using std::string;
PXR_NAMESPACE_USING_DIRECTIVE

namespace lab {

/**
 * @brief UsdSessionLayer view acts as a text editor for the session layer of
 * the current UsdStage. It allows to preview and edit the USD session layer.
 *
 */
class UsdSessionLayer {
    std::string _label;
    bool _wasFocused;
    bool _wasHovered;
    bool _wasDisplayed;
    ImRect _innerRect;
    ImVec2 _prevMousePos;

public:
    inline static const std::string VIEW_TYPE = "Session Layer";

    /**
     * @brief Construct a new UsdSessionLayer object
     *
     * @param model the Model of the new UsdSessionLayer view
     * @param label the ImGui label of the new UsdSessionLayer view
     */
    UsdSessionLayer(const std::string label = VIEW_TYPE);

    /**
     * @brief Override of the View::_GetGizmoWindowFlags
     *
     */
    ImGuiWindowFlags _GetGizmoWindowFlags();

    void SetStage(pxr::UsdStageRefPtr stage);


    void Update(const LabViewInteraction& vi)
    {
        ImGuiIO& io = ImGui::GetIO();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
        ImGui::Begin(_label.c_str(), &_wasDisplayed, _GetGizmoWindowFlags());

        // update focus state
        if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows |
                                   ImGuiFocusedFlags_RootWindow)) {
            if (!_wasFocused) {
                _FocusInEvent();
                _wasFocused = true;
            }
        }
        else if (_wasFocused) {
            _FocusOutEvent();
            _wasFocused = false;
        }

        if (_wasFocused) {
        }

        _Draw(vi);

        // update inner rect size
        _innerRect = ImGui::GetCurrentWindow()->InnerRect;
        auto mn = _innerRect.Min;
        auto mx = _innerRect.Max;

        //bool is_clicked = ImGui::IsItemClicked();
        //bool is_active = ImGui::IsItemActive();
        bool is_hovered = ImGui::IsItemHovered();
        if (is_hovered) {
            //static int foo = 0;
            //printf("hovered %d\n", foo++);
        }

        if (is_hovered) {
        //if (ImGui::IsMouseHoveringRect(_innerRect.Min, _innerRect.Max)) {
            if (!_wasHovered) {
                _wasHovered = true;
            }
        }
        else if (_wasHovered) {
            _wasHovered = false;
        }

        if (_wasHovered) {
            // update mouse pos
            ImVec2 appMousePos = ImGui::GetMousePos();
            ImVec2 viewMousePos = appMousePos - _innerRect.Min;

            _prevMousePos = viewMousePos;
        }

        ImGui::End();
        ImGui::PopStyleVar(2);
    };

private:
    TextEditor _editor;
    bool _isEditing;
    std::string _lastLoadedText;
    ImGuiWindowFlags _gizmoWindowFlags;

    /**
     * @brief Override of the View::Draw
     *
     */
    void _Draw(const LabViewInteraction& vi);

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
    std::string _GetNextAvailableIndexedPath(const std::string& primPath);

    /**
     * @brief Create a new prim to the current state
     *
     * @param primType the type of the prim to create
     */
    void _CreatePrim(pxr::TfToken primType);

    /**
     * @brief Get a Palette object for the TextEditor (ImGui plugin)
     *
     * @return TextEditor::Palette
     */
    TextEditor::Palette _GetPalette();

    /**
     * @brief Get the Usd Language Definition object for the TextEditor
     * (ImGui plugin)
     *
     * @return TextEditor::LanguageDefinition
     */
    TextEditor::LanguageDefinition* _GetUsdLanguageDefinition();

    /**
     * @brief Override of the View::_FocusInEvent
     *
     */
    void _FocusInEvent();

    /**
     * @brief Override of the View::_FocusOutEvent
     *
     */
    void _FocusOutEvent();
};


UsdSessionLayer::UsdSessionLayer(const string label)
: _label(label), _isEditing(false)
{
    _gizmoWindowFlags = ImGuiWindowFlags_MenuBar;
    
    _editor.SetPalette(_GetPalette());
    _editor.SetLanguageDefinition(_GetUsdLanguageDefinition());
    _editor.SetShowWhitespaces(false);
}


ImGuiWindowFlags UsdSessionLayer::_GetGizmoWindowFlags()
{
    return _gizmoWindowFlags;
};

void UsdSessionLayer::_Draw(const LabViewInteraction& vi)
{
    if (ImGui::BeginMenuBar()) {
#ifdef HAVE_IMGUIFD
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("New scene")) { _SetEmptyStage(); }
            
            if (ImGui::MenuItem("Load ...")) {
                ImGuiFileDialog::Instance()->OpenDialog(
                                                        "LoadFile", "Choose File", ".usd,.usdc,.usda,.usdz", ".");
            }
            
            if (ImGui::MenuItem("Export to ...")) {
                ImGuiFileDialog::Instance()->OpenDialog(
                                                        "ExportFile", "Choose File", ".usd,.usdc,.usda,.usdz",
                                                        ".");
            }
            ImGui::EndMenu();
        }
#endif
        if (ImGui::BeginMenu("Objects")) {
            if (ImGui::BeginMenu("Create")) {
                if (ImGui::MenuItem("Camera"))
                    _CreatePrim(HdPrimTypeTokens->camera);
                ImGui::Separator();
                if (ImGui::MenuItem("Capsule"))
                    _CreatePrim(HdPrimTypeTokens->capsule);
                if (ImGui::MenuItem("Cone"))
                    _CreatePrim(HdPrimTypeTokens->cone);
                if (ImGui::MenuItem("Cube"))
                    _CreatePrim(HdPrimTypeTokens->cube);
                if (ImGui::MenuItem("Cylinder"))
                    _CreatePrim(HdPrimTypeTokens->cylinder);
                if (ImGui::MenuItem("Sphere"))
                    _CreatePrim(HdPrimTypeTokens->sphere);
                ImGui::EndMenu();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    
    if (_IsUsdSessionLayerUpdated())
        _LoadSessionTextFromModel();

    _editor.Render("TextEditor");
};

string UsdSessionLayer::_GetNextAvailableIndexedPath(const string& primPath)
{
    auto usd = OpenUSDProvider::instance();
    auto stage = usd->Stage();
    UsdPrim prim;
    int i = -1;
    string newPath;
    do {
        i++;
        if (i == 0) newPath = primPath;
        else newPath = primPath + std::to_string(i);
        prim = stage->GetPrimAtPath(SdfPath(newPath));
    } while (prim.IsValid());
    return newPath;
}

void UsdSessionLayer::_CreatePrim(TfToken primType)
{
    string primPath = _GetNextAvailableIndexedPath("/" + primType.GetString());

    auto usd = OpenUSDProvider::instance();
    auto stage = usd->Stage();

    if (primType == HdPrimTypeTokens->camera) {
        auto cam = UsdGeomCamera::Define(stage, SdfPath(primPath));
        cam.CreateFocalLengthAttr(VtValue(18.46f));
    }
    else {
        UsdGeomGprim prim;
        if (primType == HdPrimTypeTokens->capsule)
            prim = UsdGeomCapsule::Define(stage, SdfPath(primPath));
        if (primType == HdPrimTypeTokens->cone)
            prim = UsdGeomCone::Define(stage, SdfPath(primPath));
        if (primType == HdPrimTypeTokens->cube)
            prim = UsdGeomCube::Define(stage, SdfPath(primPath));
        if (primType == HdPrimTypeTokens->cylinder)
            prim = UsdGeomCylinder::Define(stage, SdfPath(primPath));
        if (primType == HdPrimTypeTokens->sphere)
            prim = UsdGeomSphere::Define(stage, SdfPath(primPath));

        VtVec3fArray extent({{-1, -1, -1}, {1, 1, 1}});
        prim.CreateExtentAttr(VtValue(extent));
        
        VtVec3fArray color({{.5f, .5f, .5f}});
        prim.CreateDisplayColorAttr(VtValue(color));
    }
}

bool UsdSessionLayer::_IsUsdSessionLayerUpdated()
{
    auto usd = OpenUSDProvider::instance();
    auto sl = usd->GetSessionLayer();
    string layerText;
    sl->ExportToString(&layerText);
    return _lastLoadedText != layerText;
}

void UsdSessionLayer::_LoadSessionTextFromModel()
{
    auto usd = OpenUSDProvider::instance();
    auto sl = usd->GetSessionLayer();
    string layerText;
    sl->ExportToString(&layerText);
    _editor.SetText(layerText);
    _lastLoadedText = layerText;
}

void UsdSessionLayer::_SaveSessionTextToModel()
{
    auto usd = OpenUSDProvider::instance();
    auto sl = usd->GetSessionLayer();
    string editedText = _editor.GetText();
    sl->ImportFromString(editedText.c_str());
}

TextEditor::Palette UsdSessionLayer::_GetPalette()
{
    ImU32 normalTxtCol = ImGui::GetColorU32(ImGuiCol_Text, 1.f);
    ImU32 halfTxtCol = ImGui::GetColorU32(ImGuiCol_Text, .5f);
    ImU32 strongKeyCol = ImGui::GetColorU32(ImGuiCol_HeaderActive, 1.f);
    ImU32 lightKeyCol = ImGui::GetColorU32(ImGuiCol_HeaderActive, .2f);
    ImU32 transparentCol = IM_COL32_BLACK_TRANS;
    
    return {{
        normalTxtCol,    // None
        strongKeyCol,    // Keyword
        strongKeyCol,    // Number
        strongKeyCol,    // String
        strongKeyCol,    // Char literal
        normalTxtCol,    // Punctuation
        normalTxtCol,    // Preprocessor
        normalTxtCol,    // Identifier
        strongKeyCol,    // Known identifier
        normalTxtCol,    // Preproc identifier
        halfTxtCol,      // Comment (single line)
        halfTxtCol,      // Comment (multi line)
        transparentCol,  // Background
        normalTxtCol,    // Cursor
        lightKeyCol,     // Selection
        normalTxtCol,    // ErrorMarker
        normalTxtCol,    // Breakpoint
        normalTxtCol,    // Line number
        transparentCol,  // Current line fill
        transparentCol,  // Current line fill (inactive)
        transparentCol,  // Current line edge
    }};
}

TextEditor::LanguageDefinition* UsdSessionLayer::_GetUsdLanguageDefinition()
{
    static TextEditor::LanguageDefinition langDef =
    TextEditor::LanguageDefinition::C();
    
    static bool init = true;
    if (!init)
        return &langDef;
    
    const char* const descriptors[] = {
        "add",    "append", "prepend", "del",
        "delete", "custom", "uniform", "rel"};
    const char* const keywords[] = {
        "references",
        "payload",
        "defaultPrim",
        "doc",
        "subLayers",
        "specializes",
        "active",
        "assetInfo",
        "hidden",
        "kind",
        "inherits",
        "instanceable",
        "customData",
        "variant",
        "variants",
        "variantSets",
        "config",
        "connect",
        "default",
        "dictionary",
        "displayUnit",
        "nameChildren",
        "None",
        "offset",
        "permission",
        "prefixSubstitutions",
        "properties",
        "relocates",
        "reorder",
        "rootPrims",
        "scale",
        "suffixSubstitutions",
        "symmetryArguments",
        "symmetryFunction",
        "timeSamples"};
    
    const char* const dataTypes[] = {
        "bool",       "uchar",      "int",        "int64",      "uint",
        "uint64",     "int2",       "int3",       "int4",       "half",
        "half2",      "half3",      "half4",      "float",      "float2",
        "float3",     "float4",     "double",     "double2",    "double3",
        "double4",    "string",     "token",      "asset",      "matrix2d",
        "matrix3d",   "matrix4d",   "quatd",      "quatf",      "quath",
        "color3d",    "color3f",    "color3h",    "color4d",    "color4f",
        "color4h",    "normal3d",   "normal3f",   "normal3h",   "point3d",
        "point3f",    "point3h",    "vector3d",   "vector3f",   "vector3h",
        "frame4d",    "texCoord2d", "texCoord2f", "texCoord2h", "texCoord3d",
        "texCoord3f", "texCoord3h"};
    
    const char* const schemas[] = {
        "Xform", "Scope", "Shader", "Sphere",   "Subdiv",         "Camera",
        "Cube",  "Curve", "Mesh",   "Material", "PointInstancer", "Plane"};
    
    const char* const specifiers[] = {"def", "over", "class", "variantSet"};
    
    for (auto& k : keywords) langDef.mKeywords.insert(k);
    for (auto& k : dataTypes) langDef.mKeywords.insert(k);
    for (auto& k : schemas) langDef.mKeywords.insert(k);
    for (auto& k : descriptors) langDef.mKeywords.insert(k);
    
    langDef.mTokenRegexStrings.push_back(
                std::make_pair<std::string, TextEditor::PaletteIndex>(
                        "L?\\\"(\\\\.|[^\\\"])*\\\"", TextEditor::PaletteIndex::String));
    
    langDef.mTokenRegexStrings.push_back(
                std::make_pair<std::string, TextEditor::PaletteIndex>(
                        "[+-]?([0-9]+([.][0-9]*)?|[.][0-9]+)([eE][+-]?[0-9]+)?[fF]?",
                                                        TextEditor::PaletteIndex::Number));
    
    langDef.mCommentStart = "/*";
    langDef.mCommentEnd = "*/";
    langDef.mSingleLineComment = "#";
    
    langDef.mCaseSensitive = true;
    langDef.mAutoIndentation = true;
    
    langDef.mName = "USD";
    
    return &langDef;
}

void UsdSessionLayer::_FocusInEvent()
{
    _isEditing = true;
};
void UsdSessionLayer::_FocusOutEvent()
{
    _isEditing = false;
    _SaveSessionTextToModel();
};


struct SessionActivity::data {
    data() {
        sessionLayer = std::unique_ptr<UsdSessionLayer>(new UsdSessionLayer());
    }
    ~data() = default;

    std::unique_ptr<UsdSessionLayer> sessionLayer;
};

SessionActivity::SessionActivity() : Activity(SessionActivity::sname()) {
    _self = new SessionActivity::data();

    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<SessionActivity*>(instance)->RunUI(*vi);
    };
}

SessionActivity::~SessionActivity() {
    delete _self;
}

void SessionActivity::RunUI(const LabViewInteraction& vi) {
    auto usd = OpenUSDProvider::instance();
    auto stage = usd->Stage();
    if (!stage) {
        ImGui::SetNextWindowSize(ImVec2(200, 400), ImGuiCond_FirstUseEver);
        ImGui::Begin("Session Editor##A1");
        ImGui::TextUnformatted("Session Editor: No stage opened yet.");
        ImGui::End();
    }
    else {
        ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
        _self->sessionLayer->Update(vi);
    }
}

} // lab
