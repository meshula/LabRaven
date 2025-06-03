//
//  OutlinerActivity.cpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 12/16/23.
//  Copyright © 2023 Nick Porcino. All rights reserved.
//

#include "UsdOutlinerActivity.hpp"

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui.h"
#include "Lab/App.h"
#include "Lab/StudioCore.hpp"
#include "Lab/ImguiExt.hpp"
#include "Activities/OpenUSD/HydraActivity.hpp"
#include "Providers/Assets/AssetsProvider.hpp"
#include "Providers/Selection/SelectionProvider.hpp"
#include "Providers/OpenUSD/OpenUSDProvider.hpp"
#include "Providers/OpenUSD/UsdUtils.hpp"
#include "Providers/Sprite/SpriteProvider.hpp"
#include <pxr/usd/usd/primFlags.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/pcp/layerStack.h>
#include <pxr/usd/usd/primRange.h>
#include <pxr/usd/usdGeom/gprim.h>
#include <pxr/usd/usd/primCompositionQuery.h>

#include "iconfontcppheaders/IconsFontAwesome5.h"

#include <functional>
#include <pxr/usd/sdf/attributeSpec.h>
#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/usd/stage.h>

#include <iostream>

/// At the moment we use a HdSelection for the stages but this will change as the we want to store additional states
/// The selection implementation will move outside this header
#include <pxr/imaging/hd/selection.h>

//#include "usdtweak/src/Selection.h"
//#include "usdtweak/src/widgets/StageOutliner.h"

#include <vector>

// on Apple the LabCreateTextues come from the MetalProvider
#if defined(__APPLE__)
#include "Lab/CoreProviders/Metal/MetalProvider.hpp"
#endif

PXR_NAMESPACE_USING_DIRECTIVE

/// @todo
// when selection changes, update the outliner by closing everything and
// opening tree nodes that are in the selection path
// https://github.com/ocornut/imgui/issues/1131

namespace {
    lab::SpriteProvider::Frame gGhost[2];
    void* gGhostTextures[2];
}



/// A hash type used to track changes in selection state
using SelectionHash = std::size_t;

/// \class Selection
/// \brief Manages selection state for USD layers and stages.
///
/// This API is designed to support selection operations in a UI or editor context.
/// It abstracts over the specifics of USD types (SdfLayer, UsdStage) and handles
/// both prim and property selection. The interface is templated to support both layers
/// and stages as selection "owners".
struct Selection {
    Selection();
    ~Selection();

    /// \brief Clear all selections for a given owner (layer or stage).
    /// \tparam OwnerT Can be SdfLayerRefPtr, TfWeakPtr<SdfLayer>, UsdStageRefPtr, etc.
    /// \param owner The layer or stage whose selection is being cleared.
    template <typename OwnerT> void Clear(const OwnerT &owner);

    /// \brief Add a single selected item to the selection domain of the owner.
    /// \param owner The layer or stage where the selection applies.
    /// \param path The path of the item to select.
    template <typename OwnerT> void AddSelected(const OwnerT &owner, const SdfPath &path);

    /// \brief Remove a selected item from the owner’s selection.
    /// \note Not implemented for all types.
    template <typename OwnerT> void RemoveSelected(const OwnerT &owner, const SdfPath &path);

    /// \brief Replace current selection with a new single item.
    /// \param owner The layer or stage.
    /// \param path The item to be selected exclusively.
    template <typename OwnerT> void SetSelected(const OwnerT &owner, const SdfPath &path);

    /// \brief Check if the selection is empty for a given owner.
    template <typename OwnerT> bool IsSelectionEmpty(const OwnerT &owner) const;

    /// \brief Check if a specific path is selected in the given owner.
    template <typename OwnerT> bool IsSelected(const OwnerT &owner, const SdfPath &path) const;

    /// \brief Check whether a given SdfPrimSpec or SdfAttributeSpec is selected.
    /// \note This is only valid for items already known to belong to the currently active layer.
    /// Ownership context is inferred from the stored selection domains.
    ///
    /// Prefer using IsSelected(owner, path) when working at the path level.
    template <typename ItemT> bool IsSelected(const ItemT &item) const;

    /// \brief Update and compare the selection hash to detect changes.
    /// \returns true if the selection has changed since the last hash.
    template <typename OwnerT> bool UpdateSelectionHash(const OwnerT &owner, SelectionHash &lastSelectionHash);

    /// \brief Return one selected prim path, typically used as a reference or "anchor" for UI actions.
    /// For stages: this is usually the first selected prim in the order reported by HdSelection.
    /// For layers: this is an arbitrary selected prim path (unordered).
    template <typename OwnerT> SdfPath GetAnchorPrimPath(const OwnerT &owner) const;

    /// \brief Return one selected property path, if any exist.
    /// For layers: this is an arbitrary selected property path (unordered).
    template <typename OwnerT> SdfPath GetAnchorPropertyPath(const OwnerT &owner) const;

    /// \brief Retrieve all selected paths in the owner.
    template <typename OwnerT> std::vector<SdfPath> GetSelectedPaths(const OwnerT &owner) const;

    /// \brief Internal selection data structure. Not meant to be accessed directly.
    struct SelectionData;
    SelectionData *_data;
};

namespace std {
/// Note the following bit of code might have terrible performances when selecting thousands of paths
/// This has to be tested at some point
template <> struct hash<SdfPathVector> {
    std::size_t operator()(SdfPathVector const &paths) const noexcept {
        std::size_t hashValue = 0;
        for (const auto p : paths) {
            const auto pathHash = SdfPath::Hash{}(p);
            hashValue = pathHash ^ (hashValue << 1);
        }
        return hashValue;
    }
};

template <> struct hash<SdfSpecHandle> {
    std::size_t operator()(SdfSpecHandle const &spec) const noexcept { return hash_value(spec); }
};
} // std

struct StageSelection : public std::unique_ptr<HdSelection> {
    // We store a state to know if the selection has changed between frames. Recomputing the hash is not ideal and potentially
    // a performance killer, so it will likely change in the future
    SelectionHash selectionHash = 0;
    bool mustRecomputeHash = true;
};


struct Selection::SelectionData {
    // Selection data for the layers
    // Instead of keeping selected path for the layers, we keep handles as the paths can change when the prims are renamed or
    // moved and it invalidates the selection. The handles on spec stays consistent with renaming and moving
    std::unordered_set<SdfSpecHandle> _sdfPrimSelectionDomain;

    std::unordered_set<SdfSpecHandle> _sdfPropSelectionDomain;

    // Selection data for the stages
    // TODO: keep an anchor, basically the first selected prim, we would need a list to keep the order
    StageSelection _stageSelection;
};

Selection::Selection() { _data = new SelectionData(); }

Selection::~Selection() { delete _data; }

template <> void Selection::Clear(const SdfLayerRefPtr &layer) {
    if (!_data || !layer)
        return;
    _data->_sdfPrimSelectionDomain.clear();
}

template <> void Selection::Clear(const UsdStageRefPtr &stage) {
    if (!_data || !stage)
        return;
    _data->_stageSelection.reset(new HdSelection());
    _data->_stageSelection.mustRecomputeHash = true;
}

// Layer add a selection
template <> void Selection::AddSelected(const SdfLayerRefPtr &layer, const SdfPath &selectedPath) {
    if (!_data || !layer)
        return;
    if (selectedPath.IsPropertyPath()) {
        _data->_sdfPropSelectionDomain.insert(layer->GetObjectAtPath(selectedPath));
    } else {
        _data->_sdfPrimSelectionDomain.insert(layer->GetObjectAtPath(selectedPath));
    }
}

#define ImplementStageAddSelected(StageT)                                                                                        \
    template <> void Selection::AddSelected(const StageT &stage, const SdfPath &selectedPath) {                                  \
        if (!_data || !stage)                                                                                                    \
            return;                                                                                                              \
        if (!_data->_stageSelection) {                                                                                           \
            _data->_stageSelection.reset(new HdSelection());                                                                     \
        }                                                                                                                        \
        _data->_stageSelection->AddRprim(HdSelection::HighlightModeSelect, selectedPath);                                        \
        _data->_stageSelection.mustRecomputeHash = true;                                                                         \
    }

ImplementStageAddSelected(UsdStageRefPtr);
ImplementStageAddSelected(UsdStageWeakPtr);

// Not called at the moment
template <> void Selection::RemoveSelected(const UsdStageWeakPtr &stage, const SdfPath &path) {
    if (!_data || !stage)
        return;
}

#define ImplementLayerSetSelected(LayerT)                                                                                        \
    template <> void Selection::SetSelected(const LayerT &layer, const SdfPath &selectedPath) {                                  \
        if (!_data || !layer)                                                                                                    \
            return;                                                                                                              \
        _data->_sdfPrimSelectionDomain.clear();                                                                                  \
        _data->_sdfPropSelectionDomain.clear();                                                                                  \
        if (selectedPath.IsPropertyPath()) {                                                                                     \
            _data->_sdfPropSelectionDomain.insert(layer->GetObjectAtPath(selectedPath));                                         \
            _data->_sdfPrimSelectionDomain.insert(layer->GetObjectAtPath(selectedPath.GetPrimOrPrimVariantSelectionPath()));     \
        } else {                                                                                                                 \
            _data->_sdfPrimSelectionDomain.insert(layer->GetObjectAtPath(selectedPath));                                         \
        }                                                                                                                        \
    }

ImplementLayerSetSelected(SdfLayerRefPtr);
ImplementLayerSetSelected(SdfLayerHandle);

#define ImplementStageSetSelected(StageT)                                                                                        \
    template <> void Selection::SetSelected(const StageT &stage, const SdfPath &selectedPath) {                                  \
        if (!_data || !stage)                                                                                                    \
            return;                                                                                                              \
        _data->_stageSelection.reset(new HdSelection());                                                                         \
        _data->_stageSelection->AddRprim(HdSelection::HighlightModeSelect, selectedPath);                                        \
        _data->_stageSelection.mustRecomputeHash = true;                                                                         \
    }

ImplementStageSetSelected(UsdStageRefPtr);
ImplementStageSetSelected(UsdStageWeakPtr);

#define ImplementLayerIsSelectionEmpty(LayerT)                                                                                   \
    template <> bool Selection::IsSelectionEmpty(const LayerT &layer) const {                                                    \
        if (!_data || !layer)                                                                                                    \
            return true;                                                                                                         \
        return _data->_sdfPrimSelectionDomain.empty() && _data->_sdfPropSelectionDomain.empty();                                 \
    }

ImplementLayerIsSelectionEmpty(SdfLayerHandle);
ImplementLayerIsSelectionEmpty(SdfLayerRefPtr);

#define ImplementStageIsSelectionEmpty(StageT)                                                                                   \
    template <> bool Selection::IsSelectionEmpty(const StageT &stage) const {                                                    \
        if (!_data || !stage)                                                                                                    \
            return true;                                                                                                         \
        return !_data->_stageSelection || _data->_stageSelection->IsEmpty();                                                     \
    }

ImplementStageIsSelectionEmpty(UsdStageRefPtr);
ImplementStageIsSelectionEmpty(UsdStageWeakPtr);

template <> bool Selection::IsSelected(const SdfPrimSpecHandle &spec) const {
    if (!_data || !spec)
        return false;
    return _data->_sdfPrimSelectionDomain.find(spec) != _data->_sdfPrimSelectionDomain.end();
}

template <> bool Selection::IsSelected(const SdfAttributeSpecHandle &spec) const {
    if (!_data || !spec)
        return false;
    return _data->_sdfPropSelectionDomain.find(spec) != _data->_sdfPropSelectionDomain.end();
}

template <> bool Selection::IsSelected(const UsdStageWeakPtr &stage, const SdfPath &selectedPath) const {
    if (!_data || !stage)
        return false;
    if (_data->_stageSelection) {
        if (_data->_stageSelection->GetPrimSelectionState(HdSelection::HighlightModeSelect, selectedPath)) {
            return true;
        }
    }
    return false;
}

template <> bool Selection::UpdateSelectionHash(const UsdStageRefPtr &stage, SelectionHash &lastSelectionHash) {
    if (!_data || !stage)
        return false;

    if (_data->_stageSelection && _data->_stageSelection.mustRecomputeHash) {
        auto paths = _data->_stageSelection->GetAllSelectedPrimPaths();
        _data->_stageSelection.selectionHash = std::hash<SdfPathVector>{}(paths);
    }

    if (_data->_stageSelection.selectionHash != lastSelectionHash) {
        lastSelectionHash = _data->_stageSelection.selectionHash;
        return true;
    }
    return false;
}
// TODO: store anchor for prim and property
#define ImplementGetAnchorPrimPath(LayerT)\
template <> SdfPath Selection::GetAnchorPrimPath(const LayerT &layer) const {\
    if (!_data || !layer)\
        return {};\
    if (!_data->_sdfPrimSelectionDomain.empty()) {\
        const auto firstPrimHandle = *_data->_sdfPrimSelectionDomain.begin();\
        if (firstPrimHandle) {\
            return firstPrimHandle->GetPath();\
        }\
    }\
    return {};\
}\

ImplementGetAnchorPrimPath(SdfLayerHandle);
ImplementGetAnchorPrimPath(SdfLayerRefPtr);

#define ImplementGetAnchorPropertyPath(LayerT)\
template <> SdfPath Selection::GetAnchorPropertyPath(const LayerT &layer) const {\
    if (!_data || !layer)\
        return {};\
    if (!_data->_sdfPropSelectionDomain.empty()) {\
        const   auto firstPrimHandle = *_data->_sdfPropSelectionDomain.begin();\
        if (firstPrimHandle) {\
            return firstPrimHandle->GetPath();\
        }\
    }\
    return {};\
}\

ImplementGetAnchorPropertyPath(SdfLayerHandle);
ImplementGetAnchorPropertyPath(SdfLayerRefPtr);


template <typename PathT> inline size_t GetHash(const PathT &path) {
    // The original implementation of GetHash can return inconsistent hashes for the same path at different frames
    // This makes the stage tree flicker and look terribly buggy on version > 21.11
    // This issue appears on point instancers.
    // It is expected: https://github.com/PixarAnimationStudios/USD/issues/1922
    return path.GetHash();
    // The following is terribly unefficient but works.
    // return std::hash<std::string>()(path.GetString());
    // For now we store the paths in StageOutliner.cpp TraverseOpenedPaths which seems to work as well.
}

template <> SdfPath Selection::GetAnchorPrimPath(const UsdStageRefPtr &stage) const {
    if (!_data || !stage)
        return {};
    if (_data->_stageSelection) {
        const auto &paths = _data->_stageSelection->GetSelectedPrimPaths(HdSelection::HighlightModeSelect);
        if (!paths.empty()) {
            return paths[0];
        }
    }
    return {};
}

// This is called only once when there is a drag and drop at the moment
template <> std::vector<SdfPath> Selection::GetSelectedPaths(const SdfLayerHandle &layer) const {
    if (!_data || !layer)
        return {};
    std::vector<SdfPath> paths;
    std::transform(_data->_sdfPrimSelectionDomain.begin(), _data->_sdfPrimSelectionDomain.end(), std::back_inserter(paths),
                   [](const SdfSpecHandle &p) { return p->GetPath(); });
    std::transform(_data->_sdfPropSelectionDomain.begin(), _data->_sdfPropSelectionDomain.end(), std::back_inserter(paths),
                   [](const SdfSpecHandle &p) { return p->GetPath(); });
    return paths;
}

template <> std::vector<SdfPath> Selection::GetSelectedPaths(const UsdStageRefPtr &stage) const {
    if (!_data || !stage)
        return {};
    if (_data->_stageSelection) {
        return _data->_stageSelection->GetAllSelectedPrimPaths();
    }
    return {};
}



namespace lab {

/// Function to convert a hash from usd to ImGuiID with a seed, to avoid collision with path coming from layer and stages.
template <ImU32 seed, typename T> inline ImGuiID ToImGuiID(const T &val) {
    return ImHashData(static_cast<const void *>(&val), sizeof(T), seed);
}



// This StageOutliner is heavily adapted from usdtweak



//// Correctly indent the tree nodes using a path. This is used when we are iterating in a list of paths as opposed to a tree.
//// It allocates a vector which might not be optimal, but it should be used only on visible items, that should mitigate the
//// allocation cost
template <ImU32 seed, typename PathT> struct TreeIndenter {
    TreeIndenter(const PathT &path) {
        path.GetPrefixes(&prefixes);
        for (int i = 0; i < prefixes.size(); ++i) {
            ImGui::TreePushOverrideID(ToImGuiID<seed>(GetHash(prefixes[i])));
        }
    }
    ~TreeIndenter() {
        for (int i = 0; i < prefixes.size(); ++i) {
            ImGui::TreePop();
        }
    }
    std::vector<PathT> prefixes;
};


struct StageSelection : public std::unique_ptr<HdSelection> {
    // We store a state to know if the selection has changed between frames. Recomputing the hash is not ideal and potentially
    // a performance killer, so it will likely change in the future
    SelectionHash selectionHash = 0;
    bool mustRecomputeHash = true;
};


#define StageOutlinerSeed 2342934
#define IdOf ToImGuiID<StageOutlinerSeed, size_t>

class StageOutlinerDisplayOptions {
  public:
    StageOutlinerDisplayOptions() { ComputePrimFlagsPredicate(); }

    Usd_PrimFlagsPredicate GetPrimFlagsPredicate() const { return _displayPredicate; }

    void ToggleShowPrototypes() { _showPrototypes = !_showPrototypes; }

    void ToggleShowInactive() {
        _showInactive = !_showInactive;
        ComputePrimFlagsPredicate();
    }
    void ToggleShowAbstract() {
        _showAbstract = !_showAbstract;
        ComputePrimFlagsPredicate();
    }
    void ToggleShowUnloaded() {
        _showUnloaded = !_showUnloaded;
        ComputePrimFlagsPredicate();
    }
    void ToggleShowUndefined() {
        _showUndefined = !_showUndefined;
        ComputePrimFlagsPredicate();
    }

    bool GetShowPrototypes() const { return _showPrototypes; }
    bool GetShowInactive() const { return _showInactive; }
    bool GetShowUnloaded() const { return _showUnloaded; }
    bool GetShowAbstract() const { return _showAbstract; }
    bool GetShowUndefined() const { return _showUndefined; }

  private:
    // Default is:
    // UsdPrimIsActive && UsdPrimIsDefined && UsdPrimIsLoaded && !UsdPrimIsAbstract
    void ComputePrimFlagsPredicate() {
        Usd_PrimFlagsConjunction flags;
        if (!_showInactive) {
            flags = flags && UsdPrimIsActive;
        }
        if (!_showUndefined) {
            flags = flags && UsdPrimIsDefined;
        }
        if (!_showUnloaded) {
            flags = flags && UsdPrimIsLoaded;
        }
        if (!_showAbstract) {
            flags = flags && !UsdPrimIsAbstract;
        }
        _displayPredicate = UsdTraverseInstanceProxies(flags);
    }

    Usd_PrimFlagsPredicate _displayPredicate;
    bool _showInactive = true;
    bool _showUndefined = false;
    bool _showUnloaded = true;
    bool _showAbstract = false;
    bool _showPrototypes = true;
};

static void ExploreLayerTree(SdfLayerTreeHandle tree, PcpNodeRef node) {
    if (!tree)
        return;
    auto obj = tree->GetLayer()->GetObjectAtPath(node.GetPath());
    if (obj) {
        std::string format;
        format += tree->GetLayer()->GetDisplayName();
        format += " ";
        format += obj->GetPath().GetString();
        if (ImGui::MenuItem(format.c_str())) {
            // @TODO ExecuteAfterDraw<EditorSetSelection>(tree->GetLayer(), obj->GetPath());
        }
    }
    for (auto subTree : tree->GetChildTrees()) {
        ExploreLayerTree(subTree, node);
    }
}

static void ExploreComposition(PcpNodeRef root) {
    auto tree = root.GetLayerStack()->GetLayerTree();
    ExploreLayerTree(tree, root);
    TF_FOR_ALL(childNode, root.GetChildrenRange()) { ExploreComposition(*childNode); }
}

static void DrawUsdPrimEditMenuItems(const UsdPrim &prim) {
    if (ImGui::MenuItem("Toggle active")) {
        const bool active = !prim.IsActive();
        // @TODO ExecuteAfterDraw(&UsdPrim::SetActive, prim, active);
    }
    // TODO: Load and Unload are not in the undo redo :( ... make a command for them
    if (prim.HasAuthoredPayloads() && prim.IsLoaded() && ImGui::MenuItem("Unload")) {
        // @TODO ExecuteAfterDraw(&UsdPrim::Unload, prim);
    }
    if (prim.HasAuthoredPayloads() && !prim.IsLoaded() && ImGui::MenuItem("Load")) {
        // @TODO ExecuteAfterDraw(&UsdPrim::Load, prim, UsdLoadWithDescendants);
    }
    if (ImGui::MenuItem("Copy prim path")) {
        ImGui::SetClipboardText(prim.GetPath().GetString().c_str());
    }
    if (ImGui::BeginMenu("Edit layer")) {
        auto pcpIndex = prim.ComputeExpandedPrimIndex();
        if (pcpIndex.IsValid()) {
            auto rootNode = pcpIndex.GetRootNode();
            ExploreComposition(rootNode);
        }
        ImGui::EndMenu();
    }

#if 0
    if (ImGui::MenuItem("Create connection editor sheet")) {
        // TODO: a command ?? do we want undo redo in the node graph ??
        //AddPrimsToSession({prim});
        // TODO if the prim is a material or NodeGraph, add all its children
        CreateSession(prim, {prim});
    }
    
    if (ImGui::MenuItem("Add to connection editor")) {
        // TODO if the prim is a material or NodeGraph, add all its children
        AddPrimsToCurrentSession({prim});
    }
#endif
}

/// Predefined colors for the different widgets
#define ColorAttributeAuthored {1.0, 1.0, 1.0, 1.0}
#define ColorAttributeUnauthored {0.5, 0.5, 0.5, 1.0}
#define ColorAttributeRelationship {0.5, 0.5, 0.9, 1.0}
#define ColorAttributeConnection {1.0, 1.0, 0.7, 1.0}
#define ColorMiniButtonAuthored {0.0, 1.0, 0.0, 1.0}
#define ColorMiniButtonUnauthored {0.6, 0.6, 0.6, 1.0}
#define ColorTransparent {0.0, 0.0, 0.0, 0.0}
#define ColorPrimDefault {227.f/255.f, 227.f/255.f, 227.f/255.f, 1.0}
#define ColorPrimInactive {0.4, 0.4, 0.4, 1.0}
#define ColorPrimInstance {135.f/255.f, 206.f/255.f, 250.f/255.f, 1.0}
#define ColorPrimPrototype {118.f/255.f, 136.f/255.f, 217.f/255.f, 1.0}
#define ColorPrimUndefined {200.f/255.f, 100.f/255.f, 100.f/255.f, 1.0}
#define ColorPrimHasComposition {222.f/255.f, 158.f/255.f, 46.f/255.f, 1.0}
#define ColorGreyish {0.5, 0.5, 0.5, 1.0}
#define ColorButtonHighlight {0.5, 0.7, 0.5, 0.7}
#define ColorEditableWidgetBg {0.260f, 0.300f, 0.360f, 1.000f}
#define ColorPrimSelectedBg {0.75, 0.60, 0.33, 0.6}
#define ColorAttributeSelectedBg {0.75, 0.60, 0.33, 0.6}
#define ColorImGuiButton {1.f, 1.f, 1.f, 0.2f}
#define ColorImGuiFrameBg {0.160f, 0.160f, 0.160f, 1.000f}
#define ColorImGuiText {1.0, 1.0, 1.0, 1.000f}

/// Predefined colors for dark mode ui components
#define ColorPrimaryLight {0.900f, 0.900f, 0.900f, 1.000f}

#define ColorSecondaryLight {0.490f, 0.490f, 0.490f, 1.000f}
#define ColorSecondaryLightLighten {0.586f, 0.586f, 0.586f, 1.000f}
#define ColorSecondaryLightDarken {0.400f, 0.400f, 0.400f, 1.000f}

#define ColorPrimaryDark {0.148f, 0.148f, 0.148f, 1.000f}
#define ColorPrimaryDarkLighten {0.195f, 0.195f, 0.195f, 1.000f}
#define ColorPrimaryDarkDarken {0.098f, 0.098f, 0.098f, 1.000f}

#define ColorSecondaryDark {0.340f, 0.340f, 0.340f, 1.000f}
#define ColorSecondaryDarkLighten {0.391f, 0.391f, 0.391f, 1.000f}
#define ColorSecondaryDarkDarken {0.280f, 0.280f, 0.280f, 1.000f}

#define ColorHighlight {0.880f, 0.620f, 0.170f, 1.000f}
#define ColorHighlightDarken {0.880f, 0.620f, 0.170f, 0.781f}

#define ColorBackgroundDim {0.000f, 0.000f, 0.000f, 0.586f}
#define ColorInvisible {0.000f, 0.000f, 0.000f, 0.000f}

#define DefaultColorStyle  ImGuiCol_Text, ImVec4(ColorImGuiText), ImGuiCol_Button, ImVec4(ColorImGuiButton), ImGuiCol_FrameBg, ImVec4(ColorImGuiFrameBg)


static ImVec4 GetPrimColor(const UsdPrim &prim) {
    if (!prim.IsActive() || !prim.IsLoaded()) {
        return ImVec4(ColorPrimInactive);
    }
    if (prim.IsInstance()) {
        return ImVec4(ColorPrimInstance);
    }
    const auto hasCompositionArcs = prim.HasAuthoredReferences() || prim.HasAuthoredPayloads() || prim.HasAuthoredInherits() ||
                                    prim.HasAuthoredSpecializes() || prim.HasVariantSets();
    if (hasCompositionArcs) {
        return ImVec4(ColorPrimHasComposition);
    }
    if (prim.IsPrototype() || prim.IsInPrototype() || prim.IsInstanceProxy()) {
        return ImVec4(ColorPrimPrototype);
    }
    if (!prim.IsDefined()) {
        return ImVec4(ColorPrimUndefined);
    }
    return ImVec4(ColorPrimDefault);
}

static inline const char *GetVisibilityIcon(const TfToken &visibility) {
    if (visibility == UsdGeomTokens->inherited) {
        return ICON_FA_HAND_POINT_UP;
    } else if (visibility == UsdGeomTokens->invisible) {
        return ICON_FA_EYE_SLASH;
    } else if (visibility == UsdGeomTokens->visible) {
        return ICON_FA_EYE;
    }
    return ICON_FA_EYE;
}


/// Creates a scoped object that will push the pair of style and color passed in the constructor
/// It will pop the correct number of time when the object is destroyed
struct ScopedStyleColor {

    ScopedStyleColor() = delete;

    template <typename StyleT, typename ColorT, typename... Args>
    ScopedStyleColor(StyleT &&style, ColorT &&color, Args... args) : nbPop(1 + sizeof...(args) / 2) {
        PushStyles(style, color, args...);
    }

    template <typename StyleT, typename ColorT, typename... Args>
    static void PushStyles(StyleT &&style, ColorT &&color, Args... args) { // constexpr is
        ImGui::PushStyleColor(style, color);
        PushStyles(args...);
    }
    static void PushStyles(){};

    ~ScopedStyleColor() {
        ImGui::PopStyleColor(nbPop);
    }

    const int nbPop; // TODO: get rid of this constant and generate the correct number of pop at compile time
};



static void DrawVisibilityButton(const UsdPrim &prim) {
    // TODO: this should work with animation
    UsdGeomImageable imageable(prim);
    if (imageable) {
        ImGui::PushID(IdOf(prim.GetPath().GetHash()));
        // Get visibility value
        auto attr = imageable.GetVisibilityAttr();
        VtValue visibleValue;
        attr.Get(&visibleValue);
        TfToken visibilityToken = visibleValue.Get<TfToken>();
        const char *visibilityIcon = GetVisibilityIcon(visibilityToken);
        {
            ScopedStyleColor buttonColor(
                ImGuiCol_Text, attr.HasAuthoredValue() ? ImVec4(1.0, 1.0, 1.0, 1.0) : ImVec4(ColorPrimInactive));
            ImGui::SmallButton(visibilityIcon);
            // Menu to select the new visibility
            {
                ScopedStyleColor menuTextColor(ImGuiCol_Text, ImVec4(1.0, 1.0, 1.0, 1.0));
                if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonLeft)) {
                    if (attr.HasAuthoredValue() && ImGui::MenuItem("clear visibiliy")) {
                        // @TODO ExecuteAfterDraw(&UsdPrim::RemoveProperty, prim, attr.GetName());
                    }
                    VtValue allowedTokens;
                    attr.GetMetadata(TfToken("allowedTokens"), &allowedTokens);
                    if (allowedTokens.IsHolding<VtArray<TfToken>>()) {
                        for (const auto &token : allowedTokens.Get<VtArray<TfToken>>()) {
                            if (ImGui::MenuItem(token.GetText())) {
                                // @TODO ExecuteAfterDraw<AttributeSet>(attr, VtValue(token), UsdTimeCode::Default());
                            }
                        }
                    }
                    ImGui::EndPopup();
                }
            }
        }
        ImGui::PopID();
    }
}

// This is pretty similar to DrawBackgroundSelection in the SdfLayerSceneGraphEditor
static void DrawBackgroundSelection(const UsdPrim &prim, bool selected) {

    ImVec4 colorSelected = selected ? ImVec4(ColorPrimSelectedBg) : ImVec4(0.75, 0.60, 0.33, 0.2);
    ScopedStyleColor scopedStyle(ImGuiCol_HeaderHovered, selected ? colorSelected : ImVec4(ColorTransparent),
                                 ImGuiCol_HeaderActive, ImVec4(ColorTransparent), ImGuiCol_Header, colorSelected);
    //ImVec2 sizeArg(0.0, ImGui::GetFrameHeight());
    const ImGuiContext &g = *GImGui;
    ImVec2 sizeArg(0.0, g.FontSize);
    const auto selectableFlags = ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap;
    ImGui::Selectable("##backgroundSelectedPrim", selected, selectableFlags, sizeArg);
    ImGui::SetItemAllowOverlap();
    ImGui::SameLine();
}



static void DrawPrimTreeRow(const UsdPrim &prim,
                            Selection &selectedPaths, StageOutlinerDisplayOptions &displayOptions) {
    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_AllowItemOverlap; // for testing worse case scenario add | ImGuiTreeNodeFlags_DefaultOpen;

    // Another way ???
    const auto &children = prim.GetFilteredChildren(displayOptions.GetPrimFlagsPredicate());
    if (children.empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }

    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    DrawBackgroundSelection(prim, selectedPaths.IsSelected(prim.GetStage(), prim.GetPath()));
    bool unfolded = true;
    {
        {
            TreeIndenter<StageOutlinerSeed, SdfPath> indenter(prim.GetPath());
            ScopedStyleColor primColor(ImGuiCol_Text, GetPrimColor(prim), ImGuiCol_HeaderHovered, 0, ImGuiCol_HeaderActive, 0);
            const ImGuiID pathHash = IdOf(GetHash(prim.GetPath()));
            //ImGui::AlignTextToFramePadding();
            unfolded = ImGui::TreeNodeBehavior(pathHash, flags, prim.GetName().GetText());
            // TreeSelectionBehavior(selectedPaths, &prim);
            if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
                // TODO selection, should go in commands, ultimately the selection is passed
                // as const
                if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl)) {
                    if (selectedPaths.IsSelected(prim.GetStage(), prim.GetPath())) {
                        selectedPaths.RemoveSelected(prim.GetStage(), prim.GetPath());
                    } else {
                        selectedPaths.AddSelected(prim.GetStage(), prim.GetPath());
                    }
                } else {
                    // @TODO ExecuteAfterDraw<EditorSetSelection>(prim.GetStage(), prim.GetPath());
                }
            }
        }
        {
            ScopedStyleColor popupColor(ImGuiCol_Text, ImVec4(ColorPrimDefault));
            if (ImGui::BeginPopupContextItem()) {
                DrawUsdPrimEditMenuItems(prim);
                ImGui::EndPopup();
            }
        }
        // Visibility
        ImGui::TableSetColumnIndex(1);
        DrawVisibilityButton(prim);

        // Type
        ImGui::TableSetColumnIndex(2);
        ImGui::Text("%s", prim.GetTypeName().GetText());
    }
    if (unfolded) {
        ImGui::TreePop();
    }
}


/// Draws a menu list of all the sublayers, indented to reveal the parenting
static void DrawEditTargetSubLayersMenuItems(UsdStageWeakPtr stage, SdfLayerHandle layer, int indent = 0) {
    if (layer) {
        std::vector<std::string> subLayers = layer->GetSubLayerPaths();
        for (int layerId = 0; layerId < subLayers.size(); ++layerId) {
            const std::string &subLayerPath = subLayers[layerId];
            auto subLayer = SdfLayer::FindOrOpenRelativeToLayer(layer, subLayerPath);
            if (!subLayer) {
                subLayer = SdfLayer::FindOrOpen(subLayerPath);
            }
            const std::string layerName = std::string(indent, ' ') + (subLayer ? subLayer->GetDisplayName() : subLayerPath);
            if (subLayer) {
                if (ImGui::MenuItem(layerName.c_str())) {
                    // @TODO ExecuteAfterDraw<EditorSetEditTarget>(stage, UsdEditTarget(subLayer));
                }
            } else {
                ImGui::TextColored(ImVec4(1.0, 0.0, 0.0, 1.0), "%s", layerName.c_str());
            }

            ImGui::PushID(layerName.c_str());
            DrawEditTargetSubLayersMenuItems(stage, subLayer, indent + 4);
            ImGui::PopID();
        }
    }
}

// Second version of an edit target selector
void DrawUsdPrimEditTarget(const UsdPrim &prim) {
    if (!prim)
        return;
    ScopedStyleColor defaultStyle(DefaultColorStyle);
    if (ImGui::MenuItem("Session layer")) {
        // @TODO ExecuteAfterDraw<EditorSetEditTarget>(prim.GetStage(), UsdEditTarget(prim.GetStage()->GetSessionLayer()));
    }
    if (ImGui::MenuItem("Root layer")) {
        // @TODO ExecuteAfterDraw<EditorSetEditTarget>(prim.GetStage(), UsdEditTarget(prim.GetStage()->GetRootLayer()));
    }

    if (ImGui::BeginMenu("Sublayers")) {
        const auto &layer = prim.GetStage()->GetRootLayer();
        DrawEditTargetSubLayersMenuItems(prim.GetStage(), layer);
        ImGui::EndMenu();
    }

    if (ImGui::BeginMenu("Selected prim arcs")) {
        using Query = pxr::UsdPrimCompositionQuery;
        auto filter = UsdPrimCompositionQuery::Filter();
        pxr::UsdPrimCompositionQuery arc(prim, filter);
        auto compositionArcs = arc.GetCompositionArcs();
        for (auto a : compositionArcs) {
            // NOTE: we can use GetIntroducingLayer() and GetIntroducingPrimPath() to add more information
            if (a.GetTargetNode()) {
                std::string arcName = a.GetTargetNode().GetLayerStack()->GetIdentifier().rootLayer->GetDisplayName() + " " +
                                      a.GetTargetNode().GetPath().GetString();
                if (ImGui::MenuItem(arcName.c_str())) {
                    /* @TODO ExecuteAfterDraw<EditorSetEditTarget>(
                        prim.GetStage(),
                        UsdEditTarget(a.GetTargetNode().GetLayerStack()->GetIdentifier().rootLayer, a.GetTargetNode()),
                        a.GetTargetNode().GetPath());*/
                }
            }
        }
        ImGui::EndMenu();
    }
}


static void DrawStageTreeRow(const UsdStageRefPtr &stage, Selection &selectedPaths) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);

    ImGuiTreeNodeFlags nodeflags = ImGuiTreeNodeFlags_OpenOnArrow;
    std::string stageDisplayName(stage->GetRootLayer()->GetDisplayName());
    auto unfolded = ImGui::TreeNodeBehavior(IdOf(GetHash(SdfPath::AbsoluteRootPath())), nodeflags, stageDisplayName.c_str());

    ImGui::TableSetColumnIndex(2);
    ImGui::SmallButton(ICON_FA_PEN);
    if (ImGui::BeginPopupContextItem(nullptr, ImGuiPopupFlags_MouseButtonLeft)) {
        const UsdPrim &selected =
            selectedPaths.IsSelectionEmpty(stage) ? stage->GetPseudoRoot() : stage->GetPrimAtPath(selectedPaths.GetAnchorPrimPath(stage));
        DrawUsdPrimEditTarget(selected);
        ImGui::EndPopup();
    }
    ImGui::SameLine();
    ImGui::Text("%s", stage->GetEditTarget().GetLayer()->GetDisplayName().c_str());
    if (unfolded) {
        ImGui::TreePop();
    }
}

/// This function should be called only when the Selection has changed
/// It modifies the internal imgui tree graph state.
static void OpenSelectedPaths(const UsdStageRefPtr &stage, Selection &selectedPaths) {
    ImGuiContext &g = *GImGui;
    ImGuiWindow *window = g.CurrentWindow;
    ImGuiStorage *storage = window->DC.StateStorage;
    for (const auto &path : selectedPaths.GetSelectedPaths(stage)) {
        for (const auto &element : path.GetParentPath().GetPrefixes()) {
            ImGuiID id = IdOf(GetHash(element)); // This has changed with the optim one
            storage->SetInt(id, true);
        }
    }
}

static void TraverseRange(UsdPrimRange &range, std::vector<SdfPath> &paths) {
    static std::set<SdfPath> retainedPath; // to fix a bug with instanced prim which recreates the path at every call and give a different hash
    ImGuiContext &g = *GImGui;
    ImGuiWindow *window = g.CurrentWindow;
    ImGuiStorage *storage = window->DC.StateStorage;
    for (auto iter = range.begin(); iter != range.end(); ++iter) {
        const auto &path = iter->GetPath();
        const ImGuiID pathHash = IdOf(GetHash(path));
        const bool isOpen = storage->GetInt(pathHash, 0) != 0;
        if (!isOpen) {
            iter.PruneChildren();
        }
        // This bit of code is to avoid a bug. It appears that the SdfPath of instance proxies are not kept and the underlying memory
        // is deleted and recreated between each frame, invalidating the hash value. So for the same path we have different hash every frame :s not cool.
        // This problems appears on versions > 21.11
        // a look at the changelog shows that they were lots of changes on the SdfPath side:
        // https://github.com/PixarAnimationStudios/USD/commit/46c26f63d2a6e9c6c5dbfbcefa0235c3265457bb
        //
        // In the end we workaround this issue by keeping the instance proxy paths alive:
        if (iter->IsInstanceProxy()) {
            retainedPath.insert(path);
        }
        paths.push_back(path);
    }
}

// Traverse the stage skipping the paths closed by the tree ui.
static void TraverseOpenedPaths(UsdStageRefPtr stage, std::vector<SdfPath> &paths, StageOutlinerDisplayOptions &displayOptions) {
    if (!stage)
        return;
    ImGuiContext &g = *GImGui;
    ImGuiWindow *window = g.CurrentWindow;
    ImGuiStorage *storage = window->DC.StateStorage;
    paths.clear();
    const SdfPath &rootPath = SdfPath::AbsoluteRootPath();
    const bool rootPathIsOpen = storage->GetInt(IdOf(GetHash(rootPath)), 0) != 0;

    if (rootPathIsOpen) {
        // Stage
        auto range = UsdPrimRange::Stage(stage, displayOptions.GetPrimFlagsPredicate());
        TraverseRange(range, paths);
        // Prototypes
        if (displayOptions.GetShowPrototypes()) {
            for(const auto &proto: stage->GetPrototypes()) {
                auto range = UsdPrimRange(proto, displayOptions.GetPrimFlagsPredicate());
                TraverseRange(range, paths);
            }
        }
    }
}

static void FocusedOnFirstSelectedPath(const SdfPath &selectedPath, const std::vector<SdfPath> &paths,
                                       ImGuiListClipper &clipper) {
    // linear search! it happens only when the selection has changed. We might want to maintain a map instead
    // if the hierarchies are big.
    for (int i = 0; i < paths.size(); ++i) {
        if (paths[i] == selectedPath) {
            // scroll only if the item is not visible
            if (i < clipper.DisplayStart || i > clipper.DisplayEnd) {
                ImGui::SetScrollY(clipper.ItemsHeight * i + 1);
            }
            return;
        }
    }
}

void DrawStageOutlinerMenuBar(StageOutlinerDisplayOptions &displayOptions) {

    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("Show")) {
            if (ImGui::MenuItem("Inactive", nullptr, displayOptions.GetShowInactive())) {
                displayOptions.ToggleShowInactive();
            }
            if (ImGui::MenuItem("Undefined", nullptr, displayOptions.GetShowUndefined())) {
                displayOptions.ToggleShowUndefined();
            }
            if (ImGui::MenuItem("Unloaded", nullptr, displayOptions.GetShowUnloaded())) {
                displayOptions.ToggleShowUnloaded();
            }
            if (ImGui::MenuItem("Abstract", nullptr, displayOptions.GetShowAbstract())) {
                displayOptions.ToggleShowAbstract();
            }
            if (ImGui::MenuItem("Prototypes", nullptr, displayOptions.GetShowPrototypes())) {
                displayOptions.ToggleShowPrototypes();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}


/// Compute the remaining height on the current window when it contains a certain number of "inline" widgets
/// This is an lower bound estimate rather than an exact value as the text lines are generally
/// smaller than the inline widget.
inline
float RemainingHeight(int nbLines) {
    const ImGuiWindow *currentWindow = ImGui::GetCurrentWindow();
    const ImGuiContext& g = *GImGui;
    return currentWindow->Size[1]
    - nbLines*ImGui::GetFrameHeightWithSpacing()
    - g.Style.FramePadding.y * 4.0f
    - g.Style.ItemSpacing.y * 4.0f // Account for the other widget
    - g.Style.WindowPadding.y * 2.0f;
}


/// Draw the hierarchy of the stage
void DrawStageOutliner(UsdStageRefPtr stage, Selection &selectedPaths) {
    if (!stage)
        return;
    
    static StageOutlinerDisplayOptions displayOptions;
    DrawStageOutlinerMenuBar(displayOptions);
    
    //ImGui::PushID("StageOutliner");
    constexpr unsigned int textBufferSize = 512;
    static char buf[textBufferSize];
    bool addprimclicked = false;
    auto rootPrim = stage->GetPseudoRoot();
    auto layer = stage->GetSessionLayer();

    static SelectionHash lastSelectionHash = 0;

    const ImGuiContext &g = *GImGui;
    const ImVec2 tableOuterSize(0, RemainingHeight(2));
    constexpr ImGuiTableFlags tableFlags = ImGuiTableFlags_SizingFixedFit | /*ImGuiTableFlags_RowBg |*/ ImGuiTableFlags_ScrollY;
    if (ImGui::BeginTable("##DrawStageOutliner", 3, tableFlags, tableOuterSize)) {
        ImGui::TableSetupScrollFreeze(3, 1); // Freeze the root node of the tree (the layer)
        ImGui::TableSetupColumn("Hierarchy");
        // TODO make it relative to font size
        ImGui::TableSetupColumn("V", ImGuiTableColumnFlags_WidthFixed, g.FontSize*3);
        ImGui::TableSetupColumn("Type");

        // Unfold the selected path
        const bool selectionHasChanged = selectedPaths.UpdateSelectionHash(stage, lastSelectionHash);
        if (selectionHasChanged) {            // We could use the imgui id as well instead of a static ??
            OpenSelectedPaths(stage, selectedPaths); // Also we could have a UsdTweakFrame which contains all the changes that happened
                                              // between the last frame and the new one
        }

        // Find all the opened paths
        std::vector<SdfPath> paths;
        paths.reserve(1024);
        TraverseOpenedPaths(stage, paths, displayOptions); // This must be inside the table scope to get the correct treenode hash table

        // Draw the tree root node, the layer
        DrawStageTreeRow(stage, selectedPaths);

        // Display only the visible paths with a clipper
        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(paths.size()));
        while (clipper.Step()) {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
                ImGui::PushID(row);
                const SdfPath &path = paths[row];
                const auto &prim = stage->GetPrimAtPath(path);
                DrawPrimTreeRow(prim, selectedPaths, displayOptions);
                ImGui::PopID();
            }
        }
        if (selectionHasChanged) {
            // This function can only be called in this context and after the clipper.Step()
            FocusedOnFirstSelectedPath(selectedPaths.GetAnchorPrimPath(stage), paths, clipper);
        }
        ImGui::EndTable();
    }

    // Search prim bar
    static char patternBuffer[256];
    static bool useRegex = false;
    ImGui::SetNextItemWidth(ImGui::GetCurrentWindow()->Size[0]-g.FontSize * 12);
    auto enterPressed = ImGui::InputTextWithHint("##SearchPrims", "Find prim", patternBuffer, 256, ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::SameLine();
    ImGui::Checkbox("use regex", &useRegex);
    ImGui::SameLine();
    if (ImGui::Button("Select next") || enterPressed) {
        // @TODO ExecuteAfterDraw<EditorFindPrim>(std::string(patternBuffer), useRegex);
    }

}






















struct UsdOutlinerActivity::data {
    data() {
        memset(&gGhost, 0, sizeof(gGhost));
    }
};

void UsdOutlinerActivity::USDOutlinerUI(const LabViewInteraction& vi) {
    pxr::UsdStageRefPtr stage = OpenUSDProvider::instance()->Stage();
    if (!stage) {
        return;
    }
    lab::Orchestrator* mm = lab::Orchestrator::Canonical();
    std::weak_ptr<HydraActivity> hact;
    auto hydra = mm->LockActivity(hact);
    
    SelectionHash sh = 0;
    Selection selection;
    SdfPathVector sel = hydra->GetHdSelection();
    selection.Clear(stage);
    for (auto& s : sel) {
        selection.AddSelected(stage, s);
    }
    selection.UpdateSelectionHash(stage, sh);
    DrawStageOutliner(stage, selection);
    if (selection.UpdateSelectionHash(stage, sh)) {
        hydra->SetHdSelection(selection.GetSelectedPaths(stage));
    }
}

void UsdOutlinerActivity::RunUI(const LabViewInteraction& vi) {
    if (!IsActive() || !UIVisible())
        return;

    ImGui::SetNextWindowSize(ImVec2(200, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin("Stage Outliner");
    USDOutlinerUI(vi);
    ImGui::End();
}

void UsdOutlinerActivity::_activate() {
    if (!gGhost[0].h) {
        auto sprite = SpriteProvider::instance();
        auto assets = AssetsProvider::instance();
        std::string path = assets->resolve("$(LAB_BUNDLE)/ghost.aseprite");
        sprite->cache_sprite(path.c_str(), "ghost");
        gGhost[0] = sprite->frame("ghost", 0);
        gGhost[1] = sprite->frame("ghost", 1);
        int i = LabCreateRGBA8Texture(gGhost[0].w, gGhost[0].h, gGhost[0].pixels);
        gGhostTextures[0] = LabTextureHardwareHandle(i);
        i = LabCreateRGBA8Texture(gGhost[1].w, gGhost[1].h, gGhost[1].pixels);
        gGhostTextures[1] = LabTextureHardwareHandle(i);
    }
}

UsdOutlinerActivity::UsdOutlinerActivity()
: Activity(UsdOutlinerActivity::sname()) {
    _self = new UsdOutlinerActivity::data();
    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<UsdOutlinerActivity*>(instance)->RunUI(*vi);
    };
}

UsdOutlinerActivity::~UsdOutlinerActivity() {
    delete _self;
}

} // lab
