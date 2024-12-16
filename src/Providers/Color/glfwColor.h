#ifndef PROVIDERS_COLOR_GLFWCOLOR_H
#define PROVIDERS_COLOR_GLFWCOLOR_H
#import <AppKit/AppKit.h>

/*
    Sets the color space of the supplied GLFWwindow to the named color space.
    This should be called immediately after creating the window; eg:
    
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    GLFWindow* hdrWin = glfwCreateWindow(width, height, "HDR Wide Gamut", nullptr, nullptr);
    if (hdrWin) {
        // set up callbacks
        ...
        glfwMakeContextCurrent(mWindow);
        SetGLFWColorEnvironment(mWindow, "lin_displayp3_scene", true);
    }

    There is some confusion in the glfw issues about setting color
    spaces for windows, and much discussion revolves around setting the display
    color space. It's up to the windowing system to handle the display color
    space and GLFW does not provide a way to set the display color space. This 
    function sets the color space for the window, which is the correct way to 
    interoperate with the desktop window and display management system.

    This function does not manage any post-render color space management, such
    as can be described with an OpenColorIO configuration. It is up to the
    application to manage the color space of the rendered image, possibly using
    an OpenColorIO configuration or other color management system, and to
    ensure that the color space of the rendered image is appropriate for the
    display color space. This function is strictly responsible for setting the
    color space of the window on behalf of the desktop windowing system.

    @param window The window to set the color space for.
    @param nanoColorNamedColorSpace The named color space to set. This must be
                                  a color space named in the nanocolor library.
    @param format The framebuffer format to use.

    @return True if the color environment was set, false otherwise.
 */

enum ProviderFramebufferFormat {
    ProviderFramebufferFormat_sRGB,
    ProviderFramebufferFormat_f16SDR,
    ProviderFramebufferFormat_f16HDR
};

class GLFWwindow;
bool SetGLFWColorEnvironment(GLFWwindow* window,
                             const char* nanoColorNamedColorSpace,
                             ProviderFramebufferFormat format);
bool SetNSWindowColorEnvironment(NSWindow* nsWindow,
                                 const char* nanoColorNamedColorSpace,
                                 ProviderFramebufferFormat format);

#endif // PROVIDERS_COLOR_GLFWCOLOR_H
