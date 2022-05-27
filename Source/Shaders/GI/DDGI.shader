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
#include "./Flax/Quaternion.hlsl"
#include "./Flax/GlobalSignDistanceField.hlsl"
#include "./Flax/GI/GlobalSurfaceAtlas.hlsl"
#include "./Flax/GI/DDGI.hlsl"

// This must match C++
#define DDGI_TRACE_RAYS_LIMIT 512 // Limit of rays per-probe (runtime value can be smaller)
#define DDGI_TRACE_RAYS_GROUP_SIZE_X 32
#define DDGI_PROBE_UPDATE_BORDERS_GROUP_SIZE 8
#define DDGI_PROBE_CLASSIFY_GROUP_SIZE 32

META_CB_BEGIN(0, Data)
DDGIData DDGI;
GlobalSDFData GlobalSDF;
GlobalSurfaceAtlasData GlobalSurfaceAtlas;
GBufferData GBuffer;
float3 Padding0;
float IndirectLightingIntensity;
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
    float3 direction = GetSphericalFibonacci(rayIndex, data.RaysCount);
    return normalize(QuaternionRotate(data.RaysRotation, direction));
}

#ifdef _CS_Classify

RWTexture2D<float4> RWProbesState : register(u0);

Texture3D<float> GlobalSDFTex[4] : register(t0);

// Compute shader for updating probes state between active and inactive.
META_CS(true, FEATURE_LEVEL_SM5)
[numthreads(DDGI_PROBE_CLASSIFY_GROUP_SIZE, 1, 1)]
void CS_Classify(uint3 DispatchThreadId : SV_DispatchThreadID)
{
    uint probeIndex = DispatchThreadId.x;
    uint probesCount = DDGI.ProbesCounts.x * DDGI.ProbesCounts.y * DDGI.ProbesCounts.z;
    if (probeIndex >= probesCount)
        return;
    uint3 probeCoords = GetDDGIProbeCoords(DDGI, probeIndex);
    probeIndex = GetDDGIScrollingProbeIndex(DDGI, probeCoords);
    int2 probeDataCoords = GetDDGIProbeTexelCoords(DDGI, probeIndex);

    // Load probe state and position
    float4 probeState = RWProbesState[probeDataCoords];
    float3 probePosition = GetDDGIProbeWorldPosition(DDGI, probeCoords);
    // TODO: reset probe offset for scrolled probes
    probePosition.xyz += probeState.xyz;
    probeState.w = DDGI_PROBE_STATE_ACTIVE;

    // Use Global SDF to quickly get distance and direction to the scene geometry
    float sdf;
    float3 sdfNormal = normalize(SampleGlobalSDFGradient(GlobalSDF, GlobalSDFTex, probePosition.xyz, sdf));
    float threshold = GlobalSDF.CascadeVoxelSize[0] * 0.5f;
    float distanceLimit = length(DDGI.ProbesSpacing) * 1.5f + threshold;
    float relocateLimit = length(DDGI.ProbesSpacing) * 0.6f;
    if (abs(sdf) > distanceLimit + threshold) // Probe is too far from geometry
    {
        // Disable it
        probeState = float4(0, 0, 0, DDGI_PROBE_STATE_INACTIVE);
    }
    else if (sdf < threshold) // Probe is inside geometry
    {
        if (abs(sdf) < relocateLimit)
        {
            float3 offsetToAdd = sdfNormal * (sdf + threshold);
            if (distance(probeState.xyz, offsetToAdd) < relocateLimit)
            {
                // Relocate it
                probeState.xyz = probeState.xyz + offsetToAdd;
            }
            // TODO: maybe sample SDF at the relocated location and disable probe if it's still in the geometry?
        }
        else
        {
            // Reset relocation
            probeState.xyz = float3(0, 0, 0);
        }
    }
    else if (sdf > relocateLimit) // Probe is far enough any geometry
    {
        // Reset relocation
        probeState.xyz = float3(0, 0, 0);
    }

    RWProbesState[probeDataCoords] = probeState;
}

#endif

#ifdef _CS_TraceRays

RWTexture2D<float4> RWProbesTrace : register(u0);

Texture3D<float> GlobalSDFTex[4] : register(t0);
Texture3D<float> GlobalSDFMip[4] : register(t4);
ByteAddressBuffer GlobalSurfaceAtlasChunks : register(t8);
Buffer<float4> GlobalSurfaceAtlasCulledObjects : register(t9);
Texture2D GlobalSurfaceAtlasDepth : register(t10);
Texture2D GlobalSurfaceAtlasTex : register(t11);
Texture2D<float4> ProbesState : register(t12);
TextureCube Skybox : register(t13);

// Compute shader for tracing rays for probes using Global SDF and Global Surface Atlas.
META_CS(true, FEATURE_LEVEL_SM5)
[numthreads(DDGI_TRACE_RAYS_GROUP_SIZE_X, 1, 1)]
void CS_TraceRays(uint3 GroupId : SV_GroupID, uint3 DispatchThreadId : SV_DispatchThreadID, uint3 GroupThreadId : SV_GroupThreadID)
{
    uint rayIndex = DispatchThreadId.x;
    uint probeIndex = DispatchThreadId.y;
    uint3 probeCoords = GetDDGIProbeCoords(DDGI, probeIndex);
    probeIndex = GetDDGIScrollingProbeIndex(DDGI, probeCoords);

    // Load current probe state and position
    float4 probePositionAndState = LoadDDGIProbePositionAndState(DDGI, ProbesState, probeIndex, probeCoords);
    if (probePositionAndState.w == DDGI_PROBE_STATE_INACTIVE)
        return; // Skip disabled probes
    float3 probeRayDirection = GetProbeRayDirection(DDGI, rayIndex);

    // Trace ray with Global SDF
    GlobalSDFTrace trace;
    trace.Init(probePositionAndState.xyz, probeRayDirection, 0.0f, DDGI.RayMaxDistance);
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
            float surfaceThreshold = GetGlobalSurfaceAtlasThreshold(hit);
            float4 surfaceColor = SampleGlobalSurfaceAtlas(GlobalSurfaceAtlas, GlobalSurfaceAtlasChunks, GlobalSurfaceAtlasCulledObjects, GlobalSurfaceAtlasDepth, GlobalSurfaceAtlasTex, hitPosition, -probeRayDirection, surfaceThreshold);
            radiance = float4(surfaceColor.rgb, hit.HitTime);

            // Add some bias to prevent self occlusion artifacts in Chebyshev due to Global SDF being very incorrect in small scale
            radiance.w = max(radiance.w + GlobalSDF.CascadeVoxelSize[hit.HitCascade] * 0.5f, 0);
        }
    }
    else
    {
        // Ray hits sky
        radiance.rgb = Skybox.SampleLevel(SamplerLinearClamp, probeRayDirection, 0);
        radiance.a = 1e27f; // Sky is the limit
    }

    // Write into probes trace results
    RWProbesTrace[uint2(rayIndex, probeIndex)] = radiance;
}

#endif

#if defined(_CS_UpdateProbes) || defined(_CS_UpdateBorders)

#if DDGI_PROBE_UPDATE_MODE == 0
// Update irradiance
#define DDGI_PROBE_RESOLUTION DDGI_PROBE_RESOLUTION_IRRADIANCE
#else
// Update distance
#define DDGI_PROBE_RESOLUTION DDGI_PROBE_RESOLUTION_DISTANCE
#endif

groupshared float4 CachedProbesTraceRadiance[DDGI_TRACE_RAYS_LIMIT];
groupshared float3 CachedProbesTraceDirection[DDGI_TRACE_RAYS_LIMIT];

RWTexture2D<float4> RWOutput : register(u0);
Texture2D<float4> ProbesState : register(t0);
Texture2D<float4> ProbesTrace : register(t1);

// Compute shader for updating probes irradiance or distance texture.
META_CS(true, FEATURE_LEVEL_SM5)
META_PERMUTATION_1(DDGI_PROBE_UPDATE_MODE=0)
META_PERMUTATION_1(DDGI_PROBE_UPDATE_MODE=1)
[numthreads(DDGI_PROBE_RESOLUTION, DDGI_PROBE_RESOLUTION, 1)]
void CS_UpdateProbes(uint3 DispatchThreadId : SV_DispatchThreadID, uint GroupIndex : SV_GroupIndex)
{
    // Get probe index and atlas location in the atlas
    uint probeIndex = GetDDGIProbeIndex(DDGI, DispatchThreadId.xy, DDGI_PROBE_RESOLUTION);
    uint probesCount = DDGI.ProbesCounts.x * DDGI.ProbesCounts.y * DDGI.ProbesCounts.z;
    bool skip = probeIndex >= probesCount;
    uint2 outputCoords = uint2(1, 1) + DispatchThreadId.xy + (DispatchThreadId.xy / DDGI_PROBE_RESOLUTION) * 2;

    // Clear probes that have been scrolled to a new positions (blending with current irradiance will happen the next frame)
    uint3 probeCoords = GetDDGIProbeCoords(DDGI, probeIndex);
    UNROLL
    for (uint planeIndex = 0; planeIndex < 3; planeIndex++)
    {
        if (DDGI.ProbeScrollClear[planeIndex])
        {
            int scrollOffset = DDGI.ProbesScrollOffsets[planeIndex];
            int scrollDirection = DDGI.ProbeScrollDirections[planeIndex];
            uint probeCount = DDGI.ProbesCounts[planeIndex];
            uint coord = (probeCount + (scrollDirection ? (scrollOffset - 1) : (scrollOffset % probeCount))) % probeCount;
            if (probeCoords[planeIndex] == coord)
            {
                // Clear probe and return
                //RWOutput[outputCoords] = float4(0, 0, 0, 0);
                if (!skip)
                    RWOutput[outputCoords] = float4(0, 0, 0, 0);
                skip = true;
            }
        }
    }

    // Skip disabled probes
    float probeState = LoadDDGIProbeState(DDGI, ProbesState, probeIndex);
    if (probeState == DDGI_PROBE_STATE_INACTIVE)
        skip = true;

    // Calculate octahedral projection for probe (unwraps spherical projection into a square)
    float2 octahedralCoords = GetOctahedralCoords(DispatchThreadId.xy, DDGI_PROBE_RESOLUTION);
    float3 octahedralDirection = GetOctahedralDirection(octahedralCoords);

    // Load trace rays results into shared memory to reuse across whole thread group
    uint count = (uint)(ceil((float)(DDGI_TRACE_RAYS_LIMIT) / (float)(DDGI_PROBE_RESOLUTION * DDGI_PROBE_RESOLUTION)));
    for (uint i = 0; i < count; i++)
    {
        uint rayIndex = (GroupIndex * count) + i;
        if (rayIndex >= DDGI.RaysCount)
            break;
        CachedProbesTraceRadiance[rayIndex] = ProbesTrace[uint2(rayIndex, probeIndex)];
        CachedProbesTraceDirection[rayIndex] = GetProbeRayDirection(DDGI, rayIndex);
    }
    GroupMemoryBarrierWithGroupSync();

    // TODO: optimize probes updating to build indirect dispatch args and probes indices list before tracing rays and blending irradiance/distance
    if (skip)
    {
        // Clear probe
        //RWOutput[outputCoords] = float4(0, 0, 0, 0);
        return;
    }

    // Loop over rays
    float4 result = float4(0, 0, 0, 0);
#if DDGI_PROBE_UPDATE_MODE == 0
    uint backfacesCount = 0;
    uint backfacesLimit = uint(DDGI.RaysCount * 0.1f);
#else
    float distanceLimit = length(DDGI.ProbesSpacing) * 1.5f;
#endif
    LOOP
    for (uint rayIndex = 0; rayIndex < DDGI.RaysCount; rayIndex++)
    {
        float3 rayDirection = CachedProbesTraceDirection[rayIndex];
        float rayWeight = max(dot(octahedralDirection, rayDirection), 0.0f);
        float4 rayRadiance = CachedProbesTraceRadiance[rayIndex];

#if DDGI_PROBE_UPDATE_MODE == 0
        if (rayRadiance.w < 0.0f)
        {
            // Count backface hits
            backfacesCount++;

            // Skip further blending after reaching backfaces limit
            if (backfacesCount >= backfacesLimit)
                return;
            continue;
        }

        // Add radiance (RGB) and weight (A)
        result += float4(rayRadiance.rgb * rayWeight, rayWeight);
#else
        // Increase reaction speed for depth discontinuities
        rayWeight = pow(rayWeight, 4.0f);

        // Add distance (R), distance^2 (G) and weight (A)
        float rayDistance = min(abs(rayRadiance.w), distanceLimit);
        result += float4(rayDistance * rayWeight, (rayDistance * rayDistance) * rayWeight, 0.0f, rayWeight);
#endif
    }

    // Normalize results
    float epsilon = (float)DDGI.RaysCount * 1e-9f;
    result.rgb *= 1.0f / (2.0f * max(result.a, epsilon));

    // Blend current value with the previous probe data
    float3 previous = RWOutput[outputCoords].rgb;
    float historyWeight = DDGI.ProbeHistoryWeight;
    if (dot(previous, previous) == 0)
    {
        // Cut any blend from zero
        historyWeight = 0.0f;
    }
#if DDGI_PROBE_UPDATE_MODE == 0
    result *= IndirectLightingIntensity;
#if DDGI_SRGB_BLENDING
    result.rgb = pow(result.rgb, 1.0f / DDGI.IrradianceGamma);
#endif
    float3 irradianceDelta = result.rgb - previous.rgb;
    float irradianceDeltaMax = Max3(abs(irradianceDelta));
    if (irradianceDeltaMax > 0.25f)
    {
        // Reduce history weight after significant lighting change
        historyWeight = max(historyWeight - 0.2f, 0.0f);
    }
    if (irradianceDeltaMax > 0.8f)
    {
        // Reduce flickering during rapid brightness changes
        result.rgb = previous.rgb + (irradianceDelta * 0.25f);
    }
    float3 resultDelta = (1.0f - historyWeight) * irradianceDelta;
    if (Max3(result.rgb) < Max3(previous.rgb))
        resultDelta = min(max(abs(resultDelta), 1.0f / 1024.0f), abs(irradianceDelta)) * sign(resultDelta);
    result = float4(previous.rgb + resultDelta, 1.0f);
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
#include "./Flax/LightingCommon.hlsl"

Texture2D<float4> ProbesState : register(t4);
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
    float bias = 1.0f;
    float3 irradiance = SampleDDGIIrradiance(DDGI, ProbesState, ProbesDistance, ProbesIrradiance, gBuffer.WorldPos, gBuffer.Normal, bias);

    // Calculate lighting
    float3 diffuseColor = GetDiffuseColor(gBuffer);
    float3 diffuse = Diffuse_Lambert(diffuseColor);
    output = float4(diffuse * irradiance, 1);
}

#endif
