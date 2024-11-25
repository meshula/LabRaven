
#include "glfwColor.h"

#define GLFW_EXPOSE_NATIVE_COCOA
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>

/*
 The named color spaces provided by Nanocolor are as follows.
 Note that the names are shared with libraries such as MaterialX.

 - acescg:           The Academy Color Encoding System, a color space designed
                     for cinematic content creation and exchange, using AP1 primaries
 - adobergb:         A color space developed by Adobe Systems. It has a wider gamut
                     than sRGB and is suitable for photography and printing
 - g18_ap1:          A color space with a 1.8 gamma and an AP1 primaries color gamut
 - g18_rec709:       A color space with a 1.8 gamma, and primaries per the Rec. 709
                     standard, commonly used in HDTV
 - g22_ap1:          A color space with a 2.2 gamma and an AP1 primaries color gamut
 - g22_rec709:       A color space with a 2.2 gamma, and primaries per the Rec. 709
                     standard, commonly used in HDTV
 - identity:         Indicates that no transform is applied.
 - lin_adobergb:     The AdobeRGB gamut, and linear gamma
 - lin_ap0:          AP0 primaries, and linear gamma
 - lin_ap1:          AP1 primaries, and linear gamma; these are the ACESCg primaries
 - lin_displayp3:    DisplayP3 gamut, and linear gamma
 - lin_rec709:       A linearized version of the Rec. 709 color space.
 - lin_rec2020:      Rec2020 gamut, and linear gamma
 - lin_srgb:         sRGB gamut, linear gamma
 - raw:              Indicates that no transform is applied.
 - srgb_displayp3:   sRGB color space adapted to the Display P3 primaries
 - sRGB:             The sRGB color space.
 - srgb_texture:     The sRGB color space.
*/

// convert a named color space to a CFStringRef naming a cocoa color space. Both
// the CIF names with the _scene suffix and the shortened names are valid here; 
// ie. "lin_ap1" and "lin_ap1_scene" are both valid.
static CFStringRef NanocolorToCGColorSpace(const char* nanocolor) {
    if (strcmp(nanocolor, "acescg") == 0 || strcmp(nanocolor, "acescg_scene") == 0) {
        return kCGColorSpaceACESCGLinear;
    }
    if (strcmp(nanocolor, "adobergb") == 0 || strcmp(nanocolor, "adobergb_scene") == 0) {
        return kCGColorSpaceAdobeRGB1998;
    }
    if (strcmp(nanocolor, "g18_ap1") == 0 || strcmp(nanocolor, "g18_ap1_scene") == 0) {
        // spaces requiring gamma are not supported.
        return nil;
    }
    if (strcmp(nanocolor, "g18_rec709") == 0 || strcmp(nanocolor, "g18_rec709_scene") == 0) {
        // spaces requiring gamma are not supported.
        return nil;
    }
    if (strcmp(nanocolor, "g22_ap1") == 0 || strcmp(nanocolor, "g22_ap1_scene") == 0) {
        // spaces requiring gamma are not supported.
        return nil;
    }
    if (strcmp(nanocolor, "g22_rec709") == 0 || strcmp(nanocolor, "g22_rec709_scene") == 0) {
        // spaces requiring gamma are not supported.
        return nil;
    }
    if (strcmp(nanocolor, "identity") == 0 || strcmp(nanocolor, "identity_scene") == 0) {
        return kCGColorSpaceGenericRGBLinear;
    }
    if (strcmp(nanocolor, "lin_adobergb") == 0 || strcmp(nanocolor, "lin_adobergb_scene") == 0) {
        return kCGColorSpaceAdobeRGB1998;
    }
    if (strcmp(nanocolor, "lin_ap0") == 0 || strcmp(nanocolor, "lin_ap0_scene") == 0) {
        // it's not possible to set a color space with AP0 primaries.
        return nil;
    }
    if (strcmp(nanocolor, "lin_ap1") == 0 || strcmp(nanocolor, "lin_ap1_scene") == 0) {
        return kCGColorSpaceACESCGLinear;
    }
    if (strcmp(nanocolor, "lin_displayp3") == 0 || strcmp(nanocolor, "lin_displayp3_scene") == 0) {
        return kCGColorSpaceDisplayP3;
    }
    if (strcmp(nanocolor, "lin_rec709") == 0 || strcmp(nanocolor, "lin_rec709_scene") == 0) {
        return kCGColorSpaceGenericRGBLinear;
    }
    if (strcmp(nanocolor, "lin_rec2020") == 0 || strcmp(nanocolor, "lin_rec2020_scene") == 0) {
        return kCGColorSpaceITUR_2020;
    }
    if (strcmp(nanocolor, "lin_srgb") == 0 || strcmp(nanocolor, "lin_srgb_scene") == 0) {
        return kCGColorSpaceSRGB;
    }
    if (strcmp(nanocolor, "raw") == 0 || strcmp(nanocolor, "raw_scene") == 0) {
        return kCGColorSpaceGenericRGBLinear;
    }
    if (strcmp(nanocolor, "srgb_displayp3") == 0 || strcmp(nanocolor, "srgb_displayp3_scene") == 0) {
        // spaces requiring gamma are not supported.
        return nil;
    }
    if (strcmp(nanocolor, "sRGB") == 0 || strcmp(nanocolor, "sRGB_scene") == 0) {
        return kCGColorSpaceSRGB;
    }
    if (strcmp(nanocolor, "srgb_texture") == 0 || strcmp(nanocolor, "srgb_texture_scene") == 0) {
        return kCGColorSpaceSRGB;
    }
    return nil;
}

bool SetGLFWColorEnvironment(GLFWwindow* glfWindow,
                             const char* nanoColorNamedColorSpace,
                             ProviderFramebufferFormat format) {
    NSWindow* nsWindow = (NSWindow*)glfwGetCocoaWindow(glfWindow);
    if (!nsWindow) {
        return false;
    }

    NSView* contentView = nsWindow.contentView;
    if (!contentView) {
        return false;
    }

    CALayer* layer = contentView.layer;
    if (![layer isKindOfClass:[CAMetalLayer class]]) {
        return false;
    }

    CAMetalLayer* metalLayer = (CAMetalLayer*)layer;

    switch (format) {
        case ProviderFramebufferFormat_sRGB:
            metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm_sRGB;
            layer.wantsExtendedDynamicRangeContent = NO;
            break;
        case ProviderFramebufferFormat_f16SDR:
            metalLayer.pixelFormat = MTLPixelFormatRGBA16Float;
            layer.wantsExtendedDynamicRangeContent = NO;
            break;
        case ProviderFramebufferFormat_f16HDR:
            metalLayer.pixelFormat = MTLPixelFormatRGBA16Float;
            layer.wantsExtendedDynamicRangeContent = YES;
            break;
        default:
            return false;
    }

    const CFStringRef name = NanocolorToCGColorSpace(nanoColorNamedColorSpace);
    if (!name) {
        return false;
    }

    CGColorSpaceRef colorspace = CGColorSpaceCreateWithName(name);
    metalLayer.colorspace = colorspace;
    CGColorSpaceRelease(colorspace);
    CFRelease(name);
}
