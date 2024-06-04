// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#ifndef __SHADOWS_COMMON__
#define __SHADOWS_COMMON__

#include "./Flax/Common.hlsl"

// Maximum number of directional light shadows cascaded splits
#define MaxNumCascades 4

// Set default macros if not provided
#ifndef SHADOWS_QUALITY
#define SHADOWS_QUALITY 0
#endif
#ifndef CSM_BLENDING
#define CSM_BLENDING 0
#endif

// Structure that contains information about light
struct LightShadowData
{
    float2 ShadowMapSize;
    float Sharpness;
    float Fade;

    float NormalOffsetScale;
    float Bias;
    float FadeDistance;
    uint NumCascades;

    float4 CascadeSplits;
    float4x4 ShadowVP[6];
};

#ifdef PLATFORM_ANDROID
// #AdrenoVK_CB_STRUCT_MEMBER_ACCESS_BUG
#define DECLARE_LIGHTSHADOWDATA_ACCESS(uniformName) LightShadowData Get##uniformName##Data() { LightShadowData tmp; tmp.ShadowMapSize = uniformName.ShadowMapSize; tmp.Sharpness = uniformName.Sharpness; tmp.Fade = uniformName.Fade; tmp.NormalOffsetScale = uniformName.NormalOffsetScale; tmp.Bias = uniformName.Bias; tmp.FadeDistance = uniformName.FadeDistance; tmp.NumCascades = uniformName.NumCascades; tmp.CascadeSplits = uniformName.CascadeSplits; tmp.ShadowVP[0] = uniformName.ShadowVP[0]; tmp.ShadowVP[1] = uniformName.ShadowVP[1]; tmp.ShadowVP[2] = uniformName.ShadowVP[2]; tmp.ShadowVP[3] = uniformName.ShadowVP[3]; tmp.ShadowVP[4] = uniformName.ShadowVP[4]; tmp.ShadowVP[5] = uniformName.ShadowVP[5]; return tmp; }
#else
#define DECLARE_LIGHTSHADOWDATA_ACCESS(uniformName) LightShadowData Get##uniformName##Data() { return uniformName; }
#endif

float3 GetShadowPositionOffset(float offsetScale, float NoL, float3 normal)
{
    float normalOffsetScale = saturate(1.0f - NoL);
    return normal * (offsetScale * normalOffsetScale);
}

float CalculateSubsurfaceOcclusion(float opacity, float sceneDepth, float shadowMapDepth)
{
    float thickness = max(shadowMapDepth - sceneDepth, 0);
    float occlusion = 1 - thickness * lerp(1.0f, 100.0f, opacity);
    return shadowMapDepth < 0.01f ? 1 : occlusion;
}

#endif
