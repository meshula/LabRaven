
#ifndef AppTheme_h
#define AppTheme_h

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"

enum AppThemeCol_ {
    AppThemeCol_Background,
    AppThemeCol_Label,
    AppThemeCol_TickMajor,
    AppThemeCol_TickMinor,
    AppThemeCol_GapHovered,
    AppThemeCol_GapSelected,
    AppThemeCol_Item,
    AppThemeCol_ItemHovered,
    AppThemeCol_ItemSelected,
    AppThemeCol_Transition,
    AppThemeCol_TransitionLine,
    AppThemeCol_TransitionHovered,
    AppThemeCol_TransitionSelected,
    AppThemeCol_Effect,
    AppThemeCol_EffectHovered,
    AppThemeCol_EffectSelected,
    AppThemeCol_Playhead,
    AppThemeCol_PlayheadLine,
    AppThemeCol_PlayheadHovered,
    AppThemeCol_PlayheadSelected,
    AppThemeCol_MarkerHovered,
    AppThemeCol_MarkerSelected,
    AppThemeCol_Track,
    AppThemeCol_TrackHovered,
    AppThemeCol_TrackSelected,
    AppThemeCol_COUNT
};

struct AppTheme {
    ImU32 colors[AppThemeCol_COUNT];
};
extern const char* AppThemeColor_Names[];
extern AppTheme appTheme;

void ApplyAppStyle();

#endif
