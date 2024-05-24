// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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

#define DDGI_PROBE_STATE_INACTIVE 0
#define DDGI_PROBE_STATE_ACTIVATED 1
#define DDGI_PROBE_STATE_ACTIVE 2
#define DDGI_PROBE_RESOLUTION_IRRADIANCE 6 // Resolution (in texels) for probe irradiance data (excluding 1px padding on each side)
#define DDGI_PROBE_RESOLUTION_DISTANCE 14 // Resolution (in texels) for probe distance data (excluding 1px padding on each side)
#define DDGI_SRGB_BLENDING 1 // Enables blending in sRGB color space, otherwise irradiance blending is done in linear space

// DDGI data for a constant buffer
struct DDGIData
{
    float4 ProbesOriginAndSpacing[4];
    int4 ProbesScrollOffsets[4]; // w unused
    uint3 ProbesCounts;
    uint CascadesCount;
    float IrradianceGamma;
    float ProbeHistoryWeight;
    float RayMaxDistance;
    float IndirectLightingIntensity;
    float4 RaysRotation;
    float3 ViewPos;
    uint RaysCount;
    float3 FallbackIrradiance;
    float Padding0;
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

uint2 GetDDGIProbeTexelCoords(DDGIData data, uint cascadeIndex, uint probeIndex)
{
    uint probesPerPlane = data.ProbesCounts.x * data.ProbesCounts.z;
    uint planeIndex = probeIndex / probesPerPlane;
    uint gridSpaceX = probeIndex % data.ProbesCounts.x;
    uint gridSpaceY = probeIndex / data.ProbesCounts.x;
    uint x = gridSpaceX + (planeIndex * data.ProbesCounts.x);
    uint y = gridSpaceY % data.ProbesCounts.z + cascadeIndex * data.ProbesCounts.z;
    return uint2(x, y);
}

uint GetDDGIScrollingProbeIndex(DDGIData data, uint cascadeIndex, uint3 probeCoords)
{
    // Probes are scrolled on edges to stabilize GI when camera moves
    return GetDDGIProbeIndex(data, (probeCoords + data.ProbesCounts + data.ProbesScrollOffsets[cascadeIndex].xyz) % data.ProbesCounts);
}

float3 GetDDGIProbeWorldPosition(DDGIData data, uint cascadeIndex, uint3 probeCoords)
{
    float3 probesOrigin = data.ProbesOriginAndSpacing[cascadeIndex].xyz;
    float probesSpacing = data.ProbesOriginAndSpacing[cascadeIndex].w;
    float3 probePosition = probeCoords * probesSpacing;
    float3 probeGridOffset = (probesSpacing * (data.ProbesCounts - 1)) * 0.5f;
    return probesOrigin + probePosition - probeGridOffset + (data.ProbesScrollOffsets[cascadeIndex].xyz * probesSpacing);
}

// Loads probe probe data (encoded)
float4 LoadDDGIProbeData(DDGIData data, Texture2D<snorm float4> probesData, uint cascadeIndex, uint probeIndex)
{
    int2 probeDataCoords = GetDDGIProbeTexelCoords(data, cascadeIndex, probeIndex);
    return probesData.Load(int3(probeDataCoords, 0));
}

// Encodes probe probe data
float4 EncodeDDGIProbeData(float3 probeOffset, uint probeState)
{
    return float4(probeOffset, (float)probeState * (1.0f / 8.0f));
}

// Decodes probe state from the encoded state
uint DecodeDDGIProbeState(float4 probeData)
{
    return (uint)(probeData.w * 8.0f);
}

// Decodes probe world-space position (XYZ) from the encoded state
float3 DecodeDDGIProbePosition(DDGIData data, float4 probeData, uint cascadeIndex, uint probeIndex, uint3 probeCoords)
{
    float3 probePosition = probeData.xyz;
    probePosition *= data.ProbesOriginAndSpacing[cascadeIndex].w; // Probe offset is [-1;1] within probes spacing
    probePosition += GetDDGIProbeWorldPosition(data, cascadeIndex, probeCoords); // Place probe on a grid
    return probePosition;
}

// Calculates texture UVs for sampling probes atlas texture (irradiance or distance)
float2 GetDDGIProbeUV(DDGIData data, uint cascadeIndex, uint probeIndex, float2 octahedralCoords, uint resolution)
{
    uint2 coords = GetDDGIProbeTexelCoords(data, cascadeIndex, probeIndex);
    float probeTexelSize = resolution + 2.0f;
    float2 textureSize = float2(data.ProbesCounts.x * data.ProbesCounts.y, data.ProbesCounts.z * data.CascadesCount) * probeTexelSize;
    float2 uv = float2(coords.x * probeTexelSize, coords.y * probeTexelSize) + (probeTexelSize * 0.5f);
    uv += octahedralCoords.xy * (resolution * 0.5f);
    uv /= textureSize;
    return uv;
}

// Samples DDGI probes volume at the given world-space position and returns the irradiance.
// bias - scales the bias vector to the initial sample point to reduce self-shading artifacts
// dither - randomized per-pixel value in range 0-1, used to smooth dithering for cascades blending
float3 SampleDDGIIrradiance(DDGIData data, Texture2D<snorm float4> probesData, Texture2D<float4> probesDistance, Texture2D<float4> probesIrradiance, float3 worldPosition, float3 worldNormal, float bias = 0.2f, float dither = 0.0f)
{
    // Select the highest cascade that contains the sample location
    uint cascadeIndex = 0;
    float4 probesDatas[8];
    for (; cascadeIndex < data.CascadesCount; cascadeIndex++)
    {
        float probesSpacing = data.ProbesOriginAndSpacing[cascadeIndex].w;
        float3 probesOrigin = data.ProbesScrollOffsets[cascadeIndex].xyz * probesSpacing + data.ProbesOriginAndSpacing[cascadeIndex].xyz;
        float3 probesExtent = (data.ProbesCounts - 1) * (probesSpacing * 0.5f);
        float fadeDistance = probesSpacing * 0.5f;
        float cascadeWeight = saturate(Min3(probesExtent - abs(worldPosition - probesOrigin)) / fadeDistance);
        if (cascadeWeight > dither) // Use dither to make transition smoother
        {
            // Load probes state for this cascade
            uint activeCount = 0;
            uint3 baseProbeCoords = clamp(uint3((worldPosition - probesOrigin + probesExtent) / probesSpacing), uint3(0, 0, 0), data.ProbesCounts - uint3(1, 1, 1));
            UNROLL
            for (uint i = 0; i < 8; i++)
            {
                uint3 probeCoordsOffset = uint3(i, i >> 1, i >> 2) & 1;
                uint3 probeCoords = clamp(baseProbeCoords + probeCoordsOffset, uint3(0, 0, 0), data.ProbesCounts - uint3(1, 1, 1));
                uint probeIndex = GetDDGIScrollingProbeIndex(data, cascadeIndex, probeCoords);
                float4 probeData = LoadDDGIProbeData(data, probesData, cascadeIndex, probeIndex);
                probesDatas[i] = probeData;
                uint probeState = DecodeDDGIProbeState(probeData);
                if (probeState != DDGI_PROBE_STATE_INACTIVE)
                    activeCount++;
            }

            // Ensure there are some valid probes in this cascade
            if (activeCount >= 3)
                break;
        }
    }
    if (cascadeIndex == data.CascadesCount)
        return data.FallbackIrradiance;

    float probesSpacing = data.ProbesOriginAndSpacing[cascadeIndex].w;
    float3 probesOrigin = data.ProbesScrollOffsets[cascadeIndex].xyz * probesSpacing + data.ProbesOriginAndSpacing[cascadeIndex].xyz;
    float3 probesExtent = (data.ProbesCounts - 1) * (probesSpacing * 0.5f);

    // Bias the world-space position to reduce artifacts
    float3 viewDir = normalize(data.ViewPos - worldPosition);
    float3 surfaceBias = (worldNormal * 0.2f + viewDir * 0.8f) * (0.75f * probesSpacing * bias);
    float3 biasedWorldPosition = worldPosition + surfaceBias;

    // Get the grid coordinates of the probe nearest the biased world position
    uint3 baseProbeCoords = clamp(uint3((worldPosition - probesOrigin + probesExtent) / probesSpacing), uint3(0, 0, 0), data.ProbesCounts - uint3(1, 1, 1));
    float3 baseProbeWorldPosition = GetDDGIProbeWorldPosition(data, cascadeIndex, baseProbeCoords);
    float3 biasAlpha = saturate((biasedWorldPosition - baseProbeWorldPosition) / probesSpacing);

    // Loop over the closest probes to accumulate their contributions
    float4 irradiance = float4(0, 0, 0, 0);
    for (uint i = 0; i < 8; i++)
    {
        uint3 probeCoordsOffset = uint3(i, i >> 1, i >> 2) & 1;
        uint3 probeCoords = clamp(baseProbeCoords + probeCoordsOffset, uint3(0, 0, 0), data.ProbesCounts - uint3(1, 1, 1));
        uint probeIndex = GetDDGIScrollingProbeIndex(data, cascadeIndex, probeCoords);

        // Load probe position and state
        float4 probeData = probesDatas[i];
        uint probeState = DecodeDDGIProbeState(probeData);
        if (probeState == DDGI_PROBE_STATE_INACTIVE)
            continue;
        float3 probeBasePosition = baseProbeWorldPosition + ((probeCoords - baseProbeCoords) * probesSpacing);
        float3 probePosition = probeBasePosition + probeData.xyz * probesSpacing; // Probe offset is [-1;1] within probes spacing

        // Calculate the distance and direction from the (biased and non-biased) shading point and the probe
        float3 worldPosToProbe = normalize(probePosition - worldPosition);
        float3 biasedPosToProbe = normalize(probePosition - biasedWorldPosition);
        float biasedPosToProbeDist = length(probePosition - biasedWorldPosition) * 0.95f;

        // Smooth backface test
        float weight = Square(dot(worldPosToProbe, worldNormal) * 0.5f + 0.5f);

        // Sample distance texture
        float2 octahedralCoords = GetOctahedralCoords(-biasedPosToProbe);
        float2 uv = GetDDGIProbeUV(data, cascadeIndex, probeIndex, octahedralCoords, DDGI_PROBE_RESOLUTION_DISTANCE);
        float2 probeDistance = probesDistance.SampleLevel(SamplerLinearClamp, uv, 0).rg * 2.0f;
        float probeDistanceMean = probeDistance.x;

        // Visibility weight (Chebyshev)
        if (biasedPosToProbeDist > probeDistanceMean)
        {
            float probeDistanceMean2 = probeDistance.y;
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
        float3 trilinear = lerp(1.0f - biasAlpha, biasAlpha, (float3)probeCoordsOffset);
        weight *= max(trilinear.x * trilinear.y * trilinear.z, 0.001f);

        // Sample irradiance texture
        octahedralCoords = GetOctahedralCoords(worldNormal);
        uv = GetDDGIProbeUV(data, cascadeIndex, probeIndex, octahedralCoords, DDGI_PROBE_RESOLUTION_IRRADIANCE);
        float3 probeIrradiance = probesIrradiance.SampleLevel(SamplerLinearClamp, uv, 0).rgb;
#if DDGI_SRGB_BLENDING
        probeIrradiance = pow(probeIrradiance, data.IrradianceGamma * 0.5f);
#endif

        // Debug probe offset visualization
        //probeIrradiance = float3(max(frac(probeData.xyz) * 2, 0.1f));

        // Accumulate weighted irradiance
        irradiance += float4(probeIrradiance * weight, weight);
    }

#if 0
    // Debug DDGI cascades with colors
    if (cascadeIndex == 0)
        irradiance = float4(1, 0, 0, 1);
    else if (cascadeIndex == 1)
        irradiance = float4(0, 1, 0, 1);
    else if (cascadeIndex == 2)
        irradiance = float4(0, 0, 1, 1);
    else
        irradiance = float4(1, 0, 1, 1);
#endif

    if (irradiance.a > 0.0f)
    {
        // Normalize irradiance
        irradiance.rgb *= 1.f / irradiance.a;
#if DDGI_SRGB_BLENDING
        irradiance.rgb *= irradiance.rgb;
#endif
        irradiance.rgb *= 2.0f * PI;
    }
    return irradiance.rgb;
}
