// Copyright (c) Wojciech Figat. All rights reserved.

#ifndef __SHADOWS_SAMPLING__
#define __SHADOWS_SAMPLING__

#ifndef SHADOWS_CSM_BLENDING
#define SHADOWS_CSM_BLENDING 0
#endif
#ifndef SHADOWS_CSM_DITHERING
#define SHADOWS_CSM_DITHERING 0
#endif
#ifndef SHADOWS_PCSS
#define SHADOWS_PCSS (SHADOWS_QUALITY >= 3)
#endif

#include "./Flax/ShadowsCommon.hlsl"
#include "./Flax/GBufferCommon.hlsl"
#include "./Flax/LightingCommon.hlsl"
#if SHADOWS_CSM_DITHERING || SHADOWS_PCSS
#include "./Flax/Random.hlsl"
#endif

#if FEATURE_LEVEL >= FEATURE_LEVEL_SM5 || defined(WGSL)
#define SAMPLE_SHADOW_MAP(shadowMap, shadowUV, sceneDepth) shadowMap.SampleCmpLevelZero(ShadowSamplerLinear, shadowUV, sceneDepth)
#define SAMPLE_SHADOW_MAP_OFFSET(shadowMap, shadowUV, texelOffset, sceneDepth) shadowMap.SampleCmpLevelZero(ShadowSamplerLinear, shadowUV, sceneDepth, texelOffset)
#else
#define SAMPLE_SHADOW_MAP(shadowMap, shadowUV, sceneDepth) (sceneDepth < shadowMap.SampleLevel(SamplerLinearClamp, shadowUV, 0).r)
#define SAMPLE_SHADOW_MAP_OFFSET(shadowMap, shadowUV, texelOffset, sceneDepth) (sceneDepth < shadowMap.SampleLevel(SamplerLinearClamp, shadowUV, 0, texelOffset).r)
#endif
#if defined(WGSL)
#define LOAD_SHADOW_MAP(shadowMap, shadowUV) SAMPLE_RT_DEPTH(shadowMap, shadowUV)
#elif VULKAN || FEATURE_LEVEL < FEATURE_LEVEL_SM5
#define LOAD_SHADOW_MAP(shadowMap, shadowUV) shadowMap.SampleLevel(SamplerPointClamp, shadowUV, 0).r
#else
#define LOAD_SHADOW_MAP(shadowMap, shadowUV) shadowMap.SampleLevel(SamplerLinearClamp, shadowUV, 0).r
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

	sum += uw0 * vw0 * SAMPLE_SHADOW_MAP(shadowMap, baseUV + float2(u0, v0) * shadowMapSizeInv, sceneDepth);
	sum += uw1 * vw0 * SAMPLE_SHADOW_MAP(shadowMap, baseUV + float2(u1, v0) * shadowMapSizeInv, sceneDepth);
	sum += uw0 * vw1 * SAMPLE_SHADOW_MAP(shadowMap, baseUV + float2(u0, v1) * shadowMapSizeInv, sceneDepth);
	sum += uw1 * vw1 * SAMPLE_SHADOW_MAP(shadowMap, baseUV + float2(u1, v1) * shadowMapSizeInv, sceneDepth);

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

	sum += uw0 * vw0 * SAMPLE_SHADOW_MAP(shadowMap, baseUV + float2(u0, v0) * shadowMapSizeInv, sceneDepth);
	sum += uw1 * vw0 * SAMPLE_SHADOW_MAP(shadowMap, baseUV + float2(u1, v0) * shadowMapSizeInv, sceneDepth);
	sum += uw2 * vw0 * SAMPLE_SHADOW_MAP(shadowMap, baseUV + float2(u2, v0) * shadowMapSizeInv, sceneDepth);

	sum += uw0 * vw1 * SAMPLE_SHADOW_MAP(shadowMap, baseUV + float2(u0, v1) * shadowMapSizeInv, sceneDepth);
	sum += uw1 * vw1 * SAMPLE_SHADOW_MAP(shadowMap, baseUV + float2(u1, v1) * shadowMapSizeInv, sceneDepth);
	sum += uw2 * vw1 * SAMPLE_SHADOW_MAP(shadowMap, baseUV + float2(u2, v1) * shadowMapSizeInv, sceneDepth);

	sum += uw0 * vw2 * SAMPLE_SHADOW_MAP(shadowMap, baseUV + float2(u0, v2) * shadowMapSizeInv, sceneDepth);
	sum += uw1 * vw2 * SAMPLE_SHADOW_MAP(shadowMap, baseUV + float2(u1, v2) * shadowMapSizeInv, sceneDepth);
	sum += uw2 * vw2 * SAMPLE_SHADOW_MAP(shadowMap, baseUV + float2(u2, v2) * shadowMapSizeInv, sceneDepth);

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

	sum += uw0 * vw0 * SAMPLE_SHADOW_MAP(shadowMap, baseUV + float2(u0, v0) * shadowMapSizeInv, sceneDepth);
	sum += uw1 * vw0 * SAMPLE_SHADOW_MAP(shadowMap, baseUV + float2(u1, v0) * shadowMapSizeInv, sceneDepth);
	sum += uw2 * vw0 * SAMPLE_SHADOW_MAP(shadowMap, baseUV + float2(u2, v0) * shadowMapSizeInv, sceneDepth);
	sum += uw3 * vw0 * SAMPLE_SHADOW_MAP(shadowMap, baseUV + float2(u3, v0) * shadowMapSizeInv, sceneDepth);

	sum += uw0 * vw1 * SAMPLE_SHADOW_MAP(shadowMap, baseUV + float2(u0, v1) * shadowMapSizeInv, sceneDepth);
	sum += uw1 * vw1 * SAMPLE_SHADOW_MAP(shadowMap, baseUV + float2(u1, v1) * shadowMapSizeInv, sceneDepth);
	sum += uw2 * vw1 * SAMPLE_SHADOW_MAP(shadowMap, baseUV + float2(u2, v1) * shadowMapSizeInv, sceneDepth);
	sum += uw3 * vw1 * SAMPLE_SHADOW_MAP(shadowMap, baseUV + float2(u3, v1) * shadowMapSizeInv, sceneDepth);

	sum += uw0 * vw2 * SAMPLE_SHADOW_MAP(shadowMap, baseUV + float2(u0, v2) * shadowMapSizeInv, sceneDepth);
	sum += uw1 * vw2 * SAMPLE_SHADOW_MAP(shadowMap, baseUV + float2(u1, v2) * shadowMapSizeInv, sceneDepth);
	sum += uw2 * vw2 * SAMPLE_SHADOW_MAP(shadowMap, baseUV + float2(u2, v2) * shadowMapSizeInv, sceneDepth);
	sum += uw3 * vw2 * SAMPLE_SHADOW_MAP(shadowMap, baseUV + float2(u3, v2) * shadowMapSizeInv, sceneDepth);

	sum += uw0 * vw3 * SAMPLE_SHADOW_MAP(shadowMap, baseUV + float2(u0, v3) * shadowMapSizeInv, sceneDepth);
	sum += uw1 * vw3 * SAMPLE_SHADOW_MAP(shadowMap, baseUV + float2(u1, v3) * shadowMapSizeInv, sceneDepth);
	sum += uw2 * vw3 * SAMPLE_SHADOW_MAP(shadowMap, baseUV + float2(u2, v3) * shadowMapSizeInv, sceneDepth);
	sum += uw3 * vw3 * SAMPLE_SHADOW_MAP(shadowMap, baseUV + float2(u3, v3) * shadowMapSizeInv, sceneDepth);

	return sum * (1.0f / 2704);
#else
    return 0.0f;
#endif
}

#if SHADOWS_PCSS

// "Vogel disk" sampling pattern: https://github.com/corporateshark/poisson-disk-generator
#define SHADOWS_PCSS_SAMPLES 32
static const half2 VogelPoints[SHADOWS_PCSS_SAMPLES] = {
#if SHADOWS_PCSS_SAMPLES == 4
    // 4 samples
    half2(0.353553, 0.000000),
    half2(-0.451560, 0.413635),
    half2(0.069174, -0.787537),
    half2(0.569060, 0.742409),
#elif SHADOWS_PCSS_SAMPLES == 8
    // 8 samples
    half2(0.25000000, 0.00000000),
    half2(-0.31930089, 0.29248416),
    half2(0.04891348, -0.55687296),
    half2(0.40238643, 0.52496207),
    half2(-0.73851585, -0.13074535),
    half2(0.69968677, -0.44490278),
    half2(-0.23419666, 0.87043202),
    half2(-0.44604915, -0.85938364),
#elif SHADOWS_PCSS_SAMPLES == 16
    // 16 samples
    half2(0.17677665, 0.00000000),
    half2(-0.22577983, 0.20681751),
    half2(0.03458714, -0.39376867),
    half2(0.28453016, 0.37120426),
    half2(-0.52220953, -0.09245092),
    half2(0.49475324, -0.31459379),
    half2(-0.16560209, 0.61548841),
    half2(-0.31540442, -0.60767603),
    half2(0.68456841, 0.25023210),
    half2(-0.71235347, 0.29377294),
    half2(0.34362423, -0.73360229),
    half2(0.25340176, 0.80903494),
    half2(-0.76454973, -0.44352412),
    half2(0.89722824, -0.19680285),
    half2(-0.54790950, 0.77848911),
    half2(-0.12594837, -0.97615927),
#elif SHADOWS_PCSS_SAMPLES == 32
    // 32 samples
    half2(0.12500000, 0.00000000),
    half2(-0.15965044, 0.14624202),
    half2(0.02445674, -0.27843648),
    half2(0.20119321, 0.26248097),
    half2(-0.36925793, -0.06537271),
    half2(0.34984338, -0.22245139),
    half2(-0.11709833, 0.43521595),
    half2(-0.22302461, -0.42969179),
    half2(0.48406291, 0.17694080),
    half2(-0.50370997, 0.20772886),
    half2(0.24297905, -0.51873517),
    half2(0.17918217, 0.57207417),
    half2(-0.54061830, -0.31361890),
    half2(0.63443625, -0.13916063),
    half2(-0.38743055, 0.55047488),
    half2(-0.08905894, -0.69024885),
    half2(0.54879880, 0.46308208),
    half2(-0.73889750, 0.03009081),
    half2(0.53931046, -0.53597510),
    half2(-0.03660476, 0.77976608),
    half2(-0.51236552, -0.61490381),
    half2(0.81227481, 0.10993028),
    half2(-0.68869931, 0.47834957),
    half2(0.18879366, -0.83590192),
    half2(0.43436146, 0.75957572),
    half2(-0.85019755, -0.27210116),
    half2(0.82646751, -0.38088906),
    half2(-0.35873961, 0.85479879),
    half2(-0.31848884, -0.88836360),
    half2(0.84942913, 0.44759941),
    half2(-0.94430852, 0.24780297),
    half2(0.53754807, -0.83391672),
#endif
};

float2 SampleShadowPCSSRotate(float2 pos, float2 rotation)
{
    return float2(pos.x * rotation.x - pos.y * rotation.y, pos.y * rotation.x + pos.x * rotation.y);
}

// [Percentage-Closer Soft Shadows, Randima Fernando, NVIDIA]
float SampleShadowMapPCSS(Texture2D<float> shadowMap, float2 shadowMapUV, float sceneDepth, float4 shadowToAtlas, float sourceAngle)
{
    // Scale samples to shadow map tile
    float2 shadowMapSize;
    shadowMap.GetDimensions(shadowMapSize.x, shadowMapSize.y);
    float resolution = shadowMapSize.x * shadowToAtlas.x;
    float minRadius = 0.3f / resolution;
    float2 uvMin = shadowToAtlas.zw;
    float2 uvMax = shadowToAtlas.xy + shadowToAtlas.zw;

    // Fix penumbra size to be consistent across different shadow map resolutions
    sourceAngle *= resolution / 1024.0;

    // Rotate the sampling pattern based on the pixel position to reduce banding artifacts (use shadow-space position for stability)
    float rotationAngle = RandN1(shadowMapUV) * PI;
    float2 rotation;
    sincos(rotationAngle, rotation.x, rotation.y);

    // Search blockers
    float searchRadius = sourceAngle * saturate(sceneDepth - 0.02f) / sceneDepth;
    searchRadius = max(searchRadius, minRadius);
    uint blockers = 0;
    float avgBlockerDistance = 0.0f;
    for (uint i = 0; i < SHADOWS_PCSS_SAMPLES; i++)
    {
        float2 offset = VogelPoints[i] * searchRadius;
        offset = SampleShadowPCSSRotate(offset, rotation);
        offset = shadowMapUV + offset;
        offset = clamp(offset, uvMin, uvMax);
        float shadowMapDepth = LOAD_SHADOW_MAP(shadowMap, offset);
        if (shadowMapDepth < sceneDepth)
        {
            blockers++;
            avgBlockerDistance += shadowMapDepth;
        }
    }
    if (blockers < 1)
        return 1; // No blockers, fully lit
    avgBlockerDistance /= blockers;

    // Calculate penumbra size
    float penumbra = max(sceneDepth - avgBlockerDistance, 0.0);
#if defined(VULKAN)
    sceneDepth *= lerp(1, 0.985f, saturate(penumbra * 4.0f)); // Fix shadow bias issues on Vulkan
#endif
    float filterRadius = penumbra * sourceAngle;
    filterRadius = max(filterRadius, minRadius); // Don't use too small filter near blockers to avoid jagged edges

    // Filter shadowmap
    float shadow = 0.0f;
    for (uint i = 0; i < SHADOWS_PCSS_SAMPLES; i++)
    {
        float2 offset = VogelPoints[i] * filterRadius;
        offset = SampleShadowPCSSRotate(offset, rotation);
        offset = shadowMapUV + offset;
        offset = clamp(offset, uvMin, uvMax);
        shadow += LOAD_SHADOW_MAP(shadowMap, offset) > sceneDepth;
    }
    return shadow / (float)SHADOWS_PCSS_SAMPLES;
}

#endif

// Samples the shadow cascade for the given directional light on the material surface (supports subsurface shadowing)
ShadowSample SampleDirectionalLightShadowCascade(LightData light, Buffer<float4> shadowsBuffer, Texture2D<float> shadowMap, GBufferSample gBuffer, ShadowData shadow, float3 samplePosition, uint cascadeIndex)
{
    ShadowSample result;
    ShadowTileData shadowTile = LoadShadowsBufferTile(shadowsBuffer, light.ShadowsBufferAddress, cascadeIndex);

    // Project position into shadow atlas UV
    float4 shadowPosition;
    float2 shadowMapUV = GetLightShadowAtlasUV(shadow, shadowTile, samplePosition, shadowPosition);

    // Sample shadow map
#if SHADOWS_PCSS
    float sourceAngle = light.SourceRadius; // tan(SourceAngle * DegreesToRadians * 0.5);
    result.SurfaceShadow = SampleShadowMapPCSS(shadowMap, shadowMapUV, shadowPosition.z, shadowTile.ShadowToAtlas, sourceAngle);
#else
    result.SurfaceShadow = SampleShadowMapOptimizedPCF(shadowMap, shadowMapUV, shadowPosition.z);
#endif

    // Increase the sharpness for higher cascades to match the filter radius
    const float SharpnessScale[MaxNumCascades] = { 1.0f, 1.5f, 3.0f, 3.5f };
    shadow.Sharpness *= SharpnessScale[cascadeIndex];

    result.TransmissionShadow = 1;
#if defined(USE_GBUFFER_CUSTOM_DATA)
	// Subsurface shadowing
	BRANCH
	if (IsSubsurfaceMode(gBuffer.ShadingModel))
	{
		float opacity = gBuffer.CustomData.a;
        shadowMapUV = GetLightShadowAtlasUV(shadow, shadowTile, gBuffer.WorldPos, shadowPosition);
		float shadowMapDepth = LOAD_SHADOW_MAP(shadowMap, shadowMapUV);
		result.TransmissionShadow = CalculateSubsurfaceOcclusion(opacity, shadowPosition.z, shadowMapDepth);
        result.TransmissionShadow = PostProcessShadow(shadow, result.TransmissionShadow);
	}
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
    if (splitDist <= shadow.CascadeBlendSize && cascadeIndex != shadow.TilesCount - 1)
    {
        // Dither with the next cascade but with screen-space dithering (gets cleaned out by TAA)
        float lerpAmount = 1 - splitDist / shadow.CascadeBlendSize;
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
    if (splitDist <= shadow.CascadeBlendSize && cascadeIndex != shadow.TilesCount - 1)
    {
	    // Sample the next cascade, and blend between the two results to smooth the transition
        ShadowSample nextResult = SampleDirectionalLightShadowCascade(light, shadowsBuffer, shadowMap, gBuffer, shadow, samplePosition, cascadeIndex + 1);
		float blendAmount = splitDist / shadow.CascadeBlendSize;
		result.SurfaceShadow = lerp(nextResult.SurfaceShadow, result.SurfaceShadow, blendAmount);
		result.TransmissionShadow = lerp(nextResult.TransmissionShadow, result.TransmissionShadow, blendAmount);
    }
#endif

	result.SurfaceShadow = lerp(result.SurfaceShadow, 1, fade);
	result.TransmissionShadow = lerp(result.TransmissionShadow, 1, fade);

    return result;
}

// Samples the shadow for the given local light on the material surface (supports subsurface shadowing)
ShadowSample SampleLocalLightShadow(LightData light, Buffer<float4> shadowsBuffer, Texture2D<float> shadowMap, GBufferSample gBuffer, float3 L, float toLightLength, uint tileIndex, float sourceAngle = 0)
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
#if SHADOWS_PCSS
    result.SurfaceShadow = SampleShadowMapPCSS(shadowMap, shadowMapUV, shadowPosition.z, shadowTile.ShadowToAtlas, sourceAngle);
#else
    result.SurfaceShadow = SampleShadowMapOptimizedPCF(shadowMap, shadowMapUV, shadowPosition.z);
#endif

#if defined(USE_GBUFFER_CUSTOM_DATA)
	// Subsurface shadowing
	BRANCH
	if (IsSubsurfaceMode(gBuffer.ShadingModel))
	{
		float opacity = gBuffer.CustomData.a;
        shadowMapUV = GetLightShadowAtlasUV(shadow, shadowTile, gBuffer.WorldPos, shadowPosition);
		float shadowMapDepth = LOAD_SHADOW_MAP(shadowMap, shadowMapUV);
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

    // TODO: make it physical-based (need to compare with path tracing)
    float sourceAngle = 0.02 * light.SourceRadius * dot(-light.Direction, light.Position - gBuffer.WorldPos) / light.Radius;

    return SampleLocalLightShadow(light, shadowsBuffer, shadowMap, gBuffer, L, toLightLength, 0, sourceAngle);
}

// Samples the shadow for the given point light on the material surface (supports subsurface shadowing)
ShadowSample SamplePointLightShadow(LightData light, Buffer<float4> shadowsBuffer, Texture2D<float> shadowMap, GBufferSample gBuffer)
{
    float3 toLight = light.Position - gBuffer.WorldPos;
    float toLightLength = length(toLight);
    float3 L = toLight / toLightLength;

    // Figure out which cube face we're sampling from
    uint cubeFaceIndex = GetCubeFaceIndex(-L);
    
    // TODO: make it physical-based (need to compare with path tracing)
    float sourceAngle = 0.02 * light.SourceRadius * length(light.Position - gBuffer.WorldPos) / light.Radius;

    return SampleLocalLightShadow(light, shadowsBuffer, shadowMap, gBuffer, L, toLightLength, cubeFaceIndex, sourceAngle);
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
