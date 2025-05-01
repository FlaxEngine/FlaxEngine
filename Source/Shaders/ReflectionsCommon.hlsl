// Copyright (c) Wojciech Figat. All rights reserved.

#ifndef __REFLECTIONS_COMMON__
#define __REFLECTIONS_COMMON__

#include "./Flax/GBufferCommon.hlsl"

// Hit depth (view space) threshold to detect if sky was hit (value above it where 1.0f is default)
#define REFLECTIONS_HIT_THRESHOLD 0.9f

float GetSpecularOcclusion(float NoV, float roughnessSq, float ao)
{
    return saturate(pow(NoV + ao, roughnessSq) - 1 + ao);
}

float4 SampleReflectionProbe(float3 viewPos, TextureCube probe, ProbeData data, float3 positionWS, float3 normal, float roughness)
{
    // Calculate distance from probe to the pixel
    float3 captureVector = positionWS - data.ProbePos;
    float captureVectorLength = length(captureVector);

    // Check if cannot light pixel
    // TODO: maybe remove this check?? - test it out with dozens of probes
    BRANCH
    if (captureVectorLength >= data.ProbeRadius)
    {
        // End
        return 0;
    }

    // Fade out based on distance to capture
    float normalizedDistanceToCapture = saturate(captureVectorLength * data.ProbeInvRadius);
    float distanceAlpha = 1.0 - smoothstep(0.7, 1, normalizedDistanceToCapture);
    float fade = distanceAlpha * data.ProbeBrightness;

    // Calculate reflection vector
    float3 V = normalize(positionWS - viewPos);
    float3 R = reflect(V, normal);
    float3 D = data.ProbeInvRadius * captureVector + R;

    // Sample probe at valid mip level based on surface roughness value
    half mip = ProbeMipFromRoughness(roughness);
    float4 probeSample = probe.SampleLevel(SamplerLinearClamp, D, mip);

    return probeSample * fade;
}

// Calculates the reflective environment lighting to multiply the raw reflection color for the specular light (eg. from Env Probe or SSR).
float3 GetReflectionSpecularLighting(float3 viewPos, GBufferSample gBuffer)
{
    float3 specularColor = GetSpecularColor(gBuffer);
    float3 V = normalize(viewPos - gBuffer.WorldPos);
    float NoV = saturate(dot(gBuffer.Normal, V));
    return EnvBRDFApprox(specularColor, gBuffer.Roughness, NoV);
}

#endif
