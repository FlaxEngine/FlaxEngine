// Copyright (c) Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"
#include "./Flax/Collisions.hlsl"

// This must match C++
#define GLOBAL_SDF_RASTERIZE_CHUNK_SIZE 32
#define GLOBAL_SDF_RASTERIZE_CHUNK_MARGIN 4
#define GLOBAL_SDF_RASTERIZE_MIP_FACTOR 4
#define GLOBAL_SDF_MIP_FLOODS 5
#define GLOBAL_SDF_WORLD_SIZE 60000.0f
#define GLOBAL_SDF_MIN_VALID 0.9f
#define GLOBAL_SDF_CHUNK_MARGIN_SCALE 4.0f
#define GLOBAL_SDF_SAMPLER SamplerLinearClamp

// Global SDF data for a constant buffer
struct GlobalSDFData
{
    float4 CascadePosDistance[4];
    float4 CascadeVoxelSize;
    float4 CascadeMaxDistanceTex;
    float4 CascadeMaxDistanceMip;
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

    void Init(float3 worldPosition, float3 worldDirection, float minDistance, float maxDistance)
    {
        WorldPosition = worldPosition;
        WorldDirection = worldDirection;
        MinDistance = minDistance;
        MaxDistance = maxDistance;
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

void GetGlobalSDFCascadeUV(const GlobalSDFData data, uint cascade, float3 worldPosition, out float3 cascadeUV, out float3 textureUV)
{
    float4 cascadePosDistance = data.CascadePosDistance[cascade];
    float3 posInCascade = worldPosition - cascadePosDistance.xyz;
    float cascadeSize = cascadePosDistance.w * 2;
    cascadeUV = saturate(posInCascade / cascadeSize + 0.5f);
    textureUV = float3(((float)cascade + cascadeUV.x) / (float)data.CascadesCount, cascadeUV.y, cascadeUV.z); // Cascades are placed next to each other on X axis
}

void GetGlobalSDFCascadeUV(const GlobalSDFData data, uint cascade, float3 worldPosition, out float3 cascadeUV, out float3 textureUV, out float3 textureMipUV)
{
    float4 cascadePosDistance = data.CascadePosDistance[cascade];
    float3 posInCascade = worldPosition - cascadePosDistance.xyz;
    float cascadeSize = cascadePosDistance.w * 2;
    cascadeUV = saturate(posInCascade / cascadeSize + 0.5f);
    textureUV = float3(((float)cascade + cascadeUV.x) / (float)data.CascadesCount, cascadeUV.y, cascadeUV.z); // Cascades are placed next to each other on X axis
    float halfTexelOffsetMip = (GLOBAL_SDF_RASTERIZE_MIP_FACTOR * 0.5f) / data.Resolution;
    textureMipUV = textureUV + float3(halfTexelOffsetMip / (float)data.CascadesCount, halfTexelOffsetMip, halfTexelOffsetMip); // Mipmaps are offset by half texel to sample correctly
}

// Clamps Global SDF cascade UV to ensure it can be used for gradient sampling (clamps first and last pixels).
void ClampGlobalSDFTextureGradientUV(const GlobalSDFData data, uint cascade, float texelOffset, inout float3 textureUV)
{
    float cascadeSizeUV = 1.0f / data.CascadesCount;
    float cascadeUVStart = cascadeSizeUV * cascade + texelOffset * 2;
    float cascadeUVEnd = cascadeUVStart + cascadeSizeUV - texelOffset * 4;
    textureUV.x = clamp(textureUV.x, cascadeUVStart, cascadeUVEnd);
}

// Gets the Global SDF cascade index for the given world location.
uint GetGlobalSDFCascade(const GlobalSDFData data, float3 worldPosition)
{
    for (uint cascade = 0; cascade < data.CascadesCount; cascade++)
    {
        float3 cascadeUV, textureUV;
        GetGlobalSDFCascadeUV(data, cascade, worldPosition, cascadeUV, textureUV);
        if (all(cascadeUV > 0) && all(cascadeUV < 1))
            return cascade;
    }
    return 0;
}

// Samples the Global SDF cascade and returns the distance to the closest surface (in world units) at the given world location.
float SampleGlobalSDFCascade(const GlobalSDFData data, Texture3D<snorm float> tex, float3 worldPosition, uint cascade)
{
    float distance = GLOBAL_SDF_WORLD_SIZE;
    float3 cascadeUV, textureUV;
    GetGlobalSDFCascadeUV(data, cascade, worldPosition, cascadeUV, textureUV);
    float voxelSize = data.CascadeVoxelSize[cascade];
    float chunkMargin = voxelSize * (GLOBAL_SDF_CHUNK_MARGIN_SCALE * GLOBAL_SDF_RASTERIZE_CHUNK_MARGIN);
    float maxDistanceTex = data.CascadeMaxDistanceTex[cascade];
    float distanceTex = tex.SampleLevel(GLOBAL_SDF_SAMPLER, textureUV, 0) * maxDistanceTex;
    if (distanceTex < chunkMargin && all(cascadeUV > 0) && all(cascadeUV < 1))
        distance = distanceTex;
    return distance;
}

// Samples the Global SDF and returns the distance to the closest surface (in world units) at the given world location.
float SampleGlobalSDF(const GlobalSDFData data, Texture3D<snorm float> tex, float3 worldPosition)
{
    float distance = data.CascadePosDistance[3].w * 2.0f;
    if (distance <= 0.0f)
        return GLOBAL_SDF_WORLD_SIZE;
    for (uint cascade = 0; cascade < data.CascadesCount; cascade++)
    {
        float3 cascadeUV, textureUV;
        GetGlobalSDFCascadeUV(data, cascade, worldPosition, cascadeUV, textureUV);
        float voxelSize = data.CascadeVoxelSize[cascade];
        float chunkMargin = voxelSize * (GLOBAL_SDF_CHUNK_MARGIN_SCALE * GLOBAL_SDF_RASTERIZE_CHUNK_MARGIN);
        float maxDistanceTex = data.CascadeMaxDistanceTex[cascade];
        float distanceTex = tex.SampleLevel(GLOBAL_SDF_SAMPLER, textureUV, 0);
        if (distanceTex < chunkMargin && all(cascadeUV > 0) && all(cascadeUV < 1))
        {
            distance = distanceTex * maxDistanceTex;
            break;
        }
    }
    return distance;
}

// Samples the Global SDF and returns the distance to the closest surface (in world units) at the given world location.
float SampleGlobalSDF(const GlobalSDFData data, Texture3D<snorm float> tex, Texture3D<snorm float> mip, float3 worldPosition, uint startCascade = 0)
{
    float distance = data.CascadePosDistance[3].w * 2.0f;
    if (distance <= 0.0f)
        return GLOBAL_SDF_WORLD_SIZE;
    startCascade = min(startCascade, data.CascadesCount - 1);
    for (uint cascade = startCascade; cascade < data.CascadesCount; cascade++)
    {
        float3 cascadeUV, textureUV, textureMipUV;
        GetGlobalSDFCascadeUV(data, cascade, worldPosition, cascadeUV, textureUV, textureMipUV);
        float voxelSize = data.CascadeVoxelSize[cascade];
        float chunkSize = voxelSize * GLOBAL_SDF_RASTERIZE_CHUNK_SIZE;
        float chunkMargin = voxelSize * (GLOBAL_SDF_CHUNK_MARGIN_SCALE * GLOBAL_SDF_RASTERIZE_CHUNK_MARGIN);
        float maxDistanceMip = data.CascadeMaxDistanceMip[cascade];
        float distanceMip = mip.SampleLevel(GLOBAL_SDF_SAMPLER, textureMipUV, 0);
        if (distanceMip < chunkSize && all(cascadeUV > 0) && all(cascadeUV < 1))
        {
            distance = distanceMip * maxDistanceMip;
            float maxDistanceTex = data.CascadeMaxDistanceTex[cascade];
            float distanceTex = tex.SampleLevel(GLOBAL_SDF_SAMPLER, textureUV, 0) * maxDistanceTex;
            if (distanceTex < chunkMargin)
                distance = distanceTex;
            break;
        }
    }
    return distance;
}

// Samples the Global SDF and returns the gradient vector (derivative) at the given world location. Normalize it to get normal vector.
float3 SampleGlobalSDFGradient(const GlobalSDFData data, Texture3D<snorm float> tex, float3 worldPosition, out float distance, uint startCascade = 0)
{
    float3 gradient = float3(0, 0.00001f, 0);
    distance = GLOBAL_SDF_WORLD_SIZE;
    if (data.CascadePosDistance[3].w <= 0.0f)
        return gradient;
    startCascade = min(startCascade, data.CascadesCount - 1);
    for (uint cascade = startCascade; cascade < data.CascadesCount; cascade++)
    {
        float3 cascadeUV, textureUV;
        GetGlobalSDFCascadeUV(data, cascade, worldPosition, cascadeUV, textureUV);
        float voxelSize = data.CascadeVoxelSize[cascade];
        float chunkMargin = voxelSize * (GLOBAL_SDF_CHUNK_MARGIN_SCALE * GLOBAL_SDF_RASTERIZE_CHUNK_MARGIN);
        float maxDistanceTex = data.CascadeMaxDistanceTex[cascade];
        float distanceTex = tex.SampleLevel(GLOBAL_SDF_SAMPLER, textureUV, 0);
        if (distanceTex < chunkMargin && all(cascadeUV > 0) && all(cascadeUV < 1))
        {
            float texelOffset = 1.0f / data.Resolution;
            ClampGlobalSDFTextureGradientUV(data, cascade, texelOffset, textureUV);
            float xp = tex.SampleLevel(GLOBAL_SDF_SAMPLER, float3(textureUV.x + texelOffset, textureUV.y, textureUV.z), 0).x;
            float xn = tex.SampleLevel(GLOBAL_SDF_SAMPLER, float3(textureUV.x - texelOffset, textureUV.y, textureUV.z), 0).x;
            float yp = tex.SampleLevel(GLOBAL_SDF_SAMPLER, float3(textureUV.x, textureUV.y + texelOffset, textureUV.z), 0).x;
            float yn = tex.SampleLevel(GLOBAL_SDF_SAMPLER, float3(textureUV.x, textureUV.y - texelOffset, textureUV.z), 0).x;
            float zp = tex.SampleLevel(GLOBAL_SDF_SAMPLER, float3(textureUV.x, textureUV.y, textureUV.z + texelOffset), 0).x;
            float zn = tex.SampleLevel(GLOBAL_SDF_SAMPLER, float3(textureUV.x, textureUV.y, textureUV.z - texelOffset), 0).x;
            gradient = float3(xp - xn, yp - yn, zp - zn) * maxDistanceTex;
            distance = distanceTex * maxDistanceTex;
            break;
        }
    }
    return gradient;
}

// Samples the Global SDF and returns the gradient vector (derivative) at the given world location. Normalize it to get normal vector.
float3 SampleGlobalSDFGradient(const GlobalSDFData data, Texture3D<snorm float> tex, Texture3D<snorm float> mip, float3 worldPosition, out float distance, uint startCascade = 0)
{
    float3 gradient = float3(0, 0.00001f, 0);
    distance = GLOBAL_SDF_WORLD_SIZE;
    if (data.CascadePosDistance[3].w <= 0.0f)
        return gradient;
    startCascade = min(startCascade, data.CascadesCount - 1);
    for (uint cascade = startCascade; cascade < data.CascadesCount; cascade++)
    {
        float3 cascadeUV, textureUV, textureMipUV;
        GetGlobalSDFCascadeUV(data, cascade, worldPosition, cascadeUV, textureUV, textureMipUV);
        float voxelSize = data.CascadeVoxelSize[cascade];
        float chunkSize = voxelSize * GLOBAL_SDF_RASTERIZE_CHUNK_SIZE;
        float chunkMargin = voxelSize * (GLOBAL_SDF_CHUNK_MARGIN_SCALE * GLOBAL_SDF_RASTERIZE_CHUNK_MARGIN);
        float maxDistanceMip = data.CascadeMaxDistanceMip[cascade];
        float distanceMip = mip.SampleLevel(GLOBAL_SDF_SAMPLER, textureMipUV, 0) * maxDistanceMip;
        if (distanceMip < chunkSize && all(cascadeUV > 0) && all(cascadeUV < 1))
        {
            float maxDistanceTex = data.CascadeMaxDistanceTex[cascade];
            float distanceTex = tex.SampleLevel(GLOBAL_SDF_SAMPLER, textureUV, 0) * maxDistanceTex;
            if (distanceTex < chunkMargin)
            {
                distance = distanceTex;
                float texelOffset = 1.0f / data.Resolution;
                ClampGlobalSDFTextureGradientUV(data, cascade, texelOffset, textureUV);
                float xp = tex.SampleLevel(GLOBAL_SDF_SAMPLER, float3(textureUV.x + texelOffset, textureUV.y, textureUV.z), 0).x;
                float xn = tex.SampleLevel(GLOBAL_SDF_SAMPLER, float3(textureUV.x - texelOffset, textureUV.y, textureUV.z), 0).x;
                float yp = tex.SampleLevel(GLOBAL_SDF_SAMPLER, float3(textureUV.x, textureUV.y + texelOffset, textureUV.z), 0).x;
                float yn = tex.SampleLevel(GLOBAL_SDF_SAMPLER, float3(textureUV.x, textureUV.y - texelOffset, textureUV.z), 0).x;
                float zp = tex.SampleLevel(GLOBAL_SDF_SAMPLER, float3(textureUV.x, textureUV.y, textureUV.z + texelOffset), 0).x;
                float zn = tex.SampleLevel(GLOBAL_SDF_SAMPLER, float3(textureUV.x, textureUV.y, textureUV.z - texelOffset), 0).x;
                gradient = float3(xp - xn, yp - yn, zp - zn) * maxDistanceTex;
            }
            else
            {
                distance = distanceMip;
                float texelOffset = (float)GLOBAL_SDF_RASTERIZE_MIP_FACTOR / data.Resolution;
                ClampGlobalSDFTextureGradientUV(data, cascade, texelOffset, textureMipUV);
                float xp = mip.SampleLevel(GLOBAL_SDF_SAMPLER, float3(textureMipUV.x + texelOffset, textureMipUV.y, textureMipUV.z), 0).x;
                float xn = mip.SampleLevel(GLOBAL_SDF_SAMPLER, float3(textureMipUV.x - texelOffset, textureMipUV.y, textureMipUV.z), 0).x;
                float yp = mip.SampleLevel(GLOBAL_SDF_SAMPLER, float3(textureMipUV.x, textureMipUV.y + texelOffset, textureMipUV.z), 0).x;
                float yn = mip.SampleLevel(GLOBAL_SDF_SAMPLER, float3(textureMipUV.x, textureMipUV.y - texelOffset, textureMipUV.z), 0).x;
                float zp = mip.SampleLevel(GLOBAL_SDF_SAMPLER, float3(textureMipUV.x, textureMipUV.y, textureMipUV.z + texelOffset), 0).x;
                float zn = mip.SampleLevel(GLOBAL_SDF_SAMPLER, float3(textureMipUV.x, textureMipUV.y, textureMipUV.z - texelOffset), 0).x;
                gradient = float3(xp - xn, yp - yn, zp - zn) * maxDistanceMip;
            }
            break;
        }
    }
    return gradient;
}

// Ray traces the Global SDF.
// cascadeTraceStartBias - scales the trace start position offset (along the trace direction) by cascade voxel size (reduces artifacts on far cascades). Use it for shadow rays to prevent self-occlusion when tracing from object surface that looses quality in far cascades.
GlobalSDFHit RayTraceGlobalSDF(const GlobalSDFData data, Texture3D<snorm float> tex, Texture3D<snorm float> mip, const GlobalSDFTrace trace, float cascadeTraceStartBias = 0.0f)
{
    GlobalSDFHit hit = (GlobalSDFHit)0;
    hit.HitTime = -1.0f;
    float nextIntersectionStart = trace.MinDistance;
    float traceMaxDistance = min(trace.MaxDistance, data.CascadePosDistance[3].w * 2);
    float3 traceEndPosition = trace.WorldPosition + trace.WorldDirection * traceMaxDistance;
    LOOP
    for (uint cascade = 0; cascade < data.CascadesCount && hit.HitTime < 0.0f; cascade++)
    {
        float4 cascadePosDistance = data.CascadePosDistance[cascade];
        float voxelSize = data.CascadeVoxelSize[cascade];
        float voxelExtent = voxelSize * 0.5f;

        // Skip until cascade that contains the start location
        if (any(abs(trace.WorldPosition - cascadePosDistance.xyz) > cascadePosDistance.w))
            continue;

        // Hit the cascade bounds to find the intersection points
        float traceStartBias = voxelSize * cascadeTraceStartBias;
        float2 intersections = LineHitBox(trace.WorldPosition, traceEndPosition, cascadePosDistance.xyz - cascadePosDistance.www, cascadePosDistance.xyz + cascadePosDistance.www);
        intersections.xy *= traceMaxDistance;
        intersections.x = max(intersections.x, traceStartBias);
        intersections.x = max(intersections.x, nextIntersectionStart);
        if (intersections.x < intersections.y)
        {
            // Skip the current cascade tracing on the next cascade
            nextIntersectionStart = max(nextIntersectionStart, intersections.y - voxelSize);

            // Walk over the cascade SDF
            uint step = 0;
            float stepTime = intersections.x;
            float chunkSize = voxelSize * GLOBAL_SDF_RASTERIZE_CHUNK_SIZE;
            float chunkMargin = voxelSize * (GLOBAL_SDF_CHUNK_MARGIN_SCALE * GLOBAL_SDF_RASTERIZE_CHUNK_MARGIN);
            float maxDistanceTex = data.CascadeMaxDistanceTex[cascade];
            float maxDistanceMip = data.CascadeMaxDistanceMip[cascade];
            LOOP
            for (; step < 100 && stepTime < intersections.y && hit.HitTime < 0.0f; step++)
            {
                float3 stepPosition = trace.WorldPosition + trace.WorldDirection * stepTime;

                // Sample SDF
                float stepDistance;
                float3 cascadeUV, textureUV, textureMipUV;
                GetGlobalSDFCascadeUV(data, cascade, stepPosition, cascadeUV, textureUV, textureMipUV);
                stepDistance = min(mip.SampleLevel(GLOBAL_SDF_SAMPLER, textureMipUV, 0) * maxDistanceMip, chunkSize);
                float distanceTex = tex.SampleLevel(GLOBAL_SDF_SAMPLER, textureUV, 0) * maxDistanceTex;
                FLATTEN
                if (distanceTex < chunkMargin)
                    stepDistance = distanceTex;

                // Detect surface hit
                float minSurfaceThickness = voxelExtent * saturate(stepTime / voxelSize);
                if (stepDistance < minSurfaceThickness)
                {
                    // Surface hit
                    hit.HitTime = max(stepTime + stepDistance - minSurfaceThickness, 0.0f);
                    hit.HitCascade = cascade;
                    hit.HitSDF = stepDistance;
                }

                // Move forward
                stepTime += max(stepDistance, voxelSize);
            }
            hit.StepsCount += step;
        }
    }
    return hit;
}

// Calculates the surface threshold for Global Surface Atlas sampling which matches the Global SDF trace to reduce artifacts
float GetGlobalSurfaceAtlasThreshold(const GlobalSDFData data, const GlobalSDFHit hit)
{
    // Scale the threshold based on the hit cascade (less precision)
    return data.CascadeVoxelSize[hit.HitCascade] * 1.17f;
}
