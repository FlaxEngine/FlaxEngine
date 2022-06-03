// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

// Implementation based on:
// "Dynamic Diffuse Global Illumination with Ray-Traced Irradiance Probes", Journal of Computer Graphics Tools, April 2019
// Zander Majercik, Jean-Philippe Guertin, Derek Nowrouzezahrai, and Morgan McGuire
// https://morgan3d.github.io/articles/2019-04-01-ddgi/index.html and https://gdcvault.com/play/1026182/
//
// Additional references:
// "Scaling Probe-Based Real-Time Dynamic Global Illumination for Production", https://jcgt.org/published/0010/02/01/
// "Dynamic Diffuse Global Illumination with Ray-Traced Irradiance Fields", https://jcgt.org/published/0008/02/01/

#include "./Flax/Common.hlsl"
#include "./Flax/Math.hlsl"
#include "./Flax/Octahedral.hlsl"

#define DDGI_PROBE_STATE_ACTIVE 0
#define DDGI_PROBE_STATE_INACTIVE 1
#define DDGI_PROBE_RESOLUTION_IRRADIANCE 6 // Resolution (in texels) for probe irradiance data (excluding 1px padding on each side)
#define DDGI_PROBE_RESOLUTION_DISTANCE 14 // Resolution (in texels) for probe distance data (excluding 1px padding on each side)
#define DDGI_SRGB_BLENDING 1 // Enables blending in sRGB color space, otherwise irradiance blending is done in linear space

// DDGI data for a constant buffer
struct DDGIData
{
    float3 ProbesOrigin;
    float ProbesSpacing;
    float4 RaysRotation;
    uint3 ProbesCounts;
    float IrradianceGamma;
    int3 ProbesScrollOffsets;
    float ProbeHistoryWeight;
    float3 ViewDir;
    uint RaysCount;
    int3 ProbeScrollDirections;
    float RayMaxDistance;
    uint3 ProbeScrollClear; // TODO: pack into bits
    uint Padding0;
};

uint GetDDGIProbeIndex(DDGIData data, uint3 probeCoords)
{
    uint probesPerPlane = data.ProbesCounts.x * data.ProbesCounts.z;
    uint planeIndex = probeCoords.y;
    uint probeIndexInPlane = probeCoords.x + (data.ProbesCounts.x * probeCoords.z);
    return planeIndex * probesPerPlane + probeIndexInPlane;
}

uint GetDDGIProbeIndex(DDGIData data, uint2 texCoords, uint texResolution)
{
    uint probesPerPlane = data.ProbesCounts.x * data.ProbesCounts.z;
    uint planeIndex = texCoords.x / (data.ProbesCounts.x * texResolution);
    uint probeIndexInPlane = (texCoords.x / texResolution) - (planeIndex * data.ProbesCounts.x) + (data.ProbesCounts.x * (texCoords.y / texResolution));
    return planeIndex * probesPerPlane + probeIndexInPlane;
}

uint3 GetDDGIProbeCoords(DDGIData data, uint probeIndex)
{
    uint3 probeCoords;
    probeCoords.x = probeIndex % data.ProbesCounts.x;
    probeCoords.y = probeIndex / (data.ProbesCounts.x * data.ProbesCounts.z);
    probeCoords.z = (probeIndex / data.ProbesCounts.x) % data.ProbesCounts.z;
    return probeCoords;
}

uint2 GetDDGIProbeTexelCoords(DDGIData data, uint probeIndex)
{
    uint probesPerPlane = data.ProbesCounts.x * data.ProbesCounts.z;
    uint planeIndex = probeIndex / probesPerPlane;
    uint gridSpaceX = probeIndex % data.ProbesCounts.x;
    uint gridSpaceY = probeIndex / data.ProbesCounts.x;
    uint x = gridSpaceX + (planeIndex * data.ProbesCounts.x);
    uint y = gridSpaceY % data.ProbesCounts.z;
    return uint2(x, y);
}

uint GetDDGIScrollingProbeIndex(DDGIData data, uint3 probeCoords)
{
    // Probes are scrolled on edges to stabilize GI when camera moves
    return GetDDGIProbeIndex(data, (probeCoords + data.ProbesScrollOffsets + data.ProbesCounts) % data.ProbesCounts);
}

float3 GetDDGIProbeWorldPosition(DDGIData data, uint3 probeCoords)
{
    float3 probePosition = probeCoords * data.ProbesSpacing;
    float3 probeGridOffset = (data.ProbesSpacing * (data.ProbesCounts - 1)) * 0.5f;
    return data.ProbesOrigin + probePosition - probeGridOffset + (data.ProbesScrollOffsets * data.ProbesSpacing);
}

// Loads probe probe state
float LoadDDGIProbeState(DDGIData data, Texture2D<float4> probesState, uint probeIndex)
{
    int2 probeDataCoords = GetDDGIProbeTexelCoords(data, probeIndex);
    float4 probeState = probesState.Load(int3(probeDataCoords, 0));
    return probeState.w;
}

// Loads probe world-space position (XYZ) and probe state (W)
float4 LoadDDGIProbePositionAndState(DDGIData data, Texture2D<float4> probesState, uint probeIndex, uint3 probeCoords)
{
    int2 probeDataCoords = GetDDGIProbeTexelCoords(data, probeIndex);
    float4 probeState = probesState.Load(int3(probeDataCoords, 0));
    probeState.xyz += GetDDGIProbeWorldPosition(data, probeCoords);
    return probeState;
}

// Calculates texture UVs for sampling probes atlas texture (irradiance or distance)
float2 GetDDGIProbeUV(DDGIData data, uint probeIndex, float2 octahedralCoords, uint resolution)
{
    uint2 coords = GetDDGIProbeTexelCoords(data, probeIndex);
    float probeTexelSize = resolution + 2.0f;
    float textureWidth = probeTexelSize * (data.ProbesCounts.x * data.ProbesCounts.y);
    float textureHeight = probeTexelSize * data.ProbesCounts.z;
    float2 uv = float2(coords.x * probeTexelSize, coords.y * probeTexelSize) + (probeTexelSize * 0.5f);
    uv += octahedralCoords.xy * (resolution * 0.5f);
    uv /= float2(textureWidth, textureHeight);
    return uv;
}

// Samples DDGI probes volume at the given world-space position and returns the irradiance.
float3 SampleDDGIIrradiance(DDGIData data, Texture2D<float4> probesState, Texture2D<float4> probesDistance, Texture2D<float4> probesIrradiance, float3 worldPosition, float3 worldNormal, float bias)
{
    float4 irradiance = float4(0, 0, 0, 0);
    float3 probesOrigin = data.ProbesScrollOffsets * data.ProbesSpacing + data.ProbesOrigin;
    float3 probesExtent = (data.ProbesCounts - 1) * (data.ProbesSpacing * 0.5f);

    // Bias the world-space position to reduce artifacts
    float3 surfaceBias = (worldNormal * bias) + (data.ViewDir * (bias * -4.0f));
    float3 biasedWorldPosition = worldPosition + surfaceBias;

    // Get the grid coordinates of the probe nearest the biased world position
    uint3 baseProbeCoords = clamp(uint3((worldPosition - probesOrigin + probesExtent) / data.ProbesSpacing), 0, data.ProbesCounts - 1);
    float3 baseProbeWorldPosition = GetDDGIProbeWorldPosition(data, baseProbeCoords);
    float3 biasAlpha = saturate((biasedWorldPosition - baseProbeWorldPosition) / data.ProbesSpacing);

    // Loop over the closest probes to accumulate their contributions
    for (uint i = 0; i < 8; i++)
    {
        uint3 probeCoordsOffset = uint3(i, i >> 1, i >> 2) & 1;
        uint3 probeCoords = clamp(baseProbeCoords + probeCoordsOffset, 0, data.ProbesCounts - 1);
        uint probeIndex = GetDDGIScrollingProbeIndex(data, probeCoords);

        // Load probe position and state
        float4 probeState = probesState.Load(int3(GetDDGIProbeTexelCoords(data, probeIndex), 0));
        if (probeState.w == DDGI_PROBE_STATE_INACTIVE)
            continue;
        float3 probePosition = baseProbeWorldPosition + ((probeCoords - baseProbeCoords) * data.ProbesSpacing) + probeState.xyz;

        // Calculate the distance and direction from the (biased and non-biased) shading point and the probe
        float3 worldPosToProbe = normalize(probePosition.xyz - worldPosition);
        float3 biasedPosToProbe = normalize(probePosition.xyz - biasedWorldPosition);
        float biasedPosToProbeDist = length(probePosition.xyz - biasedWorldPosition);

        // Smooth backface test
        float weight = Square(dot(worldPosToProbe, worldNormal) * 0.5f + 0.5f);

        // Sample distance texture
        float2 octahedralCoords = GetOctahedralCoords(-biasedPosToProbe);
        float2 uv = GetDDGIProbeUV(data, probeIndex, octahedralCoords, DDGI_PROBE_RESOLUTION_DISTANCE);
        float2 probeDistance = probesDistance.SampleLevel(SamplerLinearClamp, uv, 0).rg * 2.0f;
        float probeDistanceMean = probeDistance.x;
        float probeDistanceMean2 = probeDistance.y;

        // Visibility weight (Chebyshev)
        if (biasedPosToProbeDist > probeDistanceMean)
        {
            float probeDistanceVariance = abs(Square(probeDistanceMean) - probeDistanceMean2);
            float chebyshevWeight = probeDistanceVariance / (probeDistanceVariance + Square(biasedPosToProbeDist - probeDistanceMean));
            weight *= max(chebyshevWeight * chebyshevWeight * chebyshevWeight, 0.05f);
        }

        // Avoid a weight of zero
        weight = max(weight, 0.000001f);

        // Adjust weight curve to inject a small portion of light
        const float minWeightThreshold = 0.2f;
        if (weight < minWeightThreshold)
            weight *= Square(weight) * (1.0f / (minWeightThreshold * minWeightThreshold));

        // Calculate trilinear weights based on the distance to each probe to smoothly transition between grid of 8 probes
        float3 trilinear = lerp(1.0f - biasAlpha, biasAlpha, probeCoordsOffset);
        weight *= max(trilinear.x * trilinear.y * trilinear.z, 0.001f);

        // Sample irradiance texture
        octahedralCoords = GetOctahedralCoords(worldNormal);
        uv = GetDDGIProbeUV(data, probeIndex, octahedralCoords, DDGI_PROBE_RESOLUTION_IRRADIANCE);
        float3 probeIrradiance = probesIrradiance.SampleLevel(SamplerLinearClamp, uv, 0).rgb;
#if DDGI_SRGB_BLENDING
        probeIrradiance = pow(probeIrradiance, data.IrradianceGamma * 0.5f);
#endif

        // Debug probe offset visualization
        //probeIrradiance = float3(max(frac(probeState.xyz) * 2, 0.1f));

        // Accumulate weighted irradiance
        irradiance += float4(probeIrradiance * weight, weight);
    }

    if (irradiance.a > 0.0f)
    {
        // Normalize irradiance
        irradiance.rgb *= 1.f / irradiance.a;
#if DDGI_SRGB_BLENDING
        irradiance.rgb *= irradiance.rgb;
#endif
        irradiance.rgb *= 2.0f * PI;

        // Fade-out outside the probes volume
        float fadeDistance = data.ProbesSpacing * 0.5f;
        irradiance.rgb *= saturate(Min3(probesExtent - abs(worldPosition - probesOrigin)) / fadeDistance);
    }
    return irradiance.rgb;
}
