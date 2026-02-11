// Copyright (c) Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"
#include "./Flax/BRDF.hlsl"
#include "./Flax/Random.hlsl"
#include "./Flax/MonteCarlo.hlsl"
#include "./Flax/GBufferCommon.hlsl"
#if SSR_USE_HZB
#include "./FlaxThirdParty/FidelityFX/ffx_sssr.h"
#endif

// 1:-1 to 0:1
float2 ClipToUv(float2 clipPos)
{
    return clipPos * float2(0.5, -0.5) + float2(0.5, 0.5);
}

// go into clip space (-1:1 from bottom/left to up/right)
float3 ProjectWorldToClip(float3 wsPos, float4x4 viewProjectionMatrix)
{
    float4 clipPos = mul(float4(wsPos, 1), viewProjectionMatrix);
    return clipPos.xyz / clipPos.w;
}

// go into UV space. (0:1 from top/left to bottom/right)
float3 ProjectWorldToUv(float3 wsPos, float4x4 viewProjectionMatrix)
{
    float3 clipPos = ProjectWorldToClip(wsPos, viewProjectionMatrix);
    return float3(ClipToUv(clipPos.xy), clipPos.z);
}

float3 TangentToWorld(float3 N, float4 H)
{
    float3 upVector = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
    float3 T = normalize(cross(upVector, N));
    float3 B = cross(N, T);
    return float3((T * H.x) + (B * H.y) + (N * H.z));
}

float RayAttenBorder(float2 pos, float value)
{
    float borderDist = min(1.0 - max(pos.x, pos.y), min(pos.x, pos.y));
    return saturate(borderDist > value ? 1.0 : borderDist / value);
}

// Screen Space Reflection ray tracing utility.
// Returns: xy: hitUV, z: hitMask, where hitUV is the result UV of hit pixel, hitMask is the normalized sample weight (0 if no hit).
float3 ScreenSpaceReflectionDirection(float2 uv, GBufferSample gBuffer, float3 viewPos,  bool temporal = false, float temporalTime = 0.0f, float brdfBias = 0.82f)
{
    float2 jitter = RandN2(uv + temporalTime);
    float2 Xi = jitter;
    Xi.y = lerp(Xi.y, 0.0, brdfBias);
    float3 H = temporal ? TangentToWorld(gBuffer.Normal, ImportanceSampleGGX(Xi, gBuffer.Roughness)) : gBuffer.Normal;
    float3 viewWS = normalize(gBuffer.WorldPos - viewPos);
    return reflect(viewWS, H.xyz);
}

// Screen Space Reflection ray tracing utility.
// If SSR_USE_HZB is defined, it uses Hierarchical Z-Buffer for tracing against screen (assumes that depthBuffer is a HiZ with full mip-chain).
// Returns: xy: hitUV, z: hitMask, where hitUV is the result UV of hit pixel, hitMask is the normalized sample weight (0 if no hit).
float3 TraceScreenSpaceReflection(
#if SSR_USE_HZB
    out bool uncertainHit, uint hzbMips,
#endif
    float2 uv, GBufferSample gBuffer, Texture2D depthBuffer, float3 viewPos, float4x4 viewMatrix, float4x4 viewProjectionMatrix, float stepSize, float maxSamples = 50, bool temporal = false, float temporalTime = 0.0f, float worldAntiSelfOcclusionBias = 0.1f, float brdfBias = 0.82f, float drawDistance = 5000.0f, float roughnessThreshold = 0.4f, float edgeFade = 0.1f)
{
#if SSR_USE_HZB
    uncertainHit = false;
#endif
#ifndef SSR_SKIP_INVALID_CHECK
    // Reject invalid pixels
    if (gBuffer.ShadingModel == SHADING_MODEL_UNLIT || gBuffer.Roughness > roughnessThreshold || gBuffer.ViewPos.z > drawDistance)
        return 0;
#endif

    // Calculate view space normal vector
    float3 normalVS = mul(gBuffer.Normal, (float3x3)viewMatrix);
    float3 reflectVS = normalize(reflect(gBuffer.ViewPos, normalVS));
    if (reflectVS.z < 0.001f)
        return 0; // Ray goes towards the view

    // Calculate ray path in UV space (z is raw depth)
    float3 reflectWS = ScreenSpaceReflectionDirection(uv, gBuffer, viewPos, temporal, temporalTime, brdfBias);
#if SSR_USE_HZB
    worldAntiSelfOcclusionBias *= 10.0f; // Higher bias for HZB trace to reduce artifacts
#endif
    float3 startWS = gBuffer.WorldPos + gBuffer.Normal * worldAntiSelfOcclusionBias;
    float3 startUV = ProjectWorldToUv(startWS, viewProjectionMatrix);
    float3 endUV = ProjectWorldToUv(startWS + reflectWS, viewProjectionMatrix);
    float3 rayUV = endUV - startUV;
    float2 rayUVAbs = abs(rayUV.xy);
    rayUV *= stepSize / max(rayUVAbs.x, rayUVAbs.y);
    float3 startUv = startUV + rayUV * 2;
    float3 currOffset = startUv;
    float3 rayStep = rayUV * 2;

    // Calculate number of samples
    float3 samplesToEdge = ((sign(rayStep.xyz) * 0.5 + 0.5) - currOffset.xyz) / rayStep.xyz;
    samplesToEdge.x = min(samplesToEdge.x, min(samplesToEdge.y, samplesToEdge.z)) * 1.05f;
    float numSamples = min(maxSamples, samplesToEdge.x);
    rayStep *= samplesToEdge.x / numSamples;

    // Ray trace
    float depthDiffError = 1.3f * abs(rayStep.z);
#if SSR_USE_HZB
    bool validHit = false;
    uint2 depthBufferSize;
    depthBuffer.GetDimensions(depthBufferSize.x, depthBufferSize.y);
    float3 hit = FFX_SSSR_HierarchicalRaymarch(depthBuffer, hzbMips, depthDiffError, uncertainHit, startUV, rayUV, depthBufferSize, 0, numSamples, validHit);
    if (!validHit)
        return 0;
    currOffset = hit;
#else
    float currSampleIndex = 0;
    LOOP
    while (currSampleIndex < numSamples)
    {
        // Sample depth buffer and calculate depth difference
        float currSample = SAMPLE_RT(depthBuffer, currOffset.xy).r;
        float depthDiff = currOffset.z - currSample;

        // Check intersection
        if (depthDiff >= 0)
        {
            if (depthDiff < depthDiffError)
                break;
            currOffset -= rayStep;
            rayStep *= 0.5;
        }

        // Move forward
        currOffset += rayStep;
        currSampleIndex++;
    }
    if (currSampleIndex >= numSamples)
        return 0; // All samples done but no result
#endif

    // Fade rays close to screen edge
    const float fadeStart = 0.9f;
    const float fadeEnd = 1.0f;
    const float fadeDiffRcp = 1.0f / (fadeEnd - fadeStart);
    float2 boundary = abs(currOffset.xy - float2(0.5f, 0.5f)) * 2.0f;
    float fadeOnBorder = 1.0f - saturate((boundary.x - fadeStart) * fadeDiffRcp);
    fadeOnBorder *= 1.0f - saturate((boundary.y - fadeStart) * fadeDiffRcp);
    fadeOnBorder = smoothstep(0.0f, 1.0f, fadeOnBorder);
    fadeOnBorder *= RayAttenBorder(currOffset.xy, edgeFade);

    // Fade rays on high roughness
    float roughnessFade = saturate((roughnessThreshold - gBuffer.Roughness) * 20);

    // Fade on distance
    float distanceFade = saturate((drawDistance - gBuffer.ViewPos.z) / drawDistance);

    // Fade on ray
    float reflectFade = saturate(reflectVS.z * 8.0f);

    // Output: xy: hitUV, z: hitMask
    return float3(currOffset.xy, fadeOnBorder * roughnessFade * distanceFade * reflectFade);
}
