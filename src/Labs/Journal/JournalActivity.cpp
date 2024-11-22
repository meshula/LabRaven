//
//  JournalActivity.cpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 1/7/24.
//  Copyright © 2024 Nick Porcino. All rights reserved.
//

#include "JournalActivity.hpp"
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include <vector>

namespace lab {

struct JournalActivity::data {
    bool uiVisible = true;
    JournalNode* selection = nullptr;
};

JournalActivity::JournalActivity() : Activity() {
    _self = new JournalActivity::data;
    
    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<JournalActivity*>(instance)->RunUI(*vi);
    };
}

JournalActivity::~JournalActivity() {
    delete _self;
}

ImGuiTreeNodeFlags JournalActivity::ComputeDisplayFlags(JournalNode* node)
{
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
    
    // set the flag if leaf or not
    if (node->sibling == nullptr) {
        flags |= ImGuiTreeNodeFlags_Leaf;
        flags |= ImGuiTreeNodeFlags_Bullet;
    }
    
    // if selected prim, set highlight flag
    if (node == _self->selection) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    return flags;
}

bool JournalActivity::DrawJournalNode(JournalNode* node)
{
    const char* name = node->transaction.message.c_str();
    ImGuiTreeNodeFlags flags = ComputeDisplayFlags(node);
    ImGui::PushID(node);
    bool ret = ImGui::TreeNodeEx(name, flags);
    ImGui::PopID();
    return ret;
}


void JournalActivity::DrawChildrendHierarchyDecoration(
                                ImRect parentRect,
                                std::vector<ImRect> childrenRects)
{
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    const ImColor lineColor = ImGui::GetColorU32(ImGuiCol_Text, 0.25f);
    
    // all decoration for children will start at horizStartPos
    float horizStartPos = parentRect.Min.x + 10.0f;  // hard coded
    
    // save the lineStart of the last children to draw the vertical
    // line from the parent to the last child
    ImVec2 lineStart;
    
    // loop over all children and draw every horizontal line
    for (auto rect : childrenRects) {
        const float midpoint = (rect.Min.y + rect.Max.y) / 2.0f;
        const float lineSize = 8.0f;  // hard coded
        
        lineStart = ImVec2(horizStartPos, midpoint);
        ImVec2 lineEnd = lineStart + ImVec2(lineSize, 0);
        
        drawList->AddLine(lineStart, lineEnd, lineColor);
    }
    
    // draw the vertical line from the parent to the last child
    ImVec2 lineEnd = ImVec2(horizStartPos, parentRect.Max.y + 2.0f);
    drawList->AddLine(lineStart, lineEnd, lineColor);
}

// returns the node's rectangle
ImRect JournalActivity::DrawJournalHierarchy(JournalNode* node)
{
    bool recurse = DrawJournalNode(node);
    
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        _self->selection = node;
    }
    
    const ImRect curItemRect =
        ImRect(ImGui::GetItemRectMin(), ImGui::GetItemRectMax());
    
    if (recurse) {
        // draw all children and store their rect position
        std::vector<ImRect> rects;
        JournalNode* curr = node->sibling;
        while (curr) {
            ImRect childRect = DrawJournalHierarchy(curr);
            rects.push_back(childRect);
            curr = curr->sibling;
        }
        
        if (rects.size() > 0) {
            // draw hierarchy decoration for all children
            DrawChildrendHierarchyDecoration(curItemRect, rects);
        }
        
        ImGui::TreePop();
    }
    
    return curItemRect;
}

void JournalActivity::RunUI(const LabViewInteraction&) {
    if (!_self->uiVisible)
        return;

    auto orchestrator = Orchestrator::Canonical();

    ImGui::SetNextWindowSize(ImVec2(200, 400), ImGuiCond_FirstUseEver);
    ImGui::Begin("Journal", &_self->uiVisible, ImGuiWindowFlags_NoScrollWithMouse);
    
    if (ImGui::Button("Jump")) {
        
    }
    ImGui::SameLine();
    if (ImGui::Button("Fork")) {
        
    }
    ImGui::SameLine();
    if (ImGui::Button("Trim")) {
    }

    ImGui::BeginChild("###Journal");
    auto& journal = orchestrator->Journal();
    auto curr = &journal.root;
    while (curr) {
        DrawJournalHierarchy(curr);
        curr = curr->next;
    }
    ImGui::EndChild();
    ImGui::End();
}

} // lab
