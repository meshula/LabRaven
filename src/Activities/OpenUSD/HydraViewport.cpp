#include "HydraViewport.hpp"

#include "Lab/App.h"
#include "Providers/Camera/CameraProvider.hpp"

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
    _DrawMenuBar();

    if (_GetViewportWidth() <= 0 || _GetViewportHeight() <= 0) return;

    ImGui::BeginChild("GameRender");

    _ConfigureImGuizmo();

    // read from active cam in case it is modify by another view
    if (!ImGui::IsWindowFocused())
        _UpdateViewportFromActiveCam();

    _UpdateProjection();
    _UpdateGrid();
    _UpdateHydraRender();
    _UpdateTransformGuizmo();
    _UpdateCubeGuizmo();
    _UpdatePluginLabel();

    ImGuizmo::PopID();

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

        if (ImGui::BeginMenu("cameras")) {
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
/*

 const lc_rigid_transform* cmt = &camera.mount.transform;
 lc_v3f pos = cmt->position;
 lc_v3f camera_to_focus = pos - _orbit_center;
 float distance_to_focus = length(camera_to_focus);
 const float feel = 0.02f;
 float scale = std::max(0.01f, logf(distance_to_focus) * feel);
 lc_v3f deltaX = lc_rt_right(cmt) * -delta.x * scale;
 lc_v3f dP = lc_rt_forward(cmt) * -delta.z * scale - deltaX - lc_rt_up(cmt) * -delta.y * scale;
 if (!_orbit_fixed)
     _orbit_center += dP;
 lc_mount_set_view_transform_quat_pos(&camera.mount, cmt->orientation, cmt->position + dP);

 */


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

    printf("at_0 %g %g %g\n", lookAt.center.x, lookAt.center.y, lookAt.center.z);

    HdSceneIndexPrim prim = model->GetFinalSceneIndex()->GetPrim(_activeCam);
    GfCamera gfCam = _ToGfCamera(prim);
    GfFrustum frustum = gfCam.GetFrustum();
    eye = frustum.GetPosition();
    at = frustum.ComputeLookAtPoint();

    printf("at_1 %g %g %g\n", lookAt.center.x, lookAt.center.y, lookAt.center.z);


    lookAt.eye = { (float)eye[0], (float)eye[1], (float)eye[2] };
    lookAt.center = { (float)at[0], (float)at[1], (float)at[2] };
    cp->SetLookAt(lookAt, "interactive");
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
    float nearPlane = _FREE_CAM_NEAR;
    float farPlane = _FREE_CAM_FAR;

    if (!_activeCam.IsEmpty()) {
        HdSceneIndexPrim prim = GetModel()->GetFinalSceneIndex()->GetPrim(_activeCam);
        GfCamera gfCam = _ToGfCamera(prim);
        fov = gfCam.GetFieldOfView(GfCamera::FOVVertical);
        nearPlane = gfCam.GetClippingRange().GetMin();
        farPlane = gfCam.GetClippingRange().GetMax();
    }

    GfFrustum frustum;
    double aspectRatio = _GetViewportWidth() / _GetViewportHeight();
    frustum.SetPerspective(fov, true, aspectRatio, nearPlane, farPlane);
    _proj = frustum.ComputeProjectionMatrix();
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
