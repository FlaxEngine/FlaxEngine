// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"
#include "./Flax/Collisions.hlsl"

#define GLOBAL_SDF_RASTERIZE_CHUNK_SIZE 32
#define GLOBAL_SDF_RASTERIZE_CHUNK_MARGIN 4
#define GLOBAL_SDF_MIP_FLOODS 5
#define GLOBAL_SDF_WORLD_SIZE 60000.0f

// Global SDF data for a constant buffer
struct GlobalSDFData
{
    float4 CascadePosDistance[4];
    float4 CascadeVoxelSize;
    float2 Padding;
    uint CascadesCount;
    float Resolution;
};

// Global SDF ray trace settings.
struct GlobalSDFTrace
{
    float3 WorldPosition;
    float MinDistance;
    float3 WorldDirection;
    float MaxDistance;
    float StepScale;
    bool NeedsHitNormal;

    void Init(float3 worldPosition, float3 worldDirection, float minDistance, float maxDistance, float stepScale = 1.0f)
    {
        WorldPosition = worldPosition;
        WorldDirection = worldDirection;
        MinDistance = minDistance;
        MaxDistance = maxDistance;
        StepScale = stepScale;
        NeedsHitNormal = false;
    }
};

// Global SDF ray trace hit information.
struct GlobalSDFHit
{
    float3 HitNormal;
    float HitTime;
    uint HitCascade;
    uint StepsCount;
    float HitSDF;

    bool IsHit()
    {
        return HitTime >= 0.0f;
    }

    float3 GetHitPosition(const GlobalSDFTrace trace)
    {
        return trace.WorldPosition + trace.WorldDirection * HitTime;
    }
};

void GetGlobalSDFCascadeUV(const GlobalSDFData data, uint cascade, float3 worldPosition, out float cascadeMaxDistance, out float3 cascadeUV, out float3 textureUV)
{
    float4 cascadePosDistance = data.CascadePosDistance[cascade];
    float3 posInCascade = worldPosition - cascadePosDistance.xyz;
    cascadeMaxDistance = cascadePosDistance.w * 2;
    cascadeUV = saturate(posInCascade / cascadeMaxDistance + 0.5f);
    textureUV = float3(((float)cascade + cascadeUV.x) / (float)data.CascadesCount, cascadeUV.y, cascadeUV.z); // cascades are placed next to each other on X axis
}

// Gets the Global SDF cascade index for the given world location.
uint GetGlobalSDFCascade(const GlobalSDFData data, float3 worldPosition)
{
    for (uint cascade = 0; cascade < data.CascadesCount; cascade++)
    {
        float cascadeMaxDistance;
        float3 cascadeUV, textureUV;
        GetGlobalSDFCascadeUV(data, cascade, worldPosition, cascadeMaxDistance, cascadeUV, textureUV);
        if (all(cascadeUV > 0) && all(cascadeUV < 1))
            return cascade;
    }
    return 0;
}

// Samples the Global SDF cascade and returns the distance to the closest surface (in world units) at the given world location.
float SampleGlobalSDFCascade(const GlobalSDFData data, Texture3D<float> tex, float3 worldPosition, uint cascade)
{
    float distance = GLOBAL_SDF_WORLD_SIZE;
    float cascadeMaxDistance;
    float3 cascadeUV, textureUV;
    GetGlobalSDFCascadeUV(data, cascade, worldPosition, cascadeMaxDistance, cascadeUV, textureUV);
    float cascadeDistance = tex.SampleLevel(SamplerLinearClamp, textureUV, 0);
    if (cascadeDistance < 1.0f && !any(cascadeUV < 0) && !any(cascadeUV > 1))
        distance = cascadeDistance * cascadeMaxDistance;
    return distance;
}

// Samples the Global SDF and returns the distance to the closest surface (in world units) at the given world location.
float SampleGlobalSDF(const GlobalSDFData data, Texture3D<float> tex, float3 worldPosition)
{
    float distance = data.CascadePosDistance[3].w * 2.0f;
    if (distance <= 0.0f)
        return GLOBAL_SDF_WORLD_SIZE;
    for (uint cascade = 0; cascade < data.CascadesCount; cascade++)
    {
        float cascadeMaxDistance;
        float3 cascadeUV, textureUV;
        GetGlobalSDFCascadeUV(data, cascade, worldPosition, cascadeMaxDistance, cascadeUV, textureUV);
        float cascadeDistance = tex.SampleLevel(SamplerLinearClamp, textureUV, 0);
        if (cascadeDistance < 0.9f && !any(cascadeUV < 0) && !any(cascadeUV > 1))
        {
            distance = cascadeDistance * cascadeMaxDistance;
            break;
        }
    }
    return distance;
}

// Samples the Global SDF and returns the distance to the closest surface (in world units) at the given world location.
float SampleGlobalSDF(const GlobalSDFData data, Texture3D<float> tex, Texture3D<float> mip, float3 worldPosition)
{
    float distance = data.CascadePosDistance[3].w * 2.0f;
    if (distance <= 0.0f)
        return GLOBAL_SDF_WORLD_SIZE;
    float chunkSizeDistance = (float)GLOBAL_SDF_RASTERIZE_CHUNK_SIZE / data.Resolution; // Size of the chunk in SDF distance (0-1)
    float chunkMarginDistance = (float)GLOBAL_SDF_RASTERIZE_CHUNK_MARGIN / data.Resolution; // Size of the chunk margin in SDF distance (0-1)
    for (uint cascade = 0; cascade < data.CascadesCount; cascade++)
    {
        float cascadeMaxDistance;
        float3 cascadeUV, textureUV;
        GetGlobalSDFCascadeUV(data, cascade, worldPosition, cascadeMaxDistance, cascadeUV, textureUV);
        float cascadeDistance = mip.SampleLevel(SamplerLinearClamp, textureUV, 0);
        if (cascadeDistance < chunkSizeDistance && !any(cascadeUV < 0) && !any(cascadeUV > 1))
        {
            float cascadeDistanceTex = tex.SampleLevel(SamplerLinearClamp, textureUV, 0);
            if (cascadeDistanceTex < chunkMarginDistance * 2)
                cascadeDistance = cascadeDistanceTex;
            distance = cascadeDistance * cascadeMaxDistance;
            break;
        }
    }
    return distance;
}

// Samples the Global SDF and returns the gradient vector (derivative) at the given world location. Normalize it to get normal vector.
float3 SampleGlobalSDFGradient(const GlobalSDFData data, Texture3D<float> tex, float3 worldPosition, out float distance)
{
    float3 gradient = float3(0, 0.00001f, 0);
    distance = GLOBAL_SDF_WORLD_SIZE;
    if (data.CascadePosDistance[3].w <= 0.0f)
        return gradient;
    for (uint cascade = 0; cascade < data.CascadesCount; cascade++)
    {
        float cascadeMaxDistance;
        float3 cascadeUV, textureUV;
        GetGlobalSDFCascadeUV(data, cascade, worldPosition, cascadeMaxDistance, cascadeUV, textureUV);
        float cascadeDistance = tex.SampleLevel(SamplerLinearClamp, textureUV, 0);
        if (cascadeDistance < 0.9f && !any(cascadeUV < 0) && !any(cascadeUV > 1))
        {
            float texelOffset = 1.0f / data.Resolution;
            float xp = tex.SampleLevel(SamplerLinearClamp, float3(textureUV.x + texelOffset, textureUV.y, textureUV.z), 0).x;
            float xn = tex.SampleLevel(SamplerLinearClamp, float3(textureUV.x - texelOffset, textureUV.y, textureUV.z), 0).x;
            float yp = tex.SampleLevel(SamplerLinearClamp, float3(textureUV.x, textureUV.y + texelOffset, textureUV.z), 0).x;
            float yn = tex.SampleLevel(SamplerLinearClamp, float3(textureUV.x, textureUV.y - texelOffset, textureUV.z), 0).x;
            float zp = tex.SampleLevel(SamplerLinearClamp, float3(textureUV.x, textureUV.y, textureUV.z + texelOffset), 0).x;
            float zn = tex.SampleLevel(SamplerLinearClamp, float3(textureUV.x, textureUV.y, textureUV.z - texelOffset), 0).x;
            gradient = float3(xp - xn, yp - yn, zp - zn) * cascadeMaxDistance;
            distance = cascadeDistance * cascadeMaxDistance;
            break;
        }
    }
    return gradient;
}

// Samples the Global SDF and returns the gradient vector (derivative) at the given world location. Normalize it to get normal vector.
float3 SampleGlobalSDFGradient(const GlobalSDFData data, Texture3D<float> tex, Texture3D<float> mip, float3 worldPosition, out float distance)
{
    float3 gradient = float3(0, 0.00001f, 0);
    distance = GLOBAL_SDF_WORLD_SIZE;
    if (data.CascadePosDistance[3].w <= 0.0f)
        return gradient;
    float chunkSizeDistance = (float)GLOBAL_SDF_RASTERIZE_CHUNK_SIZE / data.Resolution; // Size of the chunk in SDF distance (0-1)
    float chunkMarginDistance = (float)GLOBAL_SDF_RASTERIZE_CHUNK_MARGIN / data.Resolution; // Size of the chunk margin in SDF distance (0-1)
    for (uint cascade = 0; cascade < data.CascadesCount; cascade++)
    {
        float cascadeMaxDistance;
        float3 cascadeUV, textureUV;
        GetGlobalSDFCascadeUV(data, cascade, worldPosition, cascadeMaxDistance, cascadeUV, textureUV);
        float cascadeDistance = mip.SampleLevel(SamplerLinearClamp, textureUV, 0);
        if (cascadeDistance < chunkSizeDistance && !any(cascadeUV < 0) && !any(cascadeUV > 1))
        {
            float cascadeDistanceTex = tex.SampleLevel(SamplerLinearClamp, textureUV, 0);
            if (cascadeDistanceTex < chunkMarginDistance * 2)
                cascadeDistance = cascadeDistanceTex;
            float texelOffset = 1.0f / data.Resolution;
            float xp = tex.SampleLevel(SamplerLinearClamp, float3(textureUV.x + texelOffset, textureUV.y, textureUV.z), 0).x;
            float xn = tex.SampleLevel(SamplerLinearClamp, float3(textureUV.x - texelOffset, textureUV.y, textureUV.z), 0).x;
            float yp = tex.SampleLevel(SamplerLinearClamp, float3(textureUV.x, textureUV.y + texelOffset, textureUV.z), 0).x;
            float yn = tex.SampleLevel(SamplerLinearClamp, float3(textureUV.x, textureUV.y - texelOffset, textureUV.z), 0).x;
            float zp = tex.SampleLevel(SamplerLinearClamp, float3(textureUV.x, textureUV.y, textureUV.z + texelOffset), 0).x;
            float zn = tex.SampleLevel(SamplerLinearClamp, float3(textureUV.x, textureUV.y, textureUV.z - texelOffset), 0).x;
            gradient = float3(xp - xn, yp - yn, zp - zn) * cascadeMaxDistance;
            distance = cascadeDistance * cascadeMaxDistance;
            break;
        }
    }
    return gradient;
}

// Ray traces the Global SDF.
// cascadeTraceStartBias - scales the trace start position offset (along the trace direction) by cascade voxel size (reduces artifacts on far cascades). Use it for shadow rays to prevent self-occlusion when tracing from object surface that looses quality in far cascades.
GlobalSDFHit RayTraceGlobalSDF(const GlobalSDFData data, Texture3D<float> tex, Texture3D<float> mip, const GlobalSDFTrace trace, float cascadeTraceStartBias = 0.0f)
{
    GlobalSDFHit hit = (GlobalSDFHit)0;
    hit.HitTime = -1.0f;
    float chunkSizeDistance = (float)GLOBAL_SDF_RASTERIZE_CHUNK_SIZE / data.Resolution; // Size of the chunk in SDF distance (0-1)
    float chunkMarginDistance = (float)GLOBAL_SDF_RASTERIZE_CHUNK_MARGIN / data.Resolution; // Size of the chunk margin in SDF distance (0-1)
    float nextIntersectionStart = 0.0f;
    float traceMaxDistance = min(trace.MaxDistance, data.CascadePosDistance[3].w * 2);
    float3 traceEndPosition = trace.WorldPosition + trace.WorldDirection * traceMaxDistance;
    for (uint cascade = 0; cascade < data.CascadesCount && hit.HitTime < 0.0f; cascade++)
    {
        float4 cascadePosDistance = data.CascadePosDistance[cascade];
        float voxelSize = data.CascadeVoxelSize[cascade];
        float voxelExtent = voxelSize * 0.5f;
        float cascadeMinStep = voxelSize;
        float3 worldPosition = trace.WorldPosition + trace.WorldDirection * (voxelSize * cascadeTraceStartBias);

        // Hit the cascade bounds to find the intersection points
        float2 intersections = LineHitBox(worldPosition, traceEndPosition, cascadePosDistance.xyz - cascadePosDistance.www, cascadePosDistance.xyz + cascadePosDistance.www);
        intersections.xy *= traceMaxDistance;
        intersections.x = max(intersections.x, nextIntersectionStart);
        float stepTime = intersections.x;
        if (intersections.x >= intersections.y)
        {
            // Skip the current cascade if the ray starts outside it
            stepTime = intersections.y;
        }
        else
        {
            // Skip the current cascade tracing on the next cascade
            nextIntersectionStart = intersections.y;
        }

        // Walk over the cascade SDF
        uint step = 0;
        LOOP
        for (; step < 250 && stepTime < intersections.y; step++)
        {
            float3 stepPosition = worldPosition + trace.WorldDirection * stepTime;

            // Sample SDF
            float cascadeMaxDistance;
            float3 cascadeUV, textureUV;
            GetGlobalSDFCascadeUV(data, cascade, stepPosition, cascadeMaxDistance, cascadeUV, textureUV);
            float stepDistance = mip.SampleLevel(SamplerLinearClamp, textureUV, 0);
            if (stepDistance < chunkSizeDistance)
            {
                float stepDistanceTex = tex.SampleLevel(SamplerLinearClamp, textureUV, 0);
                if (stepDistanceTex < chunkMarginDistance * 2)
                {
                    stepDistance = stepDistanceTex;
                }
            }
            else
            {
                // Assume no SDF nearby so perform a jump
                stepDistance = chunkSizeDistance;
            }
            stepDistance *= cascadeMaxDistance;

            // Detect surface hit
            float minSurfaceThickness = voxelExtent * saturate(stepTime / (voxelExtent * 2.0f));
            if (stepDistance < minSurfaceThickness)
            {
                // Surface hit
                hit.HitTime = max(stepTime + stepDistance - minSurfaceThickness, 0.0f);
                hit.HitCascade = cascade;
                hit.HitSDF = stepDistance;
                if (trace.NeedsHitNormal)
                {
                    // Calculate hit normal from SDF gradient
                    float texelOffset = 1.0f / data.Resolution;
                    float xp = tex.SampleLevel(SamplerLinearClamp, float3(textureUV.x + texelOffset, textureUV.y, textureUV.z), 0).x;
                    float xn = tex.SampleLevel(SamplerLinearClamp, float3(textureUV.x - texelOffset, textureUV.y, textureUV.z), 0).x;
                    float yp = tex.SampleLevel(SamplerLinearClamp, float3(textureUV.x, textureUV.y + texelOffset, textureUV.z), 0).x;
                    float yn = tex.SampleLevel(SamplerLinearClamp, float3(textureUV.x, textureUV.y - texelOffset, textureUV.z), 0).x;
                    float zp = tex.SampleLevel(SamplerLinearClamp, float3(textureUV.x, textureUV.y, textureUV.z + texelOffset), 0).x;
                    float zn = tex.SampleLevel(SamplerLinearClamp, float3(textureUV.x, textureUV.y, textureUV.z - texelOffset), 0).x;
                    hit.HitNormal = normalize(float3(xp - xn, yp - yn, zp - zn));
                }
                break;
            }

            // Move forward
            stepTime += max(stepDistance * trace.StepScale, cascadeMinStep);
        }
        hit.StepsCount += step;
    }
    return hit;
}

// Calculates the surface threshold for Global Surface Atlas sampling which matches the Global SDF trace to reduce artifacts
float GetGlobalSurfaceAtlasThreshold(const GlobalSDFData data, const GlobalSDFHit hit)
{
    // Scale the threshold based on the hit cascade (less precision)
    return data.CascadeVoxelSize[hit.HitCascade] * 1.1f;
}
