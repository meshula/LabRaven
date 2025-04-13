#include "ThreePanelLayout.h"

#include "imgui.h"
#include "imgui_internal.h"

bool DrawPanelHeader(const std::string& title, bool* isLocked, float width, float height) {
    ImGuiStyle& style = ImGui::GetStyle();
    bool hideClicked = false;
    
    float startX = ImGui::GetCursorPosX();
    float buttonSize = height - 6;

    // Background
    ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(ImGuiCol_MenuBarBg));
    ImGui::Button("##Header", ImVec2(width - buttonSize * 2 - 10, height));
    ImGui::PopStyleColor();
    
    // Title text
    ImVec2 textSize = ImGui::CalcTextSize(title.c_str());
    ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + 5,
                               ImGui::GetCursorPosY() - height + (height - textSize.y) * 0.5f));
    ImGui::Text("%s", title.c_str());
    
    ImGui::SameLine();
    float buttonX = startX + width - buttonSize * 2 - 10;
    ImGui::SetCursorPosX(buttonX);
    float buttonY = ImGui::GetCursorPosY() - textSize.y * 0.5f;
    ImGui::SetCursorPosY(buttonY);

    ImGui::PushID((title + "Lock").c_str());
    if (ImGui::Button(*isLocked ? "L" : "U", ImVec2(buttonSize, buttonSize))) {
        *isLocked = !(*isLocked);
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(*isLocked ? "Unlock panel" : "Lock panel");
    }
    ImGui::PopID();
    ImGui::SameLine(buttonX + buttonSize + 2);
    ImGui::PushID((title + "Hide").c_str());
    ImGui::SetCursorPosY(buttonY);
    if (ImGui::Button("X", ImVec2(buttonSize, buttonSize))) {
        hideClicked = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Hide panel");
    }
    ImGui::PopID();
    return hideClicked;
}


void ThreePanelLayout(
    ThreePanelLayoutState& state,
    const std::function<void()>& leftPanelFn,
    const std::function<void()> middlePanelFns[],
    const std::function<void()>& rightPanelFn,
    const char* tabLabels[],
    int tabCount,
    float height
) {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec2 windowPos = ImGui::GetCursorScreenPos();
    ImVec2 availSize = ImGui::GetContentRegionAvail();
    
    // If height is specified, use it; otherwise use all available height
    if (height > 0.0f) {
        availSize.y = height;
    }
    
    // Calculate pixel positions and widths from normalized state
    float totalWidth = availSize.x - state.rightEdgePadding; // Account for right edge padding
    float leftGrabberPosPixels = totalWidth * state.leftGrabberPos;
    float rightGrabberPosPixels = totalWidth * state.rightGrabberPos;
    float mergedGrabberPosPixels = totalWidth * state.mergedGrabberPos;
    
    // Handle min width constraints
    float minWidthNormalized = state.minPanelWidth / totalWidth;
    
    // Determine stuck state for left grabber
    if (state.leftGrabberPos <= minWidthNormalized && !state.grabbersAreMerged) {
        state.leftGrabberStuck = true;
        state.leftPanelVisible = false;
        state.leftGrabberPos = 0.0f;
        leftGrabberPosPixels = 0.0f;
    }
    
    // Determine stuck state for right grabber
    if (state.rightGrabberPos >= (1.0f - minWidthNormalized) && !state.grabbersAreMerged) {
        state.rightGrabberStuck = true;
        state.rightPanelVisible = false;
        state.rightGrabberPos = 1.0f;
        rightGrabberPosPixels = totalWidth;
    }
    
    // Handle merged grabber stuck states
    if (state.grabbersAreMerged) {
        if (state.mergedGrabberPos <= minWidthNormalized) {
            state.leftPanelVisible = false;
            state.middlePanelVisible = false;
            state.rightPanelVisible = true;
            state.mergedGrabberPos = 0.0f;
            mergedGrabberPosPixels = 0.0f;
        } else if (state.mergedGrabberPos >= (1.0f - minWidthNormalized)) {
            state.leftPanelVisible = true;
            state.middlePanelVisible = false;
            state.rightPanelVisible = false;
            state.mergedGrabberPos = 1.0f;
            mergedGrabberPosPixels = totalWidth;
        }
    }
    
    float xCursor = 0.0f;
    
    // ===== DRAW PANELS AND GRABBERS =====
    
    // LEFT PANEL
    if (state.leftPanelVisible && !state.grabbersAreMerged) {
        // Calculate left panel width (from left edge to left grabber)
        float leftPanelWidth = leftGrabberPosPixels;
        
        // Draw left panel
        ImGui::BeginChild("LeftPanel", ImVec2(leftPanelWidth, availSize.y), true);
        
        // Draw header if enabled
        if (state.headerHeight > 0) {
            if (DrawPanelHeader(state.leftPanelTitle, &state.leftPanelLocked, leftPanelWidth - 16, state.headerHeight)) {
                // Hide button was clicked
                state.leftPanelVisible = false;
                state.leftGrabberStuck = true;
                state.leftGrabberPos = 0.0f;
                leftGrabberPosPixels = 0.0f;
                ImGui::EndChild();
                
                // Need to re-calculate everything in the next frame
                return;
            }
            ImGui::Separator();
        }
        
        // Panel content
        leftPanelFn();
        ImGui::EndChild();
        
        xCursor += leftPanelWidth;
    } else if (state.leftPanelVisible && state.grabbersAreMerged) {
        // When grabbers are merged, left panel goes from left edge to merged grabber
        float leftPanelWidth = mergedGrabberPosPixels;
        
        // Draw left panel
        ImGui::BeginChild("LeftPanel", ImVec2(leftPanelWidth, availSize.y), true);
        
        // Draw header if enabled
        if (state.headerHeight > 0) {
            if (DrawPanelHeader(state.leftPanelTitle, &state.leftPanelLocked, leftPanelWidth - 16, state.headerHeight)) {
                // Hide button was clicked
                state.leftPanelVisible = false;
                state.rightPanelVisible = true;
                state.mergedGrabberPos = 0.0f;
                mergedGrabberPosPixels = 0.0f;
                ImGui::EndChild();
                
                // Need to re-calculate everything in the next frame
                return;
            }
            ImGui::Separator();
        }
        
        // Panel content
        leftPanelFn();
        ImGui::EndChild();
        
        xCursor += leftPanelWidth;
    }
    
    // LEFT GRABBER
    if (!state.grabbersAreMerged) {
        ImGui::SetCursorScreenPos(ImVec2(windowPos.x + xCursor, windowPos.y));
        
        // Draw the left grabber even when stuck - just at the edge
        ImGui::InvisibleButton("LeftGrabber", ImVec2(state.grabberWidth, availSize.y), ImGuiButtonFlags_NoNavFocus);
        
        // Handle unsticking with click when stuck to edge
        if (state.leftGrabberStuck && ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
            // Unstick the grabber and set to a reasonable initial position
            state.leftGrabberStuck = false;
            state.leftPanelVisible = true;
            state.leftGrabberPos = 0.25f; // Move to 25% position
            leftGrabberPosPixels = totalWidth * state.leftGrabberPos;
        }
        // Handle normal dragging when not stuck
        else if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            float dragAmount = ImGui::GetIO().MouseDelta.x / totalWidth; // Normalize drag amount
            
            // Remember where we were before the drag to determine direction
            float prevLeftGrabberPos = state.leftGrabberPos;
            
            // Calculate new position
            float newLeftPos = ImClamp(state.leftGrabberPos + dragAmount, 0.0f,
                                       state.rightGrabberPos - minWidthNormalized);
            
            // Check if left panel is locked
            if (state.leftPanelLocked) {
                // If locked, don't allow changes to the position
                dragAmount = 0.0f;
                newLeftPos = state.leftGrabberPos;
            }
            
            // Determine if we're moving toward or away from the right grabber
            bool movingTowardRight = (newLeftPos > prevLeftGrabberPos);
            
            // Check if grabbers would cross (left tries to go past right)
            if (state.leftGrabberPos + dragAmount > state.rightGrabberPos) {
                state.grabbersAreMerged = true;
                state.middlePanelVisible = false;
                state.mergedGrabberPos = (state.leftGrabberPos + state.rightGrabberPos) / 2.0f;
                mergedGrabberPosPixels = totalWidth * state.mergedGrabberPos;
            }
            // Check if grabbers would get too close AND we're moving toward the right grabber
            else if (movingTowardRight && 
                    (state.rightGrabberPos - newLeftPos < minWidthNormalized * 2)) {
                state.grabbersAreMerged = true;
                state.middlePanelVisible = false;
                state.mergedGrabberPos = (state.leftGrabberPos + state.rightGrabberPos) / 2.0f;
                mergedGrabberPosPixels = totalWidth * state.mergedGrabberPos;
            }
            else {
                // Normal update
                state.leftGrabberPos = newLeftPos;
                leftGrabberPosPixels = totalWidth * state.leftGrabberPos;
            }
        }
        
        // Draw grabber visual with special coloring when stuck
        ImU32 grabberColor;
        if (state.leftGrabberStuck) {
            // Use a brighter/more noticeable color for stuck grabbers
            grabberColor = ImGui::GetColorU32(ImGui::IsItemHovered() ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
        } else if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
            grabberColor = ImGui::GetColorU32(ImGui::IsItemActive() ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered);
        } else {
            grabberColor = ImGui::GetColorU32(ImGuiCol_Button);
        }
        
        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2(windowPos.x + xCursor + 2, windowPos.y),
            ImVec2(windowPos.x + xCursor + state.grabberWidth - 2, windowPos.y + availSize.y),
            grabberColor
        );
        
        xCursor += state.grabberWidth;
    }
    
    // MIDDLE PANEL
    if (state.middlePanelVisible && !state.grabbersAreMerged) {
        // Calculate middle panel width (from left grabber to right grabber)
        float middlePanelWidth = rightGrabberPosPixels - leftGrabberPosPixels - state.grabberWidth;
        
        // Draw middle panel
        ImGui::SetCursorScreenPos(ImVec2(windowPos.x + xCursor, windowPos.y));
        ImGui::BeginChild("MiddlePanel", ImVec2(middlePanelWidth, availSize.y), true);
        
        // Draw header if enabled
        if (state.headerHeight > 0 && tabCount == 0) {
            // Only show header if we're not using tabs
            if (DrawPanelHeader(state.middlePanelTitle, &state.middlePanelLocked, middlePanelWidth - 16, state.headerHeight)) {
                // Hide button was clicked
                state.middlePanelVisible = false;
                state.grabbersAreMerged = true;
                state.mergedGrabberPos = (state.leftGrabberPos + state.rightGrabberPos) / 2.0f;
                mergedGrabberPosPixels = totalWidth * state.mergedGrabberPos;
                ImGui::EndChild();
                
                // Need to re-calculate everything in the next frame
                return;
            }
            ImGui::Separator();
        }
        
        // Tab bar for middle panel
        if (tabCount > 0 && ImGui::BeginTabBar("MiddlePanelTabs")) {
            for (int i = 0; i < tabCount; i++) {
                bool isSelected = (state.currentTabIndex == i);
                if (ImGui::BeginTabItem(tabLabels[i], &isSelected)) {
                    state.currentTabIndex = i;
                    
                    // If header is enabled with tabs, show lock button in tab content area
                    if (state.headerHeight > 0) {
                        float buttonSize = state.headerHeight - 6;
                        ImGui::SameLine(middlePanelWidth - buttonSize - 25);
                        
                        ImGui::PushID("MiddlePanelLock");
                        if (ImGui::Button(state.middlePanelLocked ? "L" : "U", ImVec2(buttonSize, buttonSize))) {
                            state.middlePanelLocked = !state.middlePanelLocked;
                        }
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip(state.middlePanelLocked ? "Unlock panel" : "Lock panel");
                        }
                        ImGui::PopID();
                        
                        ImGui::SameLine(middlePanelWidth - buttonSize - 5);
                        ImGui::PushID("MiddlePanelHide");
                        if (ImGui::Button("X", ImVec2(buttonSize, buttonSize))) {
                            state.middlePanelVisible = false;
                            state.grabbersAreMerged = true;
                            state.mergedGrabberPos = (state.leftGrabberPos + state.rightGrabberPos) / 2.0f;
                            mergedGrabberPosPixels = totalWidth * state.mergedGrabberPos;
                            ImGui::EndTabItem();
                            ImGui::EndTabBar();
                            ImGui::PopID();
                            ImGui::EndChild();

                            // Need to re-calculate everything in the next frame
                            return;
                        }
                        if (ImGui::IsItemHovered()) {
                            ImGui::SetTooltip("Hide panel");
                        }
                        ImGui::PopID();
                        
                        ImGui::Separator();
                    }
                    
                    middlePanelFns[i]();
                    ImGui::EndTabItem();
                }
            }
            ImGui::EndTabBar();
        }
        else if (tabCount == 0) {
            // No tabs, just show content directly
            middlePanelFns[0]();
        }
        
        ImGui::EndChild();
        
        xCursor += middlePanelWidth;
    }
    
    // MERGED GRABBER (replaces both left and right grabbers when merged)
    if (state.grabbersAreMerged) {
        ImGui::SetCursorScreenPos(ImVec2(windowPos.x + mergedGrabberPosPixels, windowPos.y));
        ImGui::InvisibleButton("MergedGrabber", ImVec2(state.grabberWidth, availSize.y), ImGuiButtonFlags_NoNavFocus);
        
        // Handle special case when merged grabber is at an edge
        bool mergedGrabberAtLeftEdge = (state.mergedGrabberPos <= minWidthNormalized);
        bool mergedGrabberAtRightEdge = (state.mergedGrabberPos >= (1.0f - minWidthNormalized));
        
        if ((mergedGrabberAtLeftEdge || mergedGrabberAtRightEdge) && ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
            // Click on edge-stuck merged grabber will unstick it
            if (mergedGrabberAtLeftEdge) {
                state.mergedGrabberPos = 0.25f; // Move to 25% position
                state.leftPanelVisible = true;
                state.rightPanelVisible = true;
            } else { // Right edge
                state.mergedGrabberPos = 0.75f; // Move to 75% position
                state.leftPanelVisible = true;
                state.rightPanelVisible = true;
            }
            mergedGrabberPosPixels = totalWidth * state.mergedGrabberPos;
        }
        // Normal drag operation
        else if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            float dragAmount = ImGui::GetIO().MouseDelta.x / totalWidth; // Normalize drag amount
            
            // Handle both middle panel and right panel locks
            if ((state.middlePanelLocked || state.rightPanelLocked) &&
                !mergedGrabberAtLeftEdge && !mergedGrabberAtRightEdge) {
                // If either panel is locked, don't allow grabber movement
                dragAmount = 0.0f;
            }
            
            // Update merged grabber position
            state.mergedGrabberPos = ImClamp(state.mergedGrabberPos + dragAmount, 0.0f, 1.0f);
            mergedGrabberPosPixels = totalWidth * state.mergedGrabberPos;
            
            // Check edge conditions
            if (state.mergedGrabberPos <= minWidthNormalized) {
                state.leftPanelVisible = false;
                state.rightPanelVisible = true;
            } else if (state.mergedGrabberPos >= (1.0f - minWidthNormalized)) {
                state.leftPanelVisible = true;
                state.rightPanelVisible = false;
            } else {
                state.leftPanelVisible = true;
                state.rightPanelVisible = true;
            }
        }
        
        // Double-click to unmerge (keeping as fallback)
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
            state.grabbersAreMerged = false;
            state.middlePanelVisible = true;
            
            // When unmerging, create space for middle panel based on current position
            if (state.mergedGrabberPos < 0.3f) {
                // If merged grabber is near left edge
                state.leftGrabberPos = 0.2f;
                state.rightGrabberPos = 0.5f;
            } else if (state.mergedGrabberPos > 0.7f) {
                // If merged grabber is near right edge
                state.leftGrabberPos = 0.5f;
                state.rightGrabberPos = 0.8f;
            } else {
                // Normal case - center the middle panel around merged position
                state.leftGrabberPos = state.mergedGrabberPos - 0.15f;
                state.rightGrabberPos = state.mergedGrabberPos + 0.15f;
            }
            
            leftGrabberPosPixels = totalWidth * state.leftGrabberPos;
            rightGrabberPosPixels = totalWidth * state.rightGrabberPos;
        }
        
        // Draw grabber visual with special coloring when at edge
        ImU32 grabberColor;
        if (mergedGrabberAtLeftEdge || mergedGrabberAtRightEdge) {
            // Use a brighter/more noticeable color for edge-stuck grabbers
            grabberColor = ImGui::GetColorU32(ImGui::IsItemHovered() ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
        } else if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
            grabberColor = ImGui::GetColorU32(ImGui::IsItemActive() ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered);
        } else {
            grabberColor = ImGui::GetColorU32(ImGuiCol_Button);
        }
        
        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2(windowPos.x + mergedGrabberPosPixels + 2, windowPos.y),
            ImVec2(windowPos.x + mergedGrabberPosPixels + state.grabberWidth - 2, windowPos.y + availSize.y),
            grabberColor
        );
        
        // Add a split button in the middle of the merged grabber
        float buttonSize = 20.0f;
        float buttonYPos = windowPos.y + availSize.y / 2.0f - buttonSize / 2.0f;
        
        // Push a new ID scope to avoid conflicts
        ImGui::PushID("SplitButton");
        
        // Position the cursor where we want the button
        ImGui::SetCursorScreenPos(ImVec2(windowPos.x + mergedGrabberPosPixels + state.grabberWidth / 2.0f - buttonSize / 2.0f, buttonYPos));
        
        // Draw a round button with a split icon
        if (ImGui::InvisibleButton("##Split", ImVec2(buttonSize, buttonSize), ImGuiButtonFlags_NoNavFocus)) {
            // Unmerge the grabbers
            state.grabbersAreMerged = false;
            state.middlePanelVisible = true;
            
            // When unmerging, create space for middle panel
            float middlePanelRatio = 0.3f; // Middle panel gets 30% of space
            float sideRatio = (1.0f - middlePanelRatio) / 2.0f; // Each side gets 35%
            
            // Calculate new positions based on current merged position
            if (state.mergedGrabberPos < 0.3f) {
                // If merged grabber is near left edge
                state.leftGrabberPos = 0.2f;
                state.rightGrabberPos = 0.5f;
            } else if (state.mergedGrabberPos > 0.7f) {
                // If merged grabber is near right edge
                state.leftGrabberPos = 0.5f;
                state.rightGrabberPos = 0.8f;
            } else {
                // Normal case - center the middle panel around merged position
                state.leftGrabberPos = state.mergedGrabberPos - 0.15f;
                state.rightGrabberPos = state.mergedGrabberPos + 0.15f;
            }
            
            leftGrabberPosPixels = totalWidth * state.leftGrabberPos;
            rightGrabberPosPixels = totalWidth * state.rightGrabberPos;
        }
        
        // Draw a split icon on the button
        ImVec2 buttonMin = ImGui::GetItemRectMin();
        ImVec2 buttonMax = ImGui::GetItemRectMax();
        ImVec2 buttonCenter((buttonMin.x + buttonMax.x) * 0.5f, (buttonMin.y + buttonMax.y) * 0.5f);
        
        // Draw split icon (vertical line with arrows pointing out)
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        float lineHalfLength = buttonSize * 0.3f;
        float arrowSize = buttonSize * 0.15f;
        
        // Vertical line
        drawList->AddLine(
            ImVec2(buttonCenter.x, buttonCenter.y - lineHalfLength),
            ImVec2(buttonCenter.x, buttonCenter.y + lineHalfLength),
            ImGui::GetColorU32(ImGuiCol_Text),
            2.0f
        );
        
        // Left arrow
        drawList->AddTriangleFilled(
            ImVec2(buttonCenter.x - arrowSize * 2, buttonCenter.y),
            ImVec2(buttonCenter.x - arrowSize, buttonCenter.y - arrowSize),
            ImVec2(buttonCenter.x - arrowSize, buttonCenter.y + arrowSize),
            ImGui::GetColorU32(ImGuiCol_Text)
        );
        
        // Right arrow
        drawList->AddTriangleFilled(
            ImVec2(buttonCenter.x + arrowSize * 2, buttonCenter.y),
            ImVec2(buttonCenter.x + arrowSize, buttonCenter.y - arrowSize),
            ImVec2(buttonCenter.x + arrowSize, buttonCenter.y + arrowSize),
            ImGui::GetColorU32(ImGuiCol_Text)
        );
        
        ImGui::PopID(); // End of split button ID scope
        
        xCursor = mergedGrabberPosPixels + state.grabberWidth;
    }
    // RIGHT GRABBER
    if (!state.grabbersAreMerged) {
        ImGui::SetCursorScreenPos(ImVec2(windowPos.x + rightGrabberPosPixels, windowPos.y));
        ImGui::InvisibleButton("RightGrabber", ImVec2(state.grabberWidth, availSize.y), ImGuiButtonFlags_NoNavFocus);
        
        // Handle unsticking with click when stuck to edge
        if (state.rightGrabberStuck && ImGui::IsItemClicked(ImGuiMouseButton_Left)) {
            // Unstick the grabber and set to a reasonable initial position
            state.rightGrabberStuck = false;
            state.rightPanelVisible = true;
            state.rightGrabberPos = 0.75f; // Move to 75% position
            rightGrabberPosPixels = totalWidth * state.rightGrabberPos;
        }
        // Handle normal dragging when not stuck
        else if (ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left)) {
            float dragAmount = ImGui::GetIO().MouseDelta.x / totalWidth; // Normalize drag amount
            
            // Remember where we were before the drag to determine direction
            float prevRightGrabberPos = state.rightGrabberPos;
            
            // Calculate new position
            float newRightPos = ImClamp(state.rightGrabberPos + dragAmount, state.leftGrabberPos + minWidthNormalized, 1.0f);
            
            // Check if right panel is locked
            if (state.rightPanelLocked) {
                // If locked, don't allow changes to the position
                dragAmount = 0.0f;
                newRightPos = state.rightGrabberPos;
            }
            
            // Determine if we're moving toward or away from the left grabber
            bool movingTowardLeft = (newRightPos < prevRightGrabberPos);
            
            // Check if grabbers would cross (right tries to go past left)
            if (state.rightGrabberPos + dragAmount < state.leftGrabberPos) {
                state.grabbersAreMerged = true;
                state.middlePanelVisible = false;
                state.mergedGrabberPos = (state.leftGrabberPos + state.rightGrabberPos) / 2.0f;
                mergedGrabberPosPixels = totalWidth * state.mergedGrabberPos;
            }
            // Check if grabbers would get too close AND we're moving toward the left grabber
            else if (movingTowardLeft && 
                    (newRightPos - state.leftGrabberPos < minWidthNormalized * 2)) {
                state.grabbersAreMerged = true;
                state.middlePanelVisible = false;
                state.mergedGrabberPos = (state.leftGrabberPos + state.rightGrabberPos) / 2.0f;
                mergedGrabberPosPixels = totalWidth * state.mergedGrabberPos;
            }
            else {
                // Normal update
                state.rightGrabberPos = newRightPos;
                rightGrabberPosPixels = totalWidth * state.rightGrabberPos;
            }
        }
        
        // Draw grabber visual with special coloring when stuck
        ImU32 grabberColor;
        if (state.rightGrabberStuck) {
            // Use a brighter/more noticeable color for stuck grabbers
            grabberColor = ImGui::GetColorU32(ImGui::IsItemHovered() ? ImGuiCol_HeaderHovered : ImGuiCol_Header);
        } else if (ImGui::IsItemHovered() || ImGui::IsItemActive()) {
            grabberColor = ImGui::GetColorU32(ImGui::IsItemActive() ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered);
        } else {
            grabberColor = ImGui::GetColorU32(ImGuiCol_Button);
        }
        
        ImGui::GetWindowDrawList()->AddRectFilled(
            ImVec2(windowPos.x + rightGrabberPosPixels + 2, windowPos.y),
            ImVec2(windowPos.x + rightGrabberPosPixels + state.grabberWidth - 2, windowPos.y + availSize.y),
            grabberColor
        );
        
        xCursor = rightGrabberPosPixels + state.grabberWidth;
    }
    
    // RIGHT PANEL
    if (state.rightPanelVisible) {
        // Calculate right panel width
        float rightStart;
        float rightWidth;
        
        if (state.grabbersAreMerged) {
            // When grabbers are merged, right panel starts at merged grabber
            rightStart = mergedGrabberPosPixels + state.grabberWidth;
            rightWidth = totalWidth - rightStart;
        } else {
            // Normal case, right panel starts at right grabber
            rightStart = rightGrabberPosPixels + state.grabberWidth;
            rightWidth = totalWidth - rightStart;
        }
        
        // Draw right panel
        ImGui::SetCursorScreenPos(ImVec2(windowPos.x + rightStart, windowPos.y));
        ImGui::BeginChild("RightPanel", ImVec2(rightWidth, availSize.y), true);
        
        // Draw header if enabled
        if (state.headerHeight > 0) {
            if (DrawPanelHeader(state.rightPanelTitle, &state.rightPanelLocked, rightWidth - 16, state.headerHeight)) {
                // Hide button was clicked
                state.rightPanelVisible = false;
                
                if (state.grabbersAreMerged) {
                    state.leftPanelVisible = true;
                    state.mergedGrabberPos = 1.0f;
                    mergedGrabberPosPixels = totalWidth;
                } else {
                    state.rightGrabberStuck = true;
                    state.rightGrabberPos = 1.0f;
                    rightGrabberPosPixels = totalWidth;
                }
                
                ImGui::EndChild();
                
                // Need to re-calculate everything in the next frame
                return;
            }
            ImGui::Separator();
        }
        
        // Panel content
        rightPanelFn();
        ImGui::EndChild();
    }
    
    // Add a small invisible area at the far right to prevent window resize from affecting grabber
    if (state.rightEdgePadding > 0) {
        ImGui::SetCursorScreenPos(ImVec2(windowPos.x + totalWidth, windowPos.y));
        ImGui::InvisibleButton("RightEdgePadding", ImVec2(state.rightEdgePadding, availSize.y));
    }
}

void ThreePanelLayoutDemo() {
    // Initialize or retrieve persistent state
    static ThreePanelLayoutState layoutState;
    
    // Setup middle panel tabs
    const char* tabLabels[] = { "Foo", "Bar", "Baz" };
    const int tabCount = IM_ARRAYSIZE(tabLabels);
    
    // Define panel content functions
    auto leftPanelFn = []() {
        ImGui::Text("Left Panel Content");
        ImGui::Separator();
        for (int i = 0; i < 10; i++) {
            ImGui::Text("Left panel item %d", i);
        }
    };
    
    // Middle panel tab functions
    std::function<void()> middlePanelFns[] = {
        []() { // Foo tab
            ImGui::Text("Foo Tab Content");
            ImGui::Separator();
            ImGui::Text("This is the Foo tab of the middle panel");
            ImGui::Button("Foo Button");
        },
        []() { // Bar tab
            ImGui::Text("Bar Tab Content");
            ImGui::Separator();
            ImGui::Text("This is the Bar tab of the middle panel");
            float value = 0.5f;
            ImGui::SliderFloat("Bar Slider", &value, 0.0f, 1.0f);
        },
        []() { // Baz tab
            ImGui::Text("Baz Tab Content");
            ImGui::Separator();
            ImGui::Text("This is the Baz tab of the middle panel");
            if (ImGui::Button("Baz Button")) {
                // Do something when clicked
            }
        }
    };
    
    auto rightPanelFn = []() {
        ImGui::Text("Right Panel Content");
        ImGui::Separator();
        static bool options[5];
        for (int i = 0; i < 5; i++) {
            ImGui::Checkbox(("Right panel option " + std::to_string(i)).c_str(), &options[i]);
        }
    };
    
    // Render the three-panel layout
    ThreePanelLayout(
        layoutState,
        leftPanelFn,
        middlePanelFns,
        rightPanelFn,
        tabLabels,
        tabCount
    );
}
