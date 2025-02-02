
#include "ImGuiHelpers.h"
#include "Constants.h"
#include "Editor.h"

#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usd/stageCache.h>

#include <vector>

// stub for usdtweak
void CreateSession(const UsdPrim &prim, const std::vector<UsdPrim> &prims) {}
void AddPrimsToCurrentSession(const std::vector<UsdPrim> &prims) {}
void DrawUsdPrimEditTarget(const UsdPrim &prim) {}

void Editor::RunLauncher(const std::string &launcherName) {}
void Editor::SetNextLayer() {}
void Editor::StopPlayback() {}
void Editor::StartPlayback() {}
bool Editor::HasUnsavedWork() { return false; }
void Editor::TogglePlayback() {}
void Editor::ConfirmShutdown(std::string why) {}
void Editor::FindOrOpenLayer(const std::string &path) {}
void Editor::SetCurrentLayer(SdfLayerRefPtr layer, bool showContentBrowser) {}
void Editor::SetCurrentStage(UsdStageCache::Id current) {}
void Editor::SetCurrentStage(UsdStageRefPtr stage) {}
void Editor::SetPreviousLayer() {}
void Editor::SetLayerPathSelection(const SdfPath &primPath) {}
void Editor::SetStagePathSelection(const SdfPath &primPath) {}
void Editor::ShowDialogSaveLayerAs(SdfLayerHandle layerToSaveAs) {}
void Editor::SelectScaleManipulator() {}
void Editor::SelectPositionManipulator() {}
void Editor::SelectRotationManipulator() {}
void Editor::SelectMouseHoverManipulator() {}
void Editor::ScaleUI(float) {}
void Editor::OpenStage(const std::string &path, bool openLoaded) {}
