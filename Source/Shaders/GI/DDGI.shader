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
#include "./Flax/Math.hlsl"
#include "./Flax/Noise.hlsl"
#include "./Flax/Quaternion.hlsl"
#include "./Flax/GlobalSignDistanceField.hlsl"
#include "./Flax/GI/GlobalSurfaceAtlas.hlsl"
#include "./Flax/GI/DDGI.hlsl"

// This must match C++
#define DDGI_TRACE_RAYS_PROBES_COUNT_LIMIT 4096 // Maximum amount of probes to update at once during rays tracing and blending
#define DDGI_TRACE_RAYS_LIMIT 256 // Limit of rays per-probe (runtime value can be smaller)
#define DDGI_TRACE_RAYS_MIN 16 // Minimum amount of rays to shoot for sleepy probes
#define DDGI_TRACE_NEGATIVE 0 // If true, rays that start inside geometry will use negative distance to indicate backface hit
#define DDGI_PROBE_UPDATE_BORDERS_GROUP_SIZE 8
#define DDGI_PROBE_CLASSIFY_GROUP_SIZE 32
#define DDGI_PROBE_RELOCATE_ITERATIVE 1 // If true, probes relocation algorithm tries to move them in additive way, otherwise all nearby locations are checked to find the best position
#define DDGI_PROBE_RELOCATE_FIND_BEST 1 // If true, probes relocation algorithm tries to move to the best matching location within nearby area
#define DDGI_DEBUG_STATS 0 // Enables additional GPU-driven stats for probe/rays count
#define DDGI_DEBUG_INSTABILITY 0 // Enables additional probe irradiance instability debugging

META_CB_BEGIN(0, Data0)
DDGIData DDGI;
GlobalSDFData GlobalSDF;
GlobalSurfaceAtlasData GlobalSurfaceAtlas;
GBufferData GBuffer;
float4 RaysRotation;
float SkyboxIntensity;
uint ProbesCount;
float ResetBlend;
float TemporalTime;
int4 ProbeScrollClears[4];
float3 ViewDir;
float Padding1;
META_CB_END

META_CB_BEGIN(1, Data1)
float2 Padding2;
uint CascadeIndex;
uint ProbeIndexOffset;
META_CB_END

// Calculates the evenly distributed direction ray on a sphere (Spherical Fibonacci lattice)
float3 GetSphericalFibonacci(float sampleIndex, float samplesCount)
{
    float b = (sqrt(5.0f) * 0.5f + 0.5f) - 1.0f;
    float s = sampleIndex * b;
    float phi = (2.0f * PI) * (s - floor(s));
    float cosTheta = 1.0f - (2.0f * sampleIndex + 1.0f) * (1.0f / samplesCount);
    float sinTheta = sqrt(saturate(1.0f - (cosTheta * cosTheta)));
    return float3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);
}

// Calculates a random normalized ray direction (based on the ray index and the current probes rotation phrase)
float3 GetProbeRayDirection(DDGIData data, uint rayIndex, uint raysCount, uint probeIndex, uint3 probeCoords)
{
    float4 rotation = RaysRotation;

    // Randomize rotation per-probe (otherwise all probes are in sync)
    float3 probePos = (float3)probeCoords / (float3)data.ProbesCounts;
    float3 randomAxis = normalize(Mod289(probePos));
    float randomAngle = (float)probeIndex / (float)ProbesCount * (2.0f * PI);
    rotation = QuaternionMultiply(rotation, QuaternionFromAxisAngle(randomAxis, randomAngle));

    // Random rotation per-ray - relative to the per-frame rays rotation
    float3 direction = GetSphericalFibonacci((float)rayIndex, (float)raysCount);
    return normalize(QuaternionRotate(rotation, direction));
}

// Calculates amount of rays to allocate for a probe
uint GetProbeRaysCount(DDGIData data, float probeAttention)
{
    //return data.RaysCount;
    probeAttention = saturate((probeAttention - DDGI_PROBE_ATTENTION_MIN) / (DDGI_PROBE_ATTENTION_MAX - DDGI_PROBE_ATTENTION_MIN));
    return DDGI_TRACE_RAYS_MIN + (uint)max(probeAttention * (float)(data.RaysCount - DDGI_TRACE_RAYS_MIN), 0.0f);
}

#ifdef _CS_Classify

RWTexture2D<snorm float4> RWProbesData : register(u0);
RWByteAddressBuffer RWActiveProbes : register(u1);

Texture3D<snorm float> GlobalSDFTex : register(t0);
Texture3D<snorm float> GlobalSDFMip : register(t1);

float3 Remap(float3 value, float3 fromMin, float3 fromMax, float3 toMin, float3 toMax)
{
    return (value - fromMin) / (fromMax - fromMin) * (toMax - toMin) + toMin;
}

// Compute shader for updating probes state between active and inactive and performing probes relocation.
META_CS(true, FEATURE_LEVEL_SM5)
[numthreads(DDGI_PROBE_CLASSIFY_GROUP_SIZE, 1, 1)]
void CS_Classify(uint3 DispatchThreadId : SV_DispatchThreadID)
{
    uint probeIndex = DispatchThreadId.x;
    if (probeIndex >= ProbesCount)
        return;
    uint3 probeCoords = GetDDGIProbeCoords(DDGI, probeIndex);
    probeIndex = GetDDGIScrollingProbeIndex(DDGI, CascadeIndex, probeCoords);
    int2 probeDataCoords = GetDDGIProbeTexelCoords(DDGI, CascadeIndex, probeIndex);
    float probesSpacing = DDGI.ProbesOriginAndSpacing[CascadeIndex].w;
    float3 probeBasePosition = GetDDGIProbeWorldPosition(DDGI, CascadeIndex, probeCoords);

    // Disable probes that are is in the range of higher-quality cascade
    if (CascadeIndex > 0)
    {
        uint prevCascade = CascadeIndex - 1;
        float prevProbesSpacing = DDGI.ProbesOriginAndSpacing[prevCascade].w;
        float3 prevProbesOrigin = DDGI.ProbesScrollOffsets[prevCascade].xyz * prevProbesSpacing + DDGI.ProbesOriginAndSpacing[prevCascade].xyz;
        float3 prevProbesExtent = (DDGI.ProbesCounts - 1) * (prevProbesSpacing * 0.5f);
        prevProbesExtent -= probesSpacing * ceil(DDGI_CASCADE_BLEND_SIZE); // Apply safe margin to allow probes on cascade edges
        float prevCascadeWeight = Min3(prevProbesExtent - abs(probeBasePosition - prevProbesOrigin));
        if (prevCascadeWeight > 0.1f)
        {
            // Disable probe
            RWProbesData[probeDataCoords] = EncodeDDGIProbeData(float3(0, 0, 0), DDGI_PROBE_STATE_INACTIVE, 0.0f);
            return;
        }
    }

    // Check if probe was scrolled
    int3 probeScrollClears = ProbeScrollClears[CascadeIndex].xyz;
    bool wasScrolled = false;
    UNROLL
    for (uint planeIndex = 0; planeIndex < 3; planeIndex++)
    {
        int probeCount = (int)DDGI.ProbesCounts[planeIndex];
        int newCoord = (int)probeCoords[planeIndex] + probeScrollClears[planeIndex];
        if (newCoord < 0 || newCoord >= probeCount)
            wasScrolled = true;
        newCoord = (int)probeCoords[planeIndex] - probeScrollClears[planeIndex];
        if (newCoord < 0 || newCoord >= probeCount)
            wasScrolled = true;
    }

    // Load probe state and position
    float4 probeData = RWProbesData[probeDataCoords];
    float probeAttention = DecodeDDGIProbeAttention(probeData);
    uint probeState = DecodeDDGIProbeState(probeData);
    uint probeStateOld = probeState;
    float3 probeOffset = probeData.xyz * probesSpacing; // Probe offset is [-1;1] within probes spacing
    if (wasScrolled || probeState == DDGI_PROBE_STATE_INACTIVE)
    {
        probeOffset = float3(0, 0, 0); // Clear offset for a new probe
        probeAttention = 1.0f; // Wake-up
    }
    float3 probeOffsetOld = probeOffset;
    float3 probePosition = probeBasePosition + probeOffset;

    // Use Global SDF to quickly get distance and direction to the scene geometry
#if DDGI_PROBE_RELOCATE_ITERATIVE
    float sdf;
    float3 sdfNormal = normalize(SampleGlobalSDFGradient(GlobalSDF, GlobalSDFTex, GlobalSDFMip, probePosition, sdf));
#else
    float sdf = SampleGlobalSDF(GlobalSDF, GlobalSDFTex, GlobalSDFMip, probePosition);
#endif
    float sdfDst = abs(sdf);
    const float ProbesDistanceLimits[4] = { 1.1f, 2.3f, 2.5f, 2.5f };
    const float ProbesRelocateLimits[4] = { 0.4f, 0.5f, 0.6f, 0.7f };
    float voxelLimit = GlobalSDF.CascadeVoxelSize[CascadeIndex] * 0.8f;
    float distanceLimit = probesSpacing * ProbesDistanceLimits[CascadeIndex];
    float relocateLimit = probesSpacing * ProbesRelocateLimits[CascadeIndex];
    if (sdfDst > distanceLimit + length(probeOffset)) // Probe is too far from geometry (or deep inside)
    {
        // Disable it
        probeOffset = float3(0, 0, 0);
        probeState = DDGI_PROBE_STATE_INACTIVE;
        probeAttention = 0.0f;
    }
    else
    {
        // Apply distance/view heuristics to probe attention
        probeState = DDGI_PROBE_STATE_ACTIVE;
        float3 viewToProbe = probePosition - GBuffer.ViewPos;
        float distanceToProbe = length(viewToProbe);
        viewToProbe /= distanceToProbe;
        float probeViewDot = dot(viewToProbe, ViewDir);
        probeAttention *= lerp(0.1f, 1.0f, saturate(probeViewDot)); // Reduce quality for probes behind the camera (or away from view dir)
        probeAttention *= lerp(1.0f, 0.5f, saturate(sdfDst / voxelLimit)); // Reduce quality for probes far away from geometry
        probeAttention += (1.0f - saturate(distanceToProbe / 1000.0f)) * 1.2f; // Boost quality for probes nearby view
        //probeAttention = 0.0f; // Debug test lowest ray count
        //probeAttention = 1.0f; // Debug test highest ray count
        probeAttention = clamp(probeAttention, DDGI_PROBE_ATTENTION_MIN, DDGI_PROBE_ATTENTION_MAX);

        // Relocate only if probe location is not good enough
        if (sdf <= voxelLimit)
        {
#if DDGI_PROBE_RELOCATE_ITERATIVE
            {
                // Use SDF gradient to relocate probe away the surface
                float iterativeRelocateSpeed = probeStateOld != DDGI_PROBE_STATE_ACTIVE ? 1.0f : 0.3f;
                float3 offsetToSet = probeOffset + sdfNormal * ((sdf + voxelLimit) * iterativeRelocateSpeed);
                if (length(offsetToSet) < relocateLimit)
                {
                    // Relocate it
                    probeOffset = offsetToSet;
                }
                else
                {
                    // Reset offset
                    probeOffset = float3(0, 0, 0);
                }

                // Read SDF at the new position for additional check
                probePosition = probeBasePosition + probeOffset;
                sdf = SampleGlobalSDF(GlobalSDF, GlobalSDFTex, GlobalSDFMip, probePosition);
                sdfDst = abs(sdf);
            }
            if (sdf <= voxelLimit * 1.1f) // Add some safe-bias to reduce artifacts
#endif
            {
#if DDGI_PROBE_RELOCATE_FIND_BEST
                // Sample Global SDF around the probe base location
                uint sdfCascade = GetGlobalSDFCascade(GlobalSDF, probeBasePosition);
                float4 CachedProbeOffsets[64];
                for (uint x = 0; x < 4; x++)
                for (uint y = 0; y < 4; y++)
                for (uint z = 0; z < 4; z++)
                {
                    float3 offset = Remap(float3(x, y, z), 0, 3, -0.707f, 0.707f) * relocateLimit;
                    float offsetSdf = SampleGlobalSDFCascade(GlobalSDF, GlobalSDFTex, probeBasePosition + offset, sdfCascade);
                    CachedProbeOffsets[x * 16 + y * 4 + z] = float4(offset, offsetSdf);
                }

                // Select the best probe location around the base position
                float4 bestOffset = CachedProbeOffsets[0];
                for (uint i = 1; i < 64; i++)
                {
                    if (CachedProbeOffsets[i].w > bestOffset.w)
                        bestOffset = CachedProbeOffsets[i];
                }
                if (bestOffset.w <= voxelLimit)
                {
                    // Disable probe that is too close to the geometry
                    probeOffset = float3(0, 0, 0);
                    probeState = DDGI_PROBE_STATE_INACTIVE;
                    probeAttention = 0.0f;
                }
                else
                {
                    // Relocate the probe to the best found location
                    probeOffset = bestOffset.xyz;
                }
#elif DDGI_PROBE_RELOCATE_ITERATIVE
                // Disable probe
                probeOffset = float3(0, 0, 0);
                probeState = DDGI_PROBE_STATE_INACTIVE;
                probeAttention = 0.0f;
#endif
            }
        }

        // If probe was in a different location or was activated now then mark it as activated
        bool wasActivated = probeStateOld == DDGI_PROBE_STATE_INACTIVE;
        bool wasRelocated = distance(probeOffset, probeOffsetOld) > 2.0f;
#if DDGI_PROBE_RELOCATE_FIND_BEST || DDGI_PROBE_RELOCATE_ITERATIVE
        if (wasRelocated && !wasActivated)
        {
            // If probe was relocated but the previous location is visible from the new one, then don't re-activate it for smoother blend
            float3 diff = probeOffsetOld - probeOffset;
            float diffLen = length(diff);
            float3 diffDir = diff / diffLen;
            GlobalSDFTrace trace;
            trace.Init(probeBasePosition + probeOffset, diffDir, 0.0f, diffLen);
            GlobalSDFHit hit = RayTraceGlobalSDF(GlobalSDF, GlobalSDFTex, GlobalSDFMip, trace);
            if (!hit.IsHit())
                wasRelocated = false;
        }
#endif
        if ((wasActivated || wasScrolled || wasRelocated) && probeState == DDGI_PROBE_STATE_ACTIVE)
        {
            probeState = DDGI_PROBE_STATE_ACTIVATED;
            probeAttention = 1.0f;
        }
    }

    // Save probe state
    probeOffset /= probesSpacing; // Move offset back to [-1;1] space
    RWProbesData[probeDataCoords] = EncodeDDGIProbeData(probeOffset, probeState, probeAttention);

    // Collect active probes
    if (probeState != DDGI_PROBE_STATE_INACTIVE)
    {
        uint activeProbeIndex;
        RWActiveProbes.InterlockedAdd(0, 1, activeProbeIndex); // Counter at 0
        RWActiveProbes.Store(activeProbeIndex * 4 + 4, DispatchThreadId.x);
    }
}

#endif

#ifdef _CS_UpdateProbesInitArgs

RWBuffer<uint> UpdateProbesInitArgs : register(u0);
ByteAddressBuffer ActiveProbes : register(t0);

// Compute shader for building indirect dispatch arguments for CS_TraceRays and CS_UpdateProbes.
META_CS(true, FEATURE_LEVEL_SM5)
[numthreads(1, 1, 1)]
void CS_UpdateProbesInitArgs()
{
    uint activeProbesCount = ActiveProbes.Load(0); // Counter at 0
    uint arg = 0;
    for (uint probesOffset = 0; probesOffset < activeProbesCount; probesOffset += DDGI_TRACE_RAYS_PROBES_COUNT_LIMIT)
    {
        uint probesBatchSize = min(activeProbesCount - probesOffset, DDGI_TRACE_RAYS_PROBES_COUNT_LIMIT);
        UpdateProbesInitArgs[arg++] = probesBatchSize;
        UpdateProbesInitArgs[arg++] = 1;
        UpdateProbesInitArgs[arg++] = 1;
    }
}

#endif

#ifdef _CS_TraceRays

RWTexture2D<float4> RWProbesTrace : register(u0);
#if DDGI_DEBUG_STATS
RWByteAddressBuffer RWStats : register(u1);
#endif

Texture3D<snorm float> GlobalSDFTex : register(t0);
Texture3D<snorm float> GlobalSDFMip : register(t1);
ByteAddressBuffer GlobalSurfaceAtlasChunks : register(t2);
ByteAddressBuffer RWGlobalSurfaceAtlasCulledObjects : register(t3);
Buffer<float4> GlobalSurfaceAtlasObjects : register(t4);
Texture2D GlobalSurfaceAtlasDepth : register(t5);
Texture2D GlobalSurfaceAtlasTex : register(t6);
Texture2D<snorm float4> ProbesData : register(t7);
TextureCube Skybox : register(t8);
ByteAddressBuffer ActiveProbes : register(t9);

// Compute shader for tracing rays for probes using Global SDF and Global Surface Atlas (1 ray per-thread).
META_CS(true, FEATURE_LEVEL_SM5)
META_PERMUTATION_1(DDGI_TRACE_RAYS_COUNT=96)
META_PERMUTATION_1(DDGI_TRACE_RAYS_COUNT=128)
META_PERMUTATION_1(DDGI_TRACE_RAYS_COUNT=192)
META_PERMUTATION_1(DDGI_TRACE_RAYS_COUNT=256)
[numthreads(1, DDGI_TRACE_RAYS_COUNT, 1)]
void CS_TraceRays(uint3 DispatchThreadId : SV_DispatchThreadID)
{
    uint rayIndex = DispatchThreadId.y;
    uint probeIndex = ActiveProbes.Load((DispatchThreadId.x + ProbeIndexOffset + 1) * 4);
    uint3 probeCoords = GetDDGIProbeCoords(DDGI, probeIndex);
    probeIndex = GetDDGIScrollingProbeIndex(DDGI, CascadeIndex, probeCoords);

    // Load current probe state and position
    float4 probeData = LoadDDGIProbeData(DDGI, ProbesData, CascadeIndex, probeIndex);
    float probeAttention = DecodeDDGIProbeAttention(probeData);
    uint probeState = DecodeDDGIProbeState(probeData);
    uint probeRaysCount = GetProbeRaysCount(DDGI, probeAttention);
    if (probeState == DDGI_PROBE_STATE_INACTIVE || rayIndex >= probeRaysCount)
        return; // Skip disabled probes or if current thread's ray is unused
    float3 probePosition = DecodeDDGIProbePosition(DDGI, probeData, CascadeIndex, probeIndex, probeCoords);
    float3 probeRayDirection = GetProbeRayDirection(DDGI, rayIndex, probeRaysCount, probeIndex, probeCoords);
    // TODO: implement ray-guiding based on the probe irradiance (prioritize directions with high luminance)

    // Trace ray with Global SDF
    GlobalSDFTrace trace;
    trace.Init(probePosition, probeRayDirection, 0.0f, DDGI.RayMaxDistance);
    GlobalSDFHit hit = RayTraceGlobalSDF(GlobalSDF, GlobalSDFTex, GlobalSDFMip, trace);

    // Calculate radiance and distance
    float4 radiance;
    if (hit.IsHit())
    {
#if DDGI_TRACE_NEGATIVE
        if (hit.HitSDF <= 0.0f && hit.HitTime <= GlobalSDF.CascadeVoxelSize[0])
        {
            // Ray starts inside geometry (mark as negative distance and reduce it's influence during irradiance blending)
            radiance = float4(0, 0, 0, hit.HitTime * -0.25f);
        }
        else
#endif
        {
            // Sample Global Surface Atlas to get the lighting at the hit location
            float3 hitPosition = hit.GetHitPosition(trace);
            float surfaceThreshold = GetGlobalSurfaceAtlasThreshold(GlobalSDF, hit);
            float4 surfaceColor = SampleGlobalSurfaceAtlas(GlobalSurfaceAtlas, GlobalSurfaceAtlasChunks, RWGlobalSurfaceAtlasCulledObjects, GlobalSurfaceAtlasObjects, GlobalSurfaceAtlasDepth, GlobalSurfaceAtlasTex, hitPosition, -probeRayDirection, surfaceThreshold);
            radiance = float4(surfaceColor.rgb, hit.HitTime);

            // Add some bias to prevent self occlusion artifacts in Chebyshev due to Global SDF being very incorrect in small scale
            radiance.w = max(radiance.w + GlobalSDF.CascadeVoxelSize[hit.HitCascade] * 0.5f, 0);
        }
    }
    else
    {
        // Ray hits sky
        radiance.rgb = Skybox.SampleLevel(SamplerLinearClamp, probeRayDirection, 0).rgb * SkyboxIntensity;
        radiance.a = 1e27f; // Sky is the limit
    }

    // Write into probes trace results
    RWProbesTrace[uint2(rayIndex, DispatchThreadId.x)] = radiance;

#if DDGI_DEBUG_STATS
    // Update stats
    uint tmp;
    RWStats.InterlockedAdd(0, 1, tmp);
    if (rayIndex == 0)
        RWStats.InterlockedAdd(4, 1, tmp);
#endif
}

#endif

#if defined(_CS_UpdateProbes)

#if DDGI_PROBE_UPDATE_MODE == 0
// Update irradiance
#define DDGI_PROBE_RESOLUTION DDGI_PROBE_RESOLUTION_IRRADIANCE
groupshared float4 CachedProbesTraceRadiance[DDGI_TRACE_RAYS_LIMIT];
groupshared float OutputInstability[DDGI_PROBE_RESOLUTION * DDGI_PROBE_RESOLUTION];
#else
// Update distance
#define DDGI_PROBE_RESOLUTION DDGI_PROBE_RESOLUTION_DISTANCE
groupshared float CachedProbesTraceDistance[DDGI_TRACE_RAYS_LIMIT];
#endif

// Source: https://github.com/turanszkij/WickedEngine
#define BorderOffsetsSize (4 * DDGI_PROBE_RESOLUTION + 4)
#if DDGI_PROBE_RESOLUTION == 6
static const uint4 BorderOffsets[BorderOffsetsSize] = {
    uint4(6, 1, 1, 0),
    uint4(5, 1, 2, 0),
    uint4(4, 1, 3, 0),
    uint4(3, 1, 4, 0),
    uint4(2, 1, 5, 0),
    uint4(1, 1, 6, 0),

    uint4(6, 6, 1, 7),
    uint4(5, 6, 2, 7),
    uint4(4, 6, 3, 7),
    uint4(3, 6, 4, 7),
    uint4(2, 6, 5, 7),
    uint4(1, 6, 6, 7),

    uint4(1, 1, 0, 6),
    uint4(1, 2, 0, 5),
    uint4(1, 3, 0, 4),
    uint4(1, 4, 0, 3),
    uint4(1, 5, 0, 2),
    uint4(1, 6, 0, 1),

    uint4(6, 1, 7, 6),
    uint4(6, 2, 7, 5),
    uint4(6, 3, 7, 4),
    uint4(6, 4, 7, 3),
    uint4(6, 5, 7, 2),
    uint4(6, 6, 7, 1),

    uint4(1, 1, 7, 7),
    uint4(6, 1, 0, 7),
    uint4(1, 6, 7, 0),
    uint4(6, 6, 0, 0)
};
#elif DDGI_PROBE_RESOLUTION == 14
static const uint4 BorderOffsets[BorderOffsetsSize] = {
    uint4(14, 1, 1, 0),
    uint4(13, 1, 2, 0),
    uint4(12, 1, 3, 0),
    uint4(11, 1, 4, 0),
    uint4(10, 1, 5, 0),
    uint4(9, 1, 6, 0),
    uint4(8, 1, 7, 0),
    uint4(7, 1, 8, 0),
    uint4(6, 1, 9, 0),
    uint4(5, 1, 10, 0),
    uint4(4, 1, 11, 0),
    uint4(3, 1, 12, 0),
    uint4(2, 1, 13, 0),
    uint4(1, 1, 14, 0),

    uint4(14, 14, 1, 15),
    uint4(13, 14, 2, 15),
    uint4(12, 14, 3, 15),
    uint4(11, 14, 4, 15),
    uint4(10, 14, 5, 15),
    uint4(9, 14, 6, 15),
    uint4(8, 14, 7, 15),
    uint4(7, 14, 8, 15),
    uint4(6, 14, 9, 15),
    uint4(5, 14, 10, 15),
    uint4(4, 14, 11, 15),
    uint4(3, 14, 12, 15),
    uint4(2, 14, 13, 15),
    uint4(1, 14, 14, 15),

    uint4(1, 14, 0, 1),
    uint4(1, 13, 0, 2),
    uint4(1, 12, 0, 3),
    uint4(1, 11, 0, 4),
    uint4(1, 10, 0, 5),
    uint4(1, 9, 0, 6),
    uint4(1, 8, 0, 7),
    uint4(1, 7, 0, 8),
    uint4(1, 6, 0, 9),
    uint4(1, 5, 0, 10),
    uint4(1, 4, 0, 11),
    uint4(1, 3, 0, 12),
    uint4(1, 2, 0, 13),
    uint4(1, 1, 0, 14),

    uint4(14, 14, 15, 1),
    uint4(14, 13, 15, 2),
    uint4(14, 12, 15, 3),
    uint4(14, 11, 15, 4),
    uint4(14, 10, 15, 5),
    uint4(14, 9, 15, 6),
    uint4(14, 8, 15, 7),
    uint4(14, 7, 15, 8),
    uint4(14, 6, 15, 9),
    uint4(14, 5, 15, 10),
    uint4(14, 4, 15, 11),
    uint4(14, 3, 15, 12),
    uint4(14, 2, 15, 13),
    uint4(14, 1, 15, 14),

    uint4(14, 14, 0, 0),
    uint4(1, 14, 15, 0),
    uint4(14, 1, 0, 15),
    uint4(1, 1, 15, 15)
};
#else
#error "Unsupported probe size for border values copy."
#endif

groupshared float3 CachedProbesTraceDirection[DDGI_TRACE_RAYS_LIMIT];

RWTexture2D<float4> RWOutput : register(u0);
#if DDGI_PROBE_UPDATE_MODE == 0
RWTexture2D<snorm float4> RWProbesData : register(u1);
#if DDGI_DEBUG_INSTABILITY
RWTexture2D<float> RWOutputInstability : register(u2);
#endif
#else
Texture2D<snorm float4> ProbesData : register(t0);
#endif
Texture2D<float4> ProbesTrace : register(t1);
ByteAddressBuffer ActiveProbes : register(t2);

// Compute shader for updating probes irradiance or distance texture.
META_CS(true, FEATURE_LEVEL_SM5)
META_PERMUTATION_1(DDGI_PROBE_UPDATE_MODE=0)
META_PERMUTATION_1(DDGI_PROBE_UPDATE_MODE=1)
[numthreads(DDGI_PROBE_RESOLUTION, DDGI_PROBE_RESOLUTION, 1)]
void CS_UpdateProbes(uint3 GroupThreadId : SV_GroupThreadID, uint3 GroupId : SV_GroupID, uint GroupIndex : SV_GroupIndex)
{
    // GroupThreadId.xy - coordinates of the probe texel: [0; DDGI_PROBE_RESOLUTION)
    // GroupId.x - index of the thread group which is probe index within a batch: [0; batchSize)
    // GroupIndex.x - index of the thread within a thread group: [0; DDGI_PROBE_RESOLUTION * DDGI_PROBE_RESOLUTION)
    uint probeIndex = ActiveProbes.Load((GroupId.x + ProbeIndexOffset + 1) * 4);
    uint3 probeCoords = GetDDGIProbeCoords(DDGI, probeIndex);
    probeIndex = GetDDGIScrollingProbeIndex(DDGI, CascadeIndex, probeCoords);

    // Load probe data
#if DDGI_PROBE_UPDATE_MODE == 0
    int2 probeDataCoords = GetDDGIProbeTexelCoords(DDGI, CascadeIndex, probeIndex);
    float4 probeData = RWProbesData[probeDataCoords];
#else
    float4 probeData = LoadDDGIProbeData(DDGI, ProbesData, CascadeIndex, probeIndex);
#endif
    float probeAttention = DecodeDDGIProbeAttention(probeData);
    uint probeState = DecodeDDGIProbeState(probeData);
    uint probeRaysCount = GetProbeRaysCount(DDGI, probeAttention);

#if DDGI_PROBE_UPDATE_MODE == 0
    uint backfacesCount = 0;
    uint backfacesLimit = uint(probeRaysCount * 0.1f);
#else
    float probesSpacing = DDGI.ProbesOriginAndSpacing[CascadeIndex].w;
    float distanceLimit = probesSpacing * 1.5f;
#endif

    // Load trace rays results into shared memory to reuse across whole thread group (raysCount per thread)
    uint raysCount = (uint)(ceil((float)probeRaysCount / (float)(DDGI_PROBE_RESOLUTION * DDGI_PROBE_RESOLUTION)));
    uint raysStart = GroupIndex * raysCount;
    raysCount = max(min(raysStart + raysCount, probeRaysCount), raysStart) - raysStart;
    for (uint i = 0; i < raysCount; i++)
    {
        uint rayIndex = raysStart + i;
#if DDGI_PROBE_UPDATE_MODE == 0
        CachedProbesTraceRadiance[rayIndex] = ProbesTrace[uint2(rayIndex, GroupId.x)];
#else
        float rayDistance = ProbesTrace[uint2(rayIndex, GroupId.x)].w;
        CachedProbesTraceDistance[rayIndex] = min(abs(rayDistance), distanceLimit);
#endif
        CachedProbesTraceDirection[rayIndex] = GetProbeRayDirection(DDGI, rayIndex, probeRaysCount, probeIndex, probeCoords);
    }
    GroupMemoryBarrierWithGroupSync();
    probeCoords = GetDDGIProbeCoords(DDGI, probeIndex);

    // Calculate octahedral projection for probe (unwraps spherical projection into a square)
    float2 octahedralCoords = GetOctahedralCoords(GroupThreadId.xy, DDGI_PROBE_RESOLUTION);
    float3 octahedralDirection = GetOctahedralDirection(octahedralCoords);

    // Loop over rays
    float4 result = float4(0, 0, 0, 0);
    LOOP
    for (uint rayIndex = 0; rayIndex < probeRaysCount; rayIndex++)
    {
        float3 rayDirection = CachedProbesTraceDirection[rayIndex];
        float rayWeight = max(dot(octahedralDirection, rayDirection), 0.0f);

#if DDGI_PROBE_UPDATE_MODE == 0
        float4 rayRadiance = CachedProbesTraceRadiance[rayIndex];
#if DDGI_TRACE_NEGATIVE
        if (rayRadiance.w < 0.0f)
        {
            // Count backface hits
            backfacesCount++;

            // Skip further blending after reaching backfaces limit
            if (backfacesCount >= backfacesLimit)
            {
                result = float4(0, 0, 0, 1);
                break;
            }
            continue;
        }
#endif

        // Add radiance (RGB) and weight (A)
        result += float4(rayRadiance.rgb * rayWeight, rayWeight);
#else
        // Increase reaction speed for depth discontinuities
        rayWeight = pow(rayWeight, 10.0f);

        // Add distance (R), distance^2 (G) and weight (A)
        float rayDistance = CachedProbesTraceDistance[rayIndex];
        result += float4(rayDistance * rayWeight, (rayDistance * rayDistance) * rayWeight, 0.0f, rayWeight);
#endif
    }

    // Normalize results
    float epsilon = (float)probeRaysCount * 1e-9f;
    result.rgb *= 1.0f / (2.0f * max(result.a, epsilon));

    // Load current probe value
    uint2 outputCoords = GetDDGIProbeTexelCoords(DDGI, CascadeIndex, probeIndex) * (DDGI_PROBE_RESOLUTION + 2) + 1 + GroupThreadId.xy;
    float3 previous = RWOutput[outputCoords].rgb;
    bool wasActivated = probeState == DDGI_PROBE_STATE_ACTIVATED || ResetBlend;
    if (wasActivated)
        previous = result.rgb;

#if DDGI_PROBE_UPDATE_MODE == 0
    // Calculate instability of the irradiance
    float previousLuma = Luminance(previous.rgb);
    float resultLuma = Luminance(result.rgb);
    float instability = abs(previousLuma - resultLuma) / previousLuma; // Percentage change in luminance of irradiance
    instability = max(instability, Max3(abs(result.rgb - previous) / previous)); // Percentage of color delta change of irradiance
    //instability *= saturate(result.a); // Reduce instability in areas with a small ray-coverage
    //instability = pow(instability, 1.2f); // Increase contrast
    instability *= 2.0f; // Make it stronger on scene changes
    //instability = saturate(instability);
    OutputInstability[GroupIndex] = instability;
#if DDGI_DEBUG_INSTABILITY
    RWOutputInstability[outputCoords] = instability;
    //RWOutputInstability[outputCoords] = probeAttention; // Debug test probe attention visualization
#endif
#endif

    // Blend current value with the previous probe data
    float historyWeightFast = DDGI.ProbeHistoryWeight;
    float historyWeightSlow = 0.97f;
#if DDGI_PROBE_UPDATE_MODE == 0
    float3 irradianceDelta = result.rgb - previous;
    float irradianceDeltaMax = Max3(abs(irradianceDelta));
    float irradianceDeltaLen = length(irradianceDelta);
    if (irradianceDeltaMax > 0.5f)
    {
        // Reduce history weight after significant lighting change
        historyWeightFast *= 0.5f;
    }
#endif
    float historyWeight = lerp(historyWeightSlow, historyWeightFast, probeAttention * probeAttention * probeAttention);
    //historyWeight = 1.0f; // Debug full-blend
    //historyWeight = 0.0f; // Debug no-blend
    if (wasActivated)
        historyWeight = 0.0f;
#if DDGI_PROBE_UPDATE_MODE == 0
    result *= DDGI.IndirectLightingIntensity;
#if DDGI_SRGB_BLENDING
    result.rgb = pow(max(result.rgb, 0), 1.0f / DDGI.IrradianceGamma);
#endif
    if (irradianceDeltaLen > 2.0f)
    {
        // Reduce flickering during rapid brightness changes
        //result.rgb = previous + (irradianceDelta * 0.25f);
    }
    result = float4(lerp(result.rgb, previous.rgb, historyWeight), 1.0f);
#else
    result = float4(lerp(result.rg, previous.rg, historyWeight), 0.0f, 1.0f);
#endif

    RWOutput[outputCoords] = result;

    GroupMemoryBarrierWithGroupSync();
    uint2 baseCoords = GetDDGIProbeTexelCoords(DDGI, CascadeIndex, probeIndex) * (DDGI_PROBE_RESOLUTION + 2);

#if DDGI_PROBE_UPDATE_MODE == 0
    // The first thread updates the probe attention based on the instability of all texels
    BRANCH
    if (GroupIndex == 0 && probeState != DDGI_PROBE_STATE_INACTIVE)
    {
        // Calculate instability statistics for a whole probe
        float instabilityAvg = 0;
        for (uint i = 0; i < DDGI_PROBE_RESOLUTION * DDGI_PROBE_RESOLUTION; i++)
            instabilityAvg += OutputInstability[i];
        instabilityAvg *= 1.0f / float(DDGI_PROBE_RESOLUTION * DDGI_PROBE_RESOLUTION);
        instabilityAvg = saturate(instabilityAvg);
        instability = instabilityAvg;

        // Calculate probe attention
        float taregAttention = lerp(0.5f, DDGI_PROBE_ATTENTION_MAX, instability); // Use some base level
        if (taregAttention >= probeAttention)
            probeAttention = taregAttention; // Quick jump up
        else
            probeAttention = lerp(probeAttention, taregAttention, 0.2f); // Slow blend down
        if (probeState == DDGI_PROBE_STATE_ACTIVATED)
            probeAttention = DDGI_PROBE_ATTENTION_MAX;

        // Update probe data for the next frame
        probeState = DDGI_PROBE_STATE_ACTIVE;
        RWProbesData[probeDataCoords] = EncodeDDGIProbeData(probeData.xyz, probeState, probeAttention);
    }

#if DDGI_DEBUG_INSTABILITY
	// Copy border pixels
	for (uint borderIndex = GroupIndex; borderIndex < BorderOffsetsSize; borderIndex += DDGI_PROBE_RESOLUTION * DDGI_PROBE_RESOLUTION)
	{
        uint4 borderOffsets = BorderOffsets[borderIndex];
		RWOutputInstability[baseCoords + borderOffsets.zw] = RWOutputInstability[baseCoords + borderOffsets.xy];
	}
#endif
#endif

    // Copy border pixels
	for (uint borderIndex = GroupIndex; borderIndex < BorderOffsetsSize; borderIndex += DDGI_PROBE_RESOLUTION * DDGI_PROBE_RESOLUTION)
	{
        uint4 borderOffsets = BorderOffsets[borderIndex];
		RWOutput[baseCoords + borderOffsets.zw] = RWOutput[baseCoords + borderOffsets.xy];
	}
}

#endif

#ifdef _PS_IndirectLighting

#include "./Flax/GBuffer.hlsl"
#include "./Flax/Random.hlsl"
#include "./Flax/LightingCommon.hlsl"

Texture2D<snorm float4> ProbesData : register(t4);
Texture2D<float4> ProbesDistance : register(t5);
Texture2D<float4> ProbesIrradiance : register(t6);

// Pixel shader for drawing indirect lighting in fullscreen
META_PS(true, FEATURE_LEVEL_SM5)
META_PERMUTATION_1(DDGI_CASCADE_BLEND_SMOOTH=0)
META_PERMUTATION_1(DDGI_CASCADE_BLEND_SMOOTH=1)
void PS_IndirectLighting(Quad_VS2PS input, out float4 output : SV_Target0)
{
    output = 0;

    // Sample GBuffer
    GBufferSample gBuffer = SampleGBuffer(GBuffer, input.TexCoord);

    // Check if cannot shadow pixel
    BRANCH
    if (gBuffer.ShadingModel == SHADING_MODEL_UNLIT)
    {
        discard;
        return;
    }

    // Sample irradiance
    float bias = 0.2f;
    float dither = RandN2(input.TexCoord + TemporalTime).x;
    float3 irradiance = SampleDDGIIrradiance(DDGI, ProbesData, ProbesDistance, ProbesIrradiance, gBuffer.WorldPos, gBuffer.Normal, bias, dither);
    
    // Calculate lighting
    float3 diffuseColor = GetDiffuseColor(gBuffer);
    float3 diffuse = Diffuse_Lambert(diffuseColor);
    output.rgb = diffuse * irradiance * gBuffer.AO;
}

#endif
