// ImGuiTexInspect, a texture inspector widget for dear imgui

#pragma once
#include "imgui.h"

namespace ImGuiTexInspect
{
struct Context;
struct Transform2D;

struct Texture
{
    ImTextureID texture; // a bindable texture handle
    ImVec2 size;
};

//-------------------------------------------------------------------------
// [SECTION] INIT & SHUTDOWN
//-------------------------------------------------------------------------
void Init();
void Shutdown();

Context *CreateContext();
void DestroyContext(Context *);
void SetCurrentContext(Context *);

//-------------------------------------------------------------------------
// [SECTION] BASIC USAGE
//-------------------------------------------------------------------------

enum InspectorAlphaMode
{
    InspectorAlphaMode_ImGui,      // Alpha is transparency so you see the ImGui panel background behind image
    InspectorAlphaMode_Black,      // Alpha is used to blend over a black background
    InspectorAlphaMode_White,      // Alpha is used to blend over a white background
    InspectorAlphaMode_CustomColor // Alpha is used to blend over a custom colour.
};

typedef ImU64 InspectorFlags;
enum InspectorFlags_
{
    InspectorFlags_ShowWrap             = 1 << 0,  // Draw beyong the [0,1] uv range. What you see will depend on API
    InspectorFlags_NoForceFilterNearest = 1 << 1,  // Normally we force nearest neighbour sampling when zoomed in. Set to disable this.
    InspectorFlags_NoGrid               = 1 << 2,  // By default a grid is shown at high zoom levels
    InspectorFlags_NoTooltip            = 1 << 3,  // Disable tooltip on hover
    InspectorFlags_FillHorizontal       = 1 << 4,  // Scale to fill available space horizontally
    InspectorFlags_FillVertical         = 1 << 5,  // Scale to fill available space vertically
    InspectorFlags_NoAutoReadTexture    = 1 << 6,  // By default texture data is read to CPU every frame for tooltip and annotations
    InspectorFlags_FlipX                = 1 << 7,  // Horizontally flip the way the texture is displayed
    InspectorFlags_FlipY                = 1 << 8,  // Vertically flip the way the texture is displayed
};

/* Use one of these Size structs if you want to specify an exact size for the inspector panel. 
 * E.g.
 * BeginInspectorPanel("MyPanel", texture_1K, ImVec2(1024,1024), 0, SizeExcludingBorder(ImVec2(1024,1024)));
 *
 * However, most of the time the default size will be fine. E.g.
 *
 * BeginInspectorPanel("MyPanel", texture_1K, ImVec2(1024,1024));
 */
struct SizeIncludingBorder {ImVec2 Size; SizeIncludingBorder(ImVec2 size):Size(size){}};
struct SizeExcludingBorder {ImVec2 size; SizeExcludingBorder(ImVec2 size):size(size){}};
/* BeginInspectorPanel
 * Returns true if panel is drawn.  Note that flags will only be considered on the first call */
bool BeginInspectorPanel(const char *name, ImTextureID, ImVec2 textureSize, InspectorFlags flags = 0);
bool BeginInspectorPanel(const char *name, ImTextureID, ImVec2 textureSize, InspectorFlags flags, SizeIncludingBorder size);
bool BeginInspectorPanel(const char *name, ImTextureID, ImVec2 textureSize, InspectorFlags flags, SizeExcludingBorder size);

/* EndInspectorPanel 
 * Always call after BeginInspectorPanel and after you have drawn any required annotations*/
void EndInspectorPanel();

/* ReleaseInspectorData
 * ImGuiTexInspect keeps texture data cached in memory.  If you know you won't 
 * be displaying a particular panel for a while you can call this to release 
 * the memory. It won't be allocated again until next time you call 
 * BeginInspectorPanel.  If id is NULL then the current (most recent) inspector 
 * will be affected.  Unless you have a lot of different Inspector instances 
 * you can probably not worry about this. Call CurrentInspector_GetID to get 
 * the ID of an inspector. 
 */
void ReleaseInspectorData(ImGuiID id);

//-------------------------------------------------------------------------
// [SECTION] CURRENT INSPECTOR MANIPULATORS
//-------------------------------------------------------------------------
/* All the functions starting with CurrentInspector_ can be used after calling 
 * BeginInspector until the end of the frame.  It is not necessary to call them 
 * before the matching EndInspectorPanel
 */

/* CurrentInspector_SetColorMatrix
 * colorMatrix describes the transform which happens to the
 * color of each texel.
 * The calculation is finalColor = colorMatrix * originalColor.
 * Where finalColor, originalColor and colorOffset are column vectors with
 * components (r,g,b,a) and colorMatrix is a column-major matrix.
 */
void CurrentInspector_SetColorMatrix(const float (&colorMatrix)[16]);
void CurrentInspector_ResetColorMatrix();
void CurrentInspector_SetOutputGamma(float gamma);
void CurrentInspector_SetInputGamma(float gamma);

/* CurrentInspector_SetAlphaMode - see enum comments for details*/
void CurrentInspector_SetAlphaMode(InspectorAlphaMode);  
void CurrentInspector_SetFlags(InspectorFlags toSet, InspectorFlags toClear = 0);
inline void CurrentInspector_ClearFlags(InspectorFlags toClear) {CurrentInspector_SetFlags(0, toClear);}
void CurrentInspector_SetGridColor(ImU32 color);
void CurrentInspector_SetMaxAnnotations(int maxAnnotations);

/* CurrentInspector_InvalidateTextureCache
 * If using the InspectorFlags_NoAutoReadTexture flag then call this to 
 * indicate your texture has changed context.
 */
void CurrentInspector_InvalidateTextureCache();                 

/* CurrentInspector_SetCustomBackgroundColor
 * If using InspectorAlphaMode_CustomColor then this is the color that will be 
 * blended as the background where alpha is less than one.
 */
void CurrentInspector_SetCustomBackgroundColor(ImVec4 color);
void CurrentInspector_SetCustomBackgroundColor(ImU32 color);

/* CurrentInspector_GetID
 * Get the ID of the current inspector.  Currently only used for calling
 * ReleaseInspectorData. 
 */
ImGuiID CurrentInspector_GetID();

/* Some convenience functions for drawing ImGui controls for the current Inspector */
void DrawColorMatrixEditor();    // ColorMatrix editor.  See comments on ColorMatrix below.
void DrawGridEditor();           // Grid editor.  Enable/Disable grid. Set Grid Color.
void DrawColorChannelSelector(); // For toggling R,G,B channels
void DrawAlphaModeSelector();    // A combo box for selecting the alpha mode

//-------------------------------------------------------------------------
// [SECTION] CONTEXT-WIDE SETTINGS
//-------------------------------------------------------------------------
/* SetZoomRate
 * factor should be greater than 1.  A value of 1.5 means one mouse wheel 
 * scroll will increase zoom level by 50%. The factor used for zooming out is 
 * 1/factor. */
void SetZoomRate(float factor); 
                                
//-------------------------------------------------------------------------
// [SECTION] ANNOTATION TOOLS
//-------------------------------------------------------------------------

/* DrawAnnotationLine
 * Convenience function to add a line to draw list using texel coordinates. 
 */
void DrawAnnotationLine(ImDrawList *drawList, ImVec2 fromTexel, ImVec2 toTexel, Transform2D texelsToPixels, ImU32 color);

//-------------------------------------------------------------------------
// [SECTION] Annotation Classes
//-------------------------------------------------------------------------

/* To draw annotations call DrawAnnotions in between BeginInspectorPanel and 
 * EndInspectorPanel.  Example usage:
 * DrawAnnotations(ValueText(ValueText::HexString));
 * 
 * To provide your own Annotation drawing class just define a class that 
 * implements the DrawAnnotation method.  See imgui_tex_inspect_demo.cpp
 * for an example.
 */
template <typename T>
void DrawAnnotations(T drawer, ImU64 maxAnnotatedTexels = 0);

/* ValueText
 * An annoation class that draws text inside each texel when zoom level is high enough for it to fit.
 * The text shows the value of the texel. E.g. "R:255, G: 128, B:0, A:255"
 */
class ValueText
{
  protected:
    int TextRowCount;
    int TextColumnCount;
    const char *TextFormatString;
    bool FormatAsFloats;

  public:
    enum Format
    {
        HexString, // E.g.  #EF97B9FF
        BytesHex,  // E.g.  R:#EF G:#97 B:#B9 A:#FF  (split over 4 lines)
        BytesDec,  // E.g.  R:239 G: 151 B:185 A:255  (split over 4 lines)
        Floats     // E.g.  0.937 0.592 0.725 1.000 (split over 4 lines)
    };
    ValueText(Format format = HexString);
    void DrawAnnotation(ImDrawList *drawList, ImVec2 texel, Transform2D texelsToPixels, ImVec4 value);
};

/* Arrow 
 * An annotation class that draws an arrow inside each texel when zoom level is 
 * high enough. The direction and length of the arrow are determined by texel 
 * values.
 * The X and Y components of the arrow is determined by the VectorIndex_x, and 
 * VectorIndex_y channels of the texel value.  Examples:

 * VectorIndex_x = 0,  VectorIndex_y = 1  means  X component is red and Y component is green
 * VectorIndex_x = 1,  VectorIndex_y = 2  means  X component is green and Y component is blue
 * VectorIndex_x = 0,  VectorIndex_y = 3  means  X component is red and Y component is alpha
 *
 * ZeroPoint is the texel value which corresponds to a zero length vector. E.g. 
 * ZeroPoint = (0.5, 0.5) means (0.5, 0.5) will be drawn as a zero length arrow
 *
 * All public properties can be directly manipulated.  There are also presets that can be set
 * by calling UsePreset.

 */
class Arrow
{
  public:
    int VectorIndex_x;
    int VectorIndex_y;
    ImVec2 LineScale;
    ImVec2 ZeroPoint = {0, 0}; 
                              
    enum Preset
    {
        NormalMap,      // For normal maps. I.e. Arrow is in (R,G) channels.  128, 128 is zero point
        NormalizedFloat // Arrow in (R,G) channels. 0,0 is zero point, (1,0) will draw an arrow exactly to
                        // right edge of texture. (0,-1) will draw exactly to the bottom etc.
    };
    Arrow(int xVectorIndex = 0, int yVectorIndex = 1, ImVec2 lineScale = ImVec2(1, 1));
    Arrow &UsePreset(Preset);
    void DrawAnnotation(ImDrawList *drawList, ImVec2 texel, Transform2D texelsToPixels, ImVec4 value);
};

//-------------------------------------------------------------------------
// [SECTION] INTERNAL
//-------------------------------------------------------------------------

struct Transform2D
{
    ImVec2 Scale;
    ImVec2 Translate;

    /* Transform a vector by this transform.  Scale is applied first */
    ImVec2 operator*(const ImVec2 &rhs) const
    {
        return ImVec2(Scale.x * rhs.x + Translate.x, Scale.y * rhs.y + Translate.y);
    }

    /* Return an inverse transform such that transform.Inverse() * transform * vector == vector*/
    Transform2D Inverse() const
    {
        ImVec2 inverseScale(1 / Scale.x, 1 / Scale.y);
        return {inverseScale, ImVec2(-inverseScale.x * Translate.x, -inverseScale.y * Translate.y)};
    }
};

struct BufferDesc
{
    float         *Data_float     = nullptr; // Only one of these 
    ImU8          *Data_uint8_t   = nullptr; // three pointers should be non NULL
    uint16_t      *Data_half      = nullptr;
    size_t         BufferByteSize = 0; // Size of buffer pointed to by one of above pointers
    int            Stride         = 0; // Measured in size of data type, not bytes!
    int            LineStride     = 0; // Measured in size of data type, not bytes!
    int            StartX         = 0; // Texel coordinates of data start
    int            StartY         = 0;
    int            Width          = 0; // Size of block of texels which are in data
    int            Height         = 0;

    unsigned char  ChannelCount   = 0; // Number of color channels in data. E.g. 2 means just red and green

    /* These 4 values describe where each color is stored relative to the beginning of the texel in memory 
     * E.g. the float containing the red value would be at:
     * Data_float[texelIndex + bufferDesc.Red]
     */
    unsigned char  Red            = 0;
    unsigned char  Green          = 0;
    unsigned char  Blue           = 0;
    unsigned char  Alpha          = 0;
};

/* We use this struct for annotations rather than the Inspector struct so that
 * the whole Inspector struct doesn't have to be exposed in this header.
 */
struct AnnotationsDesc
{
    ImDrawList  *DrawList;
    ImVec2       TexelViewSize;  // How many texels are visible for annotating
    ImVec2       TexelTopLeft;   // Coordinated in texture space of top left visible texel
    BufferDesc   Buffer;         // Description of cache texel data
    Transform2D  TexelsToPixels; // Transform to go from texel space to screen pixel space
};

//-------------------------------------------------------------------------
// [SECTION] FORWARD DECLARATIONS FOR TEMPLATE IMPLEMENTATION - Do not call directly
//-------------------------------------------------------------------------

ImVec4 GetTexel(const BufferDesc *bd, int x, int y);
bool GetAnnotationDesc(AnnotationsDesc *, ImU64 maxAnnotatedTexels);

//-------------------------------------------------------------------------
// [SECTION] TEMPLATE IMPLEMENTATION
//-------------------------------------------------------------------------
template <typename T>
void DrawAnnotations(T drawer, ImU64 maxAnnotatedTexels)
{
    AnnotationsDesc ad;
    if (GetAnnotationDesc(&ad, maxAnnotatedTexels))
    {
        ImVec2 texelBottomRight = ImVec2(ad.TexelTopLeft.x + ad.TexelViewSize.x, ad.TexelTopLeft.y + ad.TexelViewSize.y);
        for (int ty = (int)ad.TexelTopLeft.y; ty < (int)texelBottomRight.y; ++ty)
        {
            for (int tx = (int)ad.TexelTopLeft.x; tx < (int)texelBottomRight.x; ++tx)
            {
                ImVec4 color = GetTexel(&ad.Buffer, tx, ty);
                ImVec2 center = {(float)tx + 0.5f, (float)ty + 0.5f};
                drawer.DrawAnnotation(ad.DrawList, center, ad.TexelsToPixels, color);
            }
        }
    }
}
} // namespace ImGuiTexInspect

// ImGuiTexInspect, a texture inspector widget for dear imgui

#pragma once
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_internal.h"
#include "imgui_tex_inspect.h"

#include "Providers/Color/nanocolor.h"

namespace ImGuiTexInspect
{
//-------------------------------------------------------------------------
// [SECTION] UTILITIES
//-------------------------------------------------------------------------
// Returns true if a flag is set
template <typename TSet, typename TFlag>
static inline bool HasFlag(TSet set, TFlag flag)
{
    return (set & flag) == flag;
}

// Set flag or flags in set
template <typename TSet, typename TFlag>
static inline void SetFlag(TSet &set, TFlag flags)
{
    set = static_cast<TSet>(set | flags);
}

// Clear flag or flags in set
template <typename TSet, typename TFlag>
static inline void ClearFlag(TSet &set, TFlag flag)
{
    set = static_cast<TSet>(set & ~flag);
}

// Defined in recent versions of imgui_internal.h.  Included here in case user is on older
// imgui version.
static inline float ImFloorSigned(float f)
{
    return (float)((f >= 0 || (int)f == f) ? (int)f : (int)f - 1);
}

// Proper modulus operator, as opposed to remainder as calculated by %
template <typename T>
static inline T Modulus(T a, T b)
{
    return a - b * ImFloorSigned(a / b);
}

static inline float Round(float f)
{
    return ImFloorSigned(f + 0.5f);
}

static inline ImVec2 Abs(ImVec2 v)
{
    return ImVec2(ImAbs(v.x), ImAbs(v.y));
}

//-------------------------------------------------------------------------
// [SECTION] STRUCTURES
//-------------------------------------------------------------------------
struct ShaderOptions
{
    ShaderOptions();

    NcColorSpaceM33Descriptor InputColorspace = {};
    float inputK0 = 0, inputPhi = 0;
    NcColorSpaceM33Descriptor OutputColorspace = {};
    float outputK0 = 0, outputPhi = 0;

    float  ColorTransform[16] = {};   // See CurrentInspector_SetColorMatrix for details

    ImVec4 BackgroundColor       = {0,0,0,0}; // Color used for alpha blending
    float  PremultiplyAlpha      = 0;         // If 1 then color will be multiplied by alpha in shader, before blend stage
    float  DisableFinalAlpha     = 0;         // If 1 then fragment shader will always output alpha = 1

    bool   ForceNearestSampling  = false;     // If true fragment shader will always sample from texel centers

    ImVec2 GridWidth             = {0,0};     // Width in UV coords of grid line
    ImVec4 GridColor             = {0,0,0,0};

    float exposure = 1.f; // pow(2.0, exposure) will be passed to the shader
    float lift = 0.f;

    void  ResetColorTransform();
};

struct Inspector
{
    ImGuiID ID;
    bool Initialized = false;

    // Texture
    ImTextureID Texture = ImTextureID{};
    ImVec2 TextureSize = {0, 0};        // Size in texels of texture
    float PixelAspectRatio = 1;         // Values other than 1 not supported yet

    // View State
    bool IsDragging = false;            // Is user currently dragging to pan view
    ImVec2 PanPos = {0.5f, 0.5f};       // The UV value at the center of the current view
    ImVec2 Scale = {1, 1};              // 1 pixel is 1 texel

    ImVec2 PanelTopLeftPixel = {0, 0};  // Top left of view in ImGui pixel coordinates
    ImVec2 PanelSize = {0, 0};          // Size of area allocated to drawing the image in pixels.

    ImVec2 ViewTopLeftPixel = {0, 0};   // Position in ImGui pixel coordinates
    ImVec2 ViewSize = {0, 0};           // Rendered size of current image. This could be smaller than panel size if user has zoomed out.
    ImVec2 ViewSizeUV = {0, 0};         // Visible region of the texture in UV coordinates

    /* Conversion transforms to go back and forth between screen pixels  (what ImGui considers screen pixels) and texels*/
    Transform2D TexelsToPixels;
    Transform2D PixelsToTexels;

    // Cached pixel data
    bool HaveCurrentTexelData = false;
    BufferDesc Buffer;

    /* We don't actually access texel data through this pointer.  We just
     * manage its lifetime. The backend might have asked us to allocated a
     * buffer, or it might not.  The pointer we actually use to access texel
     * data is in the Buffer object above (which depending on what the backend
     * did might point to the same memory as this pointer)
     */
    ImU8 *DataBuffer = nullptr;
    size_t DataBufferSize = 0;

    // Configuration
    InspectorFlags Flags = 0;

    // Background mode
    InspectorAlphaMode AlphaMode = InspectorAlphaMode_ImGui;
    ImVec4 CustomBackgroundColor = {0, 0, 0, 1};

    // Scaling limits
    ImVec2 ScaleMin = {0.02f, 0.02f};
    ImVec2 ScaleMax = {500, 500};

    // Grid
    float MinimumGridSize = 4; // Don't draw the grid if lines would be closer than MinimumGridSize pixels

    // Annotations
    ImU32 MaxAnnotatedTexels = 0;

    // Color transformation
    ShaderOptions ActiveShaderOptions;
    ShaderOptions CachedShaderOptions;

    ~Inspector();
};

//-------------------------------------------------------------------------
// [SECTION] INTERNAL FUNCTIONS
//-------------------------------------------------------------------------

Inspector *GetByKey(const Context *ctx, ImGuiID key);
Inspector *GetOrAddByKey(Context *ctx, ImGuiID key);

void SetPanPos(Inspector *inspector, ImVec2 pos);
void SetScale(Inspector *inspector, ImVec2 scale);
void SetScale(Inspector *inspector, float scaleY);
void RoundPanPos(Inspector *inspector);

ImU8 *GetBuffer(Inspector *inspector, size_t bytes);

/* GetTexelsToPixels
 * Calculate a transform to convert from texel coordinates to screen pixel coordinates
 * */
Transform2D GetTexelsToPixels(ImVec2 screenTopLeft, ImVec2 screenViewSize, ImVec2 uvTopLeft, ImVec2 uvViewSize, ImVec2 textureSize);

//-------------------------------------------------------------------------
// [SECTION] IMGUI UTILS
//-------------------------------------------------------------------------
/* TextVector
 * Draws a single-column ImGui table with one row for each provided string
 */
void TextVector(const char *title, const char *const *strings, int n);

/* PushDisabled & PopDisabled
 * Push and Pop and ImGui styles that disable and "grey out" ImGui elements
 * by making them non interactive and transparent*/
void PushDisabled();
void PopDisabled();

//-------------------------------------------------------------------------
// [SECTION] BACKEND FUNCTIONS
//-------------------------------------------------------------------------
void BackEnd_SetShader(const ImDrawList *drawList, const ImDrawCmd *cmd, const Inspector *inspector);
bool BackEnd_GetData(Inspector *inspector, ImTextureID texture, int x, int y, int width, int height, BufferDesc *buffer);

} // namespace ImGuiTexInspect
