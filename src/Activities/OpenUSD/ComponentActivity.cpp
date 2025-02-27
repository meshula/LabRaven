//  ComponentActivity.cpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 2/27/25.
//  Copyright © 2025 Nick Porcino. All rights reserved.
//

#include "ComponentActivity.hpp"
#include "Providers/OpenUSD/OpenUSDProvider.hpp"
#include "Providers/Selection/SelectionProvider.hpp"
#include "Activities/Timeline/TimelineActivity.hpp"

#include "imgui.h"
#include "Lab/ImguiExt.hpp"
#include <pxr/usd/usd/modelAPI.h>
#include <pxr/usd/usd/prim.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/base/tf/token.h>
#include <sstream>
#include <iomanip>

namespace lab {
    using namespace PXR_NS;

    struct ComponentRecord {
        std::string kind;
        UsdPrim prim;
    };

    struct ComponentActivity::data {
        bool ui_visible = true;
        bool prune_hierarchy = false;
        bool show_analysis = true;
        std::unordered_map<std::string, int> kindCounts;
        std::unordered_map<std::string, std::vector<UsdPrim>> kindPrims;
        std::vector<ComponentRecord> components;
        int selected_component = -1;
    };

    ComponentActivity::ComponentActivity() : Activity(ComponentActivity::sname()) {
        _self = new ComponentActivity::data();
        
        activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
            static_cast<ComponentActivity*>(instance)->RunUI(*vi);
        };
        activity.Menu = [](void* instance) {
            static_cast<ComponentActivity*>(instance)->Menu();
        };
    }

    ComponentActivity::~ComponentActivity() {
        delete _self;
    }

    void ComponentActivity::PrintHierarchy(const UsdPrim& prim, int depth, bool prune) {
        if (!prim) return;

        // Retrieve the kind metadata
        TfToken kind;
        UsdModelAPI(prim).GetKind(&kind);

        // Store component records for selection
        if (!kind.IsEmpty() && 
            (kind == TfToken("component") || kind == TfToken("assembly"))) {
            ComponentRecord record;
            record.kind = kind.GetString();
            record.prim = prim;
            _self->components.push_back(record);
        }

        // Apply pruning: stop at 'component' or 'assembly'
        if (prune && (kind == TfToken("component") || kind == TfToken("assembly"))) {
            return;
        }

        // Recurse into children
        for (const auto& child : prim.GetChildren()) {
            PrintHierarchy(child, depth + 1, prune);
        }
    }

    void ComponentActivity::AnalyzeHierarchy(const UsdPrim& prim, 
                          std::unordered_map<std::string, int>& kindCounts,
                          std::unordered_map<std::string, std::vector<UsdPrim>>& kindPrims) {
        if (!prim) {
            return;
        }

        TfToken kind;
        UsdModelAPI(prim).GetKind(&kind);
        if (!kind.IsEmpty()) {
            kindCounts[kind.GetString()]++;
            kindPrims[kind.GetString()].push_back(prim);
        }

        for (const auto& child : prim.GetChildren()) {
            AnalyzeHierarchy(child, kindCounts, kindPrims);
        }
    }

    void ComponentActivity::PrintAnalysis(const std::unordered_map<std::string, int>& kindCounts) {
        std::vector<std::pair<std::string, int>> sortedKinds(kindCounts.begin(), kindCounts.end());
        std::sort(sortedKinds.begin(), sortedKinds.end(), [](const auto& a, const auto& b) {
            return b.second < a.second;
        });

        ImGui::Text("USD Stage Analysis:");
        ImGui::Separator();
        
        for (const auto& [kind, count] : sortedKinds) {
            ImGui::Text("%-15s : %d", kind.c_str(), count);
        }
    }

    void ComponentActivity::RunUI(const LabViewInteraction& vi) {
        if (!_self->ui_visible)
            return;

        auto mm = Orchestrator::Canonical();
        std::weak_ptr<TimelineActivity> tlact;
        auto timeline = mm->LockActivity(tlact);

        auto usd = OpenUSDProvider::instance();
        auto stage = usd->Stage();
        double tcps = stage->GetTimeCodesPerSecond();
        const UsdTimeCode timeCode = timeline->Playhead().to_seconds() * tcps;

        ImGui::Begin("Component Explorer", &_self->ui_visible);

        // Options
        if (ImGui::CollapsingHeader("Options", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Prune Hierarchy", &_self->prune_hierarchy);
            ImGui::SameLine();
            ImGui::Checkbox("Show Analysis", &_self->show_analysis);
            
            if (ImGui::Button("Refresh")) {
                // Clear previous data
                _self->kindCounts.clear();
                _self->kindPrims.clear();
                _self->components.clear();
                
                // Analyze the stage
                AnalyzeHierarchy(stage->GetPseudoRoot(), _self->kindCounts, _self->kindPrims);
                PrintHierarchy(stage->GetPseudoRoot(), 0, _self->prune_hierarchy);
            }
        }

        // Analysis panel
        if (_self->show_analysis && ImGui::CollapsingHeader("Analysis", ImGuiTreeNodeFlags_DefaultOpen)) {
            PrintAnalysis(_self->kindCounts);
        }

        // Component hierarchy panel
        if (ImGui::CollapsingHeader("Components", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::BeginGroup();
            ImVec2 windowSize = ImGui::GetWindowSize();
            ImVec2 pos = ImGui::GetCursorPos();
            
            windowSize.x -= 100;
            if (windowSize.x < 100)
                windowSize.x = 100;
            if (windowSize.x > 250)
                windowSize.x = 250;
            windowSize.y = 0.f;
            
            if (ImGui::BeginListBox("###ComponentsList", windowSize)) {
                for (size_t n = 0; n < _self->components.size(); n++) {
                    const bool is_selected = (_self->selected_component == n);
                    std::string label = _self->components[n].prim.GetName().GetString();
                    label += " [" + _self->components[n].kind + "]";
                    
                    if (ImGui::Selectable(label.c_str(), is_selected))
                        _self->selected_component = (int) n;

                    // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                    if (is_selected)
                        ImGui::SetItemDefaultFocus();
                }
                ImGui::EndListBox();
            }
            
            if (_self->selected_component >= 0) {
                float itemHeight = ImGui::GetTextLineHeightWithSpacing();
                float ypos = itemHeight * _self->selected_component;
                pos.x += windowSize.x;
                pos.y += ypos;
                ImGui::SetCursorPos(pos);
                if (ImGui::Button("Select")) {
                    auto selection = SelectionProvider::instance();
                    selection->SetSelectionPrims(&(*stage), {_self->components[_self->selected_component].prim});
                }
            }
            ImGui::EndGroup();
        }

        // Kind-based grouping panel
        if (ImGui::CollapsingHeader("Kinds", ImGuiTreeNodeFlags_DefaultOpen)) {
            for (const auto& [kind, prims] : _self->kindPrims) {
                if (ImGui::TreeNode(kind.c_str())) {
                    for (const auto& prim : prims) {
                        std::string primName = prim.GetName().GetString();
                        if (ImGui::Selectable(primName.c_str())) {
                            auto selection = SelectionProvider::instance();
                            selection->SetSelectionPrims(&(*stage), {prim});
                        }
                    }
                    ImGui::TreePop();
                }
            }
        }

        ImGui::End();
    }

    void ComponentActivity::Menu() {
        if (ImGui::BeginMenu("Modes")) {
            if (ImGui::MenuItem("Component Explorer", nullptr, _self->ui_visible, true)) {
                _self->ui_visible = !_self->ui_visible;
            }
            ImGui::EndMenu();
        }
    }

} // lab
