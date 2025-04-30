#include "HydraPropertiesActivity.hpp"
#include "imgui.h"
#include "ImGuizmo.h"
#include "Activities/Color/UIElements.hpp"
#include "Activities/OpenUSD/HydraActivity.hpp"

#include "Providers/OpenUSD/OpenUSDProvider.hpp"
#include "Providers/OpenUSD/UsdUtils.hpp"
#include "Providers/OpenUSD/sceneindices/colorfiltersceneindex.h"
#include "Providers/Selection/SelectionProvider.hpp"
#include "Lab/CoreProviders/Color/nanocolor.h"


#include <pxr/base/gf/matrix4d.h>
#include <pxr/base/gf/matrix4f.h>
#include <pxr/base/vt/value.h>
#include <pxr/imaging/hd/overlayContainerDataSource.h>
#include <pxr/imaging/hd/primvarSchema.h>
#include <pxr/imaging/hd/primvarsSchema.h>
#include <pxr/imaging/hd/retainedDataSource.h>
#include <pxr/imaging/hd/tokens.h>
#include <pxr/usd/usdGeom/camera.h>
#include <pxr/usd/usdGeom/gprim.h>
#include <pxr/usd/usdGeom/metrics.h>

#include <sstream>

PXR_NAMESPACE_USING_DIRECTIVE

namespace lab {

    /**
 * @class Editor
 * @brief Editor view that acts as an attribute editor. It allows to tune
 * attributes from selected UsdPrim.
 *
 */
class Editor {
    std::string _label;
    public:
        inline static const std::string VIEW_TYPE = "Editor";
        
        /**
         * @brief Construct a new Editor object
         *
         * @param model the Model of the new Editor view
         * @param label the ImGui label of the new Editor view
         */
        Editor(const std::string label = VIEW_TYPE);
  
    /**
     * @brief Override of the View::Draw
     *
     */
    void _Draw(const LabViewInteraction& vi);
    private:
        PXR_NS::SdfPath _prevSelection;
        PXR_NS::ColorFilterSceneIndexRefPtr _colorFilterSceneIndex;
        
        
        /**
         * @brief Get the SdfPath of the prim to display the attributes in the
         * editor.
         *
         * @return The current prim path to display the attributes from
         */
        PXR_NS::SdfPath _GetPrimToDisplay();
        
        /**
         * @brief Append the display color attributes of the given prim path to
         * the editor view
         *
         * @param primPath the SdfPath to get the display color attributes from
         */
        void _AppendDisplayColorAttr(PXR_NS::SdfPath primPath);
        
        /**
         * @brief Append the data source content to the editor view
         *
         * @param containerDataSource the container data source to display
         */
        void _AppendDataSourceAttrs(
                                    PXR_NS::HdContainerDataSourceHandle containerDataSource);
        
        /**
         * @brief Append the display color attributes of the given prim
         * path to the editor view
         *
         * @param primPath the SdfPath to get the display color attributes
         * from
         */
        void _AppendAllPrimAttrs(PXR_NS::SdfPath primPath);
    };


Editor::Editor(const std::string label)
: _label(label)
{
}


void Editor::_Draw(const LabViewInteraction& vi)
{
    SdfPath primPath = _GetPrimToDisplay();

    if (primPath.IsEmpty()) {
        return;
    }

    if (false && !_colorFilterSceneIndex) {
        lab::Orchestrator* mm =lab::Orchestrator::Canonical();
        std::weak_ptr<HydraActivity> hact;
        auto hydra = mm->LockActivity(hact);
        HdSceneIndexBaseRefPtr editableSceneIndex = hydra->GetEditableSceneIndex();
        _colorFilterSceneIndex = ColorFilterSceneIndex::New(editableSceneIndex);
        hydra->SetEditableSceneIndex(_colorFilterSceneIndex);
    }

    _AppendDisplayColorAttr(primPath);
    _AppendAllPrimAttrs(primPath);
}

SdfPath Editor::_GetPrimToDisplay()
{
    lab::Orchestrator* mm =lab::Orchestrator::Canonical();
    std::weak_ptr<HydraActivity> hact;
    auto hydra = mm->LockActivity(hact);
    SdfPathVector primPaths = hydra->GetHdSelection();

    if (primPaths.size() > 0 && !primPaths[0].IsEmpty())
        _prevSelection = primPaths[0];

    return _prevSelection;
}

void Editor::_AppendDisplayColorAttr(SdfPath primPath)
{
    if (!_colorFilterSceneIndex) {
        return;
    }
    GfVec3f color = _colorFilterSceneIndex->GetDisplayColor(primPath);

    if (color == GfVec3f(-1.f)) {
        return;
    }

    // save the values before change
    GfVec3f prevColor = GfVec3f(color);

    float* data = color.data();
    if (ImGui::CollapsingHeader("Display Color"))
        ImGui::SliderFloat3("", data, 0, 1);

    // add opinion only if values change
    if (color != prevColor)
        _colorFilterSceneIndex->SetDisplayColor(primPath, color);
}

void Editor::_AppendDataSourceAttrs(HdContainerDataSourceHandle containerDataSource)
{
    for (auto&& token : containerDataSource->GetNames()) {
        auto dataSource = containerDataSource->Get(token);
        const char* tokenText = token.GetText();

        auto containerDataSource =
        HdContainerDataSource::Cast(dataSource);
        if (containerDataSource) {
            bool clicked =
            ImGui::TreeNodeEx(tokenText, ImGuiTreeNodeFlags_OpenOnArrow);

            if (clicked) {
                _AppendDataSourceAttrs(containerDataSource);
                ImGui::TreePop();
            }
        }

        auto sampledDataSource = HdSampledDataSource::Cast(dataSource);
        if (sampledDataSource) {
            ImGui::Columns(2);
            VtValue value = sampledDataSource->GetValue(0);
            ImGui::Text("%s", tokenText);
            ImGui::NextColumn();
            ImGui::BeginChild(tokenText, ImVec2(0, 14), false);
            std::stringstream ss;
            ss << value;
            ImGui::Text("%s", ss.str().c_str());
            ImGui::EndChild();
            ImGui::Columns();
        }
    }
}

void Editor::_AppendAllPrimAttrs(SdfPath primPath)
{
    lab::Orchestrator* mm =lab::Orchestrator::Canonical();
    std::weak_ptr<HydraActivity> hact;
    auto hydra = mm->LockActivity(hact);

    HdSceneIndexPrim prim = hydra->GetFinalSceneIndex()->GetPrim(primPath);
    if (!prim.dataSource)
        return;

    TfTokenVector tokens = prim.dataSource->GetNames();

    if (tokens.size() < 1)
        return;

    if (ImGui::CollapsingHeader("Hd Prim attributes")) {
        _AppendDataSourceAttrs(prim.dataSource);
    }
}


struct HydraPropertiesActivity::data {
    data() = default;
    ~data() = default;

    // hd properties editor
    std::unique_ptr<Editor> editor;
};

HydraPropertiesActivity::HydraPropertiesActivity() : Activity(HydraPropertiesActivity::sname()) {
    _self = new HydraPropertiesActivity::data();
    _self->editor = std::unique_ptr<Editor>(
                            new Editor("Hd Properties Editor##A1"));

    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<HydraPropertiesActivity*>(instance)->RunUI(*vi);
    };
}

HydraPropertiesActivity::~HydraPropertiesActivity() {
    delete _self;
}

void HydraPropertiesActivity::RunUI(const LabViewInteraction& vi) {
    if (!this->UIVisible()) {
        return;
    }

    auto usd = OpenUSDProvider::instance();

    if (this->UIVisible()) {
        // make a window 800, 600
        if (_self->editor) {
            ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
            ImGui::Begin("Prim Properties##A2");
            _self->editor->_Draw(vi);
            ImGui::End();
        }
        else {
            ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_FirstUseEver);
            ImGui::Begin("Prim Properties##A2");
            ImGui::Text("No USD stage loaded.");
            ImGui::End();
        }
    }
}

} // lab
