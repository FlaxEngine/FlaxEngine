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
#include "./Flax/Quaternion.hlsl"
#include "./Flax/GlobalSignDistanceField.hlsl"
#include "./Flax/GI/GlobalSurfaceAtlas.hlsl"
#include "./Flax/GI/DDGI.hlsl"

// This must match C++
#define DDGI_TRACE_RAYS_PROBES_COUNT_LIMIT 4096 // Maximum amount of probes to update at once during rays tracing and blending
#define DDGI_TRACE_RAYS_LIMIT 256 // Limit of rays per-probe (runtime value can be smaller)
#define DDGI_PROBE_UPDATE_BORDERS_GROUP_SIZE 8
#define DDGI_PROBE_CLASSIFY_GROUP_SIZE 32

META_CB_BEGIN(0, Data0)
DDGIData DDGI;
GlobalSDFData GlobalSDF;
GlobalSurfaceAtlasData GlobalSurfaceAtlas;
GBufferData GBuffer;
float Padding0;
float ProbesDistanceLimit;
float ResetBlend;
float TemporalTime;
int4 ProbeScrollClears[4];
META_CB_END

META_CB_BEGIN(1, Data1)
float2 Padding1;
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
float3 GetProbeRayDirection(DDGIData data, uint rayIndex)
{
    float3 direction = GetSphericalFibonacci((float)rayIndex, (float)data.RaysCount);
    return normalize(QuaternionRotate(data.RaysRotation, direction));
}

// Calculates amount of rays to allocate for a probe
uint GetProbeRaysCount(DDGIData data, uint probeState)
{
    // TODO: implement variable ray count based on probe location relative to the view frustum (use probe state for storage)
    return data.RaysCount;
}

#ifdef _CS_Classify

#define DDGI_PROBE_RELOCATE_ITERATIVE 0 // If true, probes relocation algorithm tries to move them in additive way, otherwise all nearby locations are checked to find the best position

RWTexture2D<snorm float4> RWProbesData : register(u0);
RWByteAddressBuffer RWActiveProbes : register(u1);

Texture3D<float> GlobalSDFTex : register(t0);
Texture3D<float> GlobalSDFMip : register(t1);

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
    uint probesCount = DDGI.ProbesCounts.x * DDGI.ProbesCounts.y * DDGI.ProbesCounts.z;
    if (probeIndex >= probesCount)
        return;
    uint3 probeCoords = GetDDGIProbeCoords(DDGI, probeIndex);
    probeIndex = GetDDGIScrollingProbeIndex(DDGI, CascadeIndex, probeCoords);
    int2 probeDataCoords = GetDDGIProbeTexelCoords(DDGI, CascadeIndex, probeIndex);
    float probesSpacing = DDGI.ProbesOriginAndSpacing[CascadeIndex].w;

    // Load probe state and position
    float4 probeData = RWProbesData[probeDataCoords];
    uint probeState = DecodeDDGIProbeState(probeData);
    float3 probeOffset = probeData.xyz * probesSpacing; // Probe offset is [-1;1] within probes spacing
    float3 probeOffsetOld = probeOffset;
    float3 probeBasePosition = GetDDGIProbeWorldPosition(DDGI, CascadeIndex, probeCoords);
    float3 probePosition = probeBasePosition;
#if DDGI_PROBE_RELOCATE_ITERATIVE
    probePosition += probeOffset;
#endif

    // Use Global SDF to quickly get distance and direction to the scene geometry
#if DDGI_PROBE_RELOCATE_ITERATIVE
    float sdf;
    float3 sdfNormal = normalize(SampleGlobalSDFGradient(GlobalSDF, GlobalSDFTex, GlobalSDFMip, probePosition, sdf));
#else
    float sdf = SampleGlobalSDF(GlobalSDF, GlobalSDFTex, GlobalSDFMip, probePosition);
#endif
    float sdfDst = abs(sdf);
    float threshold = GlobalSDF.CascadeVoxelSize[CascadeIndex];
    float distanceLimit = length(probesSpacing) * ProbesDistanceLimit;
    float relocateLimit = length(probesSpacing) * 0.6f;
    if (sdfDst > distanceLimit) // Probe is too far from geometry
    {
        // Disable it
        probeOffset = float3(0, 0, 0);
        probeState = DDGI_PROBE_STATE_INACTIVE;
    }
    else
    {
#if DDGI_PROBE_RELOCATE_ITERATIVE
        if (sdf < threshold) // Probe is inside geometry
        {
            if (sdfDst < relocateLimit)
            {
                float3 offsetToAdd = sdfNormal * (sdf + threshold);
                if (distance(probeOffset, offsetToAdd) < relocateLimit)
                {
                    // Relocate it
                    probeOffset += offsetToAdd;
                }
            }
            else
            {
                // Reset relocation
                probeOffset = float3(0, 0, 0);
            }
        }
        else if (sdf > threshold * 4.0f) // Probe is far enough from any geometry
        {
            // Reset relocation
            probeOffset = float3(0, 0, 0);
        }

        // Check if probe is relocated but the base location is fine
        sdf = SampleGlobalSDF(GlobalSDF, GlobalSDFTex, probeBasePosition.xyz);
        if (sdf > threshold)
        {
            // Reset relocation
            probeOffset = float3(0, 0, 0);
        }
#else
        // Sample Global SDF around the probe location
        uint sdfCascade = GetGlobalSDFCascade(GlobalSDF, probePosition);
        float4 CachedProbeOffsets[64];
        // TODO: test performance diff when using shared memory and larger thread group (is it worth it?)
        for (uint x = 0; x < 4; x++)
        for (uint y = 0; y < 4; y++)
        for (uint z = 0; z < 4; z++)
        {
            float3 offset = Remap(float3(x, y, z), 0, 3, -0.5f, 0.5f) * relocateLimit;
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

        // Relocate the probe to the best found location (or zero if nothing good found)
        if (bestOffset.w <= threshold)
            bestOffset.xyz = float3(0, 0, 0);
        probeOffset = bestOffset.xyz;
#endif

        // Check if probe was scrolled
        int3 probeScrollClears = ProbeScrollClears[CascadeIndex].xyz;
        bool wasScrolled = false;
        UNROLL
        for (uint planeIndex = 0; planeIndex < 3; planeIndex++)
        {
            int probeCount = (int)DDGI.ProbesCounts[planeIndex];
            int newCord = (int)probeCoords[planeIndex] + probeScrollClears[planeIndex];
            if (newCord < 0 || newCord >= probeCount)
                wasScrolled = true;
        }

        // If probe was in different location or was inactive last frame then mark it as activated
        bool wasInactive = probeState == DDGI_PROBE_STATE_INACTIVE;
        bool wasRelocated = distance(probeOffset, probeOffsetOld) > 2.0f;
        probeState = wasInactive || wasScrolled || wasRelocated ? DDGI_PROBE_STATE_ACTIVATED : DDGI_PROBE_STATE_ACTIVE;
    }

    // Save probe state
    probeOffset /= probesSpacing; // Move offset back to [-1;1] space
    RWProbesData[probeDataCoords] = EncodeDDGIProbeData(probeOffset, probeState);

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
    uint probesCount = DDGI.ProbesCounts.x * DDGI.ProbesCounts.y * DDGI.ProbesCounts.z;
    uint activeProbesCount = ActiveProbes.Load(0);
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

Texture3D<float> GlobalSDFTex : register(t0);
Texture3D<float> GlobalSDFMip : register(t1);
ByteAddressBuffer GlobalSurfaceAtlasChunks : register(t2);
ByteAddressBuffer RWGlobalSurfaceAtlasCulledObjects : register(t3);
Buffer<float4> GlobalSurfaceAtlasObjects : register(t4);
Texture2D GlobalSurfaceAtlasDepth : register(t5);
Texture2D GlobalSurfaceAtlasTex : register(t6);
Texture2D<snorm float4> ProbesData : register(t7);
TextureCube Skybox : register(t8);
ByteAddressBuffer ActiveProbes : register(t9);

// Compute shader for tracing rays for probes using Global SDF and Global Surface Atlas.
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
    uint probeState = DecodeDDGIProbeState(probeData);
    uint probeRaysCount = GetProbeRaysCount(DDGI, probeState);
    if (probeState == DDGI_PROBE_STATE_INACTIVE || rayIndex >= probeRaysCount)
        return; // Skip disabled probes or if current thread's ray is unused
    float3 probePosition = DecodeDDGIProbePosition(DDGI, probeData, CascadeIndex, probeIndex, probeCoords);
    float3 probeRayDirection = GetProbeRayDirection(DDGI, rayIndex);

    // Trace ray with Global SDF
    GlobalSDFTrace trace;
    trace.Init(probePosition, probeRayDirection, 0.0f, DDGI.RayMaxDistance);
    GlobalSDFHit hit = RayTraceGlobalSDF(GlobalSDF, GlobalSDFTex, GlobalSDFMip, trace);

    // Calculate radiance and distance
    float4 radiance;
    if (hit.IsHit())
    {
        if (hit.HitSDF <= 0.0f && hit.HitTime <= GlobalSDF.CascadeVoxelSize[0])
        {
            // Ray starts inside geometry (mark as negative distance and reduce it's influence during irradiance blending)
            radiance = float4(0, 0, 0, hit.HitTime * -0.25f);
        }
        else
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
        radiance.rgb = Skybox.SampleLevel(SamplerLinearClamp, probeRayDirection, 0).rgb;
        radiance.a = 1e27f; // Sky is the limit
    }

    // Write into probes trace results
    RWProbesTrace[uint2(rayIndex, DispatchThreadId.x)] = radiance;
}

#endif

#if defined(_CS_UpdateProbes) || defined(_CS_UpdateBorders)

#if DDGI_PROBE_UPDATE_MODE == 0
// Update irradiance
#define DDGI_PROBE_RESOLUTION DDGI_PROBE_RESOLUTION_IRRADIANCE
groupshared float4 CachedProbesTraceRadiance[DDGI_TRACE_RAYS_LIMIT];
#else
// Update distance
#define DDGI_PROBE_RESOLUTION DDGI_PROBE_RESOLUTION_DISTANCE
groupshared float CachedProbesTraceDistance[DDGI_TRACE_RAYS_LIMIT];
#endif

groupshared float3 CachedProbesTraceDirection[DDGI_TRACE_RAYS_LIMIT];

RWTexture2D<float4> RWOutput : register(u0);
Texture2D<snorm float4> ProbesData : register(t0);
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

    // Skip disabled probes
    bool skip = false;
    float4 probeData = LoadDDGIProbeData(DDGI, ProbesData, CascadeIndex, probeIndex);
    uint probeState = DecodeDDGIProbeState(probeData);
    uint probeRaysCount = GetProbeRaysCount(DDGI, probeState);
    if (probeState == DDGI_PROBE_STATE_INACTIVE)
        skip = true;

#if DDGI_PROBE_UPDATE_MODE == 0
    uint backfacesCount = 0;
    uint backfacesLimit = uint(probeRaysCount * 0.1f);
#else
    float probesSpacing = DDGI.ProbesOriginAndSpacing[CascadeIndex].w;
    float distanceLimit = length(probesSpacing) * 1.5f;
#endif

    BRANCH
    if (!skip)
    {
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
            CachedProbesTraceDirection[rayIndex] = GetProbeRayDirection(DDGI, rayIndex);
        }
    }
    GroupMemoryBarrierWithGroupSync();
    if (skip)
        return;
    probeCoords = GetDDGIProbeCoords(DDGI, probeIndex);
    uint2 outputCoords = GetDDGIProbeTexelCoords(DDGI, CascadeIndex, probeIndex) * (DDGI_PROBE_RESOLUTION + 2) + 1 + GroupThreadId.xy;

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
    float3 previous = RWOutput[outputCoords].rgb;
    bool wasActivated = probeState == DDGI_PROBE_STATE_ACTIVATED;
    if (ResetBlend || wasActivated)
        previous = float3(0, 0, 0);

    // Blend current value with the previous probe data
    float historyWeight = DDGI.ProbeHistoryWeight;
    //historyWeight = 0.0f;
    if (ResetBlend || wasActivated || dot(previous, previous) == 0)
        historyWeight = 0.0f;
#if DDGI_PROBE_UPDATE_MODE == 0
    result *= DDGI.IndirectLightingIntensity;
#if DDGI_SRGB_BLENDING
    result.rgb = pow(result.rgb, 1.0f / DDGI.IrradianceGamma);
#endif
    float3 irradianceDelta = result.rgb - previous;
    float irradianceDeltaMax = Max3(abs(irradianceDelta));
    float irradianceDeltaLen = length(irradianceDelta);
    if (irradianceDeltaMax > 0.5f)
    {
        // Reduce history weight after significant lighting change
        historyWeight = historyWeight * 0.5f;
    }
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
}

// Compute shader for updating probes irradiance or distance texture borders (fills gaps between probes to support bilinear filtering)
META_CS(true, FEATURE_LEVEL_SM5)
META_PERMUTATION_2(DDGI_PROBE_UPDATE_MODE=0, BORDER_ROW=1)
META_PERMUTATION_2(DDGI_PROBE_UPDATE_MODE=0, BORDER_ROW=0)
META_PERMUTATION_2(DDGI_PROBE_UPDATE_MODE=1, BORDER_ROW=1)
META_PERMUTATION_2(DDGI_PROBE_UPDATE_MODE=1, BORDER_ROW=0)
[numthreads(DDGI_PROBE_UPDATE_BORDERS_GROUP_SIZE, DDGI_PROBE_UPDATE_BORDERS_GROUP_SIZE, 1)]
void CS_UpdateBorders(uint3 DispatchThreadId : SV_DispatchThreadID)
{
#define COPY_PIXEL RWOutput[threadCoordinates] = RWOutput[copyCoordinates]
#define COPY_PIXEL_DEBUG RWOutput[threadCoordinates] = float4(5, 0, 0, 1)

    uint probeSideLength = DDGI_PROBE_RESOLUTION + 2;
    uint probeSideLengthMinusOne = probeSideLength - 1;
    uint2 copyCoordinates = uint2(0, 0);
    uint2 threadCoordinates = DispatchThreadId.xy;
#if BORDER_ROW
    threadCoordinates.y *= probeSideLength;
    uint corner = DispatchThreadId.x % probeSideLength;
#else
    threadCoordinates.x *= probeSideLength;
    uint corner = threadCoordinates.y % probeSideLength;
#endif
    if (corner == 0 || corner == probeSideLengthMinusOne)
    {
#if !BORDER_ROW
        // Left corner
        copyCoordinates.x = threadCoordinates.x + DDGI_PROBE_RESOLUTION;
        copyCoordinates.y = threadCoordinates.y - sign((int)corner - 1) * DDGI_PROBE_RESOLUTION;
        COPY_PIXEL;

        // Right corner
        threadCoordinates.x += probeSideLengthMinusOne;
        copyCoordinates.x = threadCoordinates.x - DDGI_PROBE_RESOLUTION;
        COPY_PIXEL;
#endif
        return;
    }

#if BORDER_ROW
    // Top row
    uint probeStart = uint(threadCoordinates.x / probeSideLength) * probeSideLength;
    uint offset = probeSideLengthMinusOne - (threadCoordinates.x % probeSideLength);
    copyCoordinates = uint2(probeStart + offset, threadCoordinates.y + 1);
#else
    // Left column
    uint probeStart = uint(threadCoordinates.y / probeSideLength) * probeSideLength;
    uint offset = probeSideLengthMinusOne - (threadCoordinates.y % probeSideLength);
    copyCoordinates = uint2(threadCoordinates.x + 1, probeStart + offset);
#endif
    COPY_PIXEL;

#if BORDER_ROW
    // Bottom row
    threadCoordinates.y += probeSideLengthMinusOne;
    copyCoordinates = uint2(probeStart + offset, threadCoordinates.y - 1);
#else
    // Right column
    threadCoordinates.x += probeSideLengthMinusOne;
    copyCoordinates = uint2(threadCoordinates.x - 1, probeStart + offset);
#endif
    COPY_PIXEL;

#undef COPY_PIXEL
#undef COPY_PIXEL_DEBUG
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
