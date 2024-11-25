//
//  ImageInspector.metal
//  LabExcelsior
//
//  Created by Nick Porcino on 1/12/24.
//  Copyright © 2024 Nick Porcino. All rights reserved.
//

#include <metal_stdlib>
using namespace metal;

struct Uniforms {
    float4x4 projectionMatrix;
};

struct VertexIn {
    float2 position  [[attribute(0)]];
    float2 texCoords [[attribute(1)]];
    uchar4 color     [[attribute(2)]];
};

struct VertexOut {
    float4 position [[position]];
    float2 texCoords;
    float4 color;
};

vertex VertexOut vertex_main(VertexIn in                 [[stage_in]],
                             constant Uniforms &uniforms [[buffer(1)]]) {
    VertexOut out;
    out.position = uniforms.projectionMatrix * float4(in.position, 0, 1);
    out.texCoords = in.texCoords;
    out.color = float4(in.color) / float4(255.0);
    return out;
}

// metal shader that draws a grid over the image
fragment half4 fragment_main(VertexOut in [[stage_in]],
                             texture2d<half, access::sample> texture [[texture(0)]],
                             constant float2 &textureSize [[buffer(0)]],
                             constant bool &forceNearestSampling [[buffer(1)]],
                             constant float4 &grid [[buffer(2)]],
                             constant float2 &gridWidth [[buffer(3)]],
                             constant float4x4 &colorTransform [[buffer(4)]],
                             constant float4 &colorOffset [[buffer(5)]],
                             constant float3 &backgroundColor [[buffer(6)]],
                             constant float &premultiplyAlpha [[buffer(7)]],
                             constant float &disableFinalAlpha [[buffer(8)]]
                             ) {
    constexpr sampler linearSampler(coord::normalized, min_filter::linear, mag_filter::linear, mip_filter::linear);
    float2 uv = in.texCoords * textureSize;
    if (forceNearestSampling)
        uv = (floor(uv) + float2(0.5,0.5)) / textureSize;
    float4 texColor = colorTransform * float4(texture.sample(linearSampler, uv)) + colorOffset;
    texColor.rgb = texColor.rgb * mix(1.0, texColor.a, premultiplyAlpha);
    texColor.rgb += backgroundColor * (1.0-texColor.a);
    texColor.a = mix(texColor.a, 1.0, disableFinalAlpha);

    float2 texel = in.texCoords * float2(texture.get_width(), texture.get_height());
    float2 texelEdge = step(fmod(texel, float2(1.0,1.0)), gridWidth);
    float isGrid = max(texelEdge.x, texelEdge.y);
    return half4(in.color) * half4(mix(texColor, float4(grid.rgb, 1), grid.a * isGrid));
}
