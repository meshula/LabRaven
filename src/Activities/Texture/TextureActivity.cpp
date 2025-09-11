//
//  TextureActivity.cpp
//  LabExcelsior
//
//  Created by Nick Porcino on 12/7/23.
//  Copyright © 2023 Nick Porcino. All rights reserved.
//

#include "TextureActivity.hpp"
#include "Lab/App.h"
#include "Lab/CSP.hpp"
#include "Lab/LabDirectories.h"
#include "Lab/LabFileDialogManager.hpp"
#include "Lab/CoreProviders/Color/ColorProvider.hpp"
#include "Lab/CoreProviders/Color/nanocolorUtils.h"
#include "Lab/CoreProviders/Texture/TextureCache.hpp"
#include "Lab/ThreePanelLayout.h"

#ifndef IMGUI_DEFINE_MATH_OPERATORS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif
#include "imgui.h"
#include "imgui_tex_inspect/imgui_tex_inspect.h"

#include <map>
#include <mutex>
#include <string>


using std::map;
using std::string;
using std::vector;

// These are hardware specific functions that must be supplied by the application.
extern void* LabGetEncodedTexture(int texture);
extern void LabReleaseEncodedTexture(void* texture);

// Zoom slider helper with hover/active thumb scaling
float DrawZoomSlider(const char* id, float zoom, float minZoom, float maxZoom, float height)
{
    ImGuiWindow* window = ImGui::GetCurrentWindow();
    if (window->SkipItems) return zoom;

    const float lineWidth   = 2.0f;
    const float baseRadius  = 6.0f;
    const float hoverScale  = 1.3f; // thumb grows 30% when hovered
    const float activeScale = 1.6f; // thumb grows 60% when dragged

    ImVec2 pos = ImGui::GetCursorScreenPos();
    ImVec2 size(20.0f, height); // invisible interaction area
    ImRect bb(pos, pos + size);
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, ImGui::GetID(id))) return zoom;

    // Track line centered horizontally
    float centerX = pos.x + size.x * 0.5f;
    ImVec2 trackMin(centerX - lineWidth * 0.5f, pos.y);
    ImVec2 trackMax(centerX + lineWidth * 0.5f, pos.y + size.y);

    // Map zoom to slider position (inverse: bigger zoom at top)
    float t = (zoom - minZoom) / (maxZoom - minZoom);
    float thumbY = ImLerp(trackMax.y, trackMin.y, t);

    // Interaction
    bool hovered = false, held = false;
    ImGui::ButtonBehavior(bb, ImGui::GetID(id), &hovered, &held);

    // Animate thumb radius
    float scale = 1.0f;
    if (held)      scale = activeScale;
    else if (hovered) scale = hoverScale;

    float thumbRadius = baseRadius * scale;

    // Draw track
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(trackMin, trackMax, IM_COL32(150, 150, 150, 180), lineWidth);

    // Draw thumb (disc)
    ImVec2 thumbCenter(centerX, thumbY);
    drawList->AddCircleFilled(thumbCenter, thumbRadius, IM_COL32(220, 220, 220, 255));

    // Update value if dragging
    if (held)
    {
        float mouseY = ImGui::GetIO().MousePos.y;
        t = ImSaturate((trackMax.y - mouseY) / (trackMax.y - trackMin.y));
        zoom = minZoom + t * (maxZoom - minZoom);
    }

    return zoom;
}


namespace ImGuiTexInspect
{

namespace {
    std::map<int, LabImageData_t> loadedTextureMap;
    std::map<ImTextureID, int> loadedTextureMapReverse;
    std::map<std::string, Texture> hardwareTextures;
    vector<string> texture_names;
}

Texture LoadTexture(const char *path) {
    auto it = hardwareTextures.find(path);
    if (it != hardwareTextures.end()) {
        return it->second;
    }

    Texture ret = { 0, {32, 32} };
    lab::TextureCache* tc = lab::TextureCache::instance();

    std::shared_ptr<LabImageData_t> tex = tc->ReadAndCache(path);
    if (!tex)
        return ret;

    // fetch an encoded texture if possible.
    int i = tc->GetEncodedTexture(tex);
    if (i >= 0) {
        ret.texture = (ImTextureID) LabGetEncodedTexture(i);
        ret.size = {(float)tex->width, (float)tex->height};

        loadedTextureMap[i] = *tex;
        loadedTextureMapReverse[ret.texture] = i;
        hardwareTextures[path] = ret;

        texture_names.push_back(path);
    }
    return ret;
}

bool BackEnd_GetData(Inspector* inspector, ImTextureID texture,
                     int x, int y, int w, int h, BufferDesc* bufferDesc) {
    if (!bufferDesc)
        return false;

    auto rit = loadedTextureMapReverse.find(texture);
    if (rit == loadedTextureMapReverse.end())
        return false;

    int i = rit->second;
    auto it = loadedTextureMap.find(i);
    if (it == loadedTextureMap.end())
        return false;

    auto& tx = it->second;

    bufferDesc->BufferByteSize = tx.dataSize;
    bufferDesc->Red = 0;
    bufferDesc->Green = (ImU8)ImMin(1, tx.channelCount - 1);
    bufferDesc->Blue = (ImU8)ImMin(2, tx.channelCount - 1);
    bufferDesc->Alpha = (ImU8)ImMin(3, tx.channelCount - 1);
    bufferDesc->ChannelCount = tx.channelCount;
    bufferDesc->LineStride = tx.width * tx.channelCount;
    bufferDesc->Stride = tx.channelCount; // pixel to pixel stride
    bufferDesc->StartX = 0; // whole texture
    bufferDesc->StartY = 0;
    bufferDesc->Width = tx.width;
    bufferDesc->Height = tx.height;

    // inspector has to be upgraded for f16 data
    if (tx.pixelType == LAB_PIXEL_UINT8)
        bufferDesc->Data_uint8_t = tx.data;
    else if (tx.pixelType == LAB_PIXEL_FLOAT)
        bufferDesc->Data_float = reinterpret_cast<float*>(tx.data);
    else if (tx.pixelType == LAB_PIXEL_HALF)
        bufferDesc->Data_half = reinterpret_cast<uint16_t*>(tx.data);
    else {
        bufferDesc->Data_float = nullptr;
        bufferDesc->Data_uint8_t = nullptr;
    }

    return true;
}

} // ImGuiTexInspect


namespace lab {

class LoadTextureModule : public CSP_Module {
    CSP_Process LoadRequest, Loading, Error, LoadFile, Idle;
    int pendingFile = 0;
    FileDialogManager::FileReq req;
public:
    LoadTextureModule(CSP_Engine& engine)
    : CSP_Module(engine, "LoadTextureModule")
    , LoadRequest("LoadRequest",
                  [this]() {
                      printf("Entering file_load_request\n");
                      auto fdm = LabApp::instance()->fdm();
                      const char* dir = lab_pref_for_key("LoadTextureDir");
                      pendingFile = fdm->RequestOpenFile(
                          {"png","jpg","jpeg","tga","bmp","gif","hdr","exr","pfm"},
                          dir? dir: ".");
                      this->emit_event(Loading);
                  })
    , Loading("Loading",
              [this]() {
                  auto fdm = LabApp::instance()->fdm();
                  req = fdm->PopOpenedFile(pendingFile);
                  switch (req.status) {
                      case FileDialogManager::FileReq::notReady:
                          this->emit_event(Loading); // continue polling
                          break;
                      case FileDialogManager::FileReq::expired:
                      case FileDialogManager::FileReq::canceled:
                          this->emit_event(Error);
                          break;
                      default:
                          this->emit_event(LoadFile);
                          break;
                  }
              })
    , Error("Error",
            [this]() {
                printf("Entering file_error\n");
                std::cout << "Error: Failed to open file. Returning to Ready...\n";
                this->emit_event(Idle);
            })
    , LoadFile("LoadFile",
               [this]() {
                   printf("Entering file_load_success\n");
                   auto fdm = LabApp::instance()->fdm();
                   auto path = req.path.c_str();
                   auto tc = TextureCache::instance();
                   auto img = ImGuiTexInspect::LoadTexture(req.path.c_str());
                   if (img.texture >= 0) {
                       printf("Loaded texture %s\n", path);
                       // get the directory of path and save it as the default
                       std::string dir = req.path.substr(0, req.path.find_last_of("/\\"));
                       lab_set_pref_for_key("LoadTextureDir", dir.c_str());
                   }
                   else {
                       printf("Failed to load texture %s\n", path);
                   }
                   this->emit_event(Idle);
               })
    , Idle("Idle",
           []() {
              printf("Entering idle\n");
          })
    {
        add_process(LoadRequest);
        add_process(Loading);
        add_process(Error);
        add_process(LoadFile);
        add_process(Idle);
    }

    void LoadTexture() {
        emit_event(LoadRequest);
    }
};

struct TextureActivity::data {
    int utility_preset_index = 0;
    int cache_selected_texture_index = 0;
    int pending_file = -1; // file dialog manager id for in flight requests
    std::once_flag init;
    ImGuiTexInspect::Texture focussedTexture;
    LoadTextureModule loadTextureModule;

    data() : loadTextureModule(*lab::LabApp::instance()->csp()) {
        loadTextureModule.Register();
    }

    ~data() = default;
};

TextureActivity::TextureActivity()
: Activity(TextureActivity::sname()) {
    _self = new TextureActivity::data;

    activity.Menu = [](void* instance) {
        static_cast<TextureActivity*>(instance)->Menu();
    };
    activity.Update = [](void* instance, float) {
        static_cast<TextureActivity*>(instance)->Update();
    };
    activity.RunUI = [](void* instance, const LabViewInteraction* vi) {
        static_cast<TextureActivity*>(instance)->RunUI(*vi);
    };
}

TextureActivity::~TextureActivity() {
    delete _self;
}


namespace {
    bool listbox_getter(void* data, int n, const char** out_text) {
        const std::vector<std::string>* v = (std::vector<std::string>*)data;
        *out_text = (*v)[n].c_str();
        return true;
    }
}

void TextureActivity::RunTextureCachePanel() {
    /*  _            _                                   _            _ _     _
       | |_ _____  _| |_ _   _ _ __ ___    ___ __ _  ___| |__   ___  | (_)___| |_
       | __/ _ \ \/ / __| | | | '__/ _ \  / __/ _` |/ __| '_ \ / _ \ | | / __| __|
       | ||  __/>  <| |_| |_| | | |  __/ | (_| (_| | (__| | | |  __/ | | \__ \ |_
        \__\___/_/\_\\__|\__,_|_|  \___|  \___\__,_|\___|_| |_|\___| |_|_|___/\__| */

    ImVec2 windowSize = ImGui::GetWindowSize();

    if (_self->cache_selected_texture_index >= 0 && ImGuiTexInspect::texture_names.size() > 0) {
        if (ImGui::Button("Inspect")) {
            _self->focussedTexture =
            ImGuiTexInspect::LoadTexture(ImGuiTexInspect::texture_names[_self->cache_selected_texture_index].c_str());
        }
    }
    else if (ImGuiTexInspect::texture_names.size()) {
        if (ImGui::Button("Select a texture")) {
            _self->cache_selected_texture_index = 0;
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Load a texture")) {
        _self->loadTextureModule.LoadTexture();
    }

    windowSize.x = -FLT_MIN;
    windowSize.y = 0;
    if (ImGui::BeginListBox("###TextureCacheList", windowSize)) {
        for (size_t n = 0; n < ImGuiTexInspect::texture_names.size(); n++) {
            const bool is_selected = (_self->cache_selected_texture_index == n);
            if (ImGui::Selectable(ImGuiTexInspect::texture_names[n].c_str(), is_selected))
                _self->cache_selected_texture_index = (int) n;

            // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
            if (is_selected)
                ImGui::SetItemDefaultFocus();
        }
        ImGui::EndListBox();
    }

    /*
           _   _ _ _ _                                    _
     _   _| |_(_) (_) |_ _   _   _ __  _ __ ___  ___  ___| |_ ___
    | | | | __| | | | __| | | | | '_ \| '__/ _ \/ __|/ _ \ __/ __|
    | |_| | |_| | | | |_| |_| | | |_) | | |  __/\__ \  __/ |_\__ \
     \__,_|\__|_|_|_|\__|\__, | | .__/|_|  \___||___/\___|\__|___/
                         |___/  |_|                                */

    // Provide some presets that can be used to set the ColorMatrix for example purposes
    ImGui::PushItemWidth(200);
    static const char* utilities[] = {
        "Negative",
        "Swap Red & Blue",
        "Alpha",
        "Transparency",
        "Default"
    };
    if (ImGui::BeginCombo("Utility Presets##workingcombo", utilities[_self->utility_preset_index])) {
        if (ImGui::Selectable("Swap Red & Blue", _self->utility_preset_index == 1))
        {
            _self->utility_preset_index = 1;
            // Matrix which swaps red and blue channels but leaves green and alpha untouched
            float matrix[] = { 0.000f,  0.000f,  1.000f,  0.000f,
                               0.000f,  1.000f,  0.000f,  0.000f,
                               1.000f,  0.000f,  0.000f,  0.000f,
                               0.000f,  0.000f,  0.000f,  1.000f};
            ImGuiTexInspect::CurrentInspector_SetColorMatrix(matrix);
        }
        if (ImGui::Selectable("Default", _self->utility_preset_index == 4))
        {
            _self->utility_preset_index = 4;
            // Default "identity" matrix that doesn't modify colors at all
            float matrix[] = {1.000f, 0.000f, 0.000f, 0.000f,
                              0.000f, 1.000f, 0.000f, 0.000f,
                              0.000f, 0.000f, 1.000f, 0.000f,
                              0.000f, 0.000f, 0.000f, 1.000f};

            ImGuiTexInspect::CurrentInspector_SetColorMatrix(matrix);
        }
        ImGui::EndCombo();
    }

    ImGui::PopItemWidth();

    //-------------------------------------------------------------------------

    ImGui::PushItemWidth(200);

    ColorProvider* cp = ColorProvider::instance();
    const char* rcsStr = cp->RenderingColorSpace();
    std::string renderCS;
    if (rcsStr)
        renderCS.assign(rcsStr);

    /*               _
     _ __ ___   __ _| |_ _ __(_)_  __
    | '_ ` _ \ / _` | __| '__| \ \/ /
    | | | | | | (_| | |_| |  | |>  <
    |_| |_| |_|\__,_|\__|_|  |_/_/\_\ */

    ImGui::BeginGroup();
    ImGui::Text("Colour Matrix Editor:");
    // Draw Matrix editor to allow user to manipulate the ColorMatrix
    ImGuiTexInspect::DrawColorMatrixEditor();
    ImGui::EndGroup();

    ImGui::BeginGroup();

    const char** colorspaces = NcRegisteredColorSpaceNames();
    static int display_cs = 10;
    static std::string selectedRenderCS;
    if (renderCS != selectedRenderCS) {
        selectedRenderCS = renderCS;
        display_cs = 0;
        const char** names = colorspaces;
        while (*names != nullptr) {
            if (!strcmp(renderCS.c_str(), *names)) {
                break;
            }
            ++display_cs;
            ++names;
        }
    }

    /*         _
      ___ ___ | | ___  _ __   ___ _ __   __ _  ___ ___  ___
     / __/ _ \| |/ _ \| '__| / __| '_ \ / _` |/ __/ _ \/ __|
    | (_| (_) | | (_) | |    \__ \ |_) | (_| | (_|  __/\__ \
     \___\___/|_|\___/|_|    |___/ .__/ \__,_|\___\___||___/
                                 |_|                          */
    static int input_cs = 10; // rec709...
    ImGui::PushItemWidth(200);
    if (ImGui::BeginCombo("input##combo", colorspaces[input_cs]))
    {
        int n = 0;
        if (colorspaces[n] != nullptr) {
            do {
                bool is_selected = (input_cs == n);
                if (ImGui::Selectable(colorspaces[n], is_selected)) {
                    input_cs = n;
                    _self->utility_preset_index = 4;
                    auto dcs = NcGetNamedColorSpace(colorspaces[display_cs]);
                    auto ics = NcGetNamedColorSpace(colorspaces[input_cs]);
                    auto d2i = NcGetRGBToRGBMatrix(ics, dcs);
                    auto& m = d2i.m;
                    float matrix[16] = { m[0], m[3], m[6], 0,
                                         m[1], m[4], m[7], 0,
                                         m[2], m[5], m[8], 0,
                                         0,    0,    0,    1};

                    ImGuiTexInspect::CurrentInspector_SetColorMatrix(matrix);

                    NcColorSpaceM33Descriptor desc;
                    if (!NcGetColorSpaceM33Descriptor(ics, &desc))
                        desc.gamma = 1.0f;

                    ImGuiTexInspect::CurrentInspector_SetInputGamma(desc.gamma);
                }
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
                ++n;
            } while (colorspaces[n] != nullptr);
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    ImGui::PushItemWidth(200);
    if (ImGui::BeginCombo("display##workingcombo", colorspaces[display_cs]))
    {
        int n = 0;
        if (colorspaces[n] != nullptr) {
            do {
                bool is_selected = (display_cs == n);
                if (ImGui::Selectable(colorspaces[n], is_selected)) {
                    _self->utility_preset_index = 4;
                    display_cs = n;
                    auto dcs = NcGetNamedColorSpace(colorspaces[display_cs]);
                    auto ics = NcGetNamedColorSpace(colorspaces[input_cs]);
                    auto d2i = NcGetRGBToRGBMatrix(ics, dcs);
                    auto& m = d2i.m;
                    float matrix[16] = { m[0], m[3], m[6], 0,
                                         m[1], m[4], m[7], 0,
                                         m[2], m[5], m[8], 0,
                                         0,    0,    0,    1};

                    ImGuiTexInspect::CurrentInspector_SetColorMatrix(matrix);

                    NcColorSpaceM33Descriptor desc;
                    if (!NcGetColorSpaceM33Descriptor(dcs, &desc))
                        desc.gamma = 1.f;

                    ImGuiTexInspect::CurrentInspector_SetOutputGamma(desc.gamma);
                }
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
                ++n;
            } while (colorspaces[n] != nullptr);
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    ImGui::EndGroup();

    ImGui::PopItemWidth();
}

void TextureActivity::RunTextureInspectorPanel() {

    ImVec2 windowSize = ImGui::GetWindowSize();

    /*   _            _                                           _
        | |_ _____  _| |_ _   _ _ __ ___   _ __   __ _ _ __   ___| |
        | __/ _ \ \/ / __| | | | '__/ _ \ | '_ \ / _` | '_ \ / _ \ |
        | ||  __/>  <| |_| |_| | | |  __/ | |_) | (_| | | | |  __/ |
         \__\___/_/\_\\__|\__,_|_|  \___| | .__/ \__,_|_| |_|\___|_|
                                          |_|   */

    float h = std::max((int) 50, int(windowSize.y - 220));
    float w = windowSize.x - 16;
    if (w < 20)
        w = 20;
    h = windowSize.y - 20;
    if (h < 20)
        h = 20;
    ImVec2 panelSize = { w, h };
    
    if (ImGuiTexInspect::BeginInspectorPanel("##ColorMatrix",
                                             _self->focussedTexture.texture,
                                             _self->focussedTexture.size,
                                             ImGuiTexInspect::InspectorFlags_FillHorizontal |
                                                    ImGuiTexInspect::InspectorFlags_FillVertical,
                                             ImGuiTexInspect::SizeIncludingBorder(panelSize)
                                             ))
    {
        // Draw some text showing color value of each texel (you must be zoomed in to see this)
        ImGuiTexInspect::DrawAnnotations(ImGuiTexInspect::ValueText(
                                                ImGuiTexInspect::ValueText::BytesDec));
    }
    ImGuiTexInspect::EndInspectorPanel();
}

void TextureActivity::RunUI(const LabViewInteraction&)
{
    ImGui::Begin("Texture Activity");
    
    static ThreePanelLayoutState layoutState;
    std::call_once(_self->init, [this]() {
        // initialize here because the initialization can't be called before
        // imgui is actually up and running
        ImGuiTexInspect::Init();
        ImGuiTexInspect::CreateContext();
        layoutState.leftPanelTitle = "Spectral Locus";
        layoutState.middlePanelTitle = "Tone Mapping";
        layoutState.rightPanelTitle = "Navigation";
    });
    // ----------------------------------------------------
    // Top panel: fixed height child window
    // ----------------------------------------------------
    static float top_height = 300.0f;   // initial height
    const float splitter_size = 6.0f;
    const float min_height    = 100.0f;
    const float avail_height  = ImGui::GetContentRegionAvail().y;
    const float max_height    = avail_height - min_height;
    
    ImGui::BeginChild("###TopRegion_TA", ImVec2(0, top_height), true);
    // create a sub-window for the texture inspector, leaving space for the zoom slider on the right
    ImGui::BeginChild("###TextureInspectorSubWindow", ImVec2(-40, 0), false);
    RunTextureInspectorPanel();
    // end the child and make one for the zoom slider
    ImGui::EndChild();
    ImGui::SameLine();
    ImGui::BeginChild("###ZoomSliderSubWindow", ImVec2(40, 0), false);
    static float zoom = 1.0f;
    // remember the current cursor position, so we can return to it after drawing the slider
    ImVec2 cursorPos = ImGui::GetCursorPos();
    // move cursor to top-right corner of the child window, with some padding
    ImGui::SetCursorPos(ImVec2(ImGui::GetWindowWidth() - 40, 10));
    // draw the zoom slider
    float prevZoom = zoom;
    zoom = DrawZoomSlider("ZoomSlider", zoom, 0.1f, 64.0f, top_height - 64);
    if (zoom != prevZoom) {
        ImGuiTexInspect::OverrideNextScale(zoom);
    }
    // return cursor to its previous position
    ImGui::SetCursorPos(cursorPos);
    // set zoom on inspector here @TODO
    ImGui::EndChild();
    ImGui::EndChild();
    
    // ----------------------------------------------------
    // Splitter: draggable horizontal bar
    // ----------------------------------------------------
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0,0,0,0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.5f,0.5f,0.5f,0.5f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f,0.5f,0.5f,0.7f));
    ImGui::Button("##splitter", ImVec2(-1, splitter_size));
    if (ImGui::IsItemActive())
    {
        top_height += ImGui::GetIO().MouseDelta.y;
        top_height = std::max(min_height, std::min(max_height, top_height));
    }
    ImGui::PopStyleColor(3);
    
    ImGui::BeginChild("###BottomRegion_TA", ImVec2(0, 0), true);
    {
        // lower region is three panel layout
        
        // Setup middle panel tabs
        const char* tabLabels[] = { "Foo", "Bar", "Baz" };
        const int tabCount = IM_ARRAYSIZE(tabLabels);
        
        // Define panel content functions
        auto leftPanelFn = []() {
            ImGui::Text("Foo Tab Content");
            //lab::Orchestrator* mm =lab:: Orchestrator::Canonical();
            //std::weak_ptr<UsdOutlinerActivity> usdp;
            //auto usd = mm->LockActivity(usdp);
            //if (usd) {
            //    usd->USDOutlinerUI(LabViewInteraction());
            //}
        };
        
        // Middle panel tab functions
        std::function<void()> middlePanelFns[] = {
            [this]() { // Foo tab
                ImGui::Text("Foo Tab Content");
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
        
        auto rightPanelFn = [this]() {
            RunTextureCachePanel();
        };
    
        ThreePanelLayout(
            layoutState,
            leftPanelFn,
            middlePanelFns,
            rightPanelFn,
            tabLabels,
            tabCount
        );
        ImGui::EndChild();
    }

    ImGui::End();
    
    //ImGui::Begin("Loaded textures##tcp");
    //RunTextureCachePanel();
    //ImGui::End();
    //ImGui::Begin("Textures");
    //RunTextureInspectorPanel();
    //ImGui::End();
}

void TextureActivity::Update()
{
    #if 0
    auto fdm = LabApp::instance()->fdm();
    auto tc = TextureCache::instance();

    if (_self->run_open_file) {
        if (!_self->pending_file) {
            const char* dir = lab_pref_for_key("LoadTextureDir");
            if (!dir)
                dir = lab_pref_for_key("LoadStageDir");
            _self->pending_file = fdm->RequestOpenFile(
                                       {"exr", "jpg", "jpeg", "png", "gif", "pfm",
                                        "hdr", "psd", "tga", "pic", "pgm", "ppm"},
                                         dir? dir: ".");
        }
        else {
            do {
                auto req = fdm->PopOpenedFile(_self->pending_file);
                if (req.status == FileDialogManager::FileReq::notReady)
                    break;

                if (req.status == FileDialogManager::FileReq::expired ||
                    req.status == FileDialogManager::FileReq::canceled) {
                    _self->run_open_file = false;
                    _self->pending_file = 0;
                    break;
                }

                //assert(req.status == FileDialogManager::FileReq::ready);
                auto img = tc->ReadAndCache(req.path.c_str());
                if (img->dataSize > 0) {
                    // get the directory of path and save it as the default
                    // directory for the next time the user loads a stage
                    std::string dir = req.path.substr(0, req.path.find_last_of("/\\"));
                    lab_set_pref_for_key("LoadTextureDir", dir.c_str());
                }
                _self->run_open_file = false;
                _self->pending_file = 0;
            } while(false);
        }
    }
    #endif
}


void TextureActivity::Menu() {
    if (ImGui::BeginMenu("Textures")) {
        if (ImGui::MenuItem("Load Texture...")) {
            _self->loadTextureModule.LoadTexture();
        }

        if (ImGui::MenuItem("Export Texture Cache")) {
            TextureCache::instance()->ExportCache("/var/tmp");
        }
        ImGui::EndMenu();
    }
}


} //lab
