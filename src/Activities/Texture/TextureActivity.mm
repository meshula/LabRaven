//
//  TextureActivityObjC.m
//  LabExcelsior
//
//  Created by Nick Porcino on 1/12/24.
//  Copyright © 2024 Nick Porcino. All rights reserved.
//

#include "Providers/Metal/MetalProvider.h"

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>
#import <simd/simd.h>

#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui.h"
#include "imgui_tex_inspect/imgui_tex_inspect.h"

namespace ImGuiTexInspect {

struct InspectorShaderConstants {
    simd::float2 textureSize;
    bool forceNearestSampling;
    float inK0;
    float inPhi;
    float inLinearBias;
    float inputGamma;
    float outK0;
    float outPhi;
    float outLinearBias;
    float outputGamma;
    float exposure; // precomputed as pow(2.0, exposure)
    float lift;
    simd::float4 grid;
    simd::float2 gridWidth;
    simd::float4x4 colorTransform;
    simd::float3 backgroundColor;
    float premultiplyAlpha;
    float disableFinalAlpha;
};

void BackEnd_SetShader(const ImDrawList*, const ImDrawCmd*, const Inspector* inspector) {
    static id<MTLRenderPipelineState> rps = nil;
    id<MTLRenderCommandEncoder> renderEncoder = nil;
    NSError* error = nil;
    while (!rps) {
        LabMetalProvider* mr = [LabMetalProvider sharedInstance];
        if (!mr)
            break;
        
        id<MTLDevice> device = mr.device;
        if (!device)
            break;

        renderEncoder = mr.currentRenderEncoder;

        id<MTLLibrary> shaderLib = [device newDefaultLibrary];
        if(!shaderLib)
        {
            NSLog(@" ERROR: Couldnt create a default shader library");
            // assert here because if the shader libary isn't loading, nothing good will happen
            break;
        }
        
        id<MTLFunction> vertexFunction = [shaderLib newFunctionWithName:@"textureMode_vertex_main"];
        if (!vertexFunction)
        {
            NSLog(@">> ERROR: Couldn't load vertex function from default library");
            break;
        }
        
        id<MTLFunction> fragmentFunction = [shaderLib newFunctionWithName:@"textureMode_fragment_main"];
        if (!fragmentFunction)
        {
            NSLog(@" ERROR: Couldn't load fragment function from default library");
            break;
        }
        
        // this vertex descriptor MUST match that in imgui_impl_metal
        // see renderPipelineStateForFramebufferDescriptor as a sanity check
        MTLVertexDescriptor* vertexDescriptor = [MTLVertexDescriptor vertexDescriptor];
        vertexDescriptor.attributes[0].offset = offsetof(ImDrawVert, pos);
        vertexDescriptor.attributes[0].format = MTLVertexFormatFloat2; // position
        vertexDescriptor.attributes[0].bufferIndex = 0;
        vertexDescriptor.attributes[1].offset = offsetof(ImDrawVert, uv);
        vertexDescriptor.attributes[1].format = MTLVertexFormatFloat2; // texCoords
        vertexDescriptor.attributes[1].bufferIndex = 0;
        vertexDescriptor.attributes[2].offset = offsetof(ImDrawVert, col);
        vertexDescriptor.attributes[2].format = MTLVertexFormatUChar4; // color
        vertexDescriptor.attributes[2].bufferIndex = 0;
        vertexDescriptor.layouts[0].stepRate = 1;
        vertexDescriptor.layouts[0].stepFunction = MTLVertexStepFunctionPerVertex;
        vertexDescriptor.layouts[0].stride = sizeof(ImDrawVert);

        MTLRenderPassDescriptor* rpd = mr.renderpassDescriptor;
        
        MTLRenderPipelineDescriptor* pipelineDescriptor = [[MTLRenderPipelineDescriptor alloc] init];
        pipelineDescriptor.vertexFunction = vertexFunction;
        pipelineDescriptor.fragmentFunction = fragmentFunction;
        pipelineDescriptor.vertexDescriptor = vertexDescriptor;
        pipelineDescriptor.rasterSampleCount = rpd.colorAttachments[0].texture.sampleCount;
        pipelineDescriptor.colorAttachments[0].pixelFormat = rpd.colorAttachments[0].texture.pixelFormat;
        pipelineDescriptor.colorAttachments[0].blendingEnabled = YES;
        pipelineDescriptor.colorAttachments[0].rgbBlendOperation = MTLBlendOperationAdd;
        pipelineDescriptor.colorAttachments[0].sourceRGBBlendFactor = MTLBlendFactorSourceAlpha;
        pipelineDescriptor.colorAttachments[0].destinationRGBBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        pipelineDescriptor.colorAttachments[0].alphaBlendOperation = MTLBlendOperationAdd;
        pipelineDescriptor.colorAttachments[0].sourceAlphaBlendFactor = MTLBlendFactorOne;
        pipelineDescriptor.colorAttachments[0].destinationAlphaBlendFactor = MTLBlendFactorOneMinusSourceAlpha;
        pipelineDescriptor.depthAttachmentPixelFormat = rpd.depthAttachment.texture.pixelFormat;
        pipelineDescriptor.stencilAttachmentPixelFormat = rpd.stencilAttachment.texture.pixelFormat;

        rps = [device newRenderPipelineStateWithDescriptor:pipelineDescriptor error:&error];
        if (error != nil) {
            rps = nil;
            NSLog(@"Error: failed to create Metal pipeline state: %@", error);
            break;
        }
    }
    
    if (!rps || !renderEncoder)
        return;
    
    const ShaderOptions* options = &inspector->CachedShaderOptions;
    static InspectorShaderConstants sc;
    sc.inK0 = options->inputK0;
    sc.inPhi = options->inputPhi;
    sc.inLinearBias = options->InputColorspace.linearBias;
    sc.inputGamma = options->InputColorspace.gamma;
    sc.outK0 = options->outputK0;
    sc.outPhi = options->outputPhi;
    sc.outLinearBias = options->OutputColorspace.linearBias;
    sc.outputGamma = options->OutputColorspace.gamma;
    sc.exposure = powf(2.0f, options->exposure);
    sc.lift = options->lift;
    sc.textureSize = {inspector->TextureSize.x, inspector->TextureSize.y};
    sc.forceNearestSampling = options->ForceNearestSampling;
    sc.grid = {options->GridColor.x, options->GridColor.y, options->GridColor.z, options->GridColor.w };
    sc.gridWidth = {options->GridWidth.x, options->GridWidth.y};
    memcpy(&sc.colorTransform, options->ColorTransform, sizeof(float) * 16);
    sc.backgroundColor = {options->BackgroundColor.x, options->BackgroundColor.y, options->BackgroundColor.z };
    sc.premultiplyAlpha = options->PremultiplyAlpha;
    sc.disableFinalAlpha = options->DisableFinalAlpha;

    [renderEncoder setFragmentBytes:&sc
                           length:sizeof(sc)
                          atIndex:2];
    [renderEncoder setRenderPipelineState:rps];
}
    
} // ImGuiTexInspect
