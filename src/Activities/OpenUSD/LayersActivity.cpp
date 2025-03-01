//
//  LayersActivity.cpp
//  LabExcelsior
//
//  Created by Domenico Porcino on 1/March/24.
//  Copyright © 2024 Nick Porcino. All rights reserved.
//

#include "LayersActivity.hpp"

#include "Lab/App.h"
#include "imgui.h"
#include "Lab/ImguiExt.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <algorithm>
#include <cmath>

namespace lab {

// Layer information structure
struct LayerInfo {
    std::string name;
    bool muted = false;
    ImColor color;
    float angle = 0.0f;   // Position angle in radians
    bool selected = false;
};

class LayerDomain {
public:
    enum class Type {
        SOUND,
        PICTURE,
        INTERACTION
    };
    std::string name;
    Type type;
};

class USDLayerEditor {
private:
    // Core data
    std::vector<LayerInfo> layers;
    std::string currentEditTarget;

    std::vector<LayerDomain> domains;
    int currentDomain = -1;

    // Renaming state
    bool isRenaming = false;
    size_t renameLayerIndex = 0;
    char renameBuffer[128] = "";

    // Drag and drop state
    int draggedLayerIndex = -1;

    // Color allocation tracking
    size_t nextColorIndex = 0;

    // Animation state
    struct {
        bool animating = false;
        float duration = 1.0f; // Animation duration in seconds
        float timer = 0.0f;
        std::vector<float> startAngles;
        std::vector<float> targetAngles;
    } animation;

    // Visual parameters
    ImVec2 center;
    float innerRadius = 120.0f;
    float ringThickness = 20.0f;
    float maxRadius = 250.0f;

    // Colors
    ImColor storyColor = ImColor(214, 52, 97);    // Center story circle color
    ImColor inactiveColor = ImColor(80, 80, 80);  // Inactive ring color
    ImColor activeColor = ImColor(120, 180, 240); // Active layer color
    ImColor textColor = ImColor(255, 255, 255);   // Text color

    // UI state
    bool needsLayout = true;

    // Color palette for layers
    std::vector<ImColor> colorPalette = {
        ImColor(150, 200, 255),  // Light blue
        ImColor(150, 255, 150),  // Light green
        ImColor(255, 200, 150),  // Light orange
        ImColor(200, 150, 255),  // Light purple
        ImColor(255, 150, 150),  // Light red
        ImColor(255, 255, 150),  // Light yellow
        ImColor(150, 255, 255)   // Light cyan
    };

    // Allocate a color from the palette
    ImColor AllocateNextColor() {
        ImColor color = colorPalette[nextColorIndex % colorPalette.size()];
        nextColorIndex++;
        return color;
    }

    // Utility methods
    void LayoutLayers() {
        // Store current angles as start angles for animation
        animation.startAngles.resize(layers.size());
        animation.targetAngles.resize(layers.size());

        for (size_t i = 0; i < layers.size(); i++) {
            animation.startAngles[i] = layers[i].angle;
        }

        // Calculate target angles
        const float startAngle = IM_PI / 2.0f - IM_PI / 6.0f;    // Bottom - 30 degrees
        const float endAngle = -IM_PI / 2.0f + IM_PI / 6.0f; // Top + 30 degrees

        float totalAngle = endAngle - startAngle;
        float buttonAngleSpace = totalAngle / (layers.size() - 1); // Skip the root layer

        for (size_t i = 0; i < layers.size(); i++) {
            animation.targetAngles[i] = startAngle + (i - 1) * buttonAngleSpace;
        }

        // Start animation
        animation.animating = true;
        animation.timer = 0.0f;

        needsLayout = false;
    }

    // Update animation state
    void UpdateAnimation(float deltaTime) {
        if (!animation.animating) return;

        animation.timer += deltaTime;
        float progress = animation.timer / animation.duration;

        if (progress >= 1.0f) {
            // Animation complete
            for (size_t i = 0; i < layers.size(); i++) {
                layers[i].angle = animation.targetAngles[i];
            }
            animation.animating = false;
        } else {
            // Interpolate angles
            for (size_t i = 0; i < layers.size(); i++) {
                // Use smoothstep for easing
                float t = progress;
                t = t * t * (3.0f - 2.0f * t); // Smoothstep formula

                layers[i].angle = animation.startAngles[i] +
                    (animation.targetAngles[i] - animation.startAngles[i]) * t;
            }
        }
    }

    ImVec2 AngleToPosition(float angle, float radius) {
        return ImVec2(
            center.x + radius * cosf(angle),
            center.y + radius * sinf(angle)
        );
    }

public:
    // Returns true if animation is in progress, for keep-alive functionality
    bool IsAnimating() const {
        return animation.animating;
    }
    USDLayerEditor() {
        // Initialize with STORY as the first layer
        layers.push_back({"STORY", false, storyColor, 0.0f, false});

        // Add the rest of the initial layers with specific names and colors
        layers.push_back({"EDITORIAL", false, AllocateNextColor(), 0.0f, false});
        layers.push_back({"SETS", false, AllocateNextColor(), 0.0f, false});
        layers.push_back({"CAMERA", false, AllocateNextColor(), 0.0f, false});
        layers.push_back({"DIEGETIC SOUND", false, AllocateNextColor(), 0.0f, false});
        layers.push_back({"LOOK", false, AllocateNextColor(), 0.0f, false});
        layers.push_back({"LIGHTING", false, AllocateNextColor(), 0.0f, false});
        layers.push_back({"SIM", false, AllocateNextColor(), 0.0f, false});
        layers.push_back({"ANIMATE", false, AllocateNextColor(), 0.0f, false});
        layers.push_back({"MIMETIC SOUND", false, AllocateNextColor(), 0.0f, false});

        // Set initial edit target and layout the layers
        currentEditTarget = layers[0].name;

        // Position the buttons on the right side
        LayoutLayers();

        domains.push_back({"SOUND", LayerDomain::Type::SOUND});
        domains.push_back({"PICTURE", LayerDomain::Type::PICTURE});
        domains.push_back({"INTERACTION", LayerDomain::Type::INTERACTION});
    }

    void SetDomain(const std::string& label) {
        int i = 0;
        for (auto& d : domains) {
            if (d.name == label) {
                currentDomain = i;
                break;
            }
            ++i;
        }
    }

    void SetEditTarget(const std::string& layerName) {
        currentEditTarget = layerName;
        for (auto& layer : layers) {
            layer.selected = (layer.name == layerName);
        }
    }

    void MuteLayer(const std::string& layerName) {
        for (auto& layer : layers) {
            if (layer.name == layerName) {
                layer.muted = true;
                break;
            }
        }
    }

    void MuteEditTargetsAbove(const std::string& layerName, bool muteState) {
        bool startMuting = false;
        for (auto& layer : layers) {
            if (layer.name == layerName) {
                startMuting = true;
                continue;
            }

            if (startMuting) {
                layer.muted = muteState;
            }
        }
    }

    void AddLayerAbove(const std::string& layerName) {
        auto it = std::find_if(layers.begin(), layers.end(),
                              [&layerName](const LayerInfo& layer) {
                                  return layer.name == layerName;
                              });

        if (it != layers.end()) {
            size_t index = std::distance(layers.begin(), it);
            std::string newName = "NEW LAYER " + std::to_string(layers.size());
            // Assign a unique color from the palette when creating a new layer
            layers.insert(it + 1, LayerInfo{newName, false, AllocateNextColor(), 0.0f, false});
            needsLayout = true;
        }
    }

    void DeleteLayer(const std::string& layerName) {
        if (layerName == "STORY") return; // Don't allow deleting the central story layer

        auto it = std::find_if(layers.begin(), layers.end(),
                              [&layerName](const LayerInfo& layer) {
                                  return layer.name == layerName;
                              });

        if (it != layers.end()) {
            layers.erase(it);
            needsLayout = true;

            // Update current edit target if it was deleted
            if (currentEditTarget == layerName && !layers.empty()) {
                currentEditTarget = layers[0].name;
            }
        }
    }

    void Render(float deltaTime = 1.0f/60.0f) {
        // Update animation if active
        UpdateAnimation(deltaTime);
        ImGuiIO& io = ImGui::GetIO();
        ImDrawList* draw_list = ImGui::GetWindowDrawList();

        // Calculate center of the window for our visualization
        ImVec2 windowPos = ImGui::GetWindowPos();
        ImVec2 windowSize = ImGui::GetWindowSize();
        center = ImVec2(windowPos.x + windowSize.x * 0.55f, windowPos.y + windowSize.y * 0.5f);
        const float dimFactor = 0.6f;

        if (needsLayout) {
            LayoutLayers();
        }

        float maxRingRadius = innerRadius + (layers.size() - 1) * ringThickness;
        float extraDistance = 80.0f; // Distance beyond the outermost ring
        float buttonRadius = maxRingRadius + extraDistance;

        // Draw background (optional)
        draw_list->AddRectFilled(
            windowPos,
            ImVec2(windowPos.x + windowSize.x, windowPos.y + windowSize.y),
            ImColor(30, 30, 30)
        );

        // Draw the "LAYERED COMPOSITIONAL ARCHITECTURE" text
        float textScale = 1.2f;
        ImVec2 textPos = ImVec2(windowPos.x + 30, windowPos.y + windowSize.y - 150);
        draw_list->AddText(
            ImGui::GetFont(),
            ImGui::GetFontSize() * textScale,
            textPos,
            ImColor(100, 180, 240),
            "LAYERED"
        );

        textPos.y += ImGui::GetFontSize() * textScale * 1.2f;
        draw_list->AddText(
            ImGui::GetFont(),
            ImGui::GetFontSize() * textScale,
            textPos,
            ImColor(100, 180, 240),
            "COMPOSITIONAL"
        );

        textPos.y += ImGui::GetFontSize() * textScale * 1.2f;
        draw_list->AddText(
            ImGui::GetFont(),
            ImGui::GetFontSize() * textScale,
            textPos,
            ImColor(100, 180, 240),
            "ARCHITECTURE"
        );

        // Draw major mode buttons on the left
        ImVec2 buttonSize(120, 40);
        ImVec2 buttonPos(windowPos.x + 30, windowPos.y + 150);
        float buttonSpacing = 20.0f;

        // Draw the domains
        for (auto& d : domains) {
            ImGui::SetCursorPos(ImVec2(buttonPos.x - windowPos.x, buttonPos.y - windowPos.y));
            if (ImGui::Button(d.name.c_str(), buttonSize)) {
                SetDomain(d.name);
            }

            // Add arrow shape
            ImVec2 arrowStart(buttonPos.x + buttonSize.x, buttonPos.y + buttonSize.y / 2);
            ImVec2 arrowPoints[3] = {
                ImVec2(arrowStart.x - 10, arrowStart.y - 10),
                arrowStart,
                ImVec2(arrowStart.x - 10, arrowStart.y + 10)
            };

            draw_list->AddTriangleFilled(
                arrowPoints[0],
                arrowPoints[1],
                arrowPoints[2],
                ImColor(100, 180, 240)
            );

            // Draw line from button to the domain arc
            float domainArcRadius = maxRingRadius + 20;
            ImVec2 ray = arrowStart - center;
            float length = sqrtf(ray.x*ray.x + ray.y*ray.y);
            ray *= domainArcRadius / length;
            draw_list->AddLine(arrowStart,
                                ray + center,
                                ImColor(100, 180, 240),
                                2.0f);
            buttonPos.y += buttonSize.y + buttonSpacing;
        }

        // Draw the rings
        for (int i = (int) layers.size() - 1; i >= 0; i--) {
            float radius = innerRadius + i * ringThickness;

            // Draw ring
            ImColor ringColor;
            if (layers[i].name == "STORY") {
                ringColor = storyColor;
            } else {
                // Use layer color with full alpha for selected layers, reduced alpha for inactive
                ImColor baseColor = layers[i].color;
                if (layers[i].muted) {
                    // Muted layers always appear gray
                    ringColor = ImColor(50, 50, 50);
                } else if (layers[i].selected) {
                    // Selected layers at full alpha
                    ringColor = baseColor;
                } else {
                    // dim inactive layers
                    ringColor = ImColor(baseColor.Value.x,
                                        baseColor.Value.y,
                                        baseColor.Value.z,
                                        dimFactor);
                }
            }

            // Draw stroked circle instead of filled to match the diagram
            draw_list->AddCircle(center, radius, ringColor, 64, 8.0f);

            // For the central "STORY" hub
            if (i == 0) {
                // Fill the STORY circle when selected to distinguish it as not draggable
                if (layers[i].selected) {
                    draw_list->AddCircleFilled(center, radius, storyColor, 64);
                }

                draw_list->AddText(
                    ImGui::GetFont(),
                    ImGui::GetFontSize() * 1.5f,
                    ImVec2(center.x - 40, center.y - 10),
                    textColor,
                    "STORY"
                );

                // Make STORY circle clickable for selection
                ImVec2 mousePos = ImGui::GetIO().MousePos;
                float distFromCenter = sqrt(pow(mousePos.x - center.x, 2) + pow(mousePos.y - center.y, 2));

                // Check if mouse is within the STORY circle
                if (distFromCenter <= radius) {
                    // Show tooltip on hover
                    ImGui::BeginTooltip();
                    ImGui::Text("Click to edit the root layer");
                    ImGui::EndTooltip();

                    // Set as edit target when clicked
                    if (ImGui::IsMouseClicked(0)) {
                        SetEditTarget(layers[i].name);
                    }
                }

                continue; // Skip other layer handling for the STORY center
            }

            // Handle drag and drop for rings (except STORY ring)
            if (i > 0) {
                // Calculate a broad area around the ring that's draggable
                float innerArea = radius - ringThickness/2;
                float outerArea = radius + ringThickness/2;

                // Check if mouse is in this ring's area
                ImVec2 mousePos = ImGui::GetIO().MousePos;
                float distFromCenter = sqrt(pow(mousePos.x - center.x, 2) + pow(mousePos.y - center.y, 2));

                if (distFromCenter >= innerArea && distFromCenter <= outerArea) {
                    // Highlight ring when hovered
                    if (!ImGui::GetIO().MouseDown[0]) {
                        draw_list->AddCircle(center, radius, ImColor(255, 255, 100), 64, 2.0f);

                        // Show tooltip on hover
                        ImGui::BeginTooltip();
                        ImGui::Text("Click and drag to reorder layer");
                        ImGui::Text("Layer: %s", layers[i].name.c_str());
                        ImGui::EndTooltip();
                    }

                    // On drag start
                    if (ImGui::IsMouseClicked(0) && draggedLayerIndex == -1) {
                        draggedLayerIndex = i;
                    }
                }
            }
        }

        // Handle active dragging operation
        if (draggedLayerIndex != -1) {
            ImVec2 mousePos = ImGui::GetIO().MousePos;
            float distFromCenter = sqrt(pow(mousePos.x - center.x, 2) + pow(mousePos.y - center.y, 2));

            // Allow continuous movement following the mouse
            // Constrain to valid range - not too close to center, not too far out
            float minDragRadius = innerRadius + ringThickness/2; // At least half a ring from center
            float maxDragRadius = innerRadius + (layers.size() - 0.5) * ringThickness; // Not beyond last ring
            float dragRadius = std::max(minDragRadius, std::min(maxDragRadius, distFromCenter));

            // Draw the ring at the mouse position
            draw_list->AddCircle(center, dragRadius, layers[draggedLayerIndex].color, 64, 8.0f);

            // Draw a "ghost" of the original ring
            float originalRadius = innerRadius + draggedLayerIndex * ringThickness;
            draw_list->AddCircle(center, originalRadius, ImColor(255, 255, 255, 80), 64, 2.0f);

            // Show feedback about the move
            ImGui::SetTooltip("Moving '%s' - Release to place",
                             layers[draggedLayerIndex].name.c_str());

            // On mouse release, perform the actual move
            if (!ImGui::GetIO().MouseDown[0]) {
                // Calculate final position based on mouse release location
                int targetPos = round((dragRadius - innerRadius) / ringThickness);
                targetPos = std::max(1, std::min((int)layers.size() - 1, targetPos));

                if (targetPos != draggedLayerIndex) {
                    // Perform the actual layer move
                    LayerInfo temp = layers[draggedLayerIndex];
                    layers.erase(layers.begin() + draggedLayerIndex);
                    layers.insert(layers.begin() + targetPos, temp);
                    needsLayout = true;
                }

                // Set the dropped layer as the edit target as requested
                SetEditTarget(layers[targetPos].name);

                draggedLayerIndex = -1;
            }
        }

        // Draw layer buttons around the rings
        for (size_t i = 1; i < layers.size(); i++) {
            // Position buttons outside the outermost ring
            ImVec2 pos = AngleToPosition(layers[i].angle, buttonRadius);

            // Adjust button position to be centered relative to the button size
            ImVec2 buttonPos = pos;
            ImVec2 buttonSize(120, 40);

            buttonPos.x -= buttonSize.x / 2;
            buttonPos.y -= buttonSize.y / 2;

            // Draw connection line from ring to button
            float ringRadius = innerRadius + (i) * ringThickness + 3;
            // Update line to connect exactly to the ring's radius, not deeper
            ImVec2 ringPos = AngleToPosition(layers[i].angle, ringRadius);

            // Determine line color and thickness based on selection state
            ImColor lineColor;
            float lineThickness = 4.0f;

            if (layers[i].selected) {
                // Use the layer's color for selected item
                lineColor = layers[i].color;
                lineThickness += 2.0f; // Make line thicker when selected
            } else {
                // Unselected layers are dimmed
                ImColor baseColor = layers[i].color;
                lineColor = ImColor(baseColor.Value.x,
                                    baseColor.Value.y,
                                    baseColor.Value.z,
                                    dimFactor
                );
            }

            draw_list->AddLine(ringPos,
                               ImVec2(pos.x, pos.y),
                               (ImColor){0, 0, 0, 255},
                               lineThickness + 4.f);

            draw_list->AddLine(ringPos,
                               ImVec2(pos.x, pos.y),
                               lineColor,
                               lineThickness);

            // Create a button
            ImGui::SetCursorPos(ImVec2(buttonPos.x - windowPos.x, buttonPos.y - windowPos.y));
            std::string buttonLabel = layers[i].name;
            std::string buttonId = "##layer" + std::to_string(i);

            // Make button look like the blue boxes in the image, but use layer color if selected
            ImVec4 buttonColor = ImVec4(0.4f, 0.7f, 0.9f, 1.0f);
            ImVec4 buttonHoverColor = ImVec4(0.5f, 0.8f, 1.0f, 1.0f);
            ImVec4 buttonActiveColor = ImVec4(0.3f, 0.6f, 0.8f, 1.0f);
            ImVec4 buttonTextColor = ImVec4(0,0,0,1);

            if (layers[i].selected) {
                // Convert ImColor to ImVec4 for button style
                ImColor col = layers[i].color;
                buttonColor = ImVec4(col.Value.x, col.Value.y, col.Value.z, 1.0f);

                // Make hover and active states slightly brighter/darker
                buttonHoverColor = ImVec4(
                    std::min(1.0f, col.Value.x * 1.2f),
                    std::min(1.0f, col.Value.y * 1.2f),
                    std::min(1.0f, col.Value.z * 1.2f),
                    1.0f
                );

                buttonActiveColor = ImVec4(
                    col.Value.x * 0.8f,
                    col.Value.y * 0.8f,
                    col.Value.z * 0.8f,
                    1.0f
                );
            }

            ImGui::PushStyleColor(ImGuiCol_Text, buttonTextColor);
            ImGui::PushStyleColor(ImGuiCol_Button, buttonColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, buttonHoverColor);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, buttonActiveColor);

            if (ImGui::Button((buttonLabel + buttonId).c_str(), buttonSize)) {
                SetEditTarget(layers[i].name);
            }

            ImGui::PopStyleColor(4);

            // Right-click popup menu
            if (ImGui::BeginPopupContextItem(buttonId.c_str())) {
                // Rename option with text input
                static char nameBuffer[128] = "";
                if (ImGui::IsWindowAppearing()) {
                    strncpy(nameBuffer, layers[i].name.c_str(), sizeof(nameBuffer) - 1);
                }

                ImGui::Text("Layer Options");
                ImGui::Separator();

                ImGui::InputText("##rename", nameBuffer, sizeof(nameBuffer));

                if (ImGui::Button("Rename", ImVec2(80, 0))) {
                    // Update the layer name
                    std::string oldName = layers[i].name;
                    layers[i].name = nameBuffer;

                    // Update current edit target if needed
                    if (currentEditTarget == oldName) {
                        currentEditTarget = layers[i].name;
                    }

                    ImGui::CloseCurrentPopup();
                }

                ImGui::Separator();

                // Mute checkbox moved to popup menu
                bool muted = layers[i].muted;
                if (ImGui::Checkbox("Mute Layer", &muted)) {
                    layers[i].muted = muted;
                }

                // Add "Mute Higher Layers" option
                if (ImGui::Button("Mute Higher Layers", ImVec2(150, 0))) {
                    MuteEditTargetsAbove(layers[i].name, true);
                    ImGui::CloseCurrentPopup();
                }
                if (ImGui::Button("Unmute Higher Layers", ImVec2(150, 0))) {
                    MuteEditTargetsAbove(layers[i].name, false);
                    ImGui::CloseCurrentPopup();
                }

                ImGui::Separator();

                if (ImGui::Button("Add Layer Above", ImVec2(150, 0))) {
                    AddLayerAbove(layers[i].name);
                    ImGui::CloseCurrentPopup();
                }

                if (layers[i].name != "STORY" && ImGui::Button("Delete Layer", ImVec2(150, 0))) {
                    DeleteLayer(layers[i].name);
                    ImGui::CloseCurrentPopup();
                }

                ImGui::EndPopup();
            }

            // Add hint about right-click
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Right-click for options");
            }
        }
    }
};

struct LayersActivity::data {
    bool ui_visible = true;
};

LayersActivity::LayersActivity() : Activity(LayersActivity::sname()) {
    _self = new LayersActivity::data();
    
    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<LayersActivity*>(instance)->RunUI(*vi);
    };
}

LayersActivity::~LayersActivity() {
    delete _self;
}

void LayersActivity::RunUI(const LabViewInteraction& vi) {
    if (!_self->ui_visible)
        return;

    static USDLayerEditor editor;
    static float lastFrameTime = ImGui::GetTime();

    // Calculate delta time for animations
    float currentTime = ImGui::GetTime();
    float deltaTime = currentTime - lastFrameTime;
    lastFrameTime = currentTime;

    ImGui::SetNextWindowSize(ImVec2(1200, 800), ImGuiCond_FirstUseEver);
    ImGui::Begin("OpenUSD Layer Editor");

    editor.Render(deltaTime);

    ImGui::End();

    if (editor.IsAnimating()) {
        LabApp::instance()->SuspendPowerSave(10);
    }        
}

} // lab
