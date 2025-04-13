/**
 * @file usdsessionlayer.h
 * @author Raphael Jouretz (rjouretz.com)
 * @brief UsdSessionLayer view acts as a text editor for the session layer of
 * the current UsdStage. It allows to preview and edit the session layer.
 *
 * @copyright Copyright (c) 2023
 *
 */
#ifndef ACTIVITIES_USDSESSION_LAYER
#define ACTIVITIES_USDSESSION_LAYER

#include "HydraViewport.hpp"

#include <TextEditor.h>


namespace lab {

/**
 * @brief UsdSessionLayer view acts as a text editor for the session layer of
 * the current UsdStage. It allows to preview and edit the USD session layer.
 *
 */
class UsdSessionLayer : public View {
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
     * @brief Override of the View::GetViewType
     *
     */
    const std::string GetViewType() override;

    /**
     * @brief Override of the View::_GetGizmoWindowFlags
     *
     */
    ImGuiWindowFlags _GetGizmoWindowFlags() override;

    void SetStage(pxr::UsdStageRefPtr stage);


private:
    TextEditor _editor;
    bool _isEditing;
    std::string _lastLoadedText;
    ImGuiWindowFlags _gizmoWindowFlags;

    /**
     * @brief Override of the View::Draw
     *
     */
    void _Draw(const LabViewInteraction& vi) override;

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
    void _FocusInEvent() override;

    /**
     * @brief Override of the View::_FocusOutEvent
     *
     */
    void _FocusOutEvent() override;
};

} // lab

#endif
