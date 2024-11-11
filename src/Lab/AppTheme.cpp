
#include "AppTheme.h"

AppTheme appTheme;

const char* AppThemeColor_Names[] = {
    "Background",
    "Label",
    "Tick Major",
    "Tick Minor",
    "Gap Hovered",
    "Gap Selected",
    "Item",
    "Item Hovered",
    "Item Selected",
    "Transition",
    "Transition Line",
    "Transition Hovered",
    "Transition Selected",
    "Effect",
    "Effect Hovered",
    "Effect Selected",
    "Playhead",
    "Playhead Line",
    "Playhead Hovered",
    "Playhead Selected",
    "Marker Hovered",
    "Marker Selected",
    "Track",
    "Track Hovered",
    "Track Selected",
    "Invalid"
};

void ApplyAppStyle() {
    ImGuiStyle& style = ImGui::GetStyle();

    ImGui::StyleColorsDark();

    style.Alpha = 1.0f; // Global alpha applies to everything in ImGui
    style.WindowPadding = ImVec2(8, 8); // Padding within a window
    style.WindowRounding = 7.0f; // Radius of window corners rounding. Set to 0.0f to have rectangular windows
    style.WindowBorderSize = 1.0f; // Thickness of border around windows. Generally set to 0.0f or 1.0f. Other values not well tested.
    style.WindowMinSize = ImVec2(32, 32); // Minimum window size
    style.WindowTitleAlign = ImVec2(0.0f, 0.5f); // Alignment for title bar text
    style.ChildRounding = 0.0f; // Radius of child window corners rounding. Set to 0.0f to have rectangular child windows
    style.ChildBorderSize = 1.0f; // Thickness of border around child windows. Generally set to 0.0f or 1.0f. Other values not well tested.
    style.PopupRounding = 0.0f; // Radius of popup window corners rounding. Set to 0.0f to have rectangular child windows
    style.PopupBorderSize = 1.0f; // Thickness of border around popup or tooltip windows. Generally set to 0.0f or 1.0f. Other values not well tested.
    style.FramePadding = ImVec2(
        4,
        3); // Padding within a framed rectangle (used by most widgets)
    style.FrameRounding = 2.0f; // Radius of frame corners rounding. Set to 0.0f to have rectangular frames (used by most widgets).
    style.FrameBorderSize = 1.0f; // Thickness of border around frames. Generally set to 0.0f or 1.0f. Other values not well tested.
    style.ItemSpacing = ImVec2(8, 4); // Horizontal and vertical spacing between widgets/lines
    style.ItemInnerSpacing = ImVec2(
        4,
        4); // Horizontal and vertical spacing between within elements of a composed widget (e.g. a slider and its label)
    style.TouchExtraPadding = ImVec2(
        0,
        0); // Expand reactive bounding box for touch-based system where touch position is not accurate enough. Unfortunately we don't sort widgets so priority on overlap will always be given to the first widget. So don't grow this too much!
    style.IndentSpacing = 21.0f; // Horizontal spacing when e.g. entering a tree node. Generally == (FontSize + FramePadding.x*2).
    style.ColumnsMinSpacing = 6.0f; // Minimum horizontal spacing between two columns
    style.ScrollbarSize = 16.0f; // Width of the vertical scrollbar, Height of the horizontal scrollbar
    style.ScrollbarRounding = 9.0f; // Radius of grab corners rounding for scrollbar
    style.GrabMinSize = 10.0f; // Minimum width/height of a grab box for slider/scrollbar
    style.GrabRounding = 2.0f; // Radius of grabs corners rounding. Set to 0.0f to have rectangular slider grabs.
    style.TabRounding = 4.0f; // Radius of upper corners of a tab. Set to 0.0f to have rectangular tabs.
    style.TabBorderSize = 1.0f; // Thickness of border around tabs.
    style.ButtonTextAlign = ImVec2(
        0.5f,
        0.5f); // Alignment of button text when button is larger than text.
    style.DisplayWindowPadding = ImVec2(
        20,
        20); // Window position are clamped to be visible within the display area or monitors by at least this amount. Only applies to regular windows.
    style.DisplaySafeAreaPadding = ImVec2(
        3,
        3); // If you cannot see the edge of your screen (e.g. on a TV) increase the safe area padding. Covers popups/tooltips as well regular windows.
    style.MouseCursorScale = 1.0f; // Scale software rendered mouse cursor (when io.MouseDrawCursor is enabled). May be removed later.
    style.AntiAliasedLines = true; // Enable anti-aliasing on lines/borders. Disable if you are really short on CPU/GPU.
    style.AntiAliasedFill = true; // Enable anti-aliasing on filled shapes (rounded rectangles, circles, etc.)
    style.CurveTessellationTol = 1.25f; // Tessellation tolerance when using PathBezierCurveTo() without a specific number of segments. Decrease for highly tessellated curves (higher quality, more polygons), increase to reduce quality.
    style.WindowMenuButtonPosition = ImGuiDir_None;

    ImVec4* colors = ImGui::GetStyle().Colors;
    colors[ImGuiCol_Text] = ImVec4(0.96f, 0.96f, 0.96f, 0.88f);
    colors[ImGuiCol_TextDisabled] = ImVec4(0.96f, 0.96f, 0.96f, 0.28f);
    colors[ImGuiCol_WindowBg] = ImVec4(0.09f, 0.09f, 0.09f, 0.96f);
    colors[ImGuiCol_ChildBg] = ImVec4(0.09f, 0.09f, 0.09f, 0.96f);
    colors[ImGuiCol_PopupBg] = ImVec4(0.18f, 0.18f, 0.18f, 0.96f);
    colors[ImGuiCol_Border] = ImVec4(0.31f, 0.31f, 0.31f, 0.60f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_FrameBg] = ImVec4(0.15f, 0.15f, 0.15f, 0.96f);
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.25f, 0.25f, 0.25f, 0.95f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.09f, 0.42f, 0.26f, 0.96f);
    colors[ImGuiCol_TitleBg] = ImVec4(0.25f, 0.25f, 0.25f, 0.95f);
    colors[ImGuiCol_TitleBgActive] = ImVec4(0.09f, 0.42f, 0.26f, 0.96f);
    colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.09f, 0.42f, 0.26f, 0.96f);
    colors[ImGuiCol_MenuBarBg] = ImVec4(0.09f, 0.09f, 0.09f, 0.96f);
    colors[ImGuiCol_ScrollbarBg] = ImVec4(0.15f, 0.15f, 0.15f, 0.96f);
    colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 0.93f);
    colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.25f, 0.25f, 0.25f, 0.95f);
    colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.09f, 0.42f, 0.26f, 0.96f);
    colors[ImGuiCol_CheckMark] = ImVec4(0.96f, 0.96f, 0.96f, 0.88f);
    colors[ImGuiCol_SliderGrab] = ImVec4(0.31f, 0.31f, 0.31f, 0.93f);
    colors[ImGuiCol_SliderGrabActive] = ImVec4(0.15f, 0.15f, 0.15f, 0.96f);
    colors[ImGuiCol_Button] = ImVec4(0.15f, 0.15f, 0.15f, 0.96f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.25f, 0.25f, 0.25f, 0.95f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.09f, 0.42f, 0.26f, 0.96f);
    colors[ImGuiCol_Header] = ImVec4(0.25f, 0.25f, 0.25f, 0.95f);
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.31f, 0.31f, 0.31f, 0.93f);
    colors[ImGuiCol_HeaderActive] = ImVec4(0.09f, 0.42f, 0.26f, 0.96f);
    colors[ImGuiCol_Separator] = ImVec4(0.19f, 0.19f, 0.19f, 1.00f);
    colors[ImGuiCol_SeparatorHovered] = ImVec4(0.25f, 0.25f, 0.25f, 0.95f);
    colors[ImGuiCol_SeparatorActive] = ImVec4(0.09f, 0.42f, 0.26f, 0.96f);
    colors[ImGuiCol_ResizeGrip] = ImVec4(0.15f, 0.15f, 0.15f, 0.96f);
    colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.25f, 0.25f, 0.25f, 0.95f);
    colors[ImGuiCol_ResizeGripActive] = ImVec4(0.09f, 0.42f, 0.26f, 0.96f);
    colors[ImGuiCol_Tab] = ImVec4(0.05f, 0.21f, 0.13f, 0.96f);
    colors[ImGuiCol_TabHovered] = ImVec4(0.26f, 0.71f, 0.49f, 0.96f);
    colors[ImGuiCol_TabActive] = ImVec4(0.17f, 0.52f, 0.35f, 0.96f);
    colors[ImGuiCol_TabUnfocused] = ImVec4(0.07f, 0.30f, 0.19f, 0.96f);
    colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.09f, 0.42f, 0.26f, 0.96f);
    colors[ImGuiCol_DockingPreview] = ImVec4(0.09f, 0.42f, 0.26f, 0.96f);
    colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_PlotLines] = ImVec4(0.96f, 0.96f, 0.96f, 0.80f);
    colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.25f, 0.25f, 0.25f, 0.95f);
    colors[ImGuiCol_PlotHistogram] = ImVec4(0.96f, 0.96f, 0.96f, 0.80f);
    colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.25f, 0.25f, 0.25f, 0.95f);
    colors[ImGuiCol_TableHeaderBg] = ImVec4(0.20f, 0.20f, 0.20f, 1.00f);
    colors[ImGuiCol_TableBorderStrong] = ImVec4(0.35f, 0.35f, 0.35f, 1.00f);
    colors[ImGuiCol_TableBorderLight] = ImVec4(0.25f, 0.25f, 0.25f, 1.00f);
    colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.06f);
    colors[ImGuiCol_TextSelectedBg] = ImVec4(0.25f, 0.25f, 0.25f, 0.95f);
    colors[ImGuiCol_DragDropTarget] = ImVec4(0.25f, 0.25f, 0.25f, 0.95f);
    colors[ImGuiCol_NavHighlight] = ImVec4(0.25f, 0.25f, 0.25f, 0.95f);
    colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
    colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
    colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.27f, 0.27f, 0.27f, 0.73f);

    // for (int i=0; i< AppThemeCol_COUNT; i++) {
    //   appTheme.colors[i] = IM_COL32(rand()%0xff, rand()%0xff, rand()%0xff,
    //   255);
    // }
    // appTheme.colors[AppThemeCol_Background] = IM_COL32(20, 20, 20, 255);
    // appTheme.colors[AppThemeCol_GapHovered] = IM_COL32(50, 50, 50, 255);
    // appTheme.colors[AppThemeCol_GapSelected] = IM_COL32(100, 100, 200, 255);

    appTheme.colors[AppThemeCol_Background] = 0xFF141414;
    appTheme.colors[AppThemeCol_Label] = 0xFFFFFFFF;
    appTheme.colors[AppThemeCol_TickMajor] = 0xFF8C8C8C;
    appTheme.colors[AppThemeCol_TickMinor] = 0xFF363636;
    appTheme.colors[AppThemeCol_GapHovered] = 0xFF282828;
    appTheme.colors[AppThemeCol_GapSelected] = 0xFFFFFFFF;
    appTheme.colors[AppThemeCol_Item] = 0xFF507C35;
    appTheme.colors[AppThemeCol_ItemHovered] = 0xFF57A451;
    appTheme.colors[AppThemeCol_ItemSelected] = 0xFF57A451;
    appTheme.colors[AppThemeCol_Transition] = 0x8830DEE6;
    appTheme.colors[AppThemeCol_TransitionLine] = 0xFFA7E9F0;
    appTheme.colors[AppThemeCol_TransitionHovered] = 0xC830DEE6;
    appTheme.colors[AppThemeCol_TransitionSelected] = 0xFFFFFFFF;
    appTheme.colors[AppThemeCol_Effect] = 0x9A8A205A;
    appTheme.colors[AppThemeCol_EffectHovered] = 0xCC903567;
    appTheme.colors[AppThemeCol_EffectSelected] = 0xFFFFFFFF;
    appTheme.colors[AppThemeCol_Playhead] = 0xB445A7E6;
    appTheme.colors[AppThemeCol_PlayheadLine] = 0xFF9FCAE6;
    appTheme.colors[AppThemeCol_PlayheadHovered] = 0xCE73B2DA;
    appTheme.colors[AppThemeCol_PlayheadSelected] = 0xB4E6E145;
    appTheme.colors[AppThemeCol_MarkerHovered] = 0xFFFFFFFF;
    appTheme.colors[AppThemeCol_MarkerSelected] = 0xFFFFFFFF;
    appTheme.colors[AppThemeCol_Track] = 0xFF3F3F3A;
    appTheme.colors[AppThemeCol_TrackHovered] = 0xFF8DA282;
    appTheme.colors[AppThemeCol_TrackSelected] = 0xFFFFFFFF;
}
