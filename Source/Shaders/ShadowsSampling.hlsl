// Copyright (c) Wojciech Figat. All rights reserved.

#ifndef __SHADOWS_SAMPLING__
#define __SHADOWS_SAMPLING__

#ifndef SHADOWS_CSM_BLENDING
#define SHADOWS_CSM_BLENDING 0
#endif
#ifndef SHADOWS_CSM_DITHERING
#define SHADOWS_CSM_DITHERING 0
#endif

#include "./Flax/ShadowsCommon.hlsl"
#include "./Flax/GBufferCommon.hlsl"
#include "./Flax/LightingCommon.hlsl"
#if SHADOWS_CSM_DITHERING
#include "./Flax/Random.hlsl"
#endif

#if FEATURE_LEVEL >= FEATURE_LEVEL_SM5
#define SAMPLE_SHADOW_MAP(shadowMap, shadowUV, sceneDepth) shadowMap.SampleCmpLevelZero(ShadowSamplerLinear, shadowUV, sceneDepth)
#define SAMPLE_SHADOW_MAP_OFFSET(shadowMap, shadowUV, texelOffset, sceneDepth) shadowMap.SampleCmpLevelZero(ShadowSamplerLinear, shadowUV, sceneDepth, texelOffset)
#else
#define SAMPLE_SHADOW_MAP(shadowMap, shadowUV, sceneDepth) (sceneDepth < shadowMap.SampleLevel(SamplerLinearClamp, shadowUV, 0).r)
#define SAMPLE_SHADOW_MAP_OFFSET(shadowMap, shadowUV, texelOffset, sceneDepth) (sceneDepth < shadowMap.SampleLevel(SamplerLinearClamp, shadowUV, 0, texelOffset).r)
#endif
#if VULKAN || FEATURE_LEVEL < FEATURE_LEVEL_SM5
#define SAMPLE_SHADOW_MAP_SAMPLER SamplerPointClamp
#else
#define SAMPLE_SHADOW_MAP_SAMPLER SamplerLinearClamp
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
    float result = SAMPLE_SHADOW_MAP(shadowMap, shadowMapUV, sceneDepth);
#if SHADOWS_QUALITY == 1
	result += SAMPLE_SHADOW_MAP_OFFSET(shadowMap, shadowMapUV, int2(-1, 0), sceneDepth);
	result += SAMPLE_SHADOW_MAP_OFFSET(shadowMap, shadowMapUV, int2(0, -1), sceneDepth);
	result += SAMPLE_SHADOW_MAP_OFFSET(shadowMap, shadowMapUV, int2(0, 1), sceneDepth);
	result += SAMPLE_SHADOW_MAP_OFFSET(shadowMap, shadowMapUV, int2(1, 0), sceneDepth);
	result = result * (1.0f / 4.0);
#elif SHADOWS_QUALITY == 2 || SHADOWS_QUALITY == 3
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

float SampleShadowMapOptimizedPCFHelper(Texture2D<float> shadowMap, float2 baseUV, float u, float v, float2 shadowMapSizeInv, float sceneDepth)
{
    float2 uv = baseUV + float2(u, v) * shadowMapSizeInv;
    return SAMPLE_SHADOW_MAP(shadowMap, uv, sceneDepth);
}

// [Shadow map sampling method used in The Witness, https://github.com/TheRealMJP/Shadows]
float SampleShadowMapOptimizedPCF(Texture2D<float> shadowMap, float2 shadowMapUV, float sceneDepth)
{
#if SHADOWS_QUALITY != 0
    float2 shadowMapSize;
    shadowMap.GetDimensions(shadowMapSize.x, shadowMapSize.y);

    float2 uv = shadowMapUV.xy * shadowMapSize; // 1 unit - 1 texel
    float2 shadowMapSizeInv = 1.0f / shadowMapSize;

    float2 baseUV;
    baseUV.x = floor(uv.x + 0.5);
    baseUV.y = floor(uv.y + 0.5);
    float s = (uv.x + 0.5 - baseUV.x);
    float t = (uv.y + 0.5 - baseUV.y);
    baseUV -= float2(0.5, 0.5);
    baseUV *= shadowMapSizeInv;

    float sum = 0;
#endif
#if SHADOWS_QUALITY == 0
    return SAMPLE_SHADOW_MAP(shadowMap, shadowMapUV, sceneDepth);
#elif SHADOWS_QUALITY == 1
	float uw0 = (3 - 2 * s);
	float uw1 = (1 + 2 * s);

	float u0 = (2 - s) / uw0 - 1;
	float u1 = s / uw1 + 1;

	float vw0 = (3 - 2 * t);
	float vw1 = (1 + 2 * t);

	float v0 = (2 - t) / vw0 - 1;
	float v1 = t / vw1 + 1;

	sum += uw0 * vw0 * SampleShadowMapOptimizedPCFHelper(shadowMap, baseUV, u0, v0, shadowMapSizeInv, sceneDepth);
	sum += uw1 * vw0 * SampleShadowMapOptimizedPCFHelper(shadowMap, baseUV, u1, v0, shadowMapSizeInv, sceneDepth);
	sum += uw0 * vw1 * SampleShadowMapOptimizedPCFHelper(shadowMap, baseUV, u0, v1, shadowMapSizeInv, sceneDepth);
	sum += uw1 * vw1 * SampleShadowMapOptimizedPCFHelper(shadowMap, baseUV, u1, v1, shadowMapSizeInv, sceneDepth);

	return sum * 1.0f / 16;
#elif SHADOWS_QUALITY == 2
	float uw0 = (4 - 3 * s);
	float uw1 = 7;
	float uw2 = (1 + 3 * s);

	float u0 = (3 - 2 * s) / uw0 - 2;
	float u1 = (3 + s) / uw1;
	float u2 = s / uw2 + 2;

	float vw0 = (4 - 3 * t);
	float vw1 = 7;
	float vw2 = (1 + 3 * t);

	float v0 = (3 - 2 * t) / vw0 - 2;
	float v1 = (3 + t) / vw1;
	float v2 = t / vw2 + 2;

	sum += uw0 * vw0 * SampleShadowMapOptimizedPCFHelper(shadowMap, baseUV, u0, v0, shadowMapSizeInv, sceneDepth);
	sum += uw1 * vw0 * SampleShadowMapOptimizedPCFHelper(shadowMap, baseUV, u1, v0, shadowMapSizeInv, sceneDepth);
	sum += uw2 * vw0 * SampleShadowMapOptimizedPCFHelper(shadowMap, baseUV, u2, v0, shadowMapSizeInv, sceneDepth);

	sum += uw0 * vw1 * SampleShadowMapOptimizedPCFHelper(shadowMap, baseUV, u0, v1, shadowMapSizeInv, sceneDepth);
	sum += uw1 * vw1 * SampleShadowMapOptimizedPCFHelper(shadowMap, baseUV, u1, v1, shadowMapSizeInv, sceneDepth);
	sum += uw2 * vw1 * SampleShadowMapOptimizedPCFHelper(shadowMap, baseUV, u2, v1, shadowMapSizeInv, sceneDepth);

	sum += uw0 * vw2 * SampleShadowMapOptimizedPCFHelper(shadowMap, baseUV, u0, v2, shadowMapSizeInv, sceneDepth);
	sum += uw1 * vw2 * SampleShadowMapOptimizedPCFHelper(shadowMap, baseUV, u1, v2, shadowMapSizeInv, sceneDepth);
	sum += uw2 * vw2 * SampleShadowMapOptimizedPCFHelper(shadowMap, baseUV, u2, v2, shadowMapSizeInv, sceneDepth);

	return sum * 1.0f / 144;
#elif SHADOWS_QUALITY == 3
	float uw0 = (5 * s - 6);
	float uw1 = (11 * s - 28);
	float uw2 = -(11 * s + 17);
	float uw3 = -(5 * s + 1);

	float u0 = (4 * s - 5) / uw0 - 3;
	float u1 = (4 * s - 16) / uw1 - 1;
	float u2 = -(7 * s + 5) / uw2 + 1;
	float u3 = -s / uw3 + 3;

	float vw0 = (5 * t - 6);
	float vw1 = (11 * t - 28);
	float vw2 = -(11 * t + 17);
	float vw3 = -(5 * t + 1);

	float v0 = (4 * t - 5) / vw0 - 3;
	float v1 = (4 * t - 16) / vw1 - 1;
	float v2 = -(7 * t + 5) / vw2 + 1;
	float v3 = -t / vw3 + 3;

	sum += uw0 * vw0 * SampleShadowMapOptimizedPCFHelper(shadowMap, baseUV, u0, v0, shadowMapSizeInv, sceneDepth);
	sum += uw1 * vw0 * SampleShadowMapOptimizedPCFHelper(shadowMap, baseUV, u1, v0, shadowMapSizeInv, sceneDepth);
	sum += uw2 * vw0 * SampleShadowMapOptimizedPCFHelper(shadowMap, baseUV, u2, v0, shadowMapSizeInv, sceneDepth);
	sum += uw3 * vw0 * SampleShadowMapOptimizedPCFHelper(shadowMap, baseUV, u3, v0, shadowMapSizeInv, sceneDepth);

	sum += uw0 * vw1 * SampleShadowMapOptimizedPCFHelper(shadowMap, baseUV, u0, v1, shadowMapSizeInv, sceneDepth);
	sum += uw1 * vw1 * SampleShadowMapOptimizedPCFHelper(shadowMap, baseUV, u1, v1, shadowMapSizeInv, sceneDepth);
	sum += uw2 * vw1 * SampleShadowMapOptimizedPCFHelper(shadowMap, baseUV, u2, v1, shadowMapSizeInv, sceneDepth);
	sum += uw3 * vw1 * SampleShadowMapOptimizedPCFHelper(shadowMap, baseUV, u3, v1, shadowMapSizeInv, sceneDepth);

	sum += uw0 * vw2 * SampleShadowMapOptimizedPCFHelper(shadowMap, baseUV, u0, v2, shadowMapSizeInv, sceneDepth);
	sum += uw1 * vw2 * SampleShadowMapOptimizedPCFHelper(shadowMap, baseUV, u1, v2, shadowMapSizeInv, sceneDepth);
	sum += uw2 * vw2 * SampleShadowMapOptimizedPCFHelper(shadowMap, baseUV, u2, v2, shadowMapSizeInv, sceneDepth);
	sum += uw3 * vw2 * SampleShadowMapOptimizedPCFHelper(shadowMap, baseUV, u3, v2, shadowMapSizeInv, sceneDepth);

	sum += uw0 * vw3 * SampleShadowMapOptimizedPCFHelper(shadowMap, baseUV, u0, v3, shadowMapSizeInv, sceneDepth);
	sum += uw1 * vw3 * SampleShadowMapOptimizedPCFHelper(shadowMap, baseUV, u1, v3, shadowMapSizeInv, sceneDepth);
	sum += uw2 * vw3 * SampleShadowMapOptimizedPCFHelper(shadowMap, baseUV, u2, v3, shadowMapSizeInv, sceneDepth);
	sum += uw3 * vw3 * SampleShadowMapOptimizedPCFHelper(shadowMap, baseUV, u3, v3, shadowMapSizeInv, sceneDepth);

	return sum * (1.0f / 2704);
#else
    return 0.0f;
#endif
}

// Samples the shadow cascade for the given directional light on the material surface (supports subsurface shadowing)
ShadowSample SampleDirectionalLightShadowCascade(LightData light, Buffer<float4> shadowsBuffer, Texture2D<float> shadowMap, GBufferSample gBuffer, ShadowData shadow, float3 samplePosition, uint cascadeIndex)
{
    ShadowSample result;
    ShadowTileData shadowTile = LoadShadowsBufferTile(shadowsBuffer, light.ShadowsBufferAddress, cascadeIndex);

    // Project position into shadow atlas UV
    float4 shadowPosition;
    float2 shadowMapUV = GetLightShadowAtlasUV(shadow, shadowTile, samplePosition, shadowPosition);

    // Sample shadow map
    result.SurfaceShadow = SampleShadowMapOptimizedPCF(shadowMap, shadowMapUV, shadowPosition.z);

    // Increase the sharpness for higher cascades to match the filter radius
    const float SharpnessScale[MaxNumCascades] = { 1.0f, 1.5f, 3.0f, 3.5f };
    shadow.Sharpness *= SharpnessScale[cascadeIndex];
    
#if defined(USE_GBUFFER_CUSTOM_DATA)
	// Subsurface shadowing
	BRANCH
	if (IsSubsurfaceMode(gBuffer.ShadingModel))
	{
		float opacity = gBuffer.CustomData.a;
        shadowMapUV = GetLightShadowAtlasUV(shadow, shadowTile, gBuffer.WorldPos, shadowPosition);
		float shadowMapDepth = shadowMap.SampleLevel(SAMPLE_SHADOW_MAP_SAMPLER, shadowMapUV, 0).r;
		result.TransmissionShadow = CalculateSubsurfaceOcclusion(opacity, shadowPosition.z, shadowMapDepth);
        result.TransmissionShadow = PostProcessShadow(shadow, result.TransmissionShadow);
	}
#else
    result.TransmissionShadow = 1;
#endif

    result.SurfaceShadow = PostProcessShadow(shadow, result.SurfaceShadow);

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
#if SHADOWS_CSM_DITHERING || SHADOWS_CSM_BLENDING
	float nextSplit = shadow.CascadeSplits[cascadeIndex];
	float splitSize = cascadeIndex == 0 ? nextSplit : nextSplit - shadow.CascadeSplits[cascadeIndex - 1];
	float splitDist = (nextSplit - viewDepth) / splitSize;
#endif
#if SHADOWS_CSM_DITHERING && !SHADOWS_CSM_BLENDING
	const float BlendThreshold = 0.05f;
    if (splitDist <= BlendThreshold && cascadeIndex != shadow.TilesCount - 1)
    {
        // Dither with the next cascade but with screen-space dithering (gets cleaned out by TAA)
        float lerpAmount = 1 - splitDist / BlendThreshold;
        if (step(RandN2(gBuffer.ViewPos.xy + dither).x, lerpAmount))
            cascadeIndex++;
    }
#endif

    // Sample cascade
    float3 samplePosition = gBuffer.WorldPos;
#if !LIGHTING_NO_DIRECTIONAL
    // Apply normal offset bias
    samplePosition += GetShadowPositionOffset(shadow.NormalOffsetScale, NoL, gBuffer.Normal);
#endif
    result = SampleDirectionalLightShadowCascade(light, shadowsBuffer, shadowMap, gBuffer, shadow, samplePosition, cascadeIndex);

#if SHADOWS_CSM_BLENDING
	const float BlendThreshold = 0.1f;
    if (splitDist <= BlendThreshold && cascadeIndex != shadow.TilesCount - 1)
    {
	    // Sample the next cascade, and blend between the two results to smooth the transition
        ShadowSample nextResult = SampleDirectionalLightShadowCascade(light, shadowsBuffer, shadowMap, gBuffer, shadow, samplePosition, cascadeIndex + 1);
		float blendAmount = splitDist / BlendThreshold;
		result.SurfaceShadow = lerp(nextResult.SurfaceShadow, result.SurfaceShadow, blendAmount);
		result.TransmissionShadow = lerp(nextResult.TransmissionShadow, result.TransmissionShadow, blendAmount);
    }
#endif

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
    result.SurfaceShadow = SampleShadowMapOptimizedPCF(shadowMap, shadowMapUV, shadowPosition.z);

#if defined(USE_GBUFFER_CUSTOM_DATA)
	// Subsurface shadowing
	BRANCH
	if (IsSubsurfaceMode(gBuffer.ShadingModel))
	{
		float opacity = gBuffer.CustomData.a;
        shadowMapUV = GetLightShadowAtlasUV(shadow, shadowTile, gBuffer.WorldPos, shadowPosition);
		float shadowMapDepth = shadowMap.SampleLevel(SAMPLE_SHADOW_MAP_SAMPLER, shadowMapUV, 0).r;
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
