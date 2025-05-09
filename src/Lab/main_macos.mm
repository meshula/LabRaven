#include "App.h"
#include "RegisterAllActivities.h"
#include "Lab/CoreProviders/Color/glfwColor.h"
#include "Lab/CoreProviders/Metal/MetalProvider.hpp"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_metal.h"
#include "implot.h"
#include "implot3d.h"

#define GLFW_INCLUDE_NONE
#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>

#include <stdio.h>

App* app = nullptr;

// Accept and open a file path
void file_drop_callback(GLFWwindow* window, int count, const char** paths) {
    if (app && app->FileDrop) {
        app->FileDrop(count, paths);
    }
}

static void glfw_error_callback(int error, const char* description)
{
    fprintf(stderr, "Glfw Error %d: %s\n", error, description);
}

int main(int argc, char** argv)
{
    // Setup window
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit())
        return 1;

    // Create window with graphics context
    int initial_width = 1280;
    int initial_height = 768;
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWwindow* window = glfwCreateWindow(initial_width, initial_height, "Raven", NULL, NULL);
    if (window == NULL)
        return 1;

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();

    ImGui::CreateContext();
    ImPlot::CreateContext();
    ImPlot3D::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;        // Enable Docking
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;      // Enable Multi-Viewport / Platform Windows
#ifdef POWER_SAVING
    io.ConfigFlags |= ImGuiConfigFlags_EnablePowerSavingMode;
#endif

    app = gApp();
    auto orch = lab::gOrchestrator();
    RegisterAllActivities(*orch);

    // Setup style
    //ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
    {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Load Fonts
    // - If no fonts are loaded, dear imgui will use the default font. You can also load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - AddFontFromFileTTF() will return the ImFont* so you can store it if you need to select the font among multiple.
    // - If the file cannot be loaded, the function will return NULL. Please handle those errors in your application (e.g. use an assertion, or display an error and quit).
    // - The fonts will be rasterized at a given size (w/ oversampling) and stored into a texture when calling ImFontAtlas::Build()/GetTexDataAsXXXX(), which ImGui_ImplXXXX_NewFrame below will call.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use Freetype for higher quality font rendering.
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //ImFont* defaultFont = io.Fonts->AddFontDefault();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf", 18.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf", 16.0f);
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf", 18.0f, NULL, io.Fonts->GetGlyphRangesJapanese());
    //IM_ASSERT(font != NULL);
    //ImFont* cousineFont = io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf", 15.0f);
    //static const ImWchar ghostIconRanges[] = { `👻`, 0 };
    //ImFontConfig iconConfig;
    //iconConfig.MergeMode = true;
    //iconConfig.PixelSnapH = true;
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/fontawesome-webfont.ttf", 16.0f, &iconConfig, ghostIconRanges);



    id <MTLDevice> device = MTLCreateSystemDefaultDevice();

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOther(window, true);
    ImGui_ImplMetal_Init(device);

    if (app && app->Init) {
        app->Init(argc, argv, initial_width, initial_height);
    }

    NSWindow *nswin = glfwGetCocoaWindow(window);
    CAMetalLayer *layer = [CAMetalLayer layer];
    layer.device = device;
    layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
    nswin.contentView.layer = layer;
    nswin.contentView.wantsLayer = YES;
    SetGLFWColorEnvironment(window, "lin_displayp3", ProviderFramebufferFormat_f16HDR);

    static int const commandBufferPoolSize = 256;
    LabMetalProvider* metalProvider = [[LabMetalProvider alloc] initWithDevice:device
                                                         commandBufferPoolSize:commandBufferPoolSize
                                                                    colorSpace:kCGColorSpaceExtendedLinearDisplayP3];

    const bool CREATE_DEPTH_BUFFER = true;
    [metalProvider createDefaultRenderAttachments:CREATE_DEPTH_BUFFER];
    MTLRenderPassDescriptor* renderPassDescriptor = metalProvider.renderpassDescriptor;

    id <MTLCommandQueue> commandQueue = metalProvider.commandQueue;

    // Our state
    float clear_color[4] = {0.45f, 0.55f, 0.60f, 1.00f};

    // Set the drop callback
    glfwSetDropCallback(window, file_drop_callback);


    // Main loop
    while (!glfwWindowShouldClose(window) && app && app->IsRunning())
    {
        @autoreleasepool
        {
#ifdef POWER_SAVING
            // This application doesn't do any animation, so instead
            // of rendering all the time, we block waiting for events.
            // The POWER_SAVING toggle is meant to be used with this
            // fork of Dear ImGui: https://github.com/ocornut/imgui/pull/4076
            // which would provide both the power savings, and support for
            // animation, if we ever need/want to add animation to the app.
            // See also: https://github.com/ocornut/imgui/pull/5599
            ImGui_ImplGlfw_WaitForEvent();
#else
            static int cooldown = 20;
            bool noCoolDown = app->PowerSave && !app->PowerSave();
            if (cooldown <= 0) {
                if (!noCoolDown)
                    glfwWaitEvents();
                cooldown = 20;
            }
            else
                --cooldown;
#endif
            // Poll and handle events (inputs, window resize, etc.)
            // You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
            // - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
            // - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
            // Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
            glfwPollEvents();

            int width, height;
            glfwGetFramebufferSize(window, &width, &height);
            layer.drawableSize = CGSizeMake(width, height);
            id<CAMetalDrawable> drawable = [layer nextDrawable];

            id<MTLCommandBuffer> commandBuffer = [commandQueue commandBuffer];
            renderPassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(clear_color[0] * clear_color[3], clear_color[1] * clear_color[3], clear_color[2] * clear_color[3], clear_color[3]);
            renderPassDescriptor.colorAttachments[0].texture = drawable.texture;
            renderPassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
            renderPassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
            id <MTLRenderCommandEncoder> renderEncoder = [commandBuffer renderCommandEncoderWithDescriptor:renderPassDescriptor];
            [renderEncoder pushDebugGroup:@"ImGui"];
            metalProvider.currentRenderEncoder = renderEncoder;

            // Start the Dear ImGui frame
            ImGui_ImplMetal_NewFrame(renderPassDescriptor);
            ImGui_ImplGlfw_NewFrame();
            ImGui::NewFrame();

            if (app && app->Gui) {
                app->Gui();
            }

            // Rendering
            ImGui::Render();
            ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), commandBuffer, renderEncoder);

            // Update and Render additional Platform Windows
            if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
            {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault();
            }

            [renderEncoder popDebugGroup];
            [renderEncoder endEncoding];

            metalProvider.currentRenderEncoder = nil;

            [commandBuffer presentDrawable:drawable];
            [commandBuffer commit];
        }
    }


    if (app && app->Cleanup) {
        app->Cleanup();
    }

    // Cleanup
    ImGui_ImplMetal_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImPlot::DestroyContext();

    ImGui::DestroyContext(); // resource loader destructor calls DestroyContext (not ideal)

    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
