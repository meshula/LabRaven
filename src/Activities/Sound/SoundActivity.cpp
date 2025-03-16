
#include "SoundActivity.hpp"
#include "Lab/App.h"
#include "Providers/Sound/SoundProvider.hpp"
#include "Lab/LabDirectories.h"

#include "LabSound/LabSound.h"
#include "LabSound/extended/Util.h"

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>
#include <iostream>
#include <string>


#if defined(_MSC_VER)
# pragma warning(disable : 4996)
# if !defined(NOMINMAX)
#  define NOMINMAX
# endif
#endif

using namespace lab;




// Source https://gist.github.com/soufianekhiat/4d937b15bf7290abed9608569d229201
// Code by Soufiane Khiat


/* from ImGui internal */
namespace ImGui {
    IMGUI_API void          RenderFrame(ImVec2 p_min, ImVec2 p_max, ImU32 fill_col, bool border = true, float rounding = 0.0f);

    template<typename T> static inline T ImMin(T lhs, T rhs) { return lhs < rhs ? lhs : rhs; }
    template<typename T> static inline T ImMax(T lhs, T rhs) { return lhs >= rhs ? lhs : rhs; }
    template<typename T> static inline T ImClamp(T v, T mn, T mx) { return (v < mn) ? mn : (v > mx) ? mx : v; }
    static inline ImVec2 ImMin(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x < rhs.x ? lhs.x : rhs.x, lhs.y < rhs.y ? lhs.y : rhs.y); }
    static inline ImVec2 ImMax(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x >= rhs.x ? lhs.x : rhs.x, lhs.y >= rhs.y ? lhs.y : rhs.y); }
    static inline ImVec2 ImClamp(const ImVec2& v, const ImVec2& mn, ImVec2 mx) { return ImVec2((v.x < mn.x) ? mn.x : (v.x > mx.x) ? mx.x : v.x, (v.y < mn.y) ? mn.y : (v.y > mx.y) ? mx.y : v.y); }
#define IM_FLOOR(_VAL)                  ((float)(int)(_VAL))                                    // ImFloor() is not inlined in MSVC debug builds


    // Helper: ImRect (2D axis aligned bounding-box)
    // NB: we can't rely on ImVec2 math operators being available here!
    struct IMGUI_API ImRect
    {
        ImVec2      Min;    // Upper-left
        ImVec2      Max;    // Lower-right

        ImRect() : Min(0.0f, 0.0f), Max(0.0f, 0.0f) {}
        ImRect(const ImVec2& min, const ImVec2& max) : Min(min), Max(max) {}
        ImRect(const ImVec4& v) : Min(v.x, v.y), Max(v.z, v.w) {}
        ImRect(float x1, float y1, float x2, float y2) : Min(x1, y1), Max(x2, y2) {}

        ImVec2      GetCenter() const { return ImVec2((Min.x + Max.x) * 0.5f, (Min.y + Max.y) * 0.5f); }
        ImVec2      GetSize() const { return ImVec2(Max.x - Min.x, Max.y - Min.y); }
        float       GetWidth() const { return Max.x - Min.x; }
        float       GetHeight() const { return Max.y - Min.y; }
        ImVec2      GetTL() const { return Min; }                   // Top-left
        ImVec2      GetTR() const { return ImVec2(Max.x, Min.y); }  // Top-right
        ImVec2      GetBL() const { return ImVec2(Min.x, Max.y); }  // Bottom-left
        ImVec2      GetBR() const { return Max; }                   // Bottom-right
        bool        Contains(const ImVec2& p) const { return p.x >= Min.x && p.y >= Min.y && p.x < Max.x&& p.y < Max.y; }
        bool        Contains(const ImRect& r) const { return r.Min.x >= Min.x && r.Min.y >= Min.y && r.Max.x <= Max.x && r.Max.y <= Max.y; }
        bool        Overlaps(const ImRect& r) const { return r.Min.y <  Max.y&& r.Max.y >  Min.y && r.Min.x <  Max.x&& r.Max.x >  Min.x; }
        void        Add(const ImVec2& p) { if (Min.x > p.x)     Min.x = p.x;     if (Min.y > p.y)     Min.y = p.y;     if (Max.x < p.x)     Max.x = p.x;     if (Max.y < p.y)     Max.y = p.y; }
        void        Add(const ImRect& r) { if (Min.x > r.Min.x) Min.x = r.Min.x; if (Min.y > r.Min.y) Min.y = r.Min.y; if (Max.x < r.Max.x) Max.x = r.Max.x; if (Max.y < r.Max.y) Max.y = r.Max.y; }
        void        Expand(const float amount) { Min.x -= amount;   Min.y -= amount;   Max.x += amount;   Max.y += amount; }
        void        Expand(const ImVec2& amount) { Min.x -= amount.x; Min.y -= amount.y; Max.x += amount.x; Max.y += amount.y; }
        void        Translate(const ImVec2& d) { Min.x += d.x; Min.y += d.y; Max.x += d.x; Max.y += d.y; }
        void        TranslateX(float dx) { Min.x += dx; Max.x += dx; }
        void        TranslateY(float dy) { Min.y += dy; Max.y += dy; }
        void        ClipWith(const ImRect& r) { Min = ImMax(Min, r.Min); Max = ImMin(Max, r.Max); }                   // Simple version, may lead to an inverted rectangle, which is fine for Contains/Overlaps test but not for display.
        void        ClipWithFull(const ImRect& r) { Min = ImClamp(Min, r.Min, r.Max); Max = ImClamp(Max, r.Min, r.Max); } // Full version, ensure both points are fully clipped.
        void        Floor() { Min.x = IM_FLOOR(Min.x); Min.y = IM_FLOOR(Min.y); Max.x = IM_FLOOR(Max.x); Max.y = IM_FLOOR(Max.y); }
        bool        IsInverted() const { return Min.x > Max.x || Min.y > Max.y; }
        ImVec4      ToVec4() const { return ImVec4(Min.x, Min.y, Max.x, Max.y); }
    };

    static inline ImVec2 operator*(const ImVec2& lhs, const float rhs) { return ImVec2(lhs.x * rhs, lhs.y * rhs); }
    static inline ImVec2 operator/(const ImVec2& lhs, const float rhs) { return ImVec2(lhs.x / rhs, lhs.y / rhs); }
    static inline ImVec2 operator+(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x + rhs.x, lhs.y + rhs.y); }
    static inline ImVec2 operator-(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x - rhs.x, lhs.y - rhs.y); }
    static inline ImVec2 operator*(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x * rhs.x, lhs.y * rhs.y); }
    static inline ImVec2 operator/(const ImVec2& lhs, const ImVec2& rhs) { return ImVec2(lhs.x / rhs.x, lhs.y / rhs.y); }
    static inline ImVec2& operator*=(ImVec2& lhs, const float rhs) { lhs.x *= rhs; lhs.y *= rhs; return lhs; }
    static inline ImVec2& operator/=(ImVec2& lhs, const float rhs) { lhs.x /= rhs; lhs.y /= rhs; return lhs; }
    static inline ImVec2& operator+=(ImVec2& lhs, const ImVec2& rhs) { lhs.x += rhs.x; lhs.y += rhs.y; return lhs; }
    static inline ImVec2& operator-=(ImVec2& lhs, const ImVec2& rhs) { lhs.x -= rhs.x; lhs.y -= rhs.y; return lhs; }
    static inline ImVec2& operator*=(ImVec2& lhs, const ImVec2& rhs) { lhs.x *= rhs.x; lhs.y *= rhs.y; return lhs; }
    static inline ImVec2& operator/=(ImVec2& lhs, const ImVec2& rhs) { lhs.x /= rhs.x; lhs.y /= rhs.y; return lhs; }
    static inline ImVec4 operator+(const ImVec4& lhs, const ImVec4& rhs) { return ImVec4(lhs.x + rhs.x, lhs.y + rhs.y, lhs.z + rhs.z, lhs.w + rhs.w); }
    static inline ImVec4 operator-(const ImVec4& lhs, const ImVec4& rhs) { return ImVec4(lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z, lhs.w - rhs.w); }
    static inline ImVec4 operator*(const ImVec4& lhs, const ImVec4& rhs) { return ImVec4(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z, lhs.w * rhs.w); }
}
/* from ImGui internal */

using ImGui::ImRect;

bool SliderScalar2D(char const* pLabel, float* fValueX, float* fValueY, const float fMinX, const float fMaxX, const float fMinY, const float fMaxY, float const fZoom /*= 1.0f*/)
{
    assert(fMinX < fMaxX);
    assert(fMinY < fMaxY);

    ImGuiID const iID = ImGui::GetID(pLabel);

    ImVec2 const vSizeSubstract = ImGui::CalcTextSize(std::to_string(1.0f).c_str()) * 1.1f;

    float const vSizeFull = (ImGui::GetWindowContentRegionMax().x - vSizeSubstract.x) * fZoom;
    ImVec2 const vSize(vSizeFull, vSizeFull);

    float const fHeightOffset = ImGui::GetTextLineHeight();
    ImVec2 const vHeightOffset(0.0f, fHeightOffset);

    ImVec2 vPos = ImGui::GetCursorScreenPos();
    ImGui::ImRect oRect(vPos + vHeightOffset, vPos + vSize + vHeightOffset);

    ImGui::TextUnformatted(pLabel);

    ImGui::PushID(iID);

    ImU32 const uFrameCol = ImGui::GetColorU32(ImGuiCol_FrameBg);

    ImVec2 const vOriginPos = ImGui::GetCursorScreenPos();
    ImGui::RenderFrame(oRect.Min, oRect.Max, uFrameCol, false, 0.0f);

    float const fDeltaX = fMaxX - fMinX;
    float const fDeltaY = fMaxY - fMinY;

    bool bModified = false;
    ImVec2 const vSecurity(15.0f, 15.0f);
    if (ImGui::IsMouseHoveringRect(oRect.Min - vSecurity, oRect.Max + vSecurity) && ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        ImVec2 const vCursorPos = ImGui::GetMousePos() - oRect.Min;

        *fValueX = vCursorPos.x / (oRect.Max.x - oRect.Min.x) * fDeltaX + fMinX;
        *fValueY = fDeltaY - vCursorPos.y / (oRect.Max.y - oRect.Min.y) * fDeltaY + fMinY;

        bModified = true;
    }

    *fValueX = std::min(std::max(*fValueX, fMinX), fMaxX);
    *fValueY = std::min(std::max(*fValueY, fMinY), fMaxY);

    float const fScaleX = (*fValueX - fMinX) / fDeltaX;
    float const fScaleY = 1.0f - (*fValueY - fMinY) / fDeltaY;

    constexpr float fCursorOff = 10.0f;
    float const fXLimit = fCursorOff / oRect.GetWidth();
    float const fYLimit = fCursorOff / oRect.GetHeight();

    ImVec2 const vCursorPos((oRect.Max.x - oRect.Min.x) * fScaleX + oRect.Min.x, (oRect.Max.y - oRect.Min.y) * fScaleY + oRect.Min.y);

    ImDrawList* pDrawList = ImGui::GetWindowDrawList();

    ImVec4 const vBlue(70.0f / 255.0f, 102.0f / 255.0f, 230.0f / 255.0f, 1.0f); // TODO: choose from style
    ImVec4 const vOrange(255.0f / 255.0f, 128.0f / 255.0f, 64.0f / 255.0f, 1.0f); // TODO: choose from style

    ImS32 const uBlue = ImGui::GetColorU32(vBlue);
    ImS32 const uOrange = ImGui::GetColorU32(vOrange);

    constexpr float fBorderThickness = 2.0f;
    constexpr float fLineThickness = 3.0f;
    constexpr float fHandleRadius = 7.0f;
    constexpr float fHandleOffsetCoef = 2.0f;

    // Cursor
    pDrawList->AddCircleFilled(vCursorPos, 5.0f, uBlue, 16);

    // Vertical Line
    if (fScaleY > 2.0f * fYLimit)
        pDrawList->AddLine(ImVec2(vCursorPos.x, oRect.Min.y + fCursorOff), ImVec2(vCursorPos.x, vCursorPos.y - fCursorOff), uOrange, fLineThickness);
    if (fScaleY < 1.0f - 2.0f * fYLimit)
        pDrawList->AddLine(ImVec2(vCursorPos.x, oRect.Max.y - fCursorOff), ImVec2(vCursorPos.x, vCursorPos.y + fCursorOff), uOrange, fLineThickness);

    // Horizontal Line
    if (fScaleX > 2.0f * fXLimit)
        pDrawList->AddLine(ImVec2(oRect.Min.x + fCursorOff, vCursorPos.y), ImVec2(vCursorPos.x - fCursorOff, vCursorPos.y), uOrange, fLineThickness);
    if (fScaleX < 1.0f - 2.0f * fYLimit)
        pDrawList->AddLine(ImVec2(oRect.Max.x - fCursorOff, vCursorPos.y), ImVec2(vCursorPos.x + fCursorOff, vCursorPos.y), uOrange, fLineThickness);

    char pBufferX[16];
    char pBufferY[16];
    snprintf(pBufferX, IM_ARRAYSIZE(pBufferX), "%.5f", *(float const*)fValueX);
    snprintf(pBufferY, IM_ARRAYSIZE(pBufferY), "%.5f", *(float const*)fValueY);

    ImU32 const uTextCol = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_Text]);

    ImGui::SetWindowFontScale(0.75f);

    ImVec2 const vXSize = ImGui::CalcTextSize(pBufferX);
    ImVec2 const vYSize = ImGui::CalcTextSize(pBufferY);

    ImVec2 const vHandlePosX = ImVec2(vCursorPos.x, oRect.Max.y + vYSize.x * 0.5f);
    ImVec2 const vHandlePosY = ImVec2(oRect.Max.x + fHandleOffsetCoef * fCursorOff + vYSize.x, vCursorPos.y);

    if (ImGui::IsMouseHoveringRect(vHandlePosX - ImVec2(fHandleRadius, fHandleRadius) - vSecurity, vHandlePosX + ImVec2(fHandleRadius, fHandleRadius) + vSecurity) &&
        ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        ImVec2 const vCursorPos = ImGui::GetMousePos() - oRect.Min;

        *fValueX = vCursorPos.x / (oRect.Max.x - oRect.Min.x) * fDeltaX + fMinX;

        bModified = true;
    }
    else if (ImGui::IsMouseHoveringRect(vHandlePosY - ImVec2(fHandleRadius, fHandleRadius) - vSecurity, vHandlePosY + ImVec2(fHandleRadius, fHandleRadius) + vSecurity) &&
        ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        ImVec2 const vCursorPos = ImGui::GetMousePos() - oRect.Min;

        *fValueY = fDeltaY - vCursorPos.y / (oRect.Max.y - oRect.Min.y) * fDeltaY + fMinY;

        bModified = true;
    }

    pDrawList->AddText(
        ImVec2(
            std::min(std::max(vCursorPos.x - vXSize.x * 0.5f, oRect.Min.x), oRect.Min.x + vSize.x - vXSize.x),
            oRect.Max.y + fCursorOff),
        uTextCol,
        pBufferX);
    pDrawList->AddText(
        ImVec2(oRect.Max.x + fCursorOff, std::min(std::max(vCursorPos.y - vYSize.y * 0.5f, oRect.Min.y),
            oRect.Min.y + vSize.y - vYSize.y)),
        uTextCol,
        pBufferY);
    ImGui::SetWindowFontScale(1.0f);

    // Borders::Right
    pDrawList->AddCircleFilled(ImVec2(oRect.Max.x, vCursorPos.y), 2.0f, uOrange, 3);
    // Handle Right::Y
    pDrawList->AddNgonFilled(ImVec2(oRect.Max.x + fHandleOffsetCoef * fCursorOff + vYSize.x, vCursorPos.y), fHandleRadius, uBlue, 4);
    if (fScaleY > fYLimit)
        pDrawList->AddLine(ImVec2(oRect.Max.x, oRect.Min.y), ImVec2(oRect.Max.x, vCursorPos.y - fCursorOff), uBlue, fBorderThickness);
    if (fScaleY < 1.0f - fYLimit)
        pDrawList->AddLine(ImVec2(oRect.Max.x, oRect.Max.y), ImVec2(oRect.Max.x, vCursorPos.y + fCursorOff), uBlue, fBorderThickness);
    // Borders::Top
    pDrawList->AddCircleFilled(ImVec2(vCursorPos.x, oRect.Min.y), 2.0f, uOrange, 3);
    if (fScaleX > fXLimit)
        pDrawList->AddLine(ImVec2(oRect.Min.x, oRect.Min.y), ImVec2(vCursorPos.x - fCursorOff, oRect.Min.y), uBlue, fBorderThickness);
    if (fScaleX < 1.0f - fXLimit)
        pDrawList->AddLine(ImVec2(oRect.Max.x, oRect.Min.y), ImVec2(vCursorPos.x + fCursorOff, oRect.Min.y), uBlue, fBorderThickness);
    // Borders::Left
    pDrawList->AddCircleFilled(ImVec2(oRect.Min.x, vCursorPos.y), 2.0f, uOrange, 3);
    if (fScaleY > fYLimit)
        pDrawList->AddLine(ImVec2(oRect.Min.x, oRect.Min.y), ImVec2(oRect.Min.x, vCursorPos.y - fCursorOff), uBlue, fBorderThickness);
    if (fScaleY < 1.0f - fYLimit)
        pDrawList->AddLine(ImVec2(oRect.Min.x, oRect.Max.y), ImVec2(oRect.Min.x, vCursorPos.y + fCursorOff), uBlue, fBorderThickness);
    // Borders::Bottom
    pDrawList->AddCircleFilled(ImVec2(vCursorPos.x, oRect.Max.y), 2.0f, uOrange, 3);
    // Handle Bottom::X
    pDrawList->AddNgonFilled(ImVec2(vCursorPos.x, oRect.Max.y + vXSize.y * 2.0f), fHandleRadius, uBlue, 4);
    if (fScaleX > fXLimit)
        pDrawList->AddLine(ImVec2(oRect.Min.x, oRect.Max.y), ImVec2(vCursorPos.x - fCursorOff, oRect.Max.y), uBlue, fBorderThickness);
    if (fScaleX < 1.0f - fXLimit)
        pDrawList->AddLine(ImVec2(oRect.Max.x, oRect.Max.y), ImVec2(vCursorPos.x + fCursorOff, oRect.Max.y), uBlue, fBorderThickness);

    ImGui::PopID();

    ImGui::Dummy(vHeightOffset);
    ImGui::Dummy(vHeightOffset);
    ImGui::Dummy(vSize);

    char pBufferID[64];
    snprintf(pBufferID, IM_ARRAYSIZE(pBufferID), "Values##%d", *(ImS32 const*)&iID);

    if (ImGui::CollapsingHeader(pBufferID))
    {
        float const fSpeedX = fDeltaX / 64.0f;
        float const fSpeedY = fDeltaY / 64.0f;

        char pBufferXID[64];
        snprintf(pBufferXID, IM_ARRAYSIZE(pBufferXID), "X##%d", *(ImS32 const*)&iID);
        char pBufferYID[64];
        snprintf(pBufferYID, IM_ARRAYSIZE(pBufferYID), "Y##%d", *(ImS32 const*)&iID);

        bModified |= ImGui::DragScalar(pBufferXID, ImGuiDataType_Float, fValueX, fSpeedX, &fMinX, &fMaxX);
        bModified |= ImGui::DragScalar(pBufferYID, ImGuiDataType_Float, fValueY, fSpeedY, &fMinY, &fMaxY);
    }

    return bModified;
}

float dot(ImVec2 a, ImVec2 b)
{
    return a.x * b.x + a.y * b.y;
}

float DistToSegmentSqr(ImVec2 v, ImVec2 w, ImVec2 p) 
{
    // Return minimum distance between line segment vw and point p

    ImVec2 d = v - w;

    const float l2 = d.x * d.x + d.y * d.y;  // i.e. |w-v|^2 -  avoid a sqrt
    if (l2 == 0.0)
    {
        d = p - v;
        return std::sqrtf(d.x * d.x + d.y * d.y);   // v == w case
    }
    // Consider the line extending the segment, parameterized as v + t (w - v).
    // We find projection of point p onto the line. 
    // It falls where t = [(p-v) . (w-v)] / |w-v|^2
    // We clamp t from [0,1] to handle points outside the segment vw.
    const float t = std::max(0.f, std::min(1.f, dot(p - v, w - v) / l2));
    const ImVec2 projection = v + (w - v) * t;  // Projection falls on the segment

    d = p - projection;
    return std::sqrtf(d.x * d.x + d.y * d.y);
}


bool SliderScalar3D(char const* pLabel, float* pValueX, float* pValueY, float* pValueZ, float const fMinX, float const fMaxX, float const fMinY, float const fMaxY, float const fMinZ, float const fMaxZ, float const fScale /*= 1.0f*/)
{
    assert(fMinX < fMaxX);
    assert(fMinY < fMaxY);
    assert(fMinZ < fMaxZ);

    ImGuiID const iID = ImGui::GetID(pLabel);

    ImVec2 const vSizeSubstract = ImGui::CalcTextSize(std::to_string(1.0f).c_str()) * 1.1f;

    float const vSizeFull = ImGui::GetWindowContentRegionMax().x;
    float const fMinSize = (vSizeFull - vSizeSubstract.x * 0.5f) * fScale * 0.75f;
    ImVec2 const vSize(fMinSize, fMinSize);

    float const fHeightOffset = ImGui::GetTextLineHeight();
    ImVec2 const vHeightOffset(0.0f, fHeightOffset);

    ImVec2 vPos = ImGui::GetCursorScreenPos();
    ImGui::ImRect oRect(vPos + vHeightOffset, vPos + vSize + vHeightOffset);

    ImGui::TextUnformatted(pLabel);

    ImGui::PushID(iID);

    ImU32 const uFrameCol = ImGui::GetColorU32(ImGuiCol_FrameBg) | 0xFF000000;
    ImU32 const uFrameCol2 = ImGui::GetColorU32(ImGuiCol_FrameBgActive);

    float& fX = *pValueX;
    float& fY = *pValueY;
    float& fZ = *pValueZ;

    float const fDeltaX = fMaxX - fMinX;
    float const fDeltaY = fMaxY - fMinY;
    float const fDeltaZ = fMaxZ - fMinZ;

    ImVec2 const vOriginPos = ImGui::GetCursorScreenPos();

    ImDrawList* pDrawList = ImGui::GetWindowDrawList();

    float const fX3 = vSize.x / 3.0f;
    float const fY3 = vSize.y / 3.0f;

    ImVec2 const vStart = oRect.Min;

    ImVec2 aPositions[] = {
        ImVec2(vStart.x,			vStart.y + fX3),
        ImVec2(vStart.x,			vStart.y + vSize.y),
        ImVec2(vStart.x + 2.0f * fX3,	vStart.y + vSize.y),
        ImVec2(vStart.x + vSize.x,	vStart.y + 2.0f * fY3),
        ImVec2(vStart.x + vSize.x,	vStart.y),
        ImVec2(vStart.x + fX3,		vStart.y)
    };

    pDrawList->AddPolyline(&aPositions[0], 6, uFrameCol2, true, 1.0f);

    // Cube Shape
    pDrawList->AddLine(
        oRect.Min + ImVec2(0.0f, vSize.y),
        oRect.Min + ImVec2(fX3, 2.0f * fY3), uFrameCol2, 1.0f);
    pDrawList->AddLine(
        oRect.Min + ImVec2(fX3, 2.0f * fY3),
        oRect.Min + ImVec2(vSize.x, 2.0f * fY3), uFrameCol2, 1.0f);
    pDrawList->AddLine(
        oRect.Min + ImVec2(fX3, 0.0f),
        oRect.Min + ImVec2(fX3, 2.0f * fY3), uFrameCol2, 1.0f);

    ImGui::PopID();

    constexpr float fDragZOffsetX = 64.0f;

    ImRect oZDragRect(ImVec2(vStart.x + 2.0f * fX3 + fDragZOffsetX, vStart.y + 2.0f * fY3), ImVec2(vStart.x + vSize.x + fDragZOffsetX, vStart.y + vSize.y));

    ImVec2 const vMousePos = ImGui::GetMousePos();
    ImVec2 const vSecurity(15.0f, 15.0f);
    ImVec2 const vDragStart(oZDragRect.Min.x, oZDragRect.Max.y);
    ImVec2 const vDragEnd(oZDragRect.Max.x, oZDragRect.Min.y);
    bool bModified = false;
    if (ImGui::IsMouseHoveringRect(oZDragRect.Min - vSecurity, oZDragRect.Max + vSecurity) && ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        float dist = DistToSegmentSqr(vMousePos, vDragStart, vDragEnd);
        if (dist < 100.0f) // 100 is arbitrary threshold
        {
            ImVec2 d = vDragStart - vDragEnd;
            float const fMaxDist = std::sqrt(d.x * d.x + d.y * d.y);
            float const fDist = std::max(std::min(dist / fMaxDist, 1.0f), 0.0f);

            fZ = fDist * fDeltaZ * fDist + fMinZ;

            bModified = true;
        }
    }

    float const fScaleZ = (fZ - fMinZ) / fDeltaZ;

    ImVec2 const vRectStart(vStart.x, vStart.y + fX3);
    ImVec2 const vRectEnd(vStart.x + fX3, vStart.y);
    ImRect const oXYDrag((vRectEnd - vRectStart) * fScaleZ + vRectStart,
        (vRectEnd - vRectStart) * fScaleZ + vRectStart + ImVec2(2.0f * fX3, 2.0f * fY3));
    if (ImGui::IsMouseHoveringRect(oXYDrag.Min - vSecurity, oXYDrag.Max + vSecurity) && ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        ImVec2 const vLocalPos = ImGui::GetMousePos() - oXYDrag.Min;

        fX = vLocalPos.x / (oXYDrag.Max.x - oXYDrag.Min.x) * fDeltaX + fMinX;
        fY = fDeltaY - vLocalPos.y / (oXYDrag.Max.y - oXYDrag.Min.y) * fDeltaY + fMinY;

        bModified = true;
    }

    fX = std::min(std::max(fX, fMinX), fMaxX);
    fY = std::min(std::max(fY, fMinY), fMaxY);
    fZ = std::min(std::max(fZ, fMinZ), fMaxZ);

    float const fScaleX = (fX - fMinX) / fDeltaX;
    float const fScaleY = 1.0f - (fY - fMinY) / fDeltaY;

    ImVec4 const vBlue(70.0f / 255.0f, 102.0f / 255.0f, 230.0f / 255.0f, 1.0f);
    ImVec4 const vOrange(255.0f / 255.0f, 128.0f / 255.0f, 64.0f / 255.0f, 1.0f);

    ImS32 const uBlue = ImGui::GetColorU32(vBlue);
    ImS32 const uOrange = ImGui::GetColorU32(vOrange);

    constexpr float fBorderThickness = 2.0f; // TODO: move as Style
    constexpr float fLineThickness = 3.0f; // TODO: move as Style
    constexpr float fHandleRadius = 7.0f; // TODO: move as Style
    constexpr float fHandleOffsetCoef = 2.0f; // TODO: move as Style

    pDrawList->AddLine(
        vDragStart,
        vDragEnd, uFrameCol2, 1.0f);
    pDrawList->AddRectFilled(
        oXYDrag.Min, oXYDrag.Max, uFrameCol);

    constexpr float fCursorOff = 10.0f;
    float const fXLimit = fCursorOff / oXYDrag.GetWidth();
    float const fYLimit = fCursorOff / oXYDrag.GetHeight();
    float const fZLimit = fCursorOff / oXYDrag.GetWidth();

    char pBufferX[16];
    char pBufferY[16];
    char pBufferZ[16];
    snprintf(pBufferX, IM_ARRAYSIZE(pBufferX), "%.5f", *(float const*)&fX);
    snprintf(pBufferY, IM_ARRAYSIZE(pBufferY), "%.5f", *(float const*)&fY);
    snprintf(pBufferZ, IM_ARRAYSIZE(pBufferZ), "%.5f", *(float const*)&fZ);

    ImU32 const uTextCol = ImGui::ColorConvertFloat4ToU32(ImGui::GetStyle().Colors[ImGuiCol_Text]);

    ImVec2 const vCursorPos((oXYDrag.Max.x - oXYDrag.Min.x) * fScaleX + oXYDrag.Min.x, (oXYDrag.Max.y - oXYDrag.Min.y) * fScaleY + oXYDrag.Min.y);

    ImGui::SetWindowFontScale(0.75f);

    ImVec2 const vXSize = ImGui::CalcTextSize(pBufferX);
    ImVec2 const vYSize = ImGui::CalcTextSize(pBufferY);
    ImVec2 const vZSize = ImGui::CalcTextSize(pBufferZ);

    ImVec2 const vTextSlideXMin = oRect.Min + ImVec2(0.0f, vSize.y);
    ImVec2 const vTextSlideXMax = oRect.Min + ImVec2(2.0f * fX3, vSize.y);
    ImVec2 const vXTextPos((vTextSlideXMax - vTextSlideXMin) * fScaleX + vTextSlideXMin);

    ImVec2 const vTextSlideYMin = oRect.Min + ImVec2(vSize.x, 2.0f * fY3);
    ImVec2 const vTextSlideYMax = oRect.Min + ImVec2(vSize.x, 0.0f);
    ImVec2 const vYTextPos((vTextSlideYMax - vTextSlideYMin) * (1.0f - fScaleY) + vTextSlideYMin);

    ImVec2 const vTextSlideZMin = vDragStart + ImVec2(fCursorOff, fCursorOff);
    ImVec2 const vTextSlideZMax = vDragEnd + ImVec2(fCursorOff, fCursorOff);
    ImVec2 const vZTextPos((vTextSlideZMax - vTextSlideZMin) * fScaleZ + vTextSlideZMin);

    ImVec2 const vHandlePosX = vXTextPos + ImVec2(0.0f, vXSize.y + fHandleOffsetCoef * fCursorOff);
    ImVec2 const vHandlePosY = vYTextPos + ImVec2(vYSize.x + (fHandleOffsetCoef + 1.0f) * fCursorOff, 0.0f);

    if (ImGui::IsMouseHoveringRect(vHandlePosX - ImVec2(fHandleRadius, fHandleRadius) - vSecurity, vHandlePosX + ImVec2(fHandleRadius, fHandleRadius) + vSecurity) &&
        ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        float const fCursorPosX = ImGui::GetMousePos().x - vStart.x;

        fX = fDeltaX * fCursorPosX / (2.0f * fX3) + fMinX;

        bModified = true;
    }
    else if (ImGui::IsMouseHoveringRect(vHandlePosY - ImVec2(fHandleRadius, fHandleRadius) - vSecurity, vHandlePosY + ImVec2(fHandleRadius, fHandleRadius) + vSecurity) &&
        ImGui::IsMouseDown(ImGuiMouseButton_Left))
    {
        float const fCursorPosY = ImGui::GetMousePos().y - vStart.y;

        fY = fDeltaY * (1.0f - fCursorPosY / (2.0f * fY3)) + fMinY;

        bModified = true;
    }

    pDrawList->AddText(
        ImVec2(
            std::min(std::max(vXTextPos.x - vXSize.x * 0.5f, vTextSlideXMin.x), vTextSlideXMax.x - vXSize.x),
            vXTextPos.y + fCursorOff),
        uTextCol,
        pBufferX);
    pDrawList->AddText(
        ImVec2(
            vYTextPos.x + fCursorOff,
            std::min(std::max(vYTextPos.y - vYSize.y * 0.5f, vTextSlideYMax.y), vTextSlideYMin.y - vYSize.y)),
        uTextCol,
        pBufferY);
    pDrawList->AddText(
        vZTextPos,
        uTextCol,
        pBufferZ);
    ImGui::SetWindowFontScale(1.0f);

    // Handles
    pDrawList->AddNgonFilled(vHandlePosX, fHandleRadius, uBlue, 4);
    pDrawList->AddNgonFilled(vHandlePosY, fHandleRadius, uBlue, 4);

    // Vertical Line
    if (fScaleY > 2.0f * fYLimit)
        pDrawList->AddLine(ImVec2(vCursorPos.x, oXYDrag.Min.y + fCursorOff), ImVec2(vCursorPos.x, vCursorPos.y - fCursorOff), uOrange, fLineThickness);
    if (fScaleY < 1.0f - 2.0f * fYLimit)
        pDrawList->AddLine(ImVec2(vCursorPos.x, oXYDrag.Max.y - fCursorOff), ImVec2(vCursorPos.x, vCursorPos.y + fCursorOff), uOrange, fLineThickness);

    // Horizontal Line
    if (fScaleX > 2.0f * fXLimit)
        pDrawList->AddLine(ImVec2(oXYDrag.Min.x + fCursorOff, vCursorPos.y), ImVec2(vCursorPos.x - fCursorOff, vCursorPos.y), uOrange, fLineThickness);
    if (fScaleX < 1.0f - 2.0f * fYLimit)
        pDrawList->AddLine(ImVec2(oXYDrag.Max.x - fCursorOff, vCursorPos.y), ImVec2(vCursorPos.x + fCursorOff, vCursorPos.y), uOrange, fLineThickness);

    // Line To Text
    // X
    if (fScaleZ > 2.0f * fZLimit)
        pDrawList->AddLine(
            ImVec2(vCursorPos.x - 0.5f * fCursorOff, oXYDrag.Max.y + 0.5f * fCursorOff),
            ImVec2(vXTextPos.x + 0.5f * fCursorOff, vXTextPos.y - 0.5f * fCursorOff), uOrange, fLineThickness
        );
    else
        pDrawList->AddLine(
            ImVec2(vCursorPos.x, oXYDrag.Max.y),
            ImVec2(vXTextPos.x, vXTextPos.y), uOrange, 1.0f
        );
    pDrawList->AddCircleFilled(vXTextPos, 2.0f, uOrange, 3);
    // Y
    if (fScaleZ < 1.0f - 2.0f * fZLimit)
        pDrawList->AddLine(
            ImVec2(oXYDrag.Max.x + 0.5f * fCursorOff, vCursorPos.y - 0.5f * fCursorOff),
            ImVec2(vYTextPos.x - 0.5f * fCursorOff, vYTextPos.y + 0.5f * fCursorOff), uOrange, fLineThickness
        );
    else
        pDrawList->AddLine(
            ImVec2(oXYDrag.Max.x, vCursorPos.y),
            ImVec2(vYTextPos.x, vYTextPos.y), uOrange, 1.0f
        );
    pDrawList->AddCircleFilled(vYTextPos, 2.0f, uOrange, 3);

    // Borders::Right
    pDrawList->AddCircleFilled(ImVec2(oXYDrag.Max.x, vCursorPos.y), 2.0f, uOrange, 3);
    if (fScaleY > fYLimit)
        pDrawList->AddLine(ImVec2(oXYDrag.Max.x, oXYDrag.Min.y), ImVec2(oXYDrag.Max.x, vCursorPos.y - fCursorOff), uBlue, fBorderThickness);
    if (fScaleY < 1.0f - fYLimit)
        pDrawList->AddLine(ImVec2(oXYDrag.Max.x, oXYDrag.Max.y), ImVec2(oXYDrag.Max.x, vCursorPos.y + fCursorOff), uBlue, fBorderThickness);
    // Borders::Top
    pDrawList->AddCircleFilled(ImVec2(vCursorPos.x, oXYDrag.Min.y), 2.0f, uOrange, 3);
    if (fScaleX > fXLimit)
        pDrawList->AddLine(ImVec2(oXYDrag.Min.x, oXYDrag.Min.y), ImVec2(vCursorPos.x - fCursorOff, oXYDrag.Min.y), uBlue, fBorderThickness);
    if (fScaleX < 1.0f - fXLimit)
        pDrawList->AddLine(ImVec2(oXYDrag.Max.x, oXYDrag.Min.y), ImVec2(vCursorPos.x + fCursorOff, oXYDrag.Min.y), uBlue, fBorderThickness);
    // Borders::Left
    pDrawList->AddCircleFilled(ImVec2(oXYDrag.Min.x, vCursorPos.y), 2.0f, uOrange, 3);
    if (fScaleY > fYLimit)
        pDrawList->AddLine(ImVec2(oXYDrag.Min.x, oXYDrag.Min.y), ImVec2(oXYDrag.Min.x, vCursorPos.y - fCursorOff), uBlue, fBorderThickness);
    if (fScaleY < 1.0f - fYLimit)
        pDrawList->AddLine(ImVec2(oXYDrag.Min.x, oXYDrag.Max.y), ImVec2(oXYDrag.Min.x, vCursorPos.y + fCursorOff), uBlue, fBorderThickness);
    // Borders::Bottom
    pDrawList->AddCircleFilled(ImVec2(vCursorPos.x, oXYDrag.Max.y), 2.0f, uOrange, 3);
    if (fScaleX > fXLimit)
        pDrawList->AddLine(ImVec2(oXYDrag.Min.x, oXYDrag.Max.y), ImVec2(vCursorPos.x - fCursorOff, oXYDrag.Max.y), uBlue, fBorderThickness);
    if (fScaleX < 1.0f - fXLimit)
        pDrawList->AddLine(ImVec2(oXYDrag.Max.x, oXYDrag.Max.y), ImVec2(vCursorPos.x + fCursorOff, oXYDrag.Max.y), uBlue, fBorderThickness);

    pDrawList->AddLine(
        oRect.Min + ImVec2(0.0f, fY3),
        oRect.Min + ImVec2(2.0f * fX3, fY3), uFrameCol2, 1.0f);
    pDrawList->AddLine(
        oRect.Min + ImVec2(2.0f * fX3, fY3),
        oRect.Min + ImVec2(vSize.x, 0.0f), uFrameCol2, 1.0f);
    pDrawList->AddLine(
        oRect.Min + ImVec2(2.0f * fX3, fY3),
        oRect.Min + ImVec2(2.0f * fX3, vSize.y), uFrameCol2, 1.0f);

    // Cursor
    pDrawList->AddCircleFilled(vCursorPos, 5.0f, uBlue, 16);
    // CursorZ
    pDrawList->AddNgonFilled((vDragEnd - vDragStart) * fScaleZ + vDragStart, fHandleRadius, uBlue, 4);

    ImGui::Dummy(vHeightOffset);
    ImGui::Dummy(vHeightOffset * 1.25f);
    ImGui::Dummy(vSize);

    char pBufferID[64];
    snprintf(pBufferID, IM_ARRAYSIZE(pBufferID), "Values##%d", *(ImS32 const*)&iID);

    if (ImGui::CollapsingHeader(pBufferID))
    {
        float const fMoveDeltaX = fDeltaX / 64.0f; // Arbitrary
        float const fMoveDeltaY = fDeltaY / 64.0f; // Arbitrary
        float const fMoveDeltaZ = fDeltaZ / 64.0f; // Arbitrary

        char pBufferXID[64];
        snprintf(pBufferXID, IM_ARRAYSIZE(pBufferXID), "X##%d", *(ImS32 const*)&iID);
        char pBufferYID[64];
        snprintf(pBufferYID, IM_ARRAYSIZE(pBufferYID), "Y##%d", *(ImS32 const*)&iID);
        char pBufferZID[64];
        snprintf(pBufferZID, IM_ARRAYSIZE(pBufferZID), "Z##%d", *(ImS32 const*)&iID);

        bModified |= ImGui::DragScalar(pBufferXID, ImGuiDataType_Float, &fX, fMoveDeltaX, &fMinX, &fMaxX);
        bModified |= ImGui::DragScalar(pBufferYID, ImGuiDataType_Float, &fY, fMoveDeltaY, &fMinY, &fMaxY);
        bModified |= ImGui::DragScalar(pBufferZID, ImGuiDataType_Float, &fZ, fMoveDeltaZ, &fMinZ, &fMaxZ);
    }

    return bModified;
}


bool InputVec2(char const* pLabel, ImVec2* pValue, ImVec2 const vMinValue, ImVec2 const vMaxValue, float const fScale /*= 1.0f*/)
{
    return SliderScalar2D(pLabel, &pValue->x, &pValue->y, vMinValue.x, vMaxValue.x, vMinValue.y, vMaxValue.y, fScale);
}

bool InputVec3(char const* pLabel, ImVec4* pValue, ImVec4 const vMinValue, ImVec4 const vMaxValue, float const fScale /*= 1.0f*/)
{
    return SliderScalar3D(pLabel, &pValue->x, &pValue->y, &pValue->z, vMinValue.x, vMaxValue.x, vMinValue.y, vMaxValue.y, vMinValue.z, vMaxValue.z, fScale);
}


struct NodeLocation
{
    float x = 0;
    std::string name;
};


struct Demo
{
    lab::AudioContext* ac = nullptr;
    std::map<std::string, std::shared_ptr<AudioBus>> sample_cache;
    std::shared_ptr<RecorderNode> recorder;
    bool use_live = false;

    void shutdown()
    {
        ac = nullptr;
    }

    std::shared_ptr<AudioBus> MakeBusFromSampleFile(char const* const name, float sampleRate)
    {
        auto it = sample_cache.find(name);
        if (it != sample_cache.end())
            return it->second;

        std::string path_prefix = lab_application_resource_path(nullptr, nullptr);
        
        const std::string path = path_prefix + name;
        std::shared_ptr<AudioBus> bus = MakeBusFromFile(path, false, sampleRate);
        if (!bus)
            throw std::runtime_error("couldn't open " + path);

        sample_cache[name] = bus;

        return bus;
    }

};



/////////////////////////////////////
//    Graph Traversal              //
/////////////////////////////////////

// traveral chart
std::vector<NodeLocation> displayNodes;
std::set<uintptr_t> nodes;

void traverse(ContextRenderLock* r, AudioNode* root, char const* const prefix, int tab)
{
    nodes.insert(reinterpret_cast<uintptr_t>(root));
    for (int i = 0; i < tab; ++i)
        printf(" ");

    displayNodes.push_back({ static_cast<float>(tab), std::string(root->name()) });

    bool inputs_silent = root->numberOfInputs() > 0 && root->inputsAreSilent(*r);

    const char* state_name = root->isScheduledNode() ?
                    schedulingStateName(root->schedulingState()) : "active";
    const char* input_status = root->numberOfInputs() > 0 ?
                    (root->inputsAreSilent(*r) ? "inputs silent" : "inputs active") : "no inputs";
    printf("%s%s (%s) (%s)\n", prefix, root->name(), state_name, input_status);

    auto params = root->params();
    for (auto& p : params)
    {
        if (p->isConnected())
        {
            AudioBus const* const bus = p->bus();
            if (bus)
            {
                const char* input_is_zero = bus->maxAbsValue() > 0.f ? "non-zero" : "zero";
                for (int i = 0; i < tab; ++i)
                    printf(" ");
                printf("driven param has %s values\n", input_is_zero);
            }

            int c = p->numberOfRenderingConnections(*r);
            for (int j = 0; j < c; ++j)
            {
                AudioNode* n = p->renderingOutput(*r, j)->sourceNode();
                if (n)
                {
                    if (nodes.find(reinterpret_cast<uintptr_t>(n)) == nodes.end())
                    {
                        char buff[64];
                        strncpy(buff, p->name().c_str(), 64);
                        buff[63] = '\0';
                        traverse(r, n, buff, tab + 3);
                    }
                    else
                    {
                        for (int i = 0; i < tab; ++i)
                            printf(" ");
                        printf("*--> %s\n", n->name());   // just show gotos to previous nodes
                    }
                }
            }
        }
    }
    for (int i = 0; i < root->numberOfInputs(); ++i)
    {
        auto input = root->input(i);
        if (input)
        {
            const char* input_is_zero = input->bus(*r)->maxAbsValue() > 0.f ? "active signal" : "zero signal";
            for (int i = 0; i < tab; ++i)
                printf(" ");
            printf("input %d: %s\n", i, input_is_zero);
            int c = input->numberOfRenderingConnections(*r);
            for (int j = 0; j < c; ++j)
            {
                AudioNode* n = input->renderingOutput(*r, j)->sourceNode();
                if (n)
                {
                    if (nodes.find(reinterpret_cast<uintptr_t>(n)) == nodes.end())
                        traverse(r, n, "", tab + 3);
                    else
                    {
                        for (int i = 0; i < tab; ++i)
                            printf(" ");
                        printf("*--> %s\n", n->name());   // just show gotos to previous nodes
                    }
                }
            }
        }
    }
}

void traverse_ui(lab::AudioContext& context)
{
    displayNodes.clear();
    nodes.clear();
    printf("\n");
    context.synchronizeConnections();
    ContextRenderLock r(&context, "traverse");
    traverse(&r, context.destinationNode().get(), "", 0);
}



std::shared_ptr<AudioBus> MakeBusFromSampleFile(char const*const name)
{
    std::string dir = lab_application_resource_path(nullptr, nullptr);
    std::string path = dir + "/" + name;
    std::shared_ptr<AudioBus> bus = MakeBusFromFile(path, false);
    if (!bus)
        std::cerr << "couldn't open " << path << std::endl;

    return bus;
}


//////////////////////////////
//    example base class    //
//////////////////////////////

struct labsound_example
{
    explicit labsound_example(Demo& demo) : _demo(&demo) {}

    Demo* _demo;
    std::shared_ptr<lab::AudioNode> _root_node;

    void connect()
    {
        if (!_root_node)
            return;

        auto& ac = *_demo->ac;
        if (!ac.isConnected(_demo->recorder, _root_node))
        {
            // connect synchronously
            ac.connect(_demo->recorder, _root_node, 0, 0);
            ac.synchronizeConnections();
            if (_root_node->isScheduledNode()) {
                auto n = dynamic_cast<AudioScheduledSourceNode*>(_root_node.get());
                n->start(0);
            }
        }
    }

    void disconnect()
    {
        if (!_root_node)
            return;

        auto& ac = *_demo->ac;
        if (!ac.isConnected(_demo->recorder, _root_node))
        {
            ac.disconnect(_demo->recorder, _root_node);
            ac.synchronizeConnections();
        }
    }

    virtual void play() = 0;
    virtual void update() {}
    virtual char const* const name() const = 0;

    virtual void ui()
    {
        if (!_root_node || !_root_node->output(0)->isConnected())
            return;

        ImGui::BeginChild("###EXAMPLE", ImVec2{ 0, 100 }, true);
        ImGui::TextUnformatted("Example");
        if (ImGui::Button("Disconnect"))
        {
            disconnect();
        }
        ImGui::EndChild();
    }
};



/////////////////////
//    ex_simple    //
/////////////////////

// ex_simple demonstrate the use of an audio clip loaded from disk and a basic sine oscillator. 
struct ex_simple : public labsound_example
{
    std::shared_ptr<SampledAudioNode> musicClipNode;
    std::shared_ptr<GainNode> gain;
    std::shared_ptr<PeakCompNode> peakComp;

    virtual char const* const name() const override { return "Simple"; }

    explicit ex_simple(Demo& demo) : labsound_example(demo)
    {
        auto& ac = *_demo->ac;
        auto musicClip = demo.MakeBusFromSampleFile("samples/stereo-music-clip.wav", ac.sampleRate());
        if (!musicClip)
            return;

        gain = std::make_shared<GainNode>(ac);
        gain->gain()->setValue(0.5f);
        peakComp = std::make_shared<PeakCompNode>(ac);
        _root_node = peakComp;
        ac.connect(peakComp, gain, 0, 0);

        musicClipNode = std::make_shared<SampledAudioNode>(ac);
        {
            ContextRenderLock r(&ac, "ex_simple");
            musicClipNode->setBus(r, musicClip);
        }
        ac.connect(_root_node, musicClipNode, 0, 0);
    }

    virtual void play() override final
    {
        connect();
        musicClipNode->schedule(0.0);
    }
};



/////////////////////
//    ex_sfxr      //
/////////////////////

// ex_simple demonstrate the use of an audio clip loaded from disk and a basic sine oscillator. 
struct ex_sfxr : public labsound_example
{
    std::shared_ptr<SfxrNode> sfxr;

    virtual char const* const name() const override { return "Sfxr"; }

    explicit ex_sfxr(Demo& demo) : labsound_example(demo)
    {
        auto& ac = *_demo->ac;
        sfxr = std::make_shared<SfxrNode>(ac);
        _root_node = sfxr;
    }

    virtual void play() override final
    {
        connect();
        sfxr->start(0.0);
    }

    virtual void ui() override final
    {
        if (!_root_node || !_root_node->output(0)->isConnected())
            return;

        ImGui::BeginChild("###SFXR", ImVec2{ 0, 300 }, true);
        if (ImGui::Button("Default"))
        {
            sfxr->preset()->setUint32(99);  // notifications only occur on change, so send a nonsense value
            sfxr->preset()->setUint32(0);
            sfxr->start(0.0);
        }
        if (ImGui::Button("Coin"))
        {
            sfxr->preset()->setUint32(99);  // notifications only occur on change, so send a nonsense value
            sfxr->preset()->setUint32(1);
            sfxr->start(0.0);
        }
        if (ImGui::Button("Laser"))
        {
            sfxr->preset()->setUint32(99);  // notifications only occur on change, so send a nonsense value
            sfxr->preset()->setUint32(2);
            sfxr->start(0.0);
        }
        if (ImGui::Button("Explosion"))
        {
            sfxr->preset()->setUint32(99);  // notifications only occur on change, so send a nonsense value
            sfxr->preset()->setUint32(3);
            sfxr->start(0.0);
        }
        if (ImGui::Button("Power Up"))
        {
            sfxr->preset()->setUint32(99);  // notifications only occur on change, so send a nonsense value
            sfxr->preset()->setUint32(4);
            sfxr->start(0.0);
        }
        if (ImGui::Button("Hit"))
        {
            sfxr->preset()->setUint32(99);  // notifications only occur on change, so send a nonsense value
            sfxr->preset()->setUint32(5);
            sfxr->start(0.0);
        }
        if (ImGui::Button("Jump"))
        {
            sfxr->preset()->setUint32(99);  // notifications only occur on change, so send a nonsense value
            sfxr->preset()->setUint32(6);
            sfxr->start(0.0);
        }
        if (ImGui::Button("Select"))
        {
            sfxr->preset()->setUint32(99);  // notifications only occur on change, so send a nonsense value
            sfxr->preset()->setUint32(7);
            sfxr->start(0.0);
        }
        if (ImGui::Button("Mutate"))
        {
            sfxr->preset()->setUint32(99);  // notifications only occur on change, so send a nonsense value
            sfxr->preset()->setUint32(8);
            sfxr->start(0.0);
        }
        if (ImGui::Button("Random"))
        {
            sfxr->preset()->setUint32(99);  // notifications only occur on change, so send a nonsense value
            sfxr->preset()->setUint32(9);
            sfxr->start(0.0);
        }
        ImGui::EndChild();
    }
};



/////////////////////
//    ex_osc_pop   //
/////////////////////

// ex_osc_pop to test oscillator start/stop popping (it shouldn't pop). 
struct ex_osc_pop : public labsound_example
{
    std::shared_ptr<OscillatorNode> oscillator;
    std::shared_ptr<GainNode> gain;
//    std::shared_ptr<RecorderNode> recorder;

    virtual char const* const name() const override { return "Oscillator"; }

    ex_osc_pop(Demo& demo) : labsound_example(demo)
    {
        auto& ac = *_demo->ac;
        oscillator = std::make_shared<OscillatorNode>(ac);

        gain = std::make_shared<GainNode>(ac);
        _root_node = gain;

        gain->gain()->setValue(1);

        // osc -> destination
        ac.connect(gain, oscillator, 0, 0);

        oscillator->frequency()->setValue(1000.f);
        oscillator->setType(OscillatorType::SINE);

 //       AudioStreamConfig outputConfig { -1, }
 //       recorder = std::make_shared<RecorderNode>(ac, outputConfig);
    }

    virtual void play() override final
    {
        connect();
        auto& ac = *_demo->ac;
 //       ac.addAutomaticPullNode(recorder);
        oscillator->start(0);
        oscillator->stop(0.5f);
 //       recorder->startRecording();
 //       ac.connect(recorder, gain, 0, 0);
 //       recorder->stopRecording();
 //       ac.removeAutomaticPullNode(recorder);
 //       recorder->writeRecordingToWav("ex_osc_pop.wav", false);
    }

    virtual void ui() override final
    {
        ImGui::BeginChild("###OSCPOP", ImVec2{ 0, 100 }, true);
        if (ImGui::Button("Play"))
        {
            oscillator->start(0);
            oscillator->stop(0.5f);
        }
        static float f = 1000.f;
        if (ImGui::InputFloat("Frequency", &f))
        {
            oscillator->frequency()->setValue(f);
        }
        ImGui::EndChild();
    }

};



//////////////////////////////
//    ex_playback_events    //
//////////////////////////////

// ex_playback_events showcases the use of a `setOnEnded` callback on a `SampledAudioNode`
struct ex_playback_events : public labsound_example
{
    std::shared_ptr<SampledAudioNode> sampledAudio;
    bool waiting = true;

    virtual char const* const name() const override { return "Events"; }

    explicit ex_playback_events(Demo& demo) : labsound_example(demo)
    {
        auto& ac = *_demo->ac;
        auto musicClip = _demo->MakeBusFromSampleFile("samples/mono-music-clip.wav", ac.sampleRate());
        if (!musicClip)
            return;

        sampledAudio = std::make_shared<SampledAudioNode>(ac);
        _root_node = sampledAudio;
        {
            ContextRenderLock r(&ac, "ex_playback_events");
            sampledAudio->setBus(r, musicClip);
        }

        sampledAudio->setOnEnded([this]() {
            std::cout << "sampledAudio finished..." << std::endl;
            waiting = false;
        });
    }

    ~ex_playback_events()
    {
        if (sampledAudio)
        {
            sampledAudio->setOnEnded([]() {});
            sampledAudio->stop(0);
        }
    }

    virtual void play() override final
    {
        connect();
        sampledAudio->schedule(0.0);
    }

    virtual void ui() override final
    {
        ImGui::BeginChild("###EVENT", ImVec2{ 0, 100 }, true);
        if (ImGui::Button("Play"))
        {
            connect();
            sampledAudio->schedule(0.0);
            waiting = true;
        }
        if (waiting)
        {
            ImGui::TextUnformatted("Waiting for end of clip");
        }
        else
        {
            ImGui::TextUnformatted("End of clip detected");
        }
        ImGui::EndChild();
    }

};



////////////////////////////////
//    ex_offline_rendering    //
////////////////////////////////

// This sample illustrates how LabSound can be used "offline," where the graph is not
// pulled by an actual audio device, but rather a null destination. This sample shows
// how a `RecorderNode` can be used to capture the rendered audio to disk.
struct ex_offline_rendering : public labsound_example
{
    std::shared_ptr<AudioBus> musicClip;
    std::string path;

    virtual char const* const name() const override { return "Offline"; }

    explicit ex_offline_rendering(Demo& demo) : labsound_example(demo) 
    {
        auto& ac = *_demo->ac;
        musicClip = _demo->MakeBusFromSampleFile("samples/stereo-music-clip.wav", ac.sampleRate());
        path = "ex_offiline_rendering.wav";
    }

    virtual void play() override
    {
        auto& ac = *_demo->ac;
        ac.disconnect(ac.destinationNode());
        ac.synchronizeConnections();
        nodes.clear();

        AudioStreamConfig offlineConfig;
        offlineConfig.device_index = 0;
        offlineConfig.desired_samplerate = LABSOUND_DEFAULT_SAMPLERATE;
        offlineConfig.desired_channels = LABSOUND_DEFAULT_CHANNELS;
        AudioStreamConfig inputConfig = {};

        const float recording_time_ms = 1000.f;

        std::shared_ptr<lab::AudioDestinationNode> renderNode =
            std::make_shared<lab::AudioDestinationNode>(ac,
                std::make_unique<lab::AudioDevice_Null>(inputConfig, offlineConfig));

        std::shared_ptr<OscillatorNode> oscillator;
        std::shared_ptr<AudioBus> musicClip = MakeBusFromSampleFile("samples/stereo-music-clip.wav");
        std::shared_ptr<SampledAudioNode> musicClipNode;
        std::shared_ptr<GainNode> gain;

        std::shared_ptr<RecorderNode> recorder(new RecorderNode(ac, offlineConfig));

        ac.connect(renderNode, recorder);

        recorder->startRecording();
        {
            ContextRenderLock r(&ac, "ex_offline_rendering");

            gain.reset(new GainNode(ac));
            gain->gain()->setValue(0.125f);

            // osc -> gain -> recorder
            oscillator.reset(new OscillatorNode(ac));
            ac.connect(gain, oscillator, 0, 0);
            ac.connect(recorder, gain, 0, 0);
            oscillator->frequency()->setValue(880.f);
            oscillator->setType(OscillatorType::SINE);
            oscillator->start(0.0f);

            musicClipNode.reset(new SampledAudioNode(ac));
            ac.connect(recorder, musicClipNode, 0, 0);
            musicClipNode->setBus(musicClip);
            musicClipNode->schedule(0.0);
        }

        auto bus = std::make_shared<lab::AudioBus>(2, 128);
        renderNode->offlineRender(bus.get(), musicClip->length());
        recorder->stopRecording();
        printf("Recorded %f seconds of audio\n", recorder->recordedLengthInSeconds());
        recorder->writeRecordingToWav("ex_offline_rendering.wav", false);
    }
};



//////////////////////
//    ex_tremolo    //
//////////////////////

// This demonstrates the use of `connectParam` as a way of modulating one node through another. 
// Params are control signals that operate at audio rate.
struct ex_tremolo : public labsound_example
{
    std::shared_ptr<OscillatorNode> oscillator;
    std::shared_ptr<OscillatorNode> modulator;
    std::shared_ptr<GainNode> modulatorGain;
    float freq;
    float mod_freq;
    float variance;

    virtual char const* const name() const override { return "Tremolo"; }

    explicit ex_tremolo(Demo& demo) : labsound_example(demo)
    {
        auto& ac = *_demo->ac;
        modulator = std::make_shared<OscillatorNode>(ac);
        modulator->setType(OscillatorType::SINE);
        variance = 8;
        modulator->frequency()->setValue(variance);
        modulator->start(0);

        mod_freq = 10.f;
        modulatorGain = std::make_shared<GainNode>(ac);
        modulatorGain->gain()->setValue(mod_freq);

        freq = 440.f;
        oscillator = std::make_shared<OscillatorNode>(ac);
        _root_node = oscillator;
        oscillator->setType(OscillatorType::TRIANGLE);
        oscillator->frequency()->setValue(freq);

        // Set up processing chain
        // modulator > modulatorGain ---> osc frequency
        //                                osc > context
        ac.connect(modulatorGain, modulator, 0, 0);
        ac.connectParam(oscillator->detune(), modulatorGain, 0);
    }
 
    virtual void play() override final
    {
        connect();
        oscillator->start(0);
    }

    virtual void ui() override final
    {
        ImGui::BeginChild("###TREMOLO", ImVec2{ 0, 100 }, true);
        if (ImGui::Button("Play"))
        {
            oscillator->start(0);
            oscillator->stop(0.5f);
        }
        if (ImGui::InputFloat("Frequency", &freq))
        {
            oscillator->frequency()->setValue(freq);
        }
        if (ImGui::InputFloat("Speed", &mod_freq))
        {
            modulator->frequency()->setValue(mod_freq);
        }
        if (ImGui::InputFloat("Variance", &variance))
        {
            modulatorGain->gain()->setValue(variance);
        }
        ImGui::EndChild();
    }

};



///////////////////////////////////
//    ex_frequency_modulation    //
///////////////////////////////////

// This is inspired by a patch created in the ChucK audio programming language. It showcases
// LabSound's ability to construct arbitrary graphs of oscillators a-la FM synthesis.
struct ex_frequency_modulation : public labsound_example
{
    std::shared_ptr<OscillatorNode> modulator;
    std::shared_ptr<GainNode> modulatorGain;
    std::shared_ptr<OscillatorNode> osc;
    std::shared_ptr<ADSRNode> trigger;

    std::shared_ptr<GainNode> signalGain;
    std::shared_ptr<GainNode> feedbackTap;
    std::shared_ptr<DelayNode> chainDelay;

    std::chrono::steady_clock::time_point prev;
    UniformRandomGenerator fmrng;

    bool on = true;

    virtual char const* const name() const override { return "Frequence Modulation"; }

    explicit ex_frequency_modulation(Demo& demo) : labsound_example(demo)
    {
        auto& ac = *_demo->ac;
        modulator = std::make_shared<OscillatorNode>(ac);
        modulator->setType(OscillatorType::SQUARE);
        const float mod_freq = fmrng.random_float(4.f, 512.f);
        modulator->frequency()->setValue(mod_freq);
        modulator->start(0);

        modulatorGain = std::make_shared<GainNode>(ac);

        osc = std::make_shared<OscillatorNode>(ac);
        osc->setType(OscillatorType::SQUARE);
        const float carrier_freq = fmrng.random_float(80.f, 440.f);
        osc->frequency()->setValue(carrier_freq);
        osc->start(0);

        trigger = std::make_shared<ADSRNode>(ac);
        trigger->oneShot()->setBool(false);

        signalGain = std::make_shared<GainNode>(ac);
        signalGain->gain()->setValue(1.0f);

        feedbackTap = std::make_shared<GainNode>(ac);
        feedbackTap->gain()->setValue(0.5f);

        chainDelay = std::make_shared<DelayNode>(ac, 4);
        chainDelay->delayTime()->setFloat(0.0f);  // passthrough delay, not sure if this has the same DSP semantic as ChucK

        // Set up FM processing chain:
        ac.connect(modulatorGain, modulator, 0, 0);  // Modulator to Gain
        ac.connectParam(osc->frequency(), modulatorGain, 0);  // Gain to frequency parameter
        ac.connect(trigger, osc, 0, 0);  // Osc to ADSR
        ac.connect(signalGain, trigger, 0, 0);  // ADSR to signalGain
        ac.connect(feedbackTap, signalGain, 0, 0);  // Signal to Feedback
        ac.connect(chainDelay, feedbackTap, 0, 0);  // Feedback to Delay
        ac.connect(signalGain, chainDelay, 0, 0);  // Delay to signalGain
        _root_node = signalGain;// signalGain;
    }

    virtual void play() override final
    {
        connect();
        prev = std::chrono::steady_clock::now();
    }

    virtual void update() override
    {
        if (!_root_node || !_root_node->output(0)->isConnected())
            return;

        auto now = std::chrono::steady_clock::now();
        if (now - prev < std::chrono::milliseconds(500))
            return;

        if (on)
        {
            trigger->gate()->setValue(0.f);
            on = false;
            return;
        }
        on = true;

        prev = now;

        auto& ac = *_demo->ac;

        const float carrier_freq = fmrng.random_float(80.f, 440.f);
        osc->frequency()->setValue(carrier_freq);

        const float mod_freq = fmrng.random_float(4.f, 512.f);
        modulator->frequency()->setValue(mod_freq);

        const float mod_gain = fmrng.random_float(16.f, 1024.f);
        modulatorGain->gain()->setValue(mod_gain);

        const float attack_length = fmrng.random_float(0.25f, 0.5f);
        trigger->set(attack_length, 0.50f, 0.50f, 0.25f, 0.50f, 0.1f);

        double t = ac.currentTime();
        trigger->gate()->setValueAtTime(0, static_cast<float>(t));
        trigger->gate()->setValueAtTime(1, static_cast<float>(t + 0.1));

        //std::cout << "[ex_frequency_modulation] car_freq: " << carrier_freq << std::endl;
        //std::cout << "[ex_frequency_modulation] mod_freq: " << mod_freq << std::endl;
        //std::cout << "[ex_frequency_modulation] mod_gain: " << mod_gain << std::endl;
    }
};



///////////////////////////////////
//    ex_runtime_graph_update    //
///////////////////////////////////

// In most examples, nodes are not disconnected during playback. This sample shows how nodes
// can be arbitrarily connected/disconnected during runtime while the graph is live. 
struct ex_runtime_graph_update : public labsound_example
{
    std::shared_ptr<OscillatorNode> oscillator1, oscillator2;
    std::shared_ptr<GainNode> gain;
    std::chrono::steady_clock::time_point prev;
    int disconnect;

    virtual char const* const name() const override { return "Graph Update"; }

    explicit ex_runtime_graph_update(Demo& demo) : labsound_example(demo)
    {
        auto& ac = *_demo->ac;
        oscillator1 = std::make_shared<OscillatorNode>(ac);
        oscillator2 = std::make_shared<OscillatorNode>(ac);

        gain = std::make_shared<GainNode>(ac);
        gain->gain()->setValue(0.50);
        _root_node = gain;

        // osc -> gain -> destination
        ac.connect(gain, oscillator1, 0, 0);
        ac.connect(gain, oscillator2, 0, 0);

        oscillator1->setType(OscillatorType::SINE);
        oscillator1->frequency()->setValue(220.f);
        oscillator1->start(0.00f);

        oscillator2->setType(OscillatorType::SINE);
        oscillator2->frequency()->setValue(440.f);
        oscillator2->start(0.00);
        disconnect = 4;
    }

    virtual void play() override
    {
        connect();
        prev = std::chrono::steady_clock::now();
        disconnect = 1;
    }

    virtual void update() override
    {
        if (disconnect >= 4)
            return;

        auto now = std::chrono::steady_clock::now();
        auto duration = now - prev;

        auto& ac = *_demo->ac;
        if (disconnect == 1 && duration > std::chrono::milliseconds(500))
        {
            disconnect = 2;
            ac.disconnect(nullptr, oscillator1, 0, 0);
            ac.connect(gain, oscillator2, 0, 0);
        }

        if (disconnect == 2 && duration > std::chrono::milliseconds(1000))
        {
            disconnect = 3;
            ac.disconnect(nullptr, oscillator2, 0, 0);
            ac.connect(gain, oscillator1, 0, 0);
        }

        if (disconnect == 3 && duration > std::chrono::milliseconds(1500))
        {
            ac.disconnect(nullptr, oscillator1, 0, 0);
            ac.disconnect(nullptr, oscillator2, 0, 0);
            ac.disconnect(gain, _demo->recorder);
            disconnect = 4;
            std::cout << "OscillatorNode 1 use_count: " << oscillator1.use_count() << std::endl;
            std::cout << "OscillatorNode 2 use_count: " << oscillator2.use_count() << std::endl;
            std::cout << "GainNode use_count:         " << gain.use_count() << std::endl;
        }
    }

};



//////////////////////////////////
//    ex_microphone_loopback    //
//////////////////////////////////

// This example simply connects an input device (e.g. a microphone) to the output audio device (e.g. your speakers). 
// DANGER! This sample creates an open feedback loop. It is best used when the output audio device is a pair of headphones. 
struct ex_microphone_loopback : public labsound_example
{
    std::shared_ptr<AudioHardwareInputNode> input;

    virtual char const* const name() const override { return "Mic Loopback"; }

    explicit ex_microphone_loopback(Demo& demo) : labsound_example(demo)
    {
        auto& ac = *_demo->ac;
        ac.disconnect(ac.destinationNode());
        ac.synchronizeConnections();
        nodes.clear();

        ContextRenderLock r(&ac, "ex_microphone_loopback");
        std::shared_ptr<AudioHardwareInputNode> inputNode(
                new AudioHardwareInputNode(ac, ac.destinationNode()->device()->sourceProvider()));
        ac.connect(ac.destinationNode(), inputNode, 0, 0);

        _root_node = inputNode;
    }

    virtual void play() override final
    {
        connect();
    }

    virtual void ui() override final
    {
        if (!_root_node || !_root_node->output(0)->isConnected())
            return;

        ImGui::BeginChild("###LOOPBACK", ImVec2{ 0, 100 }, true);
        ImGui::TextUnformatted("Input connected directly to output");
        if (ImGui::Button("Disconnect"))
        {
            disconnect();
        }
        ImGui::EndChild();
    }

};



////////////////////////////////
//    ex_microphone_reverb    //
////////////////////////////////

// This sample takes input from a microphone and convolves it with an impulse response to create reverb (i.e. use of the `ConvolverNode`).
// The sample convolution is for a rather large room, so there is a delay.
// DANGER! This sample creates an open feedback loop. It is best used when the output audio device is a pair of headphones. 
struct ex_microphone_reverb : public labsound_example
{
    std::shared_ptr<AudioBus> impulseResponseClip;
    std::shared_ptr<AudioHardwareInputNode> input;
    std::shared_ptr<ConvolverNode> convolve;
    std::shared_ptr<GainNode> wetGain;

    virtual char const* const name() const override { return "Mic Reverb"; }

    explicit ex_microphone_reverb(Demo& demo) : labsound_example(demo)
    {
        auto& ac = *_demo->ac;
        ac.disconnect(ac.destinationNode());
        ac.synchronizeConnections();
        nodes.clear();

        std::shared_ptr<AudioBus> impulseResponseClip =
            MakeBusFromSampleFile("impulse/cardiod-rear-levelled.wav");
        std::shared_ptr<AudioHardwareInputNode> input;
        std::shared_ptr<ConvolverNode> convolve;
        std::shared_ptr<GainNode> wetGain;
        {
            ContextRenderLock r(&ac, "ex_microphone_reverb");

            std::shared_ptr<AudioHardwareInputNode> inputNode(
                    new AudioHardwareInputNode(ac, ac.destinationNode()->device()->sourceProvider()));

            convolve.reset(new ConvolverNode(ac));
            convolve->setImpulse(impulseResponseClip);

            wetGain.reset(new GainNode(ac));
            wetGain->gain()->setValue(0.6f);

            ac.connect(convolve, inputNode, 0, 0);
            ac.connect(wetGain, convolve, 0, 0);
            ac.connect(ac.destinationNode(), wetGain, 0, 0);
        }

        _root_node = wetGain;
    }

    virtual void play() override
    {
        connect();
    }

    virtual void ui() override final
    {
        if (!_root_node || !_root_node->output(0)->isConnected())
            return;

        ImGui::BeginChild("###MICREVERB", ImVec2{ 0, 100 }, true);
        ImGui::TextUnformatted("Mic reverb active");
        if (ImGui::Button("Disconnect mic"))
        {
            disconnect();
        }
        ImGui::EndChild();
    }
};



//////////////////////////////
//    ex_peak_compressor    //
//////////////////////////////

// Demonstrates the use of the `PeakCompNode` and many scheduled audio sources.
struct ex_peak_compressor : public labsound_example
{
    std::shared_ptr<SampledAudioNode> kick_node;
    std::shared_ptr<SampledAudioNode> hihat_node;
    std::shared_ptr<SampledAudioNode> snare_node;

    std::shared_ptr<BiquadFilterNode> filter;
    std::shared_ptr<PeakCompNode> peakComp;

    virtual char const* const name() const override { return "Peak Compressor"; }

    explicit ex_peak_compressor(Demo& demo) : labsound_example(demo)
    {
        auto& ac = *_demo->ac;
        ContextRenderLock r(&ac, "ex_peak_compressor");
        kick_node = std::make_shared<SampledAudioNode>(ac);
        hihat_node = std::make_shared<SampledAudioNode>(ac);
        snare_node = std::make_shared<SampledAudioNode>(ac);
        kick_node->setBus(r, _demo->MakeBusFromSampleFile("samples/kick.wav", ac.sampleRate()));
        hihat_node->setBus(r, _demo->MakeBusFromSampleFile("samples/hihat.wav", ac.sampleRate()));
        snare_node->setBus(r, _demo->MakeBusFromSampleFile("samples/snare.wav", ac.sampleRate()));

        filter = std::make_shared<BiquadFilterNode>(ac);
        filter->setType(lab::FilterType::LOWPASS);
        filter->frequency()->setValue(1800.f);

        peakComp = std::make_shared<PeakCompNode>(ac);
        _root_node = peakComp;
        ac.connect(peakComp, filter, 0, 0);
        ac.connect(filter, kick_node, 0, 0);
        ac.connect(filter, hihat_node, 0, 0);
        //hihat_node->gain()->setValue(0.2f);

        ac.connect(filter, snare_node, 0, 0);
    }

    virtual void play() override final
    {
        connect();
        hihat_node->schedule(0);

        // Speed Metal
        float startTime = 0.1f;
        float bpm = 30.f;
        float bar_length = 60.f / bpm;
        float eighthNoteTime = bar_length / 8.0f;
        for (float bar = 0; bar < 8; bar += 1)
        {
            float time = startTime + bar * bar_length;

            kick_node->schedule(time);
            kick_node->schedule(time + 4 * eighthNoteTime);

            snare_node->schedule(time + 2 * eighthNoteTime);
            snare_node->schedule(time + 6 * eighthNoteTime);

            float hihat_beat = 8;
            for (float i = 0; i < hihat_beat; i += 1)
                hihat_node->schedule(time + bar_length * i / hihat_beat);

        }
    }
};



/////////////////////////////
//    ex_stereo_panning    //
/////////////////////////////

// This illustrates the use of equal-power stereo panning.
struct ex_stereo_panning : public labsound_example
{
    std::shared_ptr<SampledAudioNode> audioClipNode;
    std::shared_ptr<StereoPannerNode> stereoPanner;
    std::chrono::steady_clock::time_point prev;
    bool autopan;
    float pos;

    virtual char const* const name() const override { return "Stereo Panning"; }

    explicit ex_stereo_panning(Demo& demo) : labsound_example(demo)
    {
        auto& ac = *_demo->ac;
        std::shared_ptr<AudioBus> audioClip = _demo->MakeBusFromSampleFile("samples/trainrolling.wav", ac.sampleRate());
        audioClipNode = std::make_shared<SampledAudioNode>(ac);
        stereoPanner = std::make_shared<StereoPannerNode>(ac);
        _root_node = stereoPanner;
        autopan = true;
        pos = 0.f;

        {
            ContextRenderLock r(&ac, "ex_stereo_panning");

            audioClipNode->setBus(r, audioClip);
            ac.connect(stereoPanner, audioClipNode, 0, 0);
        }
    }

    virtual void play() override final
    {
        if (!audioClipNode)
            return;

        connect();

        audioClipNode->schedule(0.0, -1); // -1 to loop forever
        prev = std::chrono::steady_clock::now();
    }

    virtual void update() override
    {
        if (!autopan)
            return;

        if (!_root_node || !_root_node->output(0)->isConnected())
            return;

        const float seconds = 10.f;
        auto now = std::chrono::steady_clock::now();
        auto elapsed = now - prev;
        float t = fmodf(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() * 0.001f, seconds);
        float halfTime = seconds * 0.5f;
        pos = (t - halfTime) / halfTime;

        // Put position a +up && +front, because if it goes right through the
        // listener at (0, 0, 0) it abruptly switches from left to right.
        stereoPanner->pan()->setValue(pos);
    }

    virtual void ui() override final
    {
        if (!_root_node || !_root_node->output(0)->isConnected())
            return;

        ImGui::BeginChild("###PANNING", ImVec2{ 0, 100 }, true);
        if (ImGui::SliderFloat("Pan", &pos, -1.f, 1.f, "%0.3f"))
        {
            autopan = false;
            stereoPanner->pan()->setValue(pos);
        }
        ImGui::EndChild();
    }
};



//////////////////////////////////
//    ex_hrtf_spatialization    //
//////////////////////////////////

// This illustrates 3d sound spatialization and doppler shift. Headphones are recommended for this sample.
struct ex_hrtf_spatialization : public labsound_example
{
    std::shared_ptr<AudioBus> audioClip;
    std::shared_ptr<SampledAudioNode> audioClipNode;
    std::shared_ptr<PannerNode> panner;
    std::chrono::steady_clock::time_point prev;
    ImVec4 pos;
    ImVec4 minPos;
    ImVec4 maxPos;
    bool autopan;

    virtual char const* const name() const override { return "HRTF Spatialization"; }

    explicit ex_hrtf_spatialization(Demo& demo) : labsound_example(demo)
    {
        autopan = true;
        pos = ImVec4{ 0, 0.1f, 0.1f, 0 };
        minPos = ImVec4{ -1, -1, -1, 0 };
        maxPos = ImVec4{ 1, 1, 1, 0 };

        auto& ac = *_demo->ac;
        std::cout << "Sample Rate is: " << ac.sampleRate() << std::endl;
        audioClip = _demo->MakeBusFromSampleFile("samples/trainrolling.wav", ac.sampleRate());
        audioClipNode = std::make_shared<SampledAudioNode>(ac);

        std::string hrtf_path = lab_application_resource_path(nullptr, nullptr);
        hrtf_path += "/hrtf";

        panner = std::make_shared<PannerNode>(ac);  // note hrtf search path
        _root_node = panner;

        ContextRenderLock r(&ac, "ex_hrtf_spatialization");

        panner->setPanningModel(PanningModel::HRTF);

        audioClipNode->setBus(r, audioClip);
        ac.connect(panner, audioClipNode, 0, 0);
    }

    virtual void play() override final
    {
        connect();

        auto& ac = *_demo->ac;
        ac.listener()->setPosition({ 0, 0, 0 });
        panner->setVelocity(4, 0, 0);
        prev = std::chrono::steady_clock::now();
        audioClipNode->schedule(0.0, -1); // -1 to loop forever
    }

    virtual void update() override
    {
        if (!autopan)
            return;

        if (!_root_node || !_root_node->output(0)->isConnected())
            return;

        const float seconds = 10.f;
        auto now = std::chrono::steady_clock::now();
        auto elapsed = now - prev;
        float t = fmodf(std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() * 0.001f, seconds);
        float halfTime = seconds * 0.5f;
        pos.x = (t - halfTime) / halfTime;

        panner->setPosition({ pos.x, pos.y, pos.z });
    }
    
    virtual void ui() override final
    {
        if (!_root_node || !_root_node->output(0)->isConnected())
            return;

        ImGui::BeginChild("###HRTF", ImVec2{ 0, 500 }, true);
        if (InputVec3("Pos", &pos, minPos, maxPos, 1.f))
        {
            autopan = false;
            panner->setPosition({ pos.x, pos.y, pos.z });
        }
        if (ImGui::Button("Stop"))
            disconnect();

        ImGui::EndChild();
    }
};



////////////////////////////////
//    ex_convolution_reverb    //
////////////////////////////////

// This shows the use of the `ConvolverNode` to produce reverb from an arbitrary impulse response.
struct ex_convolution_reverb : public labsound_example
{
    std::shared_ptr<ConvolverNode> convolve;
    std::shared_ptr<GainNode> wetGain;
    std::shared_ptr<GainNode> dryGain;
    std::shared_ptr<SampledAudioNode> voiceNode;
    std::shared_ptr<GainNode> masterGain;

    virtual char const* const name() const override { return "Convolution Reverb"; }

    explicit ex_convolution_reverb(Demo& demo) : labsound_example(demo)
    {
        auto& ac = *_demo->ac;
        std::shared_ptr<AudioBus> impulseResponseClip = _demo->MakeBusFromSampleFile("impulse/cardiod-rear-levelled.wav", ac.sampleRate());
        std::shared_ptr<AudioBus> voiceClip = _demo->MakeBusFromSampleFile("samples/voice.ogg", ac.sampleRate());

        if (!impulseResponseClip || !voiceClip)
        {
            std::cerr << "Could not open sample data\n";
            return;
        }

        ContextRenderLock r(&ac, "ex_convolution_reverb");
        masterGain = std::make_shared<GainNode>(ac);
        masterGain->gain()->setValue(0.5f);

        convolve = std::make_shared<ConvolverNode>(ac);
        convolve->setImpulse(impulseResponseClip);

        wetGain = std::make_shared<GainNode>(ac);
        wetGain->gain()->setValue(0.5f);
        dryGain = std::make_shared<GainNode>(ac);
        dryGain->gain()->setValue(0.1f);

        voiceNode = std::make_shared<SampledAudioNode>(ac);
        voiceNode->setBus(r, voiceClip);

        // voice --> dry --+----------------------+
        //                 |                      |
        //                 +-> convolve --> wet --+--> master --> 

        ac.connect(dryGain, voiceNode, 0, 0);
        ac.connect(convolve, dryGain, 0, 0);
        ac.connect(wetGain, convolve, 0, 0);
        ac.connect(masterGain, wetGain, 0, 0);
        ac.connect(masterGain, dryGain, 0, 0);
        _root_node = masterGain;// masterGain;
    }

    virtual void play() override final
    {
        if (!_root_node)
            return;

        connect();
        voiceNode->schedule(0.0);
    }

    virtual void ui() override final
    {
        if (!_root_node || !_root_node->output(0)->isConnected())
            return;

        ImGui::BeginChild("###CONVREVERB", ImVec2{ 0, 100 }, true);
        ImGui::TextUnformatted("Convolution reverb");
        static float dry = dryGain->gain()->value();
        if (ImGui::InputFloat("dry gain", &dry))
        {
            dryGain->gain()->setValue(dry);
        }
        static float wet = wetGain->gain()->value();
        if (ImGui::InputFloat("wet gain", &wet))
        {
            wetGain->gain()->setValue(wet);
        }
        static float master = masterGain->gain()->value();
        if (ImGui::InputFloat("master gain", &master))
        {
            masterGain->gain()->setValue(master);
        }
        if (ImGui::Button("Disconnect mic"))
        {
            disconnect();
        }
        ImGui::EndChild();
    }

    //ui - file chooser for impulse response and for voice clip
};

///////////////////
//    ex_misc    //
///////////////////

// An example with a several of nodes to verify api + functionality changes/improvements/regressions
struct ex_misc : public labsound_example
{
    std::array<int, 8> majorScale = { 0, 2, 4, 5, 7, 9, 11, 12 };
    std::array<int, 8> naturalMinorScale = { 0, 2, 3, 5, 7, 9, 11, 12 };
    std::array<int, 6> pentatonicMajor = { 0, 2, 4, 7, 9, 12 };
    std::array<int, 8> pentatonicMinor = { 0, 3, 5, 7, 10, 12 };
    std::array<int, 8> delayTimes = { 266, 533, 399 };

    std::shared_ptr<AudioBus> audioClip;
    std::shared_ptr<SampledAudioNode> audioClipNode;
    std::shared_ptr<PingPongDelayNode> pingping;

    virtual char const* const name() const override { return "PingPong Delay"; }

    explicit ex_misc(Demo& demo) : labsound_example(demo) 
    {
        auto& ac = *_demo->ac;
        audioClip = _demo->MakeBusFromSampleFile("samples/cello_pluck/cello_pluck_As0.wav", ac.sampleRate());
        audioClipNode = std::make_shared<SampledAudioNode>(ac);
        pingping = std::make_shared<PingPongDelayNode>(ac, 240.0f);

        ContextRenderLock r(&ac, "ex_misc");

        pingping->BuildSubgraph(ac);
        pingping->SetFeedback(.75f);
        pingping->SetDelayIndex(lab::TempoSync::TS_16);

        _root_node = pingping->output;

        audioClipNode->setBus(r, audioClip);
        ac.connect(pingping->input, audioClipNode, 0, 0);
    }

    virtual void play() override
    {
        connect();
        audioClipNode->schedule(0.25);
    }
};

///////////////////////////
//    ex_dalek_filter    //
///////////////////////////

// Send live audio to a Dalek filter, constructed according to the recipe at http://webaudio.prototyping.bbc.co.uk/ring-modulator/.
// This is used as an example of a complex graph constructed using the LabSound API.
struct ex_dalek_filter : public labsound_example
{
    std::shared_ptr<AudioHardwareInputNode> input;

    std::shared_ptr<OscillatorNode> vIn;
    std::shared_ptr<GainNode> vInGain;
    std::shared_ptr<GainNode> vInInverter1;
    std::shared_ptr<GainNode> vInInverter2;
    std::shared_ptr<GainNode> vInInverter3;
    std::shared_ptr<DiodeNode> vInDiode1;
    std::shared_ptr<DiodeNode> vInDiode2;
    std::shared_ptr<GainNode> vcInverter1;
    std::shared_ptr<DiodeNode> vcDiode3;
    std::shared_ptr<DiodeNode> vcDiode4;
    std::shared_ptr<GainNode> outGain;
    std::shared_ptr<DynamicsCompressorNode> compressor;
    std::shared_ptr<SampledAudioNode> audioClipNode;

    virtual char const* const name() const override { return "Mic Dalek"; }

    explicit ex_dalek_filter(Demo& demo) : labsound_example(demo)
    {
        lab::AudioContext& ac = *demo.ac;
        ac.disconnect(ac.destinationNode());
        ac.synchronizeConnections();
        nodes.clear();

#ifndef USE_LIVE
        auto audioClip = MakeBusFromSampleFile("samples/voice.ogg");
        if (!audioClip)
            return;
        std::shared_ptr<SampledAudioNode> audioClipNode = std::make_shared<SampledAudioNode>(ac);
#endif

        std::shared_ptr<AudioHardwareInputNode> inputNode;

        std::shared_ptr<OscillatorNode> vIn;
        std::shared_ptr<GainNode> vInGain;
        std::shared_ptr<GainNode> vInInverter1;
        std::shared_ptr<GainNode> vInInverter2;
        std::shared_ptr<GainNode> vInInverter3;
        std::shared_ptr<DiodeNode> vInDiode1;
        std::shared_ptr<DiodeNode> vInDiode2;
        std::shared_ptr<GainNode> vcInverter1;
        std::shared_ptr<DiodeNode> vcDiode3;
        std::shared_ptr<DiodeNode> vcDiode4;
        std::shared_ptr<GainNode> outGain;
        std::shared_ptr<DynamicsCompressorNode> compressor;

        {
            ContextRenderLock r(&ac, "ex_dalek_filter");

            vIn = std::make_shared<OscillatorNode>(ac);
            vIn->frequency()->setValue(30.0f);
            vIn->start(0.f);

            vInGain = std::make_shared<GainNode>(ac);
            vInGain->gain()->setValue(0.5f);

            // GainNodes can take negative gain which represents phase inversion
            vInInverter1 = std::make_shared<GainNode>(ac);
            vInInverter1->gain()->setValue(-1.0f);
            vInInverter2 = std::make_shared<GainNode>(ac);
            vInInverter2->gain()->setValue(-1.0f);

            vInDiode1 = std::make_shared<DiodeNode>(ac);
            vInDiode2 = std::make_shared<DiodeNode>(ac);

            vInInverter3 = std::make_shared<GainNode>(ac);
            vInInverter3->gain()->setValue(-1.0f);

            // Now we create the objects on the Vc side of the graph
            vcInverter1 = std::make_shared<GainNode>(ac);
            vcInverter1->gain()->setValue(-1.0f);

            vcDiode3 = std::make_shared<DiodeNode>(ac);
            vcDiode4 = std::make_shared<DiodeNode>(ac);

            // A gain node to control master output levels
            outGain = std::make_shared<GainNode>(ac);
            outGain->gain()->setValue(1.0f);

            // A small addition to the graph given in Parker's paper is a compressor node
            // immediately before the output. This ensures that the user's volume remains
            // somewhat constant when the distortion is increased.
            compressor = std::make_shared<DynamicsCompressorNode>(ac);
            compressor->threshold()->setValue(-14.0f);

            // Now we connect up the graph following the block diagram above (on the web page).
            // When working on complex graphs it helps to have a pen and paper handy!

#ifdef USE_LIVE
            std::shared_ptr<AudioHardwareInputNode> inputNode(
                    new AudioHardwareInputNode(ac, ac.destinationNode()->device()->sourceProvider()));
            ac.connect(vcInverter1, inputNode, 0, 0);
            ac.connect(vcDiode4, inputNode, 0, 0);
#else
            audioClipNode->setBus(r, audioClip);
            //ac.connect(vcInverter1, audioClipNode, 0, 0); // dimitri
            ac.connect(vcDiode4, audioClipNode, 0, 0);
            audioClipNode->start(0.f);
#endif

            ac.connect(vcDiode3, vcInverter1, 0, 0);

            // Then the Vin side
            ac.connect(vInGain, vIn, 0, 0);
            ac.connect(vInInverter1, vInGain, 0, 0);
            ac.connect(vcInverter1, vInGain, 0, 0);
            ac.connect(vcDiode4, vInGain, 0, 0);

            ac.connect(vInInverter2, vInInverter1, 0, 0);
            ac.connect(vInDiode2, vInInverter1, 0, 0);
            ac.connect(vInDiode1, vInInverter2, 0, 0);

            // Finally connect the four diodes to the destination via the output-stage compressor and master gain node
            ac.connect(vInInverter3, vInDiode1, 0, 0);
            ac.connect(vInInverter3, vInDiode2, 0, 0);

            ac.connect(compressor, vInInverter3, 0, 0);
            ac.connect(compressor, vcDiode3, 0, 0);
            ac.connect(compressor, vcDiode4, 0, 0);

            ac.connect(outGain, compressor, 0, 0);
            ac.connect(ac.destinationNode(), outGain, 0, 0);
        }

        _root_node = vcDiode4;// outGain;
    }

    virtual void play() override
    {
        connect();
        if (!input)
            audioClipNode->schedule(0.f);
    }

    virtual void ui() override final
    {
        if (!_root_node || !_root_node->output(0)->isConnected())
            return;

        ImGui::BeginChild("###DALEK", ImVec2{ 0, 100 }, true);
        ImGui::TextUnformatted("Dalek voice changer active");
        if (ImGui::Button("Disconnect mic"))
        {
            disconnect();
        }
        ImGui::EndChild();
    }

};

/////////////////////////////////
//    ex_redalert_synthesis    //
/////////////////////////////////

// This is another example of a non-trival graph constructed with the LabSound API. Furthermore, it incorporates
// the use of several `FunctionNodes` that are base types used for implementing complex DSP without modifying
// LabSound internals directly.
struct ex_redalert_synthesis : public labsound_example
{
    std::shared_ptr<FunctionNode> sweep;
    std::shared_ptr<FunctionNode> outputGainFunction;

    std::shared_ptr<OscillatorNode> osc;
    std::shared_ptr<GainNode> oscGain;
    std::shared_ptr<OscillatorNode> resonator;
    std::shared_ptr<GainNode> resonatorGain;
    std::shared_ptr<GainNode> resonanceSum;

    std::shared_ptr<DelayNode> delay[5];

    std::shared_ptr<GainNode> delaySum;
    std::shared_ptr<GainNode> filterSum;

    std::shared_ptr<BiquadFilterNode> filter[5];

    virtual char const* const name() const override { return "Red Alert"; }

    explicit ex_redalert_synthesis(Demo& demo) : labsound_example(demo)
    {
        auto& ac = *_demo->ac;
        ContextRenderLock r(&ac, "ex_redalert_synthesis");

        sweep = std::make_shared<FunctionNode>(ac, 1);
        sweep->setFunction([](ContextRenderLock& r, FunctionNode* me, int channel, float* values, size_t framesToProcess) 
        {
            double dt = 1.0 / r.context()->sampleRate();
            double now = fmod(me->now(), 1.2f);

            for (size_t i = 0; i < framesToProcess; ++i)
            {
                //0 to 1 in 900 ms with a 1200ms gap in between
                if (now > 0.9)
                {
                    values[i] = 487.f + 360.f;
                }
                else
                {
                    values[i] = std::sqrt((float)now * 1.f / 0.9f) * 487.f + 360.f;
                }

                now += dt;
            }
        });

        outputGainFunction = std::make_shared<FunctionNode>(ac, 1);
        outputGainFunction->setFunction([](ContextRenderLock& r, FunctionNode* me, int channel, float* values, size_t framesToProcess) 
        {
            double dt = 1.0 / r.context()->sampleRate();
            double now = fmod(me->now(), 1.2f);

            for (size_t i = 0; i < framesToProcess; ++i)
            {
                //0 to 1 in 900 ms with a 1200ms gap in between
                if (now > 0.9)
                {
                    values[i] = 0;
                }
                else
                {
                    values[i] = 0.333f;
                }

                now += dt;
            }
        });

        osc = std::make_shared<OscillatorNode>(ac);
        osc->setType(OscillatorType::SAWTOOTH);
        osc->frequency()->setValue(220);
        oscGain = std::make_shared<GainNode>(ac);
        oscGain->gain()->setValue(0.5f);

        resonator = std::make_shared<OscillatorNode>(ac);
        resonator->setType(OscillatorType::SINE);
        resonator->frequency()->setValue(220);

        resonatorGain = std::make_shared<GainNode>(ac);
        resonatorGain->gain()->setValue(0.0f);

        resonanceSum = std::make_shared<GainNode>(ac);
        resonanceSum->gain()->setValue(0.5f);

        // sweep drives oscillator frequency
        ac.connectParam(osc->frequency(), sweep, 0);

        // oscillator drives resonator frequency
        ac.connectParam(resonator->frequency(), osc, 0);

        // osc --> oscGain -------------+
        // resonator -> resonatorGain --+--> resonanceSum
        ac.connect(oscGain, osc, 0, 0);
        ac.connect(resonanceSum, oscGain, 0, 0);
        ac.connect(resonatorGain, resonator, 0, 0);
        ac.connect(resonanceSum, resonatorGain, 0, 0);

        delaySum = std::make_shared<GainNode>(ac);
        delaySum->gain()->setValue(0.2f);

        // resonanceSum --+--> delay0 --+
        //                +--> delay1 --+
        //                + ...    .. --+
        //                +--> delay4 --+---> delaySum
        float delays[5] = { 0.015f, 0.022f, 0.035f, 0.024f, 0.011f };
        for (int i = 0; i < 5; ++i)
        {
            delay[i] = std::make_shared<DelayNode>(ac, 0.04f);
            delay[i]->delayTime()->setFloat(delays[i]);
            ac.connect(delay[i], resonanceSum, 0, 0);
            ac.connect(delaySum, delay[i], 0, 0);
        }

        filterSum = std::make_shared<GainNode>(ac);
        filterSum->gain()->setValue(0.2f);

        // delaySum --+--> filter0 --+
        //            +--> filter1 --+
        //            +--> filter2 --+
        //            +--> filter3 --+
        //            +--------------+----> filterSum
        //
        ac.connect(filterSum, delaySum, 0, 0);

        float centerFrequencies[4] = { 740.f, 1400.f, 1500.f, 1600.f };
        for (int i = 0; i < 4; ++i)
        {
            filter[i] = std::make_shared<BiquadFilterNode>(ac);
            filter[i]->frequency()->setValue(centerFrequencies[i]);
            filter[i]->q()->setValue(12.f);
            ac.connect(filter[i], delaySum, 0, 0);
            ac.connect(filterSum, filter[i], 0, 0);
        }

        // filterSum --> destination
        ac.connectParam(filterSum->gain(), outputGainFunction, 0);

        _root_node = filterSum;
    }

    virtual void play() override
    {
        connect();
        sweep->start(0);
        outputGainFunction->start(0);
        osc->start(0);
        resonator->start(0);
    }

    virtual void ui() override final
    {
        if (!_root_node || !_root_node->output(0)->isConnected())
            return;

        ImGui::BeginChild("###ALERT", ImVec2{ 0, 100 }, true);
        ImGui::TextUnformatted("Red Alert");
        if (ImGui::Button("Stop"))
        {
            disconnect();
        }
        ImGui::EndChild();
    }

};

//////////////////////////
//    ex_wavepot_dsp    //
//////////////////////////

// "Unexpected Token" from Wavepot. Original by Stagas: http://wavepot.com/stagas/unexpected-token (MIT License)
// Wavepot is effectively ShaderToy but for the WebAudio API. 
// This sample shows the utility of LabSound as an experimental playground for DSP (synthesis + processing) using the `FunctionNode`. 
struct ex_wavepot_dsp : public labsound_example
{
    float note(int n, int octave = 0)
    {
        return std::pow(2.0f, (n - 33.f + (12.f * octave)) / 12.0f) * 440.f;
    }

    std::vector<std::vector<int>> bassline = {
        {7, 7, 7, 12, 10, 10, 10, 15},
        {7, 7, 7, 15, 15, 17, 10, 29},
        {7, 7, 7, 24, 10, 10, 10, 19},
        {7, 7, 7, 15, 29, 24, 15, 10} };

    std::vector<int> melody = {
        7, 15, 7, 15,
        7, 15, 10, 15,
        10, 12, 24, 19,
        7, 12, 10, 19 };

    std::vector<std::vector<int>> chords = { {7, 12, 17, 10}, {10, 15, 19, 24} };

    float quickSin(float x, float t)
    {
        return std::sin(2.0f * float(static_cast<float>(LAB_PI)) * t * x);
    }

    float quickSaw(float x, float t)
    {
        return 1.0f - 2.0f * fmod(t, (1.f / x)) * x;
    }

    float quickSqr(float x, float t)
    {
        return quickSin(x, t) > 0 ? 1.f : -1.f;
    }

    // perc family of functions implement a simple attack/decay, creating a short & percussive envelope for the signal
    float perc(float wave, float decay, float o, float t)
    {
        float env = std::max(0.f, 0.889f - (o * decay) / ((o * decay) + 1.f));
        auto ret = wave * env;
        return ret;
    }

    float perc_b(float wave, float decay, float o, float t)
    {
        float env = std::min(0.f, 0.950f - (o * decay) / ((o * decay) + 1.f));
        auto ret = wave * env;
        return ret;
    }

    float hardClip(float n, float x)
    {
        return x > n ? n : x < -n ? -n : x;
    }

    struct FastLowpass
    {
        float v = 0;
        float operator()(float n, float input)
        {
            return v += (input - v) / n;
        }
    };

    struct FastHighpass
    {
        float v = 0;
        float operator()(float n, float input)
        {
            return v += input - v * n;
        }
    };

    // http://www.musicdsp.org/showone.php?id=24
    // A Moog-style 24db resonant lowpass
    struct MoogFilter
    {
        float y1 = 0;
        float y2 = 0;
        float y3 = 0;
        float y4 = 0;

        float oldx = 0;
        float oldy1 = 0;
        float oldy2 = 0;
        float oldy3 = 0;

        float p, k, t1, t2, r, x;

        float process(float cutoff_, float resonance_, float sample_, float sampleRate)
        {
            float cutoff = 2.0f * cutoff_ / sampleRate;
            float resonance = static_cast<float>(resonance_);
            float sample = static_cast<float>(sample_);

            p = cutoff * (1.8f - 0.8f * cutoff);
            k = 2.f * std::sin(cutoff * static_cast<float>(M_PI) * 0.5f) - 1.0f;
            t1 = (1.0f - p) * 1.386249f;
            t2 = 12.0f + t1 * t1;
            r = resonance * (t2 + 6.0f * t1) / (t2 - 6.0f * t1);

            x = sample - r * y4;

            // Four cascaded one-pole filters (bilinear transform)
            y1 = x * p + oldx * p - k * y1;
            y2 = y1 * p + oldy1 * p - k * y2;
            y3 = y2 * p + oldy2 * p - k * y3;
            y4 = y3 * p + oldy3 * p - k * y4;

            // Clipping band-limited sigmoid
            y4 -= (y4 * y4 * y4) / 6.f;

            oldx = x;
            oldy1 = y1;
            oldy2 = y2;
            oldy3 = y3;

            return y4;
        }
    };

    MoogFilter lp_a[2];
    MoogFilter lp_b[2];
    MoogFilter lp_c[2];

    FastLowpass fastlp_a[2];
    FastHighpass fasthp_c[2];

    std::shared_ptr<FunctionNode> grooveBox;
    std::shared_ptr<ADSRNode> envelope;

    double elapsedTime;
    float songLenSeconds;

    virtual char const* const name() const override { return "Wavepot DSP"; }

    explicit ex_wavepot_dsp(Demo& demo) : labsound_example(demo)
    {
        elapsedTime = 0.;
        songLenSeconds = 12.0f;

        auto& ac = *_demo->ac;
        envelope = std::make_shared<ADSRNode>(ac);
        envelope->set(6.0f, 0.75f, 0.125, 14.0f, 0.0f, songLenSeconds);
        envelope->gate()->setValue(1.f);
        grooveBox = std::make_shared<FunctionNode>(ac, 2);

        grooveBox->setFunction([this](ContextRenderLock& r, FunctionNode* self, int channel, float* samples, size_t framesToProcess)
        {
            float lfo_a, lfo_b, lfo_c;
            float bassWaveform, percussiveWaveform, bassSample;
            float padWaveform, padSample;
            float kickWaveform, kickSample;
            float synthWaveform, synthPercussive, synthDegradedWaveform, synthSample;
                
            float dt = 1.f / r.context()->sampleRate();  // time duration of one sample
            float now = static_cast<float>(self->now());

            int nextMeasure = int((now / 2)) % bassline.size();
            auto bm = bassline[nextMeasure];

            int nextNote = int((now * 4.f)) % bm.size();
            float bn = note(bm[nextNote], 0);

            auto p = chords[int(now / 4) % chords.size()];

            auto mn = note(melody[int(now * 3.f) % melody.size()], int(2 - (now * 3)) % 4);

            for (size_t i = 0; i < framesToProcess; ++i)
            {
                lfo_a = quickSin(2.0f, now);
                lfo_b = quickSin(1.0f / 32.0f, now);
                lfo_c = quickSin(1.0f / 128.0f, now);

                // Bass
                bassWaveform = quickSaw(bn, now) * 1.9f + quickSqr(bn / 2.f, now) * 1.0f + quickSin(bn / 2.f, now) * 2.2f + quickSqr(bn * 3.f, now) * 3.f;
                percussiveWaveform = perc(bassWaveform / 3.f, 48.0f, fmod(now, 0.125f), now) * 1.0f;
                bassSample = lp_a[channel].process(1000.f + (lfo_b * 140.f), quickSin(0.5f, now + 0.75f) * 0.2f, percussiveWaveform, r.context()->sampleRate());

                // Pad
                padWaveform = 5.1f * quickSaw(note(p[0], 1), now) + 3.9f * quickSaw(note(p[1], 2), now) + 4.0f * quickSaw(note(p[2], 1), now) + 3.0f * quickSqr(note(p[3], 0), now);
                padSample = 1.0f - ((quickSin(2.0f, now) * 0.28f) + 0.5f) * fasthp_c[channel](0.5f, lp_c[channel].process(1100.f + (lfo_a * 150.f), 0.05f, padWaveform * 0.03f, r.context()->sampleRate()));

                // Kick
                kickWaveform = hardClip(0.37f, quickSin(note(7, -1), now)) * 2.0f + hardClip(0.07f, quickSaw(note(7, -1), now * 0.2f)) * 4.00f;
                kickSample = quickSaw(2.f, now) * 0.054f + fastlp_a[channel](240.0f, perc(hardClip(0.6f, kickWaveform), 54.f, fmod(now, 0.5f), now)) * 2.f;

                // Synth
                synthWaveform = quickSaw(mn, now + 1.0f) + quickSqr(mn * 2.02f, now) * 0.4f + quickSqr(mn * 3.f, now + 2.f);
                synthPercussive = lp_b[channel].process(3200.0f + (lfo_a * 400.f), 0.1f, perc(synthWaveform, 1.6f, fmod(now, 4.f), now) * 1.7f, r.context()->sampleRate()) * 1.8f;
                synthDegradedWaveform = synthPercussive * quickSin(note(5, 2), now);
                synthSample = 0.4f * synthPercussive + 0.05f * synthDegradedWaveform;

                // Mixer
                samples[i] = (0.66f * hardClip(0.65f, bassSample)) + (0.50f * padSample) + (0.66f * synthSample) + (2.75f * kickSample);

                now += dt;
            }

            elapsedTime += now;
        });

        ac.connect(envelope, grooveBox, 0, 0);
        _root_node = envelope;
    }

    virtual void play() override final
    {
        if (_root_node && _root_node->output(0)->isConnected())
            return;

        grooveBox->start(0);
        connect();
    }

    virtual void ui() override final
    {
        if (!_root_node || !_root_node->output(0)->isConnected())
            return;

        ImGui::BeginChild("###WAVEPOT", ImVec2{ 0, 100 }, true);
        ImGui::TextUnformatted("Wavepot DSP");
        if (ImGui::Button("Stop"))
        {
            disconnect();
        }
        ImGui::EndChild();
    }
};

///////////////////////////////
//    ex_granulation_node    //
///////////////////////////////

struct ex_granulation_node : public labsound_example
{
    std::shared_ptr<AudioBus> grain_source;
    std::shared_ptr<GranulationNode> granulation_node;
    std::shared_ptr<GainNode> gain;

    virtual char const* const name() const override { return "Granulation"; }

    explicit ex_granulation_node(Demo& demo) : labsound_example(demo)
    {
        auto& ac = *_demo->ac;
        grain_source = _demo->MakeBusFromSampleFile("samples/cello_pluck/cello_pluck_As0.wav", ac.sampleRate());
        if (!grain_source) 
            return;

        granulation_node = std::make_shared<GranulationNode>(ac);
        gain = std::make_shared<GainNode>(ac);
        //std::shared_ptr<RecorderNode> recorder;
        gain->gain()->setValue(0.75f);

        {
            ContextRenderLock r(&ac, "ex_granulation_node");
            granulation_node->setGrainSource(r, grain_source);

            //AudioStreamConfig outputConfig = { -1, 2, ac.sampleRate() };
            //recorder = std::make_shared<RecorderNode>(ac, outputConfig);
            //ac.addAutomaticPullNode(recorder);
            //recorder->startRecording();
        }

        ac.connect(gain, granulation_node, 0, 0);
        _root_node = gain;
        //ac.connect(recorder, gain, 0, 0);
        //recorder->stopRecording();
        //ac.removeAutomaticPullNode(recorder);
        //recorder->writeRecordingToWav("ex_granulation_node.wav", false);
    }

    virtual void play() override final
    {
        connect();
        granulation_node->start(0.0f);
    }
};

////////////////////////
//    ex_poly_blep    //
////////////////////////

struct ex_poly_blep : public labsound_example
{
    std::shared_ptr<PolyBLEPNode> polyBlep;
    std::shared_ptr<GainNode> gain;
    std::vector<PolyBLEPType> blepWaveforms =
    {
        PolyBLEPType::TRIANGLE,
        PolyBLEPType::SQUARE,
        PolyBLEPType::RECTANGLE,
        PolyBLEPType::SAWTOOTH,
        PolyBLEPType::RAMP,
        PolyBLEPType::MODIFIED_TRIANGLE,
        PolyBLEPType::MODIFIED_SQUARE,
        PolyBLEPType::HALF_WAVE_RECTIFIED_SINE,
        PolyBLEPType::FULL_WAVE_RECTIFIED_SINE,
        PolyBLEPType::TRIANGULAR_PULSE,
        PolyBLEPType::TRAPEZOID_FIXED,
        PolyBLEPType::TRAPEZOID_VARIABLE
    };

    std::chrono::steady_clock::time_point prev;
    int waveformIndex = 0;

    virtual char const* const name() const override { return "Poly BLEP"; }

    explicit ex_poly_blep(Demo& demo) : labsound_example(demo)
    {
        auto& ac = *_demo->ac;
        polyBlep = std::make_shared<PolyBLEPNode>(ac);
        gain = std::make_shared<GainNode>(ac);

        gain->gain()->setValue(1.0f);
        ac.connect(gain, polyBlep, 0, 0);
        _root_node = gain;

        polyBlep->frequency()->setValue(220.f);
        polyBlep->setType(PolyBLEPType::TRIANGLE);
        polyBlep->start(0.0f);
    }

    virtual void play() override final
    {
        connect();
        prev = std::chrono::steady_clock::now();
    }

    virtual void update() override
    {
        if (!_root_node || !_root_node->output(0)->isConnected())
            return;

        const uint32_t delay_time_ms = 500;
        auto now = std::chrono::steady_clock::now();
        if (now - prev < std::chrono::milliseconds(delay_time_ms))
            return;

        prev = now;

        auto waveform = blepWaveforms[waveformIndex % blepWaveforms.size()];
        polyBlep->setType(waveform);
        waveformIndex++;
    }
};


std::shared_ptr<ex_simple> simple;
std::shared_ptr<ex_sfxr> sfxr;
std::shared_ptr<ex_osc_pop> osc_pop;
std::shared_ptr<ex_playback_events> playback_events;
std::shared_ptr<ex_offline_rendering> offline_rendering;
std::shared_ptr<ex_tremolo> tremolo;
std::shared_ptr<ex_frequency_modulation> frequency_mod;
std::shared_ptr<ex_runtime_graph_update> runtime_graph_update;
std::shared_ptr<ex_microphone_loopback> microphone_loopback;
std::shared_ptr<ex_microphone_reverb> microphone_reverb;
std::shared_ptr<ex_peak_compressor> peak_compressor;
std::shared_ptr<ex_stereo_panning> stereo_panning;
std::shared_ptr<ex_hrtf_spatialization> hrtf_spatialization;
std::shared_ptr<ex_convolution_reverb> convolution_reverb;
std::shared_ptr<ex_misc> misc;
std::shared_ptr<ex_dalek_filter> dalek_filter;
std::shared_ptr<ex_redalert_synthesis> redalert_synthesis;
std::shared_ptr<ex_wavepot_dsp> wavepot_dsp;
std::shared_ptr<ex_granulation_node> granulation;
std::shared_ptr<ex_poly_blep> poly_blep;

std::vector<std::shared_ptr<labsound_example>> examples;

std::shared_ptr<labsound_example> example_ui;



void instantiate_demos(Demo& demo)
{
    simple = std::make_shared<ex_simple>(demo);
    sfxr = std::make_shared<ex_sfxr>(demo);
    osc_pop = std::make_shared<ex_osc_pop>(demo);
    playback_events = std::make_shared<ex_playback_events>(demo);
    offline_rendering = std::make_shared<ex_offline_rendering>(demo);
    tremolo = std::make_shared<ex_tremolo>(demo);
    frequency_mod = std::make_shared<ex_frequency_modulation>(demo);
    runtime_graph_update = std::make_shared<ex_runtime_graph_update>(demo);
    microphone_loopback = std::make_shared<ex_microphone_loopback>(demo);
    microphone_reverb = std::make_shared<ex_microphone_reverb>(demo);
    peak_compressor = std::make_shared<ex_peak_compressor>(demo);
    stereo_panning = std::make_shared<ex_stereo_panning>(demo);
    hrtf_spatialization = std::make_shared<ex_hrtf_spatialization>(demo);
    convolution_reverb = std::make_shared<ex_convolution_reverb>(demo);
    misc = std::make_shared<ex_misc>(demo);
    dalek_filter = std::make_shared<ex_dalek_filter>(demo);
    redalert_synthesis = std::make_shared<ex_redalert_synthesis>(demo);
    wavepot_dsp = std::make_shared<ex_wavepot_dsp>(demo);
    granulation = std::make_shared<ex_granulation_node>(demo);
    poly_blep = std::make_shared<ex_poly_blep>(demo);

    examples.push_back(simple);
    examples.push_back(sfxr);
    examples.push_back(osc_pop);
    examples.push_back(playback_events);
    examples.push_back(offline_rendering);
    examples.push_back(tremolo);
    examples.push_back(frequency_mod);
    examples.push_back(runtime_graph_update);
    examples.push_back(microphone_loopback);
    examples.push_back(microphone_reverb);
    examples.push_back(peak_compressor);
    examples.push_back(stereo_panning);
    examples.push_back(hrtf_spatialization);
    examples.push_back(convolution_reverb);
    examples.push_back(misc);
    examples.push_back(dalek_filter);
    examples.push_back(redalert_synthesis);
    examples.push_back(wavepot_dsp);
    examples.push_back(granulation);
    examples.push_back(poly_blep);
}

void run_demo_ui(Demo& demo)
{
    auto& ac = *demo.ac;
    for (auto& i : examples)
    {
        i->update();
    }

    ImGui::Columns(2);

    for (auto& i : examples)
    {
        if (ImGui::Button(i->name()))
        {
            if (example_ui)
                example_ui->disconnect();

            example_ui = i;
            i->play();
            traverse_ui(ac);
        }
    }

    ImGui::NextColumn();

    if (example_ui)
        example_ui->ui();

    ImGui::Separator();

    if (ImGui::Button("Flush debug data"))
    {
        ac.flushDebugBuffer("C:\\Projects\\foo.wav");
    }

    if (example_ui && ImGui::Button("Disconnect demo"))
    {
        example_ui.reset();
        for (auto& i : examples)
        {
            i->disconnect();
        }

        ac.synchronizeConnections();
        traverse_ui(ac);
    }

    ImVec2 pos = ImGui::GetCursorPos();
    float y = 0;
    for (auto& i : displayNodes)
    {
        ImVec2 p = pos;
        p.x += i.x * 5;
        p.y += y;
        y += 15;
        ImGui::SetCursorPos(p);
        ImGui::Button(i.name.c_str());
    }
}

void run_context_ui(Demo& demo)
{
    static std::vector<std::string> inputs;
    static std::vector<std::string> outputs;
    static std::vector<int> input_reindex;
    static std::vector<int> output_reindex;

    static int input = 0;
    static int output = 0;
    static bool* input_checks;  // nb: std::vector<bool> is a ... contraption. Not a vector of bools per se
    static bool* output_checks;

    static std::vector<AudioDeviceInfo> info;
    if (info.size() == 0)
    {
        info = lab::SoundProvider::instance()->DeviceInfo();
        int reindex = 0;
        for (auto& i : info)
        {
            if (i.num_input_channels > 0)
            {
                inputs.push_back(i.identifier);
                input_reindex.push_back(reindex);
            }
            if (i.num_output_channels > 0)
            {
                outputs.push_back(i.identifier);
                output_reindex.push_back(reindex);
            }
            ++reindex;
        }

        input_checks = reinterpret_cast<bool*>(malloc(sizeof(bool) * inputs.size()));
        output_checks = reinterpret_cast<bool*>(malloc(sizeof(bool) * outputs.size()));

        for (auto& i : info)
        {
            if (i.num_input_channels > 0)
            {
                input_checks[inputs.size()] = i.is_default_input;
            }
            if (i.num_output_channels > 0)
            {
                output_checks[outputs.size()] = i.is_default_output;
            }
        }
    }

    ImGui::BeginChild("Devices", ImVec2{ 0, 100 });
    ImGui::Columns(2);
    ImGui::TextUnformatted("Inputs");
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4);
    for (int j = 0; j < inputs.size(); ++j)
        if (ImGui::Checkbox(inputs[j].c_str(), &input_checks[j]))
        {
            for (int i = 0; i < inputs.size(); ++i)
                if (i != j)
                    input_checks[i] = false;
        }

    ImGui::NextColumn();
    ImGui::TextUnformatted("Outputs");
    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 4);
    for (int j = 0; j < outputs.size(); ++j)
        if (ImGui::Checkbox(outputs[j].c_str(), &output_checks[j]))
        {
            for (int i = 0; i < outputs.size(); ++i)
                if (i != j)
                    output_checks[i] = false;
        }
    ImGui::EndChild();
    if (ImGui::Button("Create Context"))
    {
        AudioStreamConfig inputConfig;
        for (int i = 0; i < inputs.size(); ++i)
            if (input_checks[i])
            {
                int r = input_reindex[i];
                inputConfig.device_index = r;
                inputConfig.desired_channels = info[r].num_input_channels;
                inputConfig.desired_samplerate = info[r].nominal_samplerate;
                break;
            }
        AudioStreamConfig outputConfig;
        for (int i = 0; i < outputs.size(); ++i)
            if (output_checks[i])
            {
                int r = output_reindex[i];
                outputConfig.device_index = r;
                outputConfig.desired_channels = info[r].num_output_channels;
                outputConfig.desired_samplerate = info[r].nominal_samplerate;
                break;
            }

        if (outputConfig.device_index >= 0)
        {
            demo.use_live = inputConfig.device_index >= 0;
            auto& ac = *demo.ac;
            demo.recorder = std::make_shared<RecorderNode>(ac, outputConfig);
            ac.connect(ac.destinationNode(), demo.recorder);
            ac.synchronizeConnections();
            instantiate_demos(demo);
        }
    }
}

Demo* demo = nullptr;


namespace lab {


struct SoundActivity::data {
};

SoundActivity::SoundActivity() 
: Activity(SoundActivity::sname())
, _self(new data) {
    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<SoundActivity*>(instance)->RunUI(*vi);
    };
}

SoundActivity::~SoundActivity() {
    delete _self;
}


void SoundActivity::_activate() {
}
void SoundActivity::_deactivate() {
}

void SoundActivity::RunUI(const LabViewInteraction&) {
    if (!demo) {
        demo = new Demo();
        demo->ac = SoundProvider::instance()->Context();
    }

    ImGuiIO& io = ImGui::GetIO();
    ImGui::Begin("LabSound Demo");

    if (demo->ac)
        run_demo_ui(*demo);
    else
        run_context_ui(*demo);

    ImGui::End();
}

} // namespace lab
