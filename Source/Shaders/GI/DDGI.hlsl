// Copyright (c) Wojciech Figat. All rights reserved.

// Implementation based on:
// "Dynamic Diffuse Global Illumination with Ray-Traced Irradiance Probes", Journal of Computer Graphics Tools, April 2019
// Zander Majercik, Jean-Philippe Guertin, Derek Nowrouzezahrai, and Morgan McGuire
// https://morgan3d.github.io/articles/2019-04-01-ddgi/index.html and https://gdcvault.com/play/1026182/
//
// Additional references:
// "Scaling Probe-Based Real-Time Dynamic Global Illumination for Production", https://jcgt.org/published/0010/02/01/
// "Dynamic Diffuse Global Illumination with Ray-Traced Irradiance Fields", https://jcgt.org/published/0008/02/01/

#include "./Flax/Common.hlsl"
#include "./Flax/Math/Math.hlsl"
#include "./Flax/Math/Octahedral.hlsl"

#define DDGI_PROBE_STATE_INACTIVE 0
#define DDGI_PROBE_STATE_ACTIVATED 1
#define DDGI_PROBE_STATE_ACTIVE 2
#define DDGI_PROBE_ATTENTION_MIN 0.02f // Minimum probe attention value that still makes it active.
#define DDGI_PROBE_ATTENTION_MAX 0.98f // Maximum probe attention value that still makes it active (but not activated which is 1.0f).
#define DDGI_PROBE_RESOLUTION_IRRADIANCE 6 // Resolution (in texels) for probe irradiance data (excluding 1px padding on each side)
#define DDGI_PROBE_RESOLUTION_RADIANCE 14 // Resolution (in texels) for probe radiance data (excluding 1px padding on each side)
#define DDGI_PROBE_RESOLUTION_DISTANCE 14 // Resolution (in texels) for probe distance data (excluding 1px padding on each side)
#define DDGI_CASCADE_BLEND_SIZE 2.0f // Distance in probes over which cascades blending happens
#ifndef DDGI_CASCADE_BLEND_SMOOTH
#define DDGI_CASCADE_BLEND_SMOOTH 0 // Enables smooth cascade blending, otherwise dithering will be used
#endif
#define DDGI_SRGB_BLENDING 2 // Enables blending in sRGB color space (1 - with custom gamma, 2 - simple square/sqrt), otherwise irradiance blending is done in linear space
#define DDGI_SRGB_BLENDING_GAMMA 0.75f
#define DDGI_DEFAULT_BIAS 0.2f // Default value for DDGI sampling bias
#define DDGI_FALLBACK_COORDS_ENCODE(coord) ((float3)(coord + 1) / 128.0f)
#define DDGI_FALLBACK_COORDS_DECODE(data) (uint3)(data.xyz * 128.0f - 1)
#define DDGI_FALLBACK_COORDS_VALID(data) (length(data.xyz) > 0)
#define DDGI_FALLBACK_OUTER_DEDICATED_PROBE 1 // Enables using a special probe at (0, 0, 0) of the last cascade to be used for ambient GI on far pixels outside the DDGI range
//#define DDGI_DEBUG_CASCADE 0 // Forces a specific cascade to be only in use (for debugging)

// DDGI data for a constant buffer
struct DDGIData
{
    float4 ProbesOriginAndSpacing[4];
    float4 BlendOrigin[4]; // [0] w is flag for specular usage, [1-3] w is unused
    int4 ProbesScrollOffsets[4]; // w is unused
    uint3 ProbesCounts;
    uint CascadesCount;
    float ProbeHistoryWeight;
    float RayMaxDistance;
    float IndirectLightingIntensity;
    float IndirectShadowsStrength;
    float3 ViewPos;
    uint RaysCount;
    float4 FallbackIrradiance;
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
    int3 probeCoordsOffset = (int3)data.ProbesCounts + data.ProbesScrollOffsets[cascadeIndex].xyz;
    return GetDDGIProbeIndex(data, (probeCoords + (uint3)probeCoordsOffset) % data.ProbesCounts);
}

float3 GetDDGIProbeWorldPosition(DDGIData data, uint cascadeIndex, uint3 probeCoords)
{
    float3 probesOrigin = data.ProbesOriginAndSpacing[cascadeIndex].xyz;
    float probesSpacing = data.ProbesOriginAndSpacing[cascadeIndex].w;
    float3 probePosition = probeCoords * probesSpacing;
    float3 probeGridOffset = (probesSpacing * (data.ProbesCounts - 1)) * 0.5f;
    float3 probeScrollOffset = data.ProbesScrollOffsets[cascadeIndex].xyz * probesSpacing;
    return probesOrigin + probePosition - probeGridOffset + probeScrollOffset;
}

// Loads probe probe data (encoded)
float4 LoadDDGIProbeData(DDGIData data, Texture2D<snorm float4> probesData, uint cascadeIndex, uint probeIndex)
{
    int2 probeDataCoords = GetDDGIProbeTexelCoords(data, cascadeIndex, probeIndex);
    return probesData.Load(int3(probeDataCoords, 0));
}

// Encodes probe probe data
float4 EncodeDDGIProbeData(float3 offset, uint state, float attention)
{
    // [0;1] -> [-1;1]
    attention = saturate(attention) * 2.0f - 1.0f;
    if (state == DDGI_PROBE_STATE_INACTIVE)
        attention = -1.0f;
    else if (state == DDGI_PROBE_STATE_ACTIVATED)
        attention = 1.0f;
    return float4(offset, attention);
}

// Decodes probe attention value from the encoded state
float DecodeDDGIProbeAttention(float4 probeData)
{
    // [-1;1] -> [0;1]
    if (probeData.w <= -1.0f)
        return 0.0f;
    if (probeData.w >= 1.0f)
        return 1.0f;
    return probeData.w * 0.5f + 0.5f;
}

// Decodes probe state from the encoded state
uint DecodeDDGIProbeState(float4 probeData)
{
    if (probeData.w <= -1.0f)
        return DDGI_PROBE_STATE_INACTIVE;
    if (probeData.w >= 1.0f)
        return DDGI_PROBE_STATE_ACTIVATED;
    return DDGI_PROBE_STATE_ACTIVE;
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
    uv += octahedralCoords * (resolution * 0.5f);
    uv /= textureSize;
    return uv;
}

struct DDGICascadeSampling
{
    float3 ProbesOrigin;
    uint CascadeIndex;
    float3 ProbesExtent;
    float ProbesSpacing;
    float3 BiasedWorldPosition;
    float CascadeWeight;
};

struct DDGIProbeBase
{
    uint3 ProbeCoords;
    float3 ProbeWorldPosition;
    float3 BiasAlpha;
};

struct DDGIProbeSample
{
    float2 Weights;
    uint ProbeIndex;
    float3 ProbePosition;
};

DDGIProbeBase GetDDGIProbeBase(DDGIData data, DDGICascadeSampling cascade, float3 worldPosition)
{
    // Get the grid coordinates of the probe nearest the biased world position
    DDGIProbeBase base;
    base.ProbeCoords = clamp(uint3((worldPosition - cascade.ProbesOrigin + cascade.ProbesExtent) / cascade.ProbesSpacing), uint3(0, 0, 0), data.ProbesCounts - uint3(1, 1, 1));
    base.ProbeWorldPosition = GetDDGIProbeWorldPosition(data, cascade.CascadeIndex, base.ProbeCoords);
    base.BiasAlpha = saturate((cascade.BiasedWorldPosition - base.ProbeWorldPosition) / cascade.ProbesSpacing);
    return base;
}

DDGIProbeSample SampleDDGIProbe(DDGIData data, Texture2D<snorm float4> probesData, Texture2D<float4> probesDistance, float3 worldPosition, float3 worldNormal, DDGICascadeSampling cascade, DDGIProbeBase base, uint i, inout uint fallbacks)
{
    DDGIProbeSample probe;
    uint3 probeCoordsOffset = uint3(i, i >> 2u, i >> 1u) & uint3(1u, 1u, 1u);
    uint3 probeCoords = clamp(base.ProbeCoords + probeCoordsOffset, uint3(0, 0, 0), data.ProbesCounts - uint3(1, 1, 1));
    probe.ProbeIndex = GetDDGIScrollingProbeIndex(data, cascade.CascadeIndex, probeCoords);

    // Load probe position and state
    float4 probeData = LoadDDGIProbeData(data, probesData, cascade.CascadeIndex, probe.ProbeIndex);
    uint probeState = DecodeDDGIProbeState(probeData);
    uint useVisibility = true;
    float minWight = 0.001f;
    if (probeState == DDGI_PROBE_STATE_INACTIVE)
    {
        // Use fallback probe that is closest to this one
        uint3 fallbackCoords = DDGI_FALLBACK_COORDS_DECODE(probeData);
        float fallbackToProbeDist = length((float3)probeCoords - (float3)fallbackCoords);
        useVisibility = fallbackToProbeDist <= 1.0f; // Skip visibility test that blocks too far probes due to limiting max distance to 1.5 of probe spacing
        if (fallbackToProbeDist > 2.0f) minWight = 1.0f;
        probeCoords = fallbackCoords;
        probe.ProbeIndex = GetDDGIScrollingProbeIndex(data, cascade.CascadeIndex, fallbackCoords);
        probeData = LoadDDGIProbeData(data, probesData, cascade.CascadeIndex, probe.ProbeIndex);
        fallbacks++;
        //if (DecodeDDGIProbeState(probeData) == DDGI_PROBE_STATE_INACTIVE) continue;
    }

    // Calculate probe position
    probe.ProbePosition = base.ProbeWorldPosition + (((float3)probeCoords - (float3)base.ProbeCoords) * cascade.ProbesSpacing) + probeData.xyz * cascade.ProbesSpacing;

    // Calculate the distance and direction from the (biased and non-biased) shading point and the probe
    float3 worldPosToProbe = normalize(probe.ProbePosition - worldPosition);
    float3 biasedPosToProbe = normalize(probe.ProbePosition - cascade.BiasedWorldPosition);
    float biasedPosToProbeDist = length(probe.ProbePosition - cascade.BiasedWorldPosition) * 0.95f;

    // Smooth backface test
    // x - weight, y - non-directional weight with a bias
#if LIGHTING_NO_DIRECTIONAL
    probe.Weights = float2(1, 1);
#else
    float backfaceWeight = Square(dot(worldPosToProbe, worldNormal) * 0.5f + 0.5f);
    probe.Weights = float2(max(backfaceWeight, 0.1f), backfaceWeight + 0.2f);
#endif

    // Sample distance texture
    float2 octahedralCoords = GetOctahedralCoords(-biasedPosToProbe);
    float2 uv = GetDDGIProbeUV(data, cascade.CascadeIndex, probe.ProbeIndex, octahedralCoords, DDGI_PROBE_RESOLUTION_DISTANCE);
    float2 probeDistance = probesDistance.SampleLevel(SamplerLinearClamp, uv, 0).rg;

    // Visibility weight (Chebyshev)
    if (biasedPosToProbeDist > probeDistance.x && useVisibility)
    {
        float variance = abs(Square(probeDistance.x) - probeDistance.y);
        float visibilityWeight = variance / (variance + Square(biasedPosToProbeDist - probeDistance.x));
        visibilityWeight = lerp(1, visibilityWeight, data.IndirectShadowsStrength);
        probe.Weights *= max(visibilityWeight * visibilityWeight * visibilityWeight, 0.0f);
    }

    // Avoid a weight of zero
    probe.Weights = max(probe.Weights, minWight);

    // Adjust weight curve to inject a small portion of light
    const float minWeightThreshold = 0.2f;
    if (probe.Weights.x < minWeightThreshold)
        probe.Weights.x *= (probe.Weights.x * probe.Weights.x) * (1.0f / (minWeightThreshold * minWeightThreshold));

    // Calculate trilinear weights based on the distance to each probe to smoothly transition between grid of 8 probes
    float3 trilinear = lerp(1.0f - base.BiasAlpha, base.BiasAlpha, (float3)probeCoordsOffset);
    probe.Weights *= saturate(trilinear.x * trilinear.y * trilinear.z * 2.0f);

    return probe;
}

float3 SampleDDGIIrradianceCascade(DDGIData data, Texture2D<snorm float4> probesData, Texture2D<float4> probesDistance, Texture2D<float4> probesIrradiance, float3 worldPosition, float3 worldNormal, DDGICascadeSampling cascade)
{
    bool invalidCascade = cascade.CascadeIndex >= data.CascadesCount;
    cascade.CascadeIndex = min(cascade.CascadeIndex, data.CascadesCount - 1);
    float2 octahedralCoords = GetOctahedralCoords(worldNormal);
#if DDGI_FALLBACK_OUTER_DEDICATED_PROBE
    if (invalidCascade)
    {
        // Sample a special probe as a fallback for ambient GI outside the last cascade
        float2 uv = GetDDGIProbeUV(data, cascade.CascadeIndex, 0, octahedralCoords, DDGI_PROBE_RESOLUTION_IRRADIANCE);
        float3 probeIrradiance = probesIrradiance.SampleLevel(SamplerLinearClamp, uv, 0).rgb;
#if DDGI_SRGB_BLENDING == 1
        probeIrradiance = Square(pow(probeIrradiance, DDGI_SRGB_BLENDING_GAMMA));
#endif
        probeIrradiance *= 2.0f * PI;
        return probeIrradiance;
    }
#endif

    DDGIProbeBase base = GetDDGIProbeBase(data, cascade, worldPosition);

    // Loop over the closest probes to accumulate their contributions
    float4 totalIrradiance = float4(0, 0, 0, 0);
    float4 totalIrradianceNonDir = float4(0, 0, 0, 0);
    uint fallbacks = 0;
    for (uint i = 0; i < 8; i++)
    {
        DDGIProbeSample probe = SampleDDGIProbe(data, probesData, probesDistance, worldPosition, worldNormal, cascade, base, i, fallbacks);

        // Sample irradiance texture
        float2 uv = GetDDGIProbeUV(data, cascade.CascadeIndex, probe.ProbeIndex, octahedralCoords, DDGI_PROBE_RESOLUTION_IRRADIANCE);
        float3 probeIrradiance = probesIrradiance.SampleLevel(SamplerLinearClamp, uv, 0).rgb;
#if DDGI_SRGB_BLENDING == 1
        probeIrradiance = pow(probeIrradiance, DDGI_SRGB_BLENDING_GAMMA);
#elif DDGI_SRGB_BLENDING == 2
        probeIrradiance = sqrt(probeIrradiance);
#endif

        // Accumulate weighted irradiance
        totalIrradiance += float4(probeIrradiance * probe.Weights.x, probe.Weights.x);
        totalIrradianceNonDir += float4(probeIrradiance * probe.Weights.y, probe.Weights.y);
    }

#if 0
    // Debug DDGI cascades with colors
    if (cascade.CascadeIndex == 0)
        totalIrradiance = float4(1, 0, 0, 1);
    else if (cascade.CascadeIndex == 1)
        totalIrradiance = float4(0, 1, 0, 1);
    else if (cascade.CascadeIndex == 2)
        totalIrradiance = float4(0, 0, 1, 1);
    else if (invalidCascade) // Area outside the last cascade that clamps to it
        totalIrradiance = float4(1, 0, 1, 1);
    else
        totalIrradiance = float4(0, 1, 1, 1);
#endif

    // Normalize irradiance
    totalIrradiance.a += 0.0001f; // Avoid division by zero
    float canNormalize = saturate(totalIrradianceNonDir.a * totalIrradianceNonDir.a + 0.9f); // Don't normalize when the weight is very low to preserve indirect shadowing
#if !DDGI_FALLBACK_OUTER_DEDICATED_PROBE
    canNormalize += invalidCascade ? 1 : 0; // Normalize when outside the last cascade to preserve ambient GI when not using ambient probe
#endif
    float shadowNormalization = lerp(1, totalIrradiance.a, saturate(canNormalize));
    totalIrradiance.rgb /= lerp(totalIrradiance.a, shadowNormalization, data.IndirectShadowsStrength);
    if (fallbacks >= 5 && totalIrradianceNonDir.a > 0.00001f)
    {
        // Use non-directional irradiance when sampling mostly fallback probes (out of place)
        totalIrradiance.rgb = totalIrradianceNonDir.rgb / totalIrradianceNonDir.a;
    }
#if DDGI_SRGB_BLENDING
    totalIrradiance.rgb *= totalIrradiance.rgb;
#endif
    totalIrradiance.rgb *= 2.0f * PI;
    return totalIrradiance.rgb;
}

// Cheap, deterministic 3D hash function in range [-1; 1]
float3 DDGIHash3D(float3 p)
{
    p = frac(p * float3(443.8975, 397.2973, 491.1871));
    p += dot(p.xyz, p.yzx + 19.19);
    return frac(frac(p.xxy * p.yzz) * 2.0 - 1.0);
}

float3 FilterProbeRadiance(DDGIData data, Texture2D<float4> probesRadiance, float roughness, uint cascadeIndex, uint probeIndex, float3 direction)
{
    uint2 coords = GetDDGIProbeTexelCoords(data, cascadeIndex, probeIndex);
    const float probeTexelSize = DDGI_PROBE_RESOLUTION_RADIANCE + 2.0f;
    float2 uv = float2(coords.x * probeTexelSize, coords.y * probeTexelSize) + (probeTexelSize * 0.5f);
    uv += GetOctahedralCoords(direction) * (DDGI_PROBE_RESOLUTION_RADIANCE * 0.5f);
    float2 textureSize = float2(data.ProbesCounts.x * data.ProbesCounts.y, data.ProbesCounts.z * data.CascadesCount) * probeTexelSize;
    uv /= textureSize;
    float3 radiance = probesRadiance.SampleLevel(SamplerLinearClamp, uv, 0).rgb;

    // Box-blur filter for roughness (not physically correct, but cheap)
    float2 uvBase = float2(coords.x * probeTexelSize, coords.y * probeTexelSize) + 1; // The first texel of the probe (skip 1px padding)
    float2 uvMin = uvBase / textureSize;
    float2 uvMax = (uvBase + float2(DDGI_PROBE_RESOLUTION_RADIANCE, DDGI_PROBE_RESOLUTION_RADIANCE)) / textureSize;
    float2 filterSize = (float2)1.0f / textureSize;
    float3 radiance00 = probesRadiance.SampleLevel(SamplerLinearClamp, clamp(uv - filterSize, uvMin, uvMax), 0).rgb;
    float3 radiance10 = probesRadiance.SampleLevel(SamplerLinearClamp, clamp(uv + float2(filterSize.x, -filterSize.y), uvMin, uvMax), 0).rgb;
    float3 radiance01 = probesRadiance.SampleLevel(SamplerLinearClamp, clamp(uv + float2(-filterSize.x, filterSize.y), uvMin, uvMax), 0).rgb;
    float3 radiance11 = probesRadiance.SampleLevel(SamplerLinearClamp, clamp(uv + filterSize, uvMin, uvMax), 0).rgb;
    float3 radianceRough = (radiance00 + radiance10 + radiance01 + radiance11) * 0.25f;
    float alpha = (roughness - 0.4f) / 0.6f; // Remap [0.4; 1] to [0; 1]
    radiance = lerp(radiance, radianceRough, saturate(alpha));

    return radiance;
}

float3 SampleDDGISpecularCascade(DDGIData data, Texture2D<snorm float4> probesData, Texture2D<float4> probesDistance, Texture2D<float4> probesRadiance, float3 worldPosition, float3 worldNormal, float roughness, float3 reflection, DDGICascadeSampling cascade)
{
    bool invalidCascade = cascade.CascadeIndex >= data.CascadesCount;
    cascade.CascadeIndex = min(cascade.CascadeIndex, data.CascadesCount - 1);
#if DDGI_FALLBACK_OUTER_DEDICATED_PROBE
    if (invalidCascade)
    {
        // Sample a special probe as a fallback for ambient sky reflection outside the last cascade
        return FilterProbeRadiance(data, probesRadiance, roughness, cascade.CascadeIndex, 0, reflection);
    }
#endif

    DDGIProbeBase base = GetDDGIProbeBase(data, cascade, worldPosition);
    float3 worldNoise = DDGIHash3D(worldPosition) * 0.1f;

    // Loop over the closest probes to accumulate their contributions
    float4 totalRadiance = float4(0, 0, 0, 0);
    uint fallbacks = 0;
    for (uint i = 0; i < 8; i++)
    {
        DDGIProbeSample probe = SampleDDGIProbe(data, probesData, probesDistance, worldPosition, worldNormal, cascade, base, i, fallbacks);

        // Parallax correction
        //float3 sampleVector = normalize((worldPosition - probe.ProbePosition) / (cascade.ProbesSpacing * 2) + reflection);
        float3 sampleVector = reflection;

        // Randomize sample vector to reduce blocky artifacts (due to low-res of probe)
        sampleVector += worldNoise;

        // Sample radiance texture
        float3 probeRadiance = FilterProbeRadiance(data, probesRadiance, roughness, cascade.CascadeIndex, probe.ProbeIndex, sampleVector);

        // Accumulate weighted radiance
        totalRadiance += float4(probeRadiance * probe.Weights.x, probe.Weights.x);
    }

    // Normalize radiance
    totalRadiance.rgb /= max(totalRadiance.a, 0.0001f);

    return totalRadiance.rgb;
}

float3 GetDDGISurfaceBias(float3 viewDir, float probesSpacing, float3 worldNormal, float bias)
{
#if LIGHTING_NO_DIRECTIONAL
    return 0;
#else
    // Bias the world-space position to reduce artifacts
    return (worldNormal * 0.2f + viewDir * 0.8f) * (0.6f * probesSpacing * bias);
    //return worldNormal * (0.2f * probesSpacing * bias);
#endif
}

// [Inigo Quilez, https://iquilezles.org/articles/distfunctions/]
float sdRoundBox(float3 p, float3 b, float r)
{
    float3 q = abs(p) - b + r;
    return length(max(q, 0.0f)) + min(max(q.x, max(q.y, q.z)), 0.0f) - r;
}

DDGICascadeSampling GetDDGICascade(DDGIData data, float3 worldPosition, float3 worldNormal, float bias, float dither)
{
    // Select the highest cascade that contains the sample location
    DDGICascadeSampling cascade;
    cascade.ProbesOrigin = float3(0, 0, 0);
    cascade.ProbesExtent = float3(0, 0, 0);
    cascade.BiasedWorldPosition = float3(0, 0, 0);
    cascade.ProbesSpacing = 0;
    cascade.CascadeWeight = 0;
    float3 viewDir = normalize(data.ViewPos - worldPosition);
#if DDGI_CASCADE_BLEND_SMOOTH
    dither = 0.0f;
#endif
#ifdef DDGI_DEBUG_CASCADE
    cascade.CascadeIndex = DDGI_DEBUG_CASCADE;
#else
    cascade.CascadeIndex = 0;
    for (; cascade.CascadeIndex < data.CascadesCount; cascade.CascadeIndex++)
    {
        // Get cascade data
        cascade.ProbesSpacing = data.ProbesOriginAndSpacing[cascade.CascadeIndex].w;
        cascade.ProbesOrigin = data.ProbesScrollOffsets[cascade.CascadeIndex].xyz * cascade.ProbesSpacing + data.ProbesOriginAndSpacing[cascade.CascadeIndex].xyz;
        cascade.ProbesExtent = (data.ProbesCounts - 1) * (cascade.ProbesSpacing * 0.5f);
        cascade.BiasedWorldPosition = worldPosition + GetDDGISurfaceBias(viewDir, cascade.ProbesSpacing, worldNormal, bias);

        // Calculate cascade blending weight (use input bias to smooth transition)
        float fadeDistance = cascade.ProbesSpacing * DDGI_CASCADE_BLEND_SIZE;
        float3 blendPos = worldPosition - data.BlendOrigin[cascade.CascadeIndex].xyz;
        cascade.CascadeWeight = sdRoundBox(blendPos, cascade.ProbesExtent - cascade.ProbesSpacing, cascade.ProbesSpacing * 2) + fadeDistance;
        cascade.CascadeWeight = 1 - saturate(cascade.CascadeWeight / fadeDistance);
        if (cascade.CascadeWeight > dither)
            break;
    }
#endif
    return cascade;
}

// Samples DDGI probes volume at the given world-space position and returns the irradiance.
// bias - scales the bias vector to the initial sample point to reduce self-shading artifacts
// dither - randomized per-pixel value in range 0-1, used to smooth dithering for cascades blending
float3 SampleDDGIIrradiance(DDGIData data, Texture2D<snorm float4> probesData, Texture2D<float4> probesDistance, Texture2D<float4> probesIrradiance, float3 worldPosition, float3 worldNormal, float bias = DDGI_DEFAULT_BIAS, float dither = 0.0f)
{
    if (data.CascadesCount == 0)
        return float3(0, 0, 0);

    // Select the cascade
    DDGICascadeSampling cascade = GetDDGICascade(data, worldPosition, worldNormal, bias, dither);

    // Sample cascade
    float3 result = SampleDDGIIrradianceCascade(data, probesData, probesDistance, probesIrradiance, worldPosition, worldNormal, cascade);

#if DDGI_CASCADE_BLEND_SMOOTH && !defined(DDGI_DEBUG_CASCADE)
    // Blend with the next cascade
    cascade.CascadeIndex++;
    if (cascade.CascadeIndex <= data.CascadesCount && cascade.CascadeWeight < 0.99f)
    {
        uint cascadeIndexTmp = cascade.CascadeIndex;
        cascade.CascadeIndex = min(cascade.CascadeIndex, data.CascadesCount - 1);
        cascade.ProbesSpacing = data.ProbesOriginAndSpacing[cascade.CascadeIndex].w;
        cascade.ProbesOrigin = data.ProbesScrollOffsets[cascade.CascadeIndex].xyz * cascade.ProbesSpacing + data.ProbesOriginAndSpacing[cascade.CascadeIndex].xyz;
        cascade.ProbesExtent = (data.ProbesCounts - 1) * (cascade.ProbesSpacing * 0.5f);
        float3 viewDir = normalize(data.ViewPos - worldPosition);
        cascade.BiasedWorldPosition = worldPosition + GetDDGISurfaceBias(viewDir, cascade.ProbesSpacing, worldNormal, bias);
        float3 resultNext = SampleDDGIIrradianceCascade(data, probesData, probesDistance, probesIrradiance, worldPosition, worldNormal, cascade);
        result *= cascade.CascadeWeight;
        result += resultNext * (1 - cascade.CascadeWeight);
    }
#endif

    // Blend between the last cascade and the fallback irradiance
    if (cascade.CascadeIndex >= data.CascadesCount)
    {
        float fallbackWeight = (1 - cascade.CascadeWeight) * data.FallbackIrradiance.a;
        result = lerp(result, data.FallbackIrradiance.rgb, fallbackWeight);
    }

    return result;
}

// Samples DDGI probes volume at the given world-space position and returns the specular reflections.
// bias - scales the bias vector to the initial sample point to reduce self-shading artifacts
// dither - randomized per-pixel value in range 0-1, used to smooth dithering for cascades blending
float3 SampleDDGISpecular(DDGIData data, Texture2D<snorm float4> probesData, Texture2D<float4> probesDistance, Texture2D<float4> probesRadiance, float3 worldPosition, float3 worldNormal, float roughness, float bias = DDGI_DEFAULT_BIAS, float dither = 0.0f)
{
    if (data.CascadesCount == 0)
        return float3(0, 0, 0);

    // Select the cascade
    DDGICascadeSampling cascade = GetDDGICascade(data, worldPosition, worldNormal, bias, dither);

    // Sample cascade
    float3 reflection = reflect(normalize(worldPosition - data.ViewPos), worldNormal);
    float3 specular = SampleDDGISpecularCascade(data, probesData, probesDistance, probesRadiance, worldPosition, worldNormal, roughness, reflection, cascade);

    return specular;
}
