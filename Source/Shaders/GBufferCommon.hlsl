// Copyright (c) Wojciech Figat. All rights reserved.

#ifndef __GBUFFER_COMMON__
#define __GBUFFER_COMMON__

#include "./Flax/Common.hlsl"

// GBuffer sample data structure
struct GBufferSample
{
    float3 Normal;
    float Roughness;
    float3 WorldPos;
    float Metalness;
    float3 Color;
    float Specular;
    float3 ViewPos;
    float AO;
    int ShadingModel;
#if defined(USE_GBUFFER_CUSTOM_DATA)
	float4 CustomData;
#endif
};

bool IsSubsurfaceMode(int shadingModel)
{
    return shadingModel == SHADING_MODEL_SUBSURFACE || shadingModel == SHADING_MODEL_FOLIAGE;
}

float3 GetDiffuseColor(in float3 color, in float metalness)
{
    return color - color * metalness;
}

float3 GetSpecularColor(in float3 color, in float specular, in float metalness)
{
    return lerp(0.08 * specular.xxx, color.rgb, metalness.xxx);
}

// Calculate material diffuse color
float3 GetDiffuseColor(in GBufferSample gBuffer)
{
    return gBuffer.Color - gBuffer.Color * gBuffer.Metalness;
}

// Calculate material specular color
float3 GetSpecularColor(in GBufferSample gBuffer)
{
    return lerp(0.08 * gBuffer.Specular.xxx, gBuffer.Color.rgb, gBuffer.Metalness.xxx);
}

// Compact Normal Storage for Small G-Buffers
// [Aras Pranckevièius 2010, http://aras-p.info/texts/CompactNormalStorage.html]
float3 EncodeNormal(float3 n)
{
    // Pack to [0;1] range
    return n * 0.5 + 0.5;
}

// Compact Normal Storage for Small G-Buffers
// [Aras Pranckevièius 2010, http://aras-p.info/texts/CompactNormalStorage.html]
float3 DecodeNormal(float3 enc)
{
    // Unpack from [0;1] range
    return normalize(enc * 2 - 1);
}

#endif
