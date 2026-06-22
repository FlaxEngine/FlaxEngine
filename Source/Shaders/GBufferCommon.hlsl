// Copyright (c) Wojciech Figat. All rights reserved.

#ifndef __GBUFFER_COMMON__
#define __GBUFFER_COMMON__

#include "./Flax/Common.hlsl"

// Structure that contains information about GBuffer
struct GBufferData
{
    // If reverse Z enabled:
    //     x-1/Projection[0,0], y-1/Projection[1,1], z-(-Near / (Far - Near)), w-((Far * Near) / (Far - Near) / Far)
    // Otherwise:
    //     x-1/Projection[0,0], y-1/Projection[1,1], z-(Far / (Far - Near)), w-(-(Far * Near) / (Far - Near) / Far)
    float4 ViewInfo;
    float4 ScreenSize; // x-Width, y-Height, z-1/Width, w-1/Height
    float3 ViewPos; // view position (in world space)
    float ViewFar; // view far plane distance (in world space)
    float4x4 InvViewMatrix; // inverse view matrix (4 rows by 4 columns)
    float4x4 InvProjectionMatrix; // inverse projection matrix (4 rows by 4 columns)
};

#ifdef PLATFORM_ANDROID
// #AdrenoVK_CB_STRUCT_MEMBER_ACCESS_BUG
#define DECLARE_GBUFFERDATA_ACCESS(uniformName) GBufferData Get##uniformName##Data() { GBufferData tmp; tmp.ViewInfo = uniformName.ViewInfo; tmp.ScreenSize = uniformName.ScreenSize; tmp.ViewPos = uniformName.ViewPos; tmp.ViewFar = uniformName.ViewFar; tmp.InvViewMatrix = uniformName.InvViewMatrix; tmp.InvProjectionMatrix = uniformName.InvProjectionMatrix; return tmp; }
#else
#define DECLARE_GBUFFERDATA_ACCESS(uniformName) GBufferData Get##uniformName##Data() { return uniformName; }
#endif

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

float3 GetDiffuseColor(float3 color, float metalness)
{
    return color * (1.0 - metalness);
}

// [https://google.github.io/filament/Filament.md.html]
float3 GetSpecularColor(float3 color, float specular, float metalness)
{
    float dielectricF0 = 0.16 * specular * specular;
    return lerp(dielectricF0.xxx, color, metalness.xxx);
}

// Calculate material diffuse color
float3 GetDiffuseColor(GBufferSample gBuffer)
{
    return GetDiffuseColor(gBuffer.Color, gBuffer.Metalness);
}

// Calculate material specular color
float3 GetSpecularColor(GBufferSample gBuffer)
{
    return GetSpecularColor(gBuffer.Color, gBuffer.Specular, gBuffer.Metalness);
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
