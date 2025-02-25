#include "HydraViewport.hpp"

#include "Lab/App.h"
#include "Providers/Camera/CameraProvider.hpp"
#include "Providers/OpenUSD/OpenUSDProvider.hpp"
#include "Providers/OpenUSD/UsdUtils.hpp"
#include "Lab/ImguiExt.hpp"

#include <pxr/base/gf/camera.h>
#include <pxr/base/gf/frustum.h>
#include <pxr/base/gf/matrix4f.h>
#include <pxr/base/plug/plugin.h>
#include <pxr/imaging/cameraUtil/framing.h>
#include <pxr/imaging/hd/cameraSchema.h>
#include <pxr/imaging/hd/extentSchema.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/imaging/hgi/blitCmdsOps.h>
#include <pxr/imaging/hgi/texture.h>
#include <pxr/imaging/hdSt/renderBuffer.h>

extern "C"
int LabCreateRGBAf16Texture(int width, int height, uint8_t* rgba_pixels);
extern "C"
void* LabTextureHardwareHandle(int texture);
extern "C"
void LabRemoveTexture(int texture);
extern "C"
void LabUpdateRGBAf16Texture(int texture, uint8_t* rgba_pixels);


struct TextureCapture {
    int width = 0;
    int height = 0;
    int handle = -1;
};

TextureCapture texcap;
PXR_NAMESPACE_USING_DIRECTIVE

namespace lab {

using std::string;
using pxr::Model;

HydraViewport::HydraViewport(Model* model, const string label) : View(model, label)
{
    _gizmoWindowFlags = ImGuiWindowFlags_MenuBar;
    _isAmbientLightEnabled = true;
    _isDomeLightEnabled = false;
    _isGridEnabled = true;
    _guiInterceptedMouse = false;

    _curOperation = ImGuizmo::TRANSLATE;
    _curMode = ImGuizmo::LOCAL;

    _UpdateActiveCamFromViewport();

    _gridSceneIndex = GridSceneIndex::New();
    GetModel()->AddSceneIndexBase(_gridSceneIndex);

    auto editableSceneIndex = GetModel()->GetEditableSceneIndex();
    _xformSceneIndex = XformFilterSceneIndex::New(editableSceneIndex);
    GetModel()->SetEditableSceneIndex(_xformSceneIndex);

    TfToken plugin = Engine::GetDefaultRendererPlugin();
    _engine = new Engine(GetModel()->GetFinalSceneIndex(), plugin);
};

HydraViewport::~HydraViewport()
{
    delete _engine;
}

const string HydraViewport::GetViewType()
{
    return VIEW_TYPE;
};

ImGuiWindowFlags HydraViewport::_GetGizmoWindowFlags()
{
    return _gizmoWindowFlags;
};

float HydraViewport::_GetViewportWidth()
{
    return GetInnerRect().GetWidth();
}

float HydraViewport::_GetViewportHeight()
{
    return GetInnerRect().GetHeight();
}

void HydraViewport::_Draw()
{
    // This routine is inside a Begin/End()

    _guiInterceptedMouse = false;

    _DrawMenuBar();

    if (_GetViewportWidth() <= 0 || _GetViewportHeight() <= 0) return;

    ImGui::BeginChild("GameRender");

    const float dpad_width = 32;
    // record the current upper left corner of the ImGui window
    ImVec2 upperLeft = ImGui::GetCursorPos();
    ImVec2 dollyWidgetPos = upperLeft;
    // add the width of the window and subtract a hundred for a widget pos
    dollyWidgetPos.x += _GetViewportWidth() - 150;
    dollyWidgetPos.y += 2;
    ImVec2 craneWidgetPos = dollyWidgetPos;
    craneWidgetPos.x -= (dpad_width + 8);
    ImVec2 panWidgetPos = craneWidgetPos;
    panWidgetPos.x -= (dpad_width + 8);

    _ConfigureImGuizmo();

    // read from active cam in case it was modified by another view
    if (!ImGui::IsWindowFocused())
        _UpdateViewportFromActiveCam();

    _UpdateProjection();
    _UpdateGrid();
    _UpdateHydraRender();
    _UpdateTransformGuizmo();
    _UpdateCubeGuizmo();
    _UpdatePluginLabel();

    ImGuizmo::PopID();

    static float x = 0;
    static float y = 0;
    ImGui::SetCursorPos(dollyWidgetPos);
    if (imgui_dpad("dolly", &x, &y, false, dpad_width, 0.1)) {
        _guiInterceptedMouse = true;
    }
    static float cx = 0;
    static float cy = 0;
    ImGui::SetCursorPos(craneWidgetPos);
    if (imgui_dpad("crane", &cx, &cy, false, dpad_width, 0.1)) {
        _guiInterceptedMouse = true;
    }
    static float px = 0;
    static float py = 0;
    ImGui::SetCursorPos(panWidgetPos);
    if (imgui_dpad("pan", &px, &py, false, dpad_width, 0.1)) {
        _guiInterceptedMouse = true;
    }

    ImGui::SetCursorPos(upperLeft);
    static bool centerLocked = true;
    // checkbox
    if (ImGui::Checkbox("lock center", &centerLocked)) {
        _guiInterceptedMouse = true;
    }

    if (ImGui::Button("Home")) {
        _guiInterceptedMouse = true;
        auto cp = CameraProvider::instance();
        cp->SetLookAt(cp->GetHome().lookAt, "interactive");
    }
    if (ImGui::Button("Center")) {
        _guiInterceptedMouse = true;
        auto cp = CameraProvider::instance();
        auto lookAt = cp->GetLookAt("interactive").lookAt;

        auto usd = OpenUSDProvider::instance();
        auto model = usd->Model();
        auto selection = model->GetSelection();

        if (selection.size()) {
            auto box = ComputeWorldBounds(model->GetStage(), UsdTimeCode::Default(), selection);
            GfVec3d center = box.ComputeCentroid();
            lookAt.center = { (float) center[0], (float) center[1], (float) center[2] };
            cp->LerpLookAt(lookAt, 0.25f, "interactive");
        }
        else {
            //lookAt.center = { 0, 0, 0 };
        }
    }
    if (ImGui::Button("Frame")) {
        _guiInterceptedMouse = true;
        auto cp = CameraProvider::instance();
        auto camData = cp->GetLookAt("interactive");
        auto& lookAt = camData.lookAt;

        auto usd = OpenUSDProvider::instance();
        auto model = usd->Model();
        auto selection = model->GetSelection();

        if (selection.size()) {
            auto box = ComputeWorldBounds(model->GetStage(), UsdTimeCode::Default(), selection);
            GfVec3d center = box.ComputeCentroid();
            if (true || (box.GetVolume() > 0)) {
                printf("Selection extent, framing\n");
                auto bboxRange = box.ComputeAlignedRange();
                GfVec3d rect = bboxRange.GetMax() - bboxRange.GetMin();
                float selectionSize = std::max(rect[0], rect[1]) * 2; // This reset the selection size
                lc_radians fov = camData.HFOV;
                auto lengthToFit = selectionSize * 0.5;
                float dist = lengthToFit / atan(fov.rad * 0.5);

                GfVec3d eye(lookAt.eye.x, lookAt.eye.y, lookAt.eye.z);
                GfVec3d eyeToCenter = eye - center;
                eyeToCenter.Normalize();
                eyeToCenter *= dist;
                eye = center + eyeToCenter;

                lookAt.eye = { (float) eye[0], (float) eye[1], (float) eye[2] };
                lookAt.center = { (float) center[0], (float) center[1], (float) center[2] };
                lookAt.up = { 0, 1, 0 };
                cp->LerpLookAt(lookAt, 0.25f, "interactive");
            }
            else {
                printf("Selection has no extent, look at\n");
                lookAt.center = { (float) center[0], (float) center[1], (float) center[2] };
                cp->LerpLookAt(lookAt, 0.25f, "interactive");
            }
        }
        else {
            // no selection, nothing to do
        }
    }

    ImGui::EndChild();
};

void HydraViewport::_DrawMenuBar()
{
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("transform")) {
            if (ImGui::MenuItem("local translate")) {
                _curOperation = ImGuizmo::TRANSLATE;
                _curMode = ImGuizmo::LOCAL;
            }
            if (ImGui::MenuItem("local rotation")) {
                _curOperation = ImGuizmo::ROTATE;
                _curMode = ImGuizmo::LOCAL;
            }
            if (ImGui::MenuItem("local scale")) {
                _curOperation = ImGuizmo::SCALE;
                _curMode = ImGuizmo::LOCAL;
            }
            if (ImGui::MenuItem("global translate")) {
                _curOperation = ImGuizmo::TRANSLATE;
                _curMode = ImGuizmo::WORLD;
            }
            if (ImGui::MenuItem("global rotation")) {
                _curOperation = ImGuizmo::ROTATE;
                _curMode = ImGuizmo::WORLD;
            }
            if (ImGui::MenuItem("global scale")) {
                _curOperation = ImGuizmo::SCALE;
                _curMode = ImGuizmo::WORLD;
            }

            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("renderer")) {
            // get all possible renderer plugins
            TfTokenVector plugins = _engine->GetRendererPlugins();
            TfToken curPlugin = _engine->GetCurrentRendererPlugin();
            for (auto p : plugins) {
                bool enabled = (p == curPlugin);
                string name = _engine->GetRendererPluginName(p);
                if (ImGui::MenuItem(name.c_str(), NULL, enabled)) {
                    delete _engine;
                    _engine = new Engine(GetModel()->GetFinalSceneIndex(), p);
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("hd cameras")) {
            bool enabled = (_activeCam.IsEmpty());
            if (ImGui::MenuItem("free camera", NULL, &enabled)) {
                _SetFreeCamAsActive();
            }
            for (SdfPath path : GetModel()->GetCameras()) {
                bool enabled = (path == _activeCam);
                if (ImGui::MenuItem(path.GetName().c_str(), NULL, enabled)) {
                    _SetActiveCam(path);
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("lights")) {
            ImGui::MenuItem("ambient light", NULL, &_isAmbientLightEnabled);
            ImGui::MenuItem("dome light", NULL, &_isDomeLightEnabled);
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("show")) {
            ImGui::MenuItem("grid", NULL, &_isGridEnabled);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
}

void HydraViewport::_ConfigureImGuizmo()
{
    ImGuizmo::BeginFrame();

    // convert last label char to ID
    string label = GetViewLabel();
    ImGuizmo::PushID(int(label[label.size() - 1]));

    ImGuizmo::SetDrawlist();
    ImGuizmo::SetRect(GetInnerRect().Min.x, GetInnerRect().Min.y,
                      _GetViewportWidth(), _GetViewportHeight());
}

void HydraViewport::_UpdateGrid()
{
    _gridSceneIndex->Populate(_isGridEnabled);

    if (!_isGridEnabled) return;

    GfMatrix4f viewF(_getCurViewMatrix());
    GfMatrix4f projF(_proj);
    GfMatrix4f identity(1);

    ImGuizmo::DrawGrid(viewF.data(), projF.data(), identity.data(), 10);
}

static uint8_t*
GetGPUTexture(
    Hgi& hgi,
    HgiTextureHandle const& texHandle,
    int width,
    int height,
    HgiFormat format)
{
    // Copy the pixels from gpu into a cpu buffer so we can save it to disk.
    const size_t bufferByteSize =
        width * height * HgiGetDataSizeOfFormat(format);
    static uint8_t* buffer = nullptr;
    static size_t sz = 0;
    if (sz < bufferByteSize) {
        if (buffer) {
            free(buffer);
            buffer = nullptr;
        }
        sz = 0;
    }
    if (!buffer) {
        buffer = (uint8_t*) malloc(bufferByteSize);
        sz = bufferByteSize;
    }

    HgiTextureGpuToCpuOp copyOp;
    copyOp.gpuSourceTexture = texHandle;
    copyOp.sourceTexelOffset = GfVec3i(0);
    copyOp.mipLevel = 0;
    copyOp.cpuDestinationBuffer = buffer;
    copyOp.destinationByteOffset = 0;
    copyOp.destinationBufferByteSize = bufferByteSize;

    HgiBlitCmdsUniquePtr blitCmds = hgi.CreateBlitCmds();
    blitCmds->CopyTextureGpuToCpu(copyOp);
    hgi.SubmitCmds(blitCmds.get(), HgiSubmitWaitTypeWaitUntilCompleted);

    return buffer;
}


void HydraViewport::_UpdateHydraRender()
{
    auto model = GetModel();
    if (!model)
        return;

    GfMatrix4d view = _getCurViewMatrix();
    float width = _GetViewportWidth();
    float height = _GetViewportHeight();

    // set selection
    SdfPathVector paths;
    for (auto&& prim : model->GetSelection())
        paths.push_back(prim.GetPrimPath());

    _engine->SetSelection(paths);
    _engine->SetRenderSize(width, height);
    _engine->SetCameraMatrices(view, _proj);
    _engine->Prepare();

    // do the render
    _engine->Render();

    auto tc = _engine->GetHdxTaskController();
    HdRenderBuffer* buffer = tc->GetRenderOutput(HdAovTokens->color);
    buffer->Resolve();
    VtValue aov = buffer->GetResource(false);
    if (aov.IsHolding<HgiTextureHandle>()) {
        HgiTextureHandle textureHandle = aov.Get<HgiTextureHandle>();
        if (textureHandle) {
            HgiTextureDesc const& desc = textureHandle->GetDescriptor();

            Hgi* hgi = _engine->GetHgi();
            if (hgi) {
                auto buffer = GetGPUTexture(*hgi,
                                            textureHandle,
                                            width, height,
                                            desc.format);

                if (texcap.width != width || texcap.height != height || texcap.handle < 0) {
                    if (texcap.handle > 0) {
                        LabRemoveTexture(texcap.handle);
                    }
                    texcap.width = width;
                    texcap.height = height;
                    texcap.handle = LabCreateRGBAf16Texture(width, height, buffer);
                }
                else {
                    LabUpdateRGBAf16Texture(texcap.handle, (uint8_t*) buffer);
                }

                ImGui::Image((ImTextureID) LabTextureHardwareHandle(texcap.handle),
                             ImVec2(width, height),
                             ImVec2(0, 1), ImVec2(1, 0));
            }
        }
    }
}

void HydraViewport::_UpdateTransformGuizmo()
{
    SdfPathVector primPaths = GetModel()->GetSelection();
    if (primPaths.size() == 0 || primPaths[0].IsEmpty()) return;

    SdfPath primPath = primPaths[0];

    GfMatrix4d transform = _xformSceneIndex->GetXform(primPath);
    GfMatrix4f transformF(transform);

    GfMatrix4d view = _getCurViewMatrix();

    GfMatrix4f viewF(view);
    GfMatrix4f projF(_proj);

    ImGuizmo::Manipulate(viewF.data(), projF.data(), _curOperation, _curMode,
                         transformF.data());

    if (transformF != GfMatrix4f(transform))
        _xformSceneIndex->SetXform(primPath, GfMatrix4d(transformF));
}

void HydraViewport::_UpdateCubeGuizmo()
{
    GfMatrix4d view = _getCurViewMatrix();
    GfMatrix4f viewF(view);
    GfMatrix4f currView(viewF);

    auto cp = CameraProvider::instance();
    auto lookAt = cp->GetLookAt("interactive").lookAt;

    GfVec3d toCenter(lookAt.eye.x - lookAt.center.x,
                     lookAt.eye.y - lookAt.center.y,
                     lookAt.eye.z - lookAt.center.z);
    double dist = toCenter.GetLength();

    ImGuizmo::ViewManipulate(
        viewF.data(), dist,
        ImVec2(GetInnerRect().Max.x - 128, GetInnerRect().Min.y + 18),
        ImVec2(128, 128), IM_COL32_BLACK_TRANS);

    if (viewF != currView) {
        GfFrustum frustum;
        frustum.SetViewDistance(dist);

        view = GfMatrix4d(viewF);
        frustum.SetPositionAndRotationFromMatrix(view.GetInverse());
        GfVec3d eye = frustum.GetPosition();
        GfVec3d at = frustum.ComputeLookAtPoint();
        lookAt.eye = { (float)eye[0], (float)eye[1], (float)eye[2] };
        lookAt.center = { (float)at[0], (float)at[1], (float)at[2] };
        cp->SetLookAt(lookAt, "interactive");

        _UpdateActiveCamFromViewport();

        LabApp::instance()->SuspendPowerSave(10);
    }
}

void HydraViewport::_UpdatePluginLabel()
{
    TfToken curPlugin = _engine->GetCurrentRendererPlugin();
    string pluginText = _engine->GetRendererPluginName(curPlugin);
    string text = pluginText;

    ImDrawList* draw_list = ImGui::GetWindowDrawList();

    ImVec2 textSize = ImGui::CalcTextSize(text.c_str());
    float margin = 6;
    float xPos = (GetInnerRect().Max.x - 64 - textSize.x / 2);
    float yPos = GetInnerRect().Min.y + margin * 2;
    // draw background color
    draw_list->AddRectFilled(
        ImVec2(xPos - margin, yPos - margin),
        ImVec2(xPos + textSize.x + margin, yPos + textSize.y + margin),
        ImColor(.0f, .0f, .0f, .2f), margin);
    // draw text
    draw_list->AddText(ImVec2(xPos, yPos), ImColor(1.f, 1.f, 1.f),
                       text.c_str());
}

void HydraViewport::_PanActiveCam(ImVec2 mouseDeltaPos)
{
    auto cp = CameraProvider::instance();
    auto cam = cp->GetLookAt("interactive").lookAt;
    GfVec3d at(cam.center.x, cam.center.y, cam.center.z);
    GfVec3d eye(cam.eye.x, cam.eye.y, cam.eye.z);
    GfVec3d up(cam.up.x, cam.up.y, cam.up.z);

    GfVec3d camFront = at - eye;
    GfVec3d camRight = GfCross(camFront, up).GetNormalized();
    GfVec3d camUp = GfCross(camRight, camFront).GetNormalized();

    GfVec3d delta =
        camRight * -mouseDeltaPos.x / 100.f + camUp * mouseDeltaPos.y / 100.f;

    eye += delta;
    at += delta;

    cam.eye = { (float)eye[0], (float)eye[1], (float)eye[2] };
    cam.center = { (float)at[0], (float)at[1], (float)at[2] };
    cp->SetLookAt(cam, "interactive");

    _UpdateActiveCamFromViewport();
}

void HydraViewport::_OrbitActiveCam(ImVec2 mouseDeltaPos)
{
    auto cp = CameraProvider::instance();
    auto cam = cp->GetLookAt("interactive").lookAt;
    GfVec3d at(cam.center.x, cam.center.y, cam.center.z);
    GfVec3d eye(cam.eye.x, cam.eye.y, cam.eye.z);
    GfVec3d up(cam.up.x, cam.up.y, cam.up.z);

    GfRotation rot(up, mouseDeltaPos.x / 2);
    GfMatrix4d rotMatrix = GfMatrix4d(1).SetRotate(rot);
    GfVec3d e = eye - at;
    GfVec4d vec4 = rotMatrix * GfVec4d(e[0], e[1], e[2], 1.f);
    eye = at + GfVec3d(vec4[0], vec4[1], vec4[2]);

    GfVec3d camFront = at - eye;
    GfVec3d camRight = GfCross(camFront, up).GetNormalized();
    rot = GfRotation(camRight, mouseDeltaPos.y / 2);
    rotMatrix = GfMatrix4d(1).SetRotate(rot);
    e = eye - at;
    vec4 = rotMatrix * GfVec4d(e[0], e[1], e[2], 1.f);
    eye = at + GfVec3d(vec4[0], vec4[1], vec4[2]);

    cam.eye = { (float)eye[0], (float)eye[1], (float)eye[2] };
    cam.center = { (float)at[0], (float)at[1], (float)at[2] };
    cp->SetLookAt(cam, "interactive");

    _UpdateActiveCamFromViewport();
}

void HydraViewport::_ZoomActiveCam(ImVec2 mouseDeltaPos)
{
    auto cp = CameraProvider::instance();
    auto cam = cp->GetLookAt("interactive").lookAt;
    GfVec3d at(cam.center.x, cam.center.y, cam.center.z);
    GfVec3d eye(cam.eye.x, cam.eye.y, cam.eye.z);
    GfVec3d up(cam.up.x, cam.up.y, cam.up.z);

    GfVec3d camToFocus = eye - at;
    float focusDistance = camToFocus.GetLength();
    const float feel = 0.02f;
    float scale = std::max(0.01f, logf(focusDistance * feel));
    GfVec3d camFront = (at - eye).GetNormalized() * mouseDeltaPos.y * scale;
    eye += camFront;

    cam.eye = { (float)eye[0], (float)eye[1], (float)eye[2] };
    cam.center = { (float)at[0], (float)at[1], (float)at[2] };
    cp->SetLookAt(cam, "interactive");

    _UpdateActiveCamFromViewport();
}

void HydraViewport::_ZoomActiveCam(float scrollWheel)
{
    auto cp = CameraProvider::instance();
    auto cam = cp->GetLookAt("interactive").lookAt;
    GfVec3d at(cam.center.x, cam.center.y, cam.center.z);
    GfVec3d eye(cam.eye.x, cam.eye.y, cam.eye.z);
    GfVec3d up(cam.up.x, cam.up.y, cam.up.z);

    GfVec3d camToFocus = eye - at;
    float focusDistance = camToFocus.GetLength();
    const float feel = 0.02f;
    float scale = std::max(0.01f, logf(focusDistance * feel));
    GfVec3d camFront = (at - eye).GetNormalized() * scrollWheel * scale;
    eye += camFront;

    cam.eye = { (float)eye[0], (float)eye[1], (float)eye[2] };
    cam.center = { (float)at[0], (float)at[1], (float)at[2] };
    cp->SetLookAt(cam, "interactive");

    _UpdateActiveCamFromViewport();
}

void HydraViewport::_SetFreeCamAsActive()
{
    _activeCam = SdfPath();
}

void HydraViewport::_SetActiveCam(SdfPath primPath)
{
    _activeCam = primPath;
    _UpdateViewportFromActiveCam();
}

void HydraViewport::_UpdateViewportFromActiveCam()
{
    if (_activeCam.IsEmpty())
        return;

    auto cp = CameraProvider::instance();
    auto lookAt = cp->GetLookAt("interactive").lookAt;
    GfVec3d at(lookAt.center.x, lookAt.center.y, lookAt.center.z);
    GfVec3d eye(lookAt.eye.x, lookAt.eye.y, lookAt.eye.z);
    GfVec3d up(lookAt.up.x, lookAt.up.y, lookAt.up.z);

    auto model = GetModel();
    model->SetActiveCamera(_activeCam);

    HdSceneIndexPrim prim = model->GetFinalSceneIndex()->GetPrim(_activeCam);
    GfCamera gfCam = _ToGfCamera(prim);
    GfFrustum frustum = gfCam.GetFrustum();
    eye = frustum.GetPosition();
    at = frustum.ComputeLookAtPoint();

    lookAt.eye = { (float)eye[0], (float)eye[1], (float)eye[2] };
    lookAt.center = { (float)at[0], (float)at[1], (float)at[2] };
    cp->SetLookAt(lookAt, "interactive");
    cp->SetHFOV({ (float)gfCam.GetFieldOfView(GfCamera::FOVHorizontal) }, "interactive");
}

GfMatrix4d HydraViewport::_getCurViewMatrix()
{
    auto cp = CameraProvider::instance();
    auto lookAt = cp->GetLookAt("interactive").lookAt;
    GfVec3d at(lookAt.center.x, lookAt.center.y, lookAt.center.z);
    GfVec3d eye(lookAt.eye.x, lookAt.eye.y, lookAt.eye.z);
    GfVec3d up(lookAt.up.x, lookAt.up.y, lookAt.up.z);
    return GfMatrix4d().SetLookAt(eye, at, up);
}

void HydraViewport::_UpdateActiveCamFromViewport()
{
    if (_activeCam.IsEmpty())
        return;

    HdSceneIndexPrim prim = GetModel()->GetFinalSceneIndex()->GetPrim(_activeCam);
    GfCamera gfCam = _ToGfCamera(prim);

    GfFrustum prevFrustum = gfCam.GetFrustum();

    GfMatrix4d view = _getCurViewMatrix();
    GfMatrix4d prevView = prevFrustum.ComputeViewMatrix();
    GfMatrix4d prevProj = prevFrustum.ComputeProjectionMatrix();

    if (view == prevView && _proj == prevProj)
        return;

    _xformSceneIndex->SetXform(_activeCam, view.GetInverse());
}

void HydraViewport::_UpdateProjection()
{
    float fov = _FREE_CAM_FOV;

    if (!_activeCam.IsEmpty()) {
        HdSceneIndexPrim prim = GetModel()->GetFinalSceneIndex()->GetPrim(_activeCam);
        GfCamera gfCam = _ToGfCamera(prim);
        fov = gfCam.GetFieldOfView(GfCamera::FOVVertical);
        _zNear = gfCam.GetClippingRange().GetMin();
        _zFar = gfCam.GetClippingRange().GetMax();
    }

    GfFrustum frustum;
    double aspectRatio = _GetViewportWidth() / _GetViewportHeight();
    frustum.SetPerspective(fov, true, aspectRatio, _zNear, _zFar);
    _proj = frustum.ComputeProjectionMatrix();
}

float HydraViewport::GetAspect()
{
    return _GetViewportWidth() / _GetViewportHeight();
}


void HydraViewport::SetCameraFromGfCamera(const GfCamera& gfCam) {
    _zNear = gfCam.GetClippingRange().GetMin();
    _zFar = gfCam.GetClippingRange().GetMax();
    GfFrustum frustum = gfCam.GetFrustum();     /// @TODO need to respect the camera's aspect ratio and fit it in the view port and set up reticle
    _proj = frustum.ComputeProjectionMatrix();

    GfVec3d eye = frustum.GetPosition();
    GfVec3d at = frustum.ComputeLookAtPoint();
    CameraProvider::LookAt lookAt;
    auto cp = CameraProvider::instance();
    lookAt.eye = { (float)eye[0], (float)eye[1], (float)eye[2] };
    lookAt.center = { (float)at[0], (float)at[1], (float)at[2] };
    lookAt.up = { 0, 1, 0 };
    cp->SetLookAt(lookAt, "interactive");
    cp->SetHFOV({ (float) (gfCam.GetFieldOfView(GfCamera::FOVVertical) * 2.0 * M_PI / 360.0) }, "interactive");
}


GfCamera HydraViewport::_ToGfCamera(HdSceneIndexPrim prim)
{
    GfCamera cam;

    if (prim.primType != HdPrimTypeTokens->camera)
        return cam;

    HdSampledDataSource::Time time(0);

    HdXformSchema xformSchema = HdXformSchema::GetFromParent(prim.dataSource);

    GfMatrix4d xform =
        xformSchema.GetMatrix()->GetValue(time).Get<GfMatrix4d>();

    HdCameraSchema camSchema = HdCameraSchema::GetFromParent(prim.dataSource);

    TfToken projection =
        camSchema.GetProjection()->GetValue(time).Get<TfToken>();
    float hAperture =
        camSchema.GetHorizontalAperture()->GetValue(time).Get<float>();
    float vAperture =
        camSchema.GetVerticalAperture()->GetValue(time).Get<float>();
    float hApertureOffest =
        camSchema.GetHorizontalApertureOffset()->GetValue(time).Get<float>();
    float vApertureOffest =
        camSchema.GetVerticalApertureOffset()->GetValue(time).Get<float>();
    float focalLength =
        camSchema.GetFocalLength()->GetValue(time).Get<float>();
    GfVec2f clippingRange =
        camSchema.GetClippingRange()->GetValue(time).Get<GfVec2f>();

    cam.SetTransform(xform);
    cam.SetProjection(projection == HdCameraSchemaTokens->orthographic
                          ? GfCamera::Orthographic
                          : GfCamera::Perspective);
    cam.SetHorizontalAperture(hAperture / GfCamera::APERTURE_UNIT);
    cam.SetVerticalAperture(vAperture / GfCamera::APERTURE_UNIT);
    cam.SetHorizontalApertureOffset(hApertureOffest / GfCamera::APERTURE_UNIT);
    cam.SetVerticalApertureOffset(vApertureOffest / GfCamera::APERTURE_UNIT);
    cam.SetFocalLength(focalLength / GfCamera::FOCAL_LENGTH_UNIT);
    cam.SetClippingRange(GfRange1f(clippingRange[0], clippingRange[1]));

    _cam = cam;
    return cam;
}

void HydraViewport::_FocusOnPrim(SdfPath primPath)
{
    if (primPath.IsEmpty()) return;

    HdSceneIndexPrim prim = GetModel()->GetFinalSceneIndex()->GetPrim(primPath);

    HdExtentSchema extentSchema =
        HdExtentSchema::GetFromParent(prim.dataSource);
    if (!extentSchema.IsDefined()) {
        TF_WARN("Prim at %s has no extent; skipping focus.",
                primPath.GetAsString().c_str());
        return;
    }

    HdSampledDataSource::Time time(0);
    GfVec3d extentMin = extentSchema.GetMin()->GetValue(time).Get<GfVec3d>();
    GfVec3d extentMax = extentSchema.GetMax()->GetValue(time).Get<GfVec3d>();

    GfRange3d extentRange(extentMin, extentMax);

    auto cp = CameraProvider::instance();
    auto cam = cp->GetLookAt("interactive").lookAt;
    GfVec3d at(cam.center.x, cam.center.y, cam.center.z);
    GfVec3d eye(cam.eye.x, cam.eye.y, cam.eye.z);
    GfVec3d up(cam.up.x, cam.up.y, cam.up.z);

    at = extentRange.GetMidpoint();
    eye = at + (eye - at).GetNormalized() *
                     extentRange.GetSize().GetLength() * 2;

    cam.eye = { (float)eye[0], (float)eye[1], (float)eye[2] };
    cam.center = { (float)at[0], (float)at[1], (float)at[2] };
    cp->SetLookAt(cam, "interactive");

    _UpdateActiveCamFromViewport();
}

void HydraViewport::_KeyPressEvent(ImGuiKey key)
{
    if (_guiInterceptedMouse)
        return;

    if (key == ImGuiKey_F) {
        SdfPathVector primPaths = GetModel()->GetSelection();
        if (primPaths.size() > 0) _FocusOnPrim(primPaths[0]);
    }
    else if (key == ImGuiKey_W) {
        _curOperation = ImGuizmo::TRANSLATE;
        _curMode = ImGuizmo::LOCAL;
    }
    else if (key == ImGuiKey_E) {
        _curOperation = ImGuizmo::ROTATE;
        _curMode = ImGuizmo::LOCAL;
    }
    else if (key == ImGuiKey_R) {
        _curOperation = ImGuizmo::SCALE;
        _curMode = ImGuizmo::LOCAL;
    }
}

void HydraViewport::_MouseMoveEvent(ImVec2 prevPos, ImVec2 curPos)
{
    if (_guiInterceptedMouse)
        return;

    ImVec2 deltaMousePos = curPos - prevPos;

    ImGuiIO& io = ImGui::GetIO();
    if (io.MouseWheel) _ZoomActiveCam(io.MouseWheel);

    if (ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
        (ImGui::IsKeyDown(ImGuiKey_LeftAlt) ||
         ImGui::IsKeyDown(ImGuiKey_RightAlt))) {
        _OrbitActiveCam(deltaMousePos);
    }
    if (ImGui::IsMouseDown(ImGuiMouseButton_Left) &&
        (ImGui::IsKeyDown(ImGuiKey_LeftShift) ||
         ImGui::IsKeyDown(ImGuiKey_RightShift))) {
        _PanActiveCam(deltaMousePos);
    }
    if (ImGui::IsMouseDown(ImGuiMouseButton_Right) &&
        (ImGui::IsKeyDown(ImGuiKey_LeftAlt) ||
         ImGui::IsKeyDown(ImGuiKey_RightAlt))) {
        _ZoomActiveCam(deltaMousePos);
    }
}

void HydraViewport::_MouseReleaseEvent(ImGuiMouseButton_ button, ImVec2 mousePos)
{
    if (_guiInterceptedMouse)
        return;

    if (button == ImGuiMouseButton_Left) {
        ImVec2 delta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Left);
        if (fabs(delta.x) + fabs(delta.y) < 0.001f) {
            GfVec2f gfMousePos(mousePos[0], mousePos[1]);
            Engine::IntersectionResult intr = _engine->FindIntersection(gfMousePos);

            if (intr.path.IsEmpty())
                GetModel()->SetSelection({});
            else {
                GetModel()->SetSelection({intr.path});
                GetModel()->SetHit(intr.worldSpaceHitPoint, intr.worldSpaceHitNormal);
            }
        }
    }
}

void HydraViewport::_HoverInEvent()
{
    _gizmoWindowFlags |= ImGuiWindowFlags_NoMove;
}
void HydraViewport::_HoverOutEvent()
{
    _gizmoWindowFlags &= ~ImGuiWindowFlags_NoMove;
}

} // namespace lab
