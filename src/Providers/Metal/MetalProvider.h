//
//  MetalProvider.h
//  LabExcelsior
//
//  Created by Domenico Porcino on 1/8/24.
//  Copyright © 2024 Nick Porcino. All rights reserved.
//

#ifndef MetalProvider_h
#define MetalProvider_h

#ifdef __cplusplus
#define LMR_EXTERN extern "C"
#else
#define LMR_EXTERN extern
#endif

#if defined(__OBJC__)

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <MetalKit/MetalKit.h>

@interface LabMetalProvider : NSObject

// global singleton
+ (LabMetalProvider*)sharedInstance;

@property (nonatomic, strong) id<MTLDevice> device;
@property (nonatomic, strong) MTKTextureLoader *textureLoader;
@property (nonatomic, strong) MTLRenderPassDescriptor *renderpassDescriptor;
@property (nonatomic, strong) id<MTLCommandQueue> commandQueue;
@property (nonatomic, strong) id<MTLCommandBuffer> commandBuffer;
@property (nonatomic, strong) id<MTLRenderCommandEncoder> currentRenderEncoder;

// this property is set to match the viewport in order that
// LabCreateTextureMatchingViewportFormat may function
@property (nonatomic) MTLPixelFormat drawableFormat;

@property (nonatomic, strong) NSString* colorSpaceName;

- (instancetype)initWithDevice:(id<MTLDevice>)device
         commandBufferPoolSize:(int) commandBufferPoolSize
                    colorSpace:(CFStringRef) colorSpace;
- (void) createDefaultRenderAttachments:(bool)withDepthBuffer;
- (void) attachmentsMustClear:(bool)clear;
- (void) BeginFrameToTexture:(id<MTLTexture>) texture
               viewportWidth:(int) viewportWidth
              viewportHeight:(int) viewportHeight
                 depthFormat:(MTLPixelFormat) depthFormat;
- (void) CommitFrameToDrawable:(id<CAMetalDrawable>) drawable;


- (int)CreateRGBA8Texture:(int)width height:(int) height rgba_pixels:(uint8_t*) rgba_pixels;
- (int)CreateRGBAf16Texture:(int)width height:(int) height rgba_pixels:(uint8_t*) rgba_pixels;
- (int)CreateRGBAf32Texture:(int)width height:(int) height rgba_pixels:(uint8_t*) rgba_pixels;
- (int)CreateYf32Texture:(int)width height:(int) height rgba_pixels:(uint8_t*) rgba_pixels;
- (id<MTLTexture>)Texture:(int)texture;
- (void)RemoveTexture:(int)texture;
- (int)LoadBundleTexture:(const char*)path mipmaps:(bool)mipmaps sRGB:(bool)sRGB;

// returns the shared DepthTexture, which is recreated if w/h/f differ
- (id<MTLTexture>)DepthTexture:(int)width height:(int) height
                         format:(MTLPixelFormat) format;
@end

#endif // __OBJC__

void* LabGetEncodedTexture(int texture);
void LabReleaseEncodedTexture(void* texture);

#endif /* MetalResources_h */