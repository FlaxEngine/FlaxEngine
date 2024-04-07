// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#ifndef __SHADOWS_SAMPLING__
#define __SHADOWS_SAMPLING__

#include "./Flax/ShadowsCommon.hlsl"
#include "./Flax/GBufferCommon.hlsl"
#include "./Flax/LightingCommon.hlsl"
#ifdef SHADOWS_CSM_BLENDING
#include "./Flax/Random.hlsl"
#endif

#if FEATURE_LEVEL >= FEATURE_LEVEL_SM5
#define SAMPLE_SHADOW_MAP(shadowMap, shadowUV, sceneDepth) shadowMap.SampleCmpLevelZero(ShadowSamplerLinear, shadowUV, sceneDepth)
#define SAMPLE_SHADOW_MAP_OFFSET(shadowMap, shadowUV, texelOffset, sceneDepth) shadowMap.SampleCmpLevelZero(ShadowSamplerLinear, shadowUV, sceneDepth, texelOffset)
#else
#define SAMPLE_SHADOW_MAP(shadowMap, shadowUV, sceneDepth) (sceneDepth < shadowMap.SampleLevel(SamplerLinearClamp, shadowUV, 0).r)
#define SAMPLE_SHADOW_MAP_OFFSET(shadowMap, shadowUV, texelOffset, sceneDepth) (sceneDepth < shadowMap.SampleLevel(SamplerLinearClamp, shadowUV, 0, texelOffset).r)
#endif

float4 GetShadowMask(ShadowSample shadow)
{
    return float4(shadow.SurfaceShadow, shadow.TransmissionShadow, 1, 1);
}

// Gets the cube texture face index to use for shadow map sampling for the given view-to-light direction vector
// Where: direction = normalize(worldPosition - lightPosition)
uint GetCubeFaceIndex(float3 direction)
{
    uint cubeFaceIndex;
    float3 absDirection = abs(direction);
    float maxDirection = max(absDirection.x, max(absDirection.y, absDirection.z));
    if (maxDirection == absDirection.x)
        cubeFaceIndex = absDirection.x == direction.x ? 0 : 1;
    else if (maxDirection == absDirection.y)
        cubeFaceIndex = absDirection.y == direction.y ? 2 : 3;
    else
        cubeFaceIndex = absDirection.z == direction.z ? 4 : 5;
    return cubeFaceIndex;
}

float2 GetLightShadowAtlasUV(ShadowData shadow, ShadowTileData shadowTile, float3 samplePosition, out float4 shadowPosition)
{
    // Project into shadow space (WorldToShadow is pre-multiplied to convert Clip Space to UV Space)
    shadowPosition = mul(float4(samplePosition, 1.0f), shadowTile.WorldToShadow);
    shadowPosition.z -= shadow.Bias;
    shadowPosition.xyz /= shadowPosition.w;

    // UV Space -> Atlas Tile UV Space
    float2 shadowMapUV = saturate(shadowPosition.xy);
    shadowMapUV = shadowMapUV * shadowTile.ShadowToAtlas.xy + shadowTile.ShadowToAtlas.zw;
    return shadowMapUV;
}

float SampleShadowMap(Texture2D<float> shadowMap, float2 shadowMapUV, float sceneDepth)
{
    // Single hardware sample with filtering
    float result = SAMPLE_SHADOW_MAP(shadowMap, shadowMapUV, sceneDepth);
    
#if SHADOWS_QUALITY == 1
	result += SAMPLE_SHADOW_MAP_OFFSET(shadowMap, shadowMapUV, int2(-1, 0), sceneDepth);
	result += SAMPLE_SHADOW_MAP_OFFSET(shadowMap, shadowMapUV, int2(0, -1), sceneDepth);
	result += SAMPLE_SHADOW_MAP_OFFSET(shadowMap, shadowMapUV, int2(0, 1), sceneDepth);
	result += SAMPLE_SHADOW_MAP_OFFSET(shadowMap, shadowMapUV, int2(1, 0), sceneDepth);
	result = result * (1.0f / 4.0);
#elif SHADOWS_QUALITY == 2 || SHADOWS_QUALITY == 3
    // TODO: implement Percentage-Closer Soft Shadows (PCSS) for Ultra quality
	result += SAMPLE_SHADOW_MAP_OFFSET(shadowMap, shadowMapUV, int2(-1, -1), sceneDepth);
	result += SAMPLE_SHADOW_MAP_OFFSET(shadowMap, shadowMapUV, int2(-1, 0), sceneDepth);
	result += SAMPLE_SHADOW_MAP_OFFSET(shadowMap, shadowMapUV, int2(-1, 1), sceneDepth);
	result += SAMPLE_SHADOW_MAP_OFFSET(shadowMap, shadowMapUV, int2(0, -1), sceneDepth);
	result += SAMPLE_SHADOW_MAP_OFFSET(shadowMap, shadowMapUV, int2(0, 1), sceneDepth);
	result += SAMPLE_SHADOW_MAP_OFFSET(shadowMap, shadowMapUV, int2(1, -1), sceneDepth);
	result += SAMPLE_SHADOW_MAP_OFFSET(shadowMap, shadowMapUV, int2(1, 0), sceneDepth);
	result += SAMPLE_SHADOW_MAP_OFFSET(shadowMap, shadowMapUV, int2(1, 1), sceneDepth);
	result = result * (1.0f / 9.0);
#endif

    return result;
}

// Samples the shadow for the given directional light on the material surface (supports subsurface shadowing)
ShadowSample SampleDirectionalLightShadow(LightData light, Buffer<float4> shadowsBuffer, Texture2D<float> shadowMap, GBufferSample gBuffer, float dither = 0.0f)
{
#if !LIGHTING_NO_DIRECTIONAL
    // Skip if surface is in a full shadow
    float NoL = dot(gBuffer.Normal, light.Direction);
    BRANCH
    if (NoL <= 0
#if defined(USE_GBUFFER_CUSTOM_DATA)
        && !IsSubsurfaceMode(gBuffer.ShadingModel)
#endif
        )
        return (ShadowSample)0;
#endif

    ShadowSample result;
    result.SurfaceShadow = 1;
    result.TransmissionShadow = 1;
    
    // Load shadow data
    if (light.ShadowsBufferAddress == 0)
        return result; // No shadow assigned
    ShadowData shadow = LoadShadowsBuffer(shadowsBuffer, light.ShadowsBufferAddress);

    // Create a blend factor which is one before and at the fade plane
    float viewDepth = gBuffer.ViewPos.z;
    float fade = saturate((viewDepth - shadow.CascadeSplits[shadow.TilesCount - 1] + shadow.FadeDistance) / shadow.FadeDistance);
    BRANCH
    if (fade >= 1.0)
        return result;

    // Figure out which cascade to sample from
    uint cascadeIndex = 0;
    for (uint i = 0; i < shadow.TilesCount - 1; i++)
    {
        if (viewDepth > shadow.CascadeSplits[i])
            cascadeIndex = i + 1;
    }
#ifdef SHADOWS_CSM_BLENDING
	const float BlendThreshold = 0.05f;
	float nextSplit = shadow.CascadeSplits[cascadeIndex];
	float splitSize = cascadeIndex == 0 ? nextSplit : nextSplit - shadow.CascadeSplits[cascadeIndex - 1];
	float splitDist = (nextSplit - viewDepth) / splitSize;
    if (splitDist <= BlendThreshold && cascadeIndex != shadow.TilesCount - 1)
    {
        // Blend with the next cascade but with screen-space dithering (gets cleaned out by TAA)
        float lerpAmount = 1 - splitDist / BlendThreshold;
        if (step(RandN2(gBuffer.ViewPos.xy + dither).x, lerpAmount))
            cascadeIndex++;
    }
#endif
    ShadowTileData shadowTile = LoadShadowsBufferTile(shadowsBuffer, light.ShadowsBufferAddress, cascadeIndex);

    float3 samplePosition = gBuffer.WorldPos;
#if !LIGHTING_NO_DIRECTIONAL
    // Apply normal offset bias
    samplePosition += GetShadowPositionOffset(shadow.NormalOffsetScale, NoL, gBuffer.Normal);
#endif

    // Project position into shadow atlas UV
    float4 shadowPosition;
    float2 shadowMapUV = GetLightShadowAtlasUV(shadow, shadowTile, samplePosition, shadowPosition);

    // Sample shadow map
    result.SurfaceShadow = SampleShadowMap(shadowMap, shadowMapUV, shadowPosition.z);

    // Increase the sharpness for higher cascades to match the filter radius
    //const float SharpnessScale[MaxNumCascades] = { 1.0f, 1.5f, 3.0f, 3.5f };
    //shadow.Sharpness *= SharpnessScale[cascadeIndex];
    
#if defined(USE_GBUFFER_CUSTOM_DATA)
	// Subsurface shadowing
	BRANCH
	if (IsSubsurfaceMode(gBuffer.ShadingModel))
	{
		float opacity = gBuffer.CustomData.a;
        shadowMapUV = GetLightShadowAtlasUV(shadow, shadowTile, gBuffer.WorldPos, shadowPosition);
		float shadowMapDepth = shadowMap.SampleLevel(SamplerLinearClamp, shadowMapUV, 0).r;
		result.TransmissionShadow = CalculateSubsurfaceOcclusion(opacity, shadowPosition.z, shadowMapDepth);
        result.TransmissionShadow = PostProcessShadow(shadow, result.TransmissionShadow);
	}
#endif

    result.SurfaceShadow = PostProcessShadow(shadow, result.SurfaceShadow);
    return result;
}

// Samples the shadow for the given local light on the material surface (supports subsurface shadowing)
ShadowSample SampleLocalLightShadow(LightData light, Buffer<float4> shadowsBuffer, Texture2D<float> shadowMap, GBufferSample gBuffer, float3 L, float toLightLength, uint tileIndex)
{
#if !LIGHTING_NO_DIRECTIONAL
    // Skip if surface is in a full shadow
    float NoL = dot(gBuffer.Normal, L);
    BRANCH
    if (NoL <= 0
#if defined(USE_GBUFFER_CUSTOM_DATA)
        && !IsSubsurfaceMode(gBuffer.ShadingModel)
#endif
        )
        return (ShadowSample)0;
#endif

    ShadowSample result;
    result.SurfaceShadow = 1;
    result.TransmissionShadow = 1;

    // Skip pixels outside of the light influence
    BRANCH
    if (toLightLength > light.Radius)
        return result;

    // Load shadow data
    if (light.ShadowsBufferAddress == 0)
        return result; // No shadow assigned
    ShadowData shadow = LoadShadowsBuffer(shadowsBuffer, light.ShadowsBufferAddress);
    ShadowTileData shadowTile = LoadShadowsBufferTile(shadowsBuffer, light.ShadowsBufferAddress, tileIndex);

    float3 samplePosition = gBuffer.WorldPos;
#if !LIGHTING_NO_DIRECTIONAL
    // Apply normal offset bias
    samplePosition += GetShadowPositionOffset(shadow.NormalOffsetScale, NoL, gBuffer.Normal);
#endif

    // Project position into shadow atlas UV
    float4 shadowPosition;
    float2 shadowMapUV = GetLightShadowAtlasUV(shadow, shadowTile, samplePosition, shadowPosition);

    // Sample shadow map
    result.SurfaceShadow = SampleShadowMap(shadowMap, shadowMapUV, shadowPosition.z);

#if defined(USE_GBUFFER_CUSTOM_DATA)
	// Subsurface shadowing
	BRANCH
	if (IsSubsurfaceMode(gBuffer.ShadingModel))
	{
		float opacity = gBuffer.CustomData.a;
        shadowMapUV = GetLightShadowAtlasUV(shadow, shadowTile, gBuffer.WorldPos, shadowPosition);
		float shadowMapDepth = shadowMap.SampleLevel(SamplerLinearClamp, shadowMapUV, 0).r;
		result.TransmissionShadow = CalculateSubsurfaceOcclusion(opacity, shadowPosition.z, shadowMapDepth);
        result.TransmissionShadow = PostProcessShadow(shadow, result.TransmissionShadow);
	}
#endif

    result.SurfaceShadow = PostProcessShadow(shadow, result.SurfaceShadow);
    return result;
}

// Samples the shadow for the given spot light on the material surface (supports subsurface shadowing)
ShadowSample SampleSpotLightShadow(LightData light, Buffer<float4> shadowsBuffer, Texture2D<float> shadowMap, GBufferSample gBuffer)
{
    float3 toLight = light.Position - gBuffer.WorldPos;
    float toLightLength = length(toLight);
    float3 L = toLight / toLightLength;
    return SampleLocalLightShadow(light, shadowsBuffer, shadowMap, gBuffer, L, toLightLength, 0);
}

// Samples the shadow for the given point light on the material surface (supports subsurface shadowing)
ShadowSample SamplePointLightShadow(LightData light, Buffer<float4> shadowsBuffer, Texture2D<float> shadowMap, GBufferSample gBuffer)
{
    float3 toLight = light.Position - gBuffer.WorldPos;
    float toLightLength = length(toLight);
    float3 L = toLight / toLightLength;

    // Figure out which cube face we're sampling from
    uint cubeFaceIndex = GetCubeFaceIndex(-L);

    return SampleLocalLightShadow(light, shadowsBuffer, shadowMap, gBuffer, L, toLightLength, cubeFaceIndex);
}

GBufferSample GetDummyGBufferSample(float3 worldPosition)
{
    GBufferSample gBuffer = (GBufferSample)0;
    gBuffer.ShadingModel = SHADING_MODEL_LIT;
    gBuffer.WorldPos = worldPosition;
    return gBuffer;
}

// Samples the shadow for the given directional light at custom location
ShadowSample SampleDirectionalLightShadow(LightData light, Buffer<float4> shadowsBuffer, Texture2D<float> shadowMap, float3 worldPosition, float viewDepth, float dither = 0.0f)
{
    GBufferSample gBuffer = GetDummyGBufferSample(worldPosition);
    gBuffer.ViewPos.z = viewDepth;
    return SampleDirectionalLightShadow(light, shadowsBuffer, shadowMap, gBuffer, dither);
}

// Samples the shadow for the given spot light at custom location
ShadowSample SampleSpotLightShadow(LightData light, Buffer<float4> shadowsBuffer, Texture2D<float> shadowMap, float3 worldPosition)
{
    GBufferSample gBuffer = GetDummyGBufferSample(worldPosition);
    return SampleSpotLightShadow(light, shadowsBuffer, shadowMap, gBuffer);
}

// Samples the shadow for the given point light at custom location
ShadowSample SamplePointLightShadow(LightData light, Buffer<float4> shadowsBuffer, Texture2D<float> shadowMap, float3 worldPosition)
{
    GBufferSample gBuffer = GetDummyGBufferSample(worldPosition);
    return SamplePointLightShadow(light, shadowsBuffer, shadowMap, gBuffer);
}

#endif
