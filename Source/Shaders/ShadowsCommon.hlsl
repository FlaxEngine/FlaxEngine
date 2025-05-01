// Copyright (c) Wojciech Figat. All rights reserved.

#ifndef __SHADOWS_COMMON__
#define __SHADOWS_COMMON__

#include "./Flax/Common.hlsl"

// Maximum number of directional light shadows cascaded splits
#define MaxNumCascades 4

// Set default macros if not provided
#ifndef SHADOWS_QUALITY
#define SHADOWS_QUALITY 0
#endif

// Shadow data for the light
struct ShadowData
{
    float Sharpness;
    float Fade;
    float FadeDistance;
    float NormalOffsetScale;
    float Bias;
    uint TilesCount;
    float4 CascadeSplits;
};

// Shadow projection tile data for the light
struct ShadowTileData
{
    float4 ShadowToAtlas;
    float4x4 WorldToShadow;
};

// Loads the shadow data of the light in the shadow buffer
ShadowData LoadShadowsBuffer(Buffer<float4> shadowsBuffer, uint shadowsBufferAddress)
{
    // This must match C++
    float4 vector0 = shadowsBuffer.Load(shadowsBufferAddress + 0);
    float4 vector1 = shadowsBuffer.Load(shadowsBufferAddress + 1);
    ShadowData shadow;
    uint packed0x = asuint(vector0.x);
    shadow.Sharpness = (packed0x & 0x000000ff) * (10.0f / 255.0f);
    shadow.Fade = ((packed0x & 0x0000ff00) >> 8) * (1.0f / 255.0f);
    shadow.TilesCount = ((packed0x & 0x00ff0000) >> 16);
    shadow.FadeDistance = vector0.y;
    shadow.NormalOffsetScale = vector0.z;
    shadow.Bias = vector0.w;
    shadow.CascadeSplits = vector1;
    return shadow;
}

// Loads the shadow tile data of the light in the shadow buffer
ShadowTileData LoadShadowsBufferTile(Buffer<float4> shadowsBuffer, uint shadowsBufferAddress, uint tileIndex)
{
    // This must match C++
    shadowsBufferAddress += tileIndex * 5 + 2;
    ShadowTileData tile;
    tile.ShadowToAtlas = shadowsBuffer.Load(shadowsBufferAddress + 0);
    tile.WorldToShadow[0] = shadowsBuffer.Load(shadowsBufferAddress + 1);
    tile.WorldToShadow[1] = shadowsBuffer.Load(shadowsBufferAddress + 2);
    tile.WorldToShadow[2] = shadowsBuffer.Load(shadowsBufferAddress + 3);
    tile.WorldToShadow[3] = shadowsBuffer.Load(shadowsBufferAddress + 4);
    return tile;
}

float3 GetShadowPositionOffset(float offsetScale, float NoL, float3 normal)
{
    float normalOffsetScale = saturate(1.0f - NoL);
    return normal * (offsetScale * normalOffsetScale);
}

float CalculateSubsurfaceOcclusion(float opacity, float sceneDepth, float shadowMapDepth)
{
    float thickness = max(sceneDepth - shadowMapDepth, 0);
    float occlusion = 1 - saturate(thickness * lerp(1.0f, 100.0f, opacity));
    return shadowMapDepth > 0.99f ? 1 : occlusion;
}

float PostProcessShadow(ShadowData lightShadow, float shadow)
{
    // Apply shadow fade and sharpness
    shadow = saturate((shadow - 0.51f) * lightShadow.Sharpness + 0.5f);
    shadow = lerp(1.0f, shadow, lightShadow.Fade);
    return shadow;
}

#endif
