
#import "MetalProvider.h"


id<MTLTexture> CreateRGBA8Texture(id<MTLDevice> device, int width, int height,
                                  uint8_t* pixels) {
    if (!width || !height || !pixels)
        return nil;
    
    MTLTextureDescriptor *descriptor =
            [MTLTextureDescriptor
                     texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA8Unorm
                     width:width
                     height:height
                     mipmapped:NO];
    descriptor.storageMode = MTLStorageModeShared;
    descriptor.usage = MTLTextureUsageUnknown;
    
    id<MTLTexture> texture = [device newTextureWithDescriptor:descriptor];
    if (texture == nil) {
        printf("Could not create a Metal texture of size %d x %d\n", width, height);
        return nil;
    }
    
    if (pixels) {
        [texture replaceRegion:MTLRegionMake2D(0, 0, width, height)
                   mipmapLevel:0
                     withBytes:pixels
                   bytesPerRow:width * 4];
    }
    return texture;
}

id<MTLTexture> CreateBGRA8Texture(id<MTLDevice> device, int width, int height,
                                  uint8_t* pixels) {
    if (!width || !height || !pixels)
        return nil;
    
    MTLTextureDescriptor *descriptor =
            [MTLTextureDescriptor
                     texture2DDescriptorWithPixelFormat:MTLPixelFormatBGRA8Unorm
                     width:width
                     height:height
                     mipmapped:NO];
    descriptor.storageMode = MTLStorageModeShared;
    descriptor.usage = MTLTextureUsageUnknown;
    
    id<MTLTexture> texture = [device newTextureWithDescriptor:descriptor];
    if (texture == nil) {
        printf("Could not create a Metal texture of size %d x %d\n", width, height);
        return nil;
    }
    
    if (pixels) {
        [texture replaceRegion:MTLRegionMake2D(0, 0, width, height)
                   mipmapLevel:0
                     withBytes:pixels
                   bytesPerRow:width * 4];
    }
    return texture;
}

id<MTLTexture> CreateRGBAf16Texture(id<MTLDevice> device, int width, int height,
                                  uint8_t* rgba8_pixels) {
    MTLTextureDescriptor *descriptor =
            [MTLTextureDescriptor
                     texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA16Float
                     width:width
                     height:height
                     mipmapped:NO];
    descriptor.storageMode = MTLStorageModeShared;
    descriptor.usage = MTLTextureUsageUnknown;

    id<MTLTexture> texture = [device newTextureWithDescriptor:descriptor];
    if (texture == nil) {
        printf("Could not create a Metal texture of size %d x %d\n", width, height);
        return nullptr;
    }
    
    if (rgba8_pixels) {
        [texture replaceRegion:MTLRegionMake2D(0, 0, width, height)
                   mipmapLevel:0
                     withBytes:rgba8_pixels
                   bytesPerRow:width * 8];
    }
    
    return texture;
}

id<MTLTexture> CreateRGBAf32Texture(id<MTLDevice> device, int width, int height,
                                  uint8_t* rgba8_pixels) {
    MTLTextureDescriptor *descriptor =
            [MTLTextureDescriptor
                     texture2DDescriptorWithPixelFormat:MTLPixelFormatRGBA32Float
                     width:width
                     height:height
                     mipmapped:NO];
    descriptor.storageMode = MTLStorageModeShared;
    descriptor.usage = MTLTextureUsageUnknown;

    id<MTLTexture> texture = [device newTextureWithDescriptor:descriptor];
    if (texture == nil) {
        printf("Could not create a Metal texture of size %d x %d\n", width, height);
        return nullptr;
    }
    
    if (rgba8_pixels) {
        [texture replaceRegion:MTLRegionMake2D(0, 0, width, height)
                   mipmapLevel:0
                     withBytes:rgba8_pixels
                   bytesPerRow:width * 16];
    }
    
    return texture;
}


id<MTLTexture> CreateYf32Texture(id<MTLDevice> device, int width, int height,
                                  uint8_t* rgba8_pixels) {
    MTLTextureDescriptor *descriptor =
            [MTLTextureDescriptor
                     texture2DDescriptorWithPixelFormat:MTLPixelFormatR32Float
                     width:width
                     height:height
                     mipmapped:NO];
    descriptor.storageMode = MTLStorageModeShared;
    descriptor.usage = MTLTextureUsageUnknown;

    id<MTLTexture> texture = [device newTextureWithDescriptor:descriptor];
    if (texture == nil) {
        printf("Could not create a Metal texture of size %d x %d\n", width, height);
        return nullptr;
    }
    
    if (rgba8_pixels) {
        [texture replaceRegion:MTLRegionMake2D(0, 0, width, height)
                   mipmapLevel:0
                     withBytes:rgba8_pixels
                   bytesPerRow:width * 4];
    }
    
    return texture;
}


@implementation LabMetalProvider
{
    int _idCounter;
    NSMutableDictionary<NSNumber*, id<MTLTexture>>* _textures;

    id<MTLTexture>   _depthTexture;
    int _depthWidth, _depthHeight;
    MTLPixelFormat   _depthFormat;
}

// global singleton
static LabMetalProvider* gLabMetalProvider = nil;

+ (LabMetalProvider*)sharedInstance {
    return gLabMetalProvider;
}

- (instancetype)initWithDevice:(id<MTLDevice>)device
         commandBufferPoolSize:(int) commandBufferPoolSize
                    colorSpace:(CFStringRef) colorSpace
{
    self = [super init];
    if (self) {
        self.device = device;
        self->_textures = [NSMutableDictionary<NSNumber *, id<MTLTexture>> new];
        self->_idCounter = 0;
        self->_depthTexture = nil;
        self->_depthWidth = 0;
        self->_depthHeight = 0;
        self->_depthFormat = MTLPixelFormatInvalid;
        self.textureLoader = [[MTKTextureLoader alloc] initWithDevice:device];

        self->_commandQueue = [device newCommandQueueWithMaxCommandBufferCount:
                               commandBufferPoolSize];

    }

    if (CFStringCompare(colorSpace, kCGColorSpaceACESCGLinear, 0) == kCFCompareEqualTo) {
        self.colorSpaceName = @"lin_ap1";
    }
    else if (CFStringCompare(colorSpace, kCGColorSpaceAdobeRGB1998, 0) == kCFCompareEqualTo) {
        self.colorSpaceName = @"lin_adobergb";
    }
    else if (CFStringCompare(colorSpace, kCGColorSpaceITUR_709, 0) == kCFCompareEqualTo) {
        self.colorSpaceName = @"lin_rec709";
    }
    else if (CFStringCompare(colorSpace, kCGColorSpaceGenericRGBLinear, 0) == kCFCompareEqualTo) {
        self.colorSpaceName = @"lin_rec709";
    }
    else if (CFStringCompare(colorSpace, kCGColorSpaceGenericRGB, 0) == kCFCompareEqualTo) {
        self.colorSpaceName = @"lin_rec709";
    }
    else if (CFStringCompare(colorSpace, kCGColorSpaceLinearITUR_2020, 0) == kCFCompareEqualTo) {
        self.colorSpaceName = @"lin_rec2020";
    }
    else if (CFStringCompare(colorSpace, kCGColorSpaceExtendedLinearITUR_2020, 0) == kCFCompareEqualTo) {
        self.colorSpaceName = @"lin_rec2020";
    }
    else if (CFStringCompare(colorSpace, kCGColorSpaceLinearDisplayP3, 0) == kCFCompareEqualTo) {
        self.colorSpaceName = @"lin_displayp3";
    }
    else if (CFStringCompare(colorSpace, kCGColorSpaceExtendedLinearDisplayP3, 0) == kCFCompareEqualTo) {
        self.colorSpaceName = @"lin_displayp3";
    }
    else if (CFStringCompare(colorSpace, kCGColorSpaceLinearSRGB, 0) == kCFCompareEqualTo) {
        self.colorSpaceName = @"lin_rec709";
    }
    else if (CFStringCompare(colorSpace, kCGColorSpaceExtendedLinearSRGB, 0) == kCFCompareEqualTo) {
        self.colorSpaceName = @"lin_rec709";
    }
    else if (CFStringCompare(colorSpace, kCGColorSpaceSRGB, 0) == kCFCompareEqualTo) {
        self.colorSpaceName = @"srgb_texture";
    }
    else {
        self.colorSpaceName = @"lin_rec709";
    }
    
    gLabMetalProvider = self;
    return self;
}

-(void)dealloc {
    gLabMetalProvider = nil;
}

- (void) createDefaultRenderAttachments:(bool)withDepthBuffer {
    self.renderpassDescriptor = [MTLRenderPassDescriptor new];
    self.renderpassDescriptor.colorAttachments[0].loadAction = MTLLoadActionClear;
    self.renderpassDescriptor.colorAttachments[0].storeAction = MTLStoreActionStore;
    self.renderpassDescriptor.colorAttachments[0].clearColor = MTLClearColorMake(0, 1, 1, 1);

    if (withDepthBuffer) {
        self.renderpassDescriptor.depthAttachment.loadAction = MTLLoadActionClear;
        self.renderpassDescriptor.depthAttachment.storeAction = MTLStoreActionDontCare;
        self.renderpassDescriptor.depthAttachment.clearDepth = 1.0;
    }
}

- (void) attachmentsMustClear:(bool)clear {
    self.renderpassDescriptor.colorAttachments[0].loadAction = 
                                clear ? MTLLoadActionClear : MTLLoadActionLoad;
    self.renderpassDescriptor.depthAttachment.loadAction =
                                clear ? MTLLoadActionClear : MTLLoadActionLoad;
}


- (id<MTLTexture>)DepthTexture:(int)width height:(int) height
                        format:(MTLPixelFormat) format {
    if (!width || !height)
        return nil;
    
    if (_depthWidth != width || _depthHeight != height ||
        _depthFormat != format || _depthTexture == nil) {
        MTLTextureDescriptor *depthTargetDescriptor = [MTLTextureDescriptor new];
        depthTargetDescriptor.width       = width;
        depthTargetDescriptor.height      = height;
        depthTargetDescriptor.pixelFormat = format;
        depthTargetDescriptor.storageMode = MTLStorageModePrivate;
        depthTargetDescriptor.usage       = MTLTextureUsageRenderTarget;
        _depthTexture = [self.device newTextureWithDescriptor:depthTargetDescriptor];
    }
    return _depthTexture;
}

- (void)BeginFrameToTexture:(id<MTLTexture>) texture
              viewportWidth:(int) viewportWidth
             viewportHeight:(int) viewportHeight
                depthFormat:(MTLPixelFormat) depthFormat{
    
    // Create a new command buffer for each render pass to the current drawable.
    self.commandBuffer = [_commandQueue commandBuffer];
    self.renderpassDescriptor.colorAttachments[0].texture = texture;
    static vector_float4 clear_color = {0.19f, 0.19f, 0.19f, 1.00f};
    self.renderpassDescriptor.colorAttachments[0].clearColor =
                                MTLClearColorMake(clear_color.x * clear_color.w,
                                                  clear_color.y * clear_color.w,
                                                  clear_color.z * clear_color.w,
                                                  clear_color.w);

    self.renderpassDescriptor.depthAttachment.texture =
                                    [self DepthTexture:viewportWidth
                                              height:viewportHeight
                                              format:depthFormat];

    [self attachmentsMustClear:true];

    id <MTLRenderCommandEncoder> renderEncoder =
        [self.commandBuffer renderCommandEncoderWithDescriptor:self.renderpassDescriptor];
    [renderEncoder setLabel:@"LabMetalProvider clear buffers"];
    [renderEncoder endEncoding];
}

- (void) CommitFrameToDrawable:(id<CAMetalDrawable>)drawable {
    [self.commandBuffer presentDrawable:drawable];
    [self.commandBuffer commit];
    self.commandBuffer = nil;
}

- (int)CreateRGBA8Texture:(int) width height:(int) height rgba_pixels:(uint8_t*) pixels {
    id<MTLTexture> txt = ::CreateRGBA8Texture(self.device, width, height, pixels);
    int newId = ++self->_idCounter;
    self->_textures[@(newId)] = txt;
    return newId;
}

- (int)CreateBGRA8Texture:(int) width height:(int) height bgra_pixels:(uint8_t*) pixels {
    id<MTLTexture> txt = ::CreateBGRA8Texture(self.device, width, height, pixels);
    int newId = ++self->_idCounter;
    self->_textures[@(newId)] = txt;
    return newId;
}

- (int)CreateRGBAf16Texture:(int) width height:(int) height rgba_pixels:(uint8_t*) rgba_pixels {
    id<MTLTexture> txt = ::CreateRGBAf16Texture(self.device, width, height, rgba_pixels);
    int newId = ++self->_idCounter;
    self->_textures[@(newId)] = txt;
    return newId;
}

- (int)CreateRGBAf32Texture:(int) width height:(int) height rgba_pixels:(uint8_t*) rgba_pixels {
    id<MTLTexture> txt = ::CreateRGBAf32Texture(self.device, width, height, rgba_pixels);
    int newId = ++self->_idCounter;
    self->_textures[@(newId)] = txt;
    return newId;
}

- (int)CreateYf32Texture:(int) width height:(int) height rgba_pixels:(uint8_t*) rgba_pixels {
    id<MTLTexture> txt = ::CreateYf32Texture(self.device, width, height, rgba_pixels);
    int newId = ++self->_idCounter;
    self->_textures[@(newId)] = txt;
    return newId;
}

- (int)RecordTexture:(id<MTLTexture>) texture {
    int newId = ++self->_idCounter;
    self->_textures[@(newId)] = texture;
    return newId;
}

- (id<MTLTexture>)Texture:(int)texture {
    return self->_textures[@(texture)];
}

- (void)RemoveTexture:(int)texture {
    [self->_textures removeObjectForKey:@(texture)];
}

// not publicly exposed
- (void)RemoveMTLTexture:(id<MTLTexture>)texture {
    for (NSNumber* key in self->_textures) {
        if ([self->_textures[key] isEqual:texture]) {
            [self->_textures removeObjectForKey:key];
            break;
        }
    }
}

- (int)LoadBundleTexture:(const char*)filePathCString mipmaps:(bool)mipmaps sRGB:(bool)sRGB {
    if (self.textureLoader) {
        NSString *filePath = [NSString stringWithCString:filePathCString encoding:NSUTF8StringEncoding];
        NSString *path = [filePath stringByDeletingPathExtension];
        NSString *fileExtension = [filePath pathExtension];
        NSString *resourcePath = [[NSBundle mainBundle] pathForResource:path ofType:fileExtension];
        NSData *data = [NSData dataWithContentsOfFile:resourcePath];
        
        NSDictionary *loaderOptions =
                @{
                  MTKTextureLoaderOptionGenerateMipmaps : @(mipmaps),
                  MTKTextureLoaderOptionSRGB : @(sRGB),
                  };
        
        NSError *error = nil;
        id<MTLTexture> texture = [self.textureLoader newTextureWithData:data 
                                                                options:loaderOptions error:&error];
        
        if (texture && !error) {
            int newId = ++self->_idCounter;
            self->_textures[@(newId)] = texture;
            return newId;
        }
    }
    return -1;
}

@end

// C API
// This function is used to get the texture from the texture cache and returns
// a bridged and retained id<MTLTexture> object.
void* LabGetEncodedTexture(int texture) {
    LabMetalProvider* provider = [LabMetalProvider sharedInstance];
    if (provider == nil) {
        return nil;
    }
    return (__bridge_retained void*)[provider Texture:texture];
}

// This function is used to remove a retain count from the id<MTLTexture> object
// and remove the texture from the texture cache if the count is zero.
void LabReleaseEncodedTexture(void* texture) {
    id<MTLTexture> txt = (__bridge_transfer id<MTLTexture>)texture;
    LabMetalProvider* provider = [LabMetalProvider sharedInstance];
    if (!txt || !provider) {
        return;
    }

    CFIndex retainCount = CFGetRetainCount((__bridge CFTypeRef)txt);
    if (retainCount > 1) {
        return;
    }
    [provider RemoveMTLTexture:txt];
}


extern "C"
int LabCreateRGBA8Texture(int width, int height, uint8_t* pixels) {
    if (gLabMetalProvider)
        return [gLabMetalProvider CreateRGBA8Texture:width height:height rgba_pixels:pixels];
    return 0;
}

extern "C"
int LabCreateBGRA8Texture(int width, int height, uint8_t* pixels) {
    if (gLabMetalProvider)
        return [gLabMetalProvider CreateBGRA8Texture:width height:height bgra_pixels:pixels];
    return 0;
}

extern "C"
void LabUpdateRGBA8Texture(int texture, uint8_t* pixels) {
    if (gLabMetalProvider) {
        id<MTLTexture> txt = [gLabMetalProvider Texture:texture];
        if (txt) {
            [txt replaceRegion:MTLRegionMake2D(0, 0, txt.width, txt.height)
                   mipmapLevel:0
                     withBytes:pixels
                   bytesPerRow:txt.width * 4];
        }
    }
}

extern "C"
void LabUpdateBGRA8Texture(int texture, uint8_t* pixels) {
    LabUpdateRGBA8Texture(texture, pixels);
}

extern "C"
int LabCreateRGBAf16Texture(int width, int height, uint8_t* rgba_pixels) {
    if (gLabMetalProvider)
        return [gLabMetalProvider CreateRGBAf16Texture:width height:height rgba_pixels:rgba_pixels];
    return 0;
}

extern "C"
int LabCreateRGBAf32Texture(int width, int height, uint8_t* rgba_pixels) {
    if (gLabMetalProvider)
        return [gLabMetalProvider CreateRGBAf32Texture:width height:height rgba_pixels:rgba_pixels];
    return 0;
}

extern "C"
int LabCreateYf32Texture(int width, int height, uint8_t* rgba_pixels) {
    if (gLabMetalProvider)
        return [gLabMetalProvider CreateYf32Texture:width height:height rgba_pixels:rgba_pixels];
    return 0;
}

extern "C"
void LabRemoveTexture(int texture) {
    if (gLabMetalProvider)
        [gLabMetalProvider RemoveTexture:texture];
}

extern "C"
void* LabTextureHardwareHandle(int texture) {
    if (!gLabMetalProvider)
        return nullptr;
    
    id<MTLTexture> tx = [gLabMetalProvider Texture:texture];
    return (__bridge void*)tx;
}
