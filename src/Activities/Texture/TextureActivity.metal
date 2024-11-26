//
//  TextureActivityShaders.metal
//  LabExcelsior
//
//  Created by Domenico Porcino on 7/3/24.
//  Copyright © 2024 Nick Porcino. All rights reserved.
//
#include <metal_stdlib>
using namespace metal;

// set up by Dear ImGui
struct Uniforms {
    float4x4 projectionMatrix;
};

struct InspectorShaderConstants {
    float2 textureSize;
    bool forceNearestSampling;
    float inK0;
    float inPhi;
    float inLinearBias;
    float inputGamma;
    float outK0;
    float outPhi;
    float outLinearBias;
    float outputGamma;
    float exposure;
    float lift;
    float4 grid;
    float2 gridWidth;
    float4x4 colorTransform;
    float3 backgroundColor;
    float premultiplyAlpha;
    float disableFinalAlpha;
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


float3 nc_ToLinear(float3 c, float gamma, float K0, float phi, float linearBias) {
    return pow(c, gamma);
    float t = c.r;
    if (t < K0)
        c.r = t / phi;
    else
        c.r = pow((t + linearBias) / (1.f + linearBias), gamma);
    t = c.g;
    if (t < K0)
        c.g = t / phi;
    else
        c.g = pow((t + linearBias) / (1.f + linearBias), gamma);
    t = c.b;
    if (t < K0)
        c.b = t / phi;
    else
        c.b = pow((t + linearBias) / (1.f + linearBias), gamma);
    return c;
}

inline float3 nc_FromLinear(float3 c, float gamma, float K0, float phi, float linearBias) {
    //return pow(c, 1.0/gamma);
    float t = c.r;
    if (t < K0 / phi)
        c.r = t * phi;
    else
        c.r = (1.f + linearBias) * pow(t, 1.f / gamma) - linearBias;
    t = c.g;
    if (t < K0 / phi)
        c.g = t * phi;
    else
        c.g = (1.f + linearBias) * pow(t, 1.f / gamma) - linearBias;
    t = c.b;
    if (t < K0 / phi)
        c.b = t * phi;
    else
        c.b = (1.f + linearBias) * pow(t, 1.f / gamma) - linearBias;
    return c;
}

vertex VertexOut textureMode_vertex_main(VertexIn in                 [[stage_in]],
                             constant Uniforms &uniforms [[buffer(1)]]) {
    VertexOut out;
    out.position = uniforms.projectionMatrix * float4(in.position, 0, 1);
    out.texCoords = in.texCoords;
    out.color = float4(in.color) / float4(255.0);
    return out;
}

// metal shader that draws a grid over the image
fragment half4 textureMode_fragment_main(VertexOut in [[stage_in]],
                             texture2d<half, access::sample> texture [[texture(0)]],
                             constant InspectorShaderConstants& isc [[buffer(2)]]
                             )
{
    constexpr sampler linearSampler(coord::normalized, min_filter::linear, mag_filter::linear, mip_filter::linear);
    float2 uv;
    if (isc.forceNearestSampling) {
        uv = (floor(in.texCoords * isc.textureSize) + float2(0.5,0.5)) / isc.textureSize;
    }
    else {
        uv = in.texCoords;
    }
    float3 c_lin = nc_ToLinear(float3(texture.sample(linearSampler, uv).rgb),
                               isc.inputGamma, isc.inK0, isc.inPhi, isc.inLinearBias);
    float3 texColor_lin = (isc.colorTransform * float4(c_lin, 1.0)).rgb;
    float4 texColor = float4(nc_FromLinear(texColor_lin,
                               isc.outputGamma, isc.outK0, isc.outPhi, isc.outLinearBias), 1.0);

    texColor.rgb = texColor.rgb * mix(1.0, texColor.a, isc.premultiplyAlpha) * isc.exposure + isc.lift;
    texColor.rgb += isc.backgroundColor * (1.0-texColor.a);
    texColor.a = mix(texColor.a, 1.0, isc.disableFinalAlpha);

    float2 texel = in.texCoords * float2(texture.get_width(), texture.get_height());
    float2 texelEdge = step(fmod(texel, float2(1.0,1.0)), isc.gridWidth);
    float isGrid = max(texelEdge.x, texelEdge.y);
    return half4(in.color) * half4(mix(texColor, float4(isc.grid.rgb, 1), isc.grid.a * isGrid));
}

