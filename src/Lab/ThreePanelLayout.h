#ifndef IMGUI_THREE_PANEL_LAYOUT_H
#define IMGUI_THREE_PANEL_LAYOUT_H

#include <imgui.h>
#include <functional>
#include <string>

/**
 * @brief State structure for the ThreePanelLayout widget
 * 
 * This structure maintains the state of the ThreePanelLayout across frames.
 * It tracks the position of grabbers, panel visibility, and the current tab index
 * for the middle panel.
 */
struct ThreePanelLayoutState {
    // Normalized grabber positions (0.0-1.0 of total available width)
    float leftGrabberPos = 0.25f;    // Default: 25% of width
    float rightGrabberPos = 0.75f;   // Default: 75% of width
    
    // Panel visibility states
    bool leftPanelVisible = true;
    bool middlePanelVisible = true;
    bool rightPanelVisible = true;
    
    // Panel lock states (when locked, panel width is constrained)
    bool leftPanelLocked = false;
    bool middlePanelLocked = false;
    bool rightPanelLocked = false;
    
    // Panel titles
    std::string leftPanelTitle = "Left Panel";
    std::string middlePanelTitle = "Middle Panel";
    std::string rightPanelTitle = "Right Panel";
    
    // Merged state tracking
    bool grabbersAreMerged = false;
    float mergedGrabberPos = 0.5f;   // Position when grabbers are merged
    
    // Middle panel tab state
    int currentTabIndex = 0;
    
    // Minimum panel widths (in pixels)
    float minPanelWidth = 150.0f;
    
    // Grabber width (in pixels)
    float grabberWidth = 10.0f;
    
    // Right edge padding (in pixels)
    float rightEdgePadding = 5.0f;
    
    // Header height (in pixels, 0 to disable headers)
    float headerHeight = 25.0f;
    
    // Panel stuck to edges
    bool leftGrabberStuck = false;
    bool rightGrabberStuck = false;
    
    // Initialize with default values or custom settings
    ThreePanelLayoutState() = default;
    
    ThreePanelLayoutState(float leftPos, float rightPos, float minWidth = 150.0f, float grabWidth = 10.0f, float rightPadding = 5.0f, float hdrHeight = 25.0f)
        : leftGrabberPos(leftPos), rightGrabberPos(rightPos),
          minPanelWidth(minWidth), grabberWidth(grabWidth), rightEdgePadding(rightPadding), headerHeight(hdrHeight) {}
};

/**
 * @brief Helper function to draw a panel header with title, lock, and hide buttons
 * 
 * @param title Title to display in the header
 * @param isLocked Pointer to lock state
 * @param width Width of the header
 * @param height Height of the header
 * @return true if the hide button was clicked
 */
bool DrawPanelHeader(const std::string& title, bool* isLocked, float width, float height);

/**
 * @brief A three-panel layout widget with collapsible panels and resizable columns
 * 
 * The ThreePanelLayout provides a flexible UI component consisting of three panels:
 * left, middle, and right. Each panel can be resized using draggable grabbers.
 * 
 * User Interaction:
 * - Drag the grabber areas between panels to resize them
 * - If a grabber is dragged to an edge, it "sticks" and the panel becomes hidden
 * - Unstick a grabber by clicking on it
 * - If the left and right grabbers are pushed together, they merge into a single
 *   middle grabber, and the middle panel becomes hidden
 * - The middle grabber can be dragged to adjust the proportion of left and right panels
 * - The middle panel supports tabs for switching between different content views
 * - When headers are enabled, each panel has a title, lock button, and hide button
 * 
 * Panel States:
 * - Normal: All three panels visible and independently resizable
 * - Left Hidden: Left grabber stuck to left edge, left panel not drawn
 * - Right Hidden: Right grabber stuck to right edge, right panel not drawn
 * - Middle Hidden: Left and right grabbers merged, middle panel not drawn
 * - Left+Middle Hidden: Merged grabber stuck to left edge, only right panel visible
 * - Right+Middle Hidden: Merged grabber stuck to right edge, only left panel visible
 * - Locked Panels: When a panel is locked, its width cannot be changed
 * 
 * @param state Persistent state for the layout widget
 * @param leftPanelFn Function to render the left panel contents
 * @param middlePanelFns Array of functions to render the middle panel's tabbed contents
 * @param rightPanelFn Function to render the right panel contents
 * @param tabLabels Labels for the middle panel tabs
 * @param tabCount Number of tabs in the middle panel
 * @param height Height of the entire layout (use -1 for remaining window height)
 * @return void
 */
void ThreePanelLayout(
    ThreePanelLayoutState& state,
    const std::function<void()>& leftPanelFn,
    const std::function<void()> middlePanelFns[],
    const std::function<void()>& rightPanelFn,
    const char* tabLabels[],
    int tabCount,
    float height = -1.0f
);

/**
 * @brief Example function demonstrating usage of ThreePanelLayout
 * 
 * This function creates a simple demo with stub implementations for the panel functions.
 */
void ThreePanelLayoutDemo();

#endif // IMGUI_THREE_PANEL_LAYOUT_H
