// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#ifndef __SHADOWS_SAMPLING__
#define __SHADOWS_SAMPLING__

#include "./Flax/ShadowsCommon.hlsl"
#include "./Flax/GBufferCommon.hlsl"
#include "./Flax/LightingCommon.hlsl"

// Select shadows filter based on quality
// Supported sampling kernel sizes fo each shadowing technique:
// CSM: 2, 3, 5, 7, 9
// Cube: 2, 5, 12, 29
// Spot: 2, 5, 12, 29
#if SHADOWS_QUALITY == 0

#define FilterSizeCSM 2
#define FilterSizeCube 2
#define FilterSizeSpot 2

#elif SHADOWS_QUALITY == 1

    #define FilterSizeCSM 3
    #define FilterSizeCube 5
    #define FilterSizeSpot 5
    
#elif SHADOWS_QUALITY == 2

    #define FilterSizeCSM 5
    #define FilterSizeCube 12
    #define FilterSizeSpot 12
    
#else // SHADOWS_QUALITY == 3

    #define FilterSizeCSM 7
    #define FilterSizeCube 12
    #define FilterSizeSpot 12
    
#endif

#if SHADOWS_QUALITY != 0
#include "./Flax/PCFKernels.hlsl"
#endif

// Gets the cube texture face index to use for shadow map sampling for the given view-to-light direction vector
// Where: direction = normalize(worldPosition - lightPosition)
int GetCubeFaceIndex(float3 direction)
{
    int cubeFaceIndex;
    float3 absDirection = abs(direction);
    float maxDirection = max(absDirection.x, max(absDirection.y, absDirection.z));
    if (maxDirection == absDirection.x)
        cubeFaceIndex = absDirection.x == direction.x ? 0 : 1;
    else if (maxDirection == absDirection.y)
        cubeFaceIndex = absDirection.y == direction.y ? 2 : 3;
    else
        cubeFaceIndex = absDirection.z == direction.z ? 4 : 5;
    return cubeFaceIndex;
}

// Samples the shadow map with a fixed-size PCF kernel optimized with GatherCmpRed.
// Uses code from "Fast Conventional Shadow Filtering" by Holger Gruen, in GPU Pro.
float SampleShadowMapFixedSizePCF(Texture2DArray shadowMap, float2 shadowMapSize, float sceneDepth, float2 shadowPos, uint cascadeIndex)
{
#if FilterSizeCSM == 2

#if FEATURE_LEVEL >= FEATURE_LEVEL_SM5

#if FLAX_REVERSE_Z
    return 1 - shadowMap.SampleCmpLevelZero(ShadowSamplerPCF, float3(shadowPos.xy, cascadeIndex), sceneDepth);
#else
    return shadowMap.SampleCmpLevelZero(ShadowSamplerPCF, float3(shadowPos.xy, cascadeIndex), sceneDepth);
#endif

#else

#if FLAX_REVERSE_Z
    return sceneDepth > shadowMap.SampleLevel(SamplerLinearClamp, float3(shadowPos.xy, cascadeIndex), 0).r;
#else
    return sceneDepth < shadowMap.SampleLevel(SamplerLinearClamp, float3(shadowPos.xy, cascadeIndex), 0).r;
#endif

#endif

#else

    const int FS_2 = FilterSizeCSM / 2;
    float2 tc = shadowPos.xy;
    float4 s = 0.0f;
    float2 stc = (shadowMapSize * tc.xy) + float2(0.5f, 0.5f);
    float2 tcs = floor(stc);
    float2 fc;
    int row;
    int col;
    float4 v1[FS_2 + 1];
    float2 v0[FS_2 + 1];
    float3 baseUV = float3(tc.xy, cascadeIndex);
    float2 shadowMapSizeInv = 1.0f / shadowMapSize;

    fc.xy = stc - tcs;
    tc.xy = tcs * shadowMapSizeInv;
    
    // Loop over the rows
    UNROLL
    for (row = -FS_2; row <= FS_2; row += 2)
    {
        UNROLL
        for (col = -FS_2; col <= FS_2; col += 2)
        {
            float value = CSMFilterWeights[row + FS_2][col + FS_2];

            if (col > -FS_2)
                value += CSMFilterWeights[row + FS_2][col + FS_2 - 1];

            if (col < FS_2)
                value += CSMFilterWeights[row + FS_2][col + FS_2 + 1];

            if (row > -FS_2) {
                value += CSMFilterWeights[row + FS_2 - 1][col + FS_2];

                if (col < FS_2)
                    value += CSMFilterWeights[row + FS_2 - 1][col + FS_2 + 1];

                if (col > -FS_2)
                    value += CSMFilterWeights[row + FS_2 - 1][col + FS_2 - 1];
            }

            if (value != 0.0f)
            {
                // Gather returns xyzw which is counter clockwise order starting with the sample to the lower left of the queried location
#if CAN_USE_GATHER

#if FLAX_REVERSE_Z
                // Revert the sign if reverse z is enabled, the same below
                v1[(col + FS_2) / 2] = 1 - shadowMap.GatherCmp(ShadowSampler, baseUV, sceneDepth, int2(col, row));
#else
                v1[(col + FS_2) / 2] = shadowMap.GatherCmp(ShadowSampler, baseUV, sceneDepth, int2(col, row));
#endif

#else
                
                float4 gather;
#if FLAX_REVERSE_Z
                gather.x = sceneDepth > shadowMap.SampleLevel(SamplerPointClamp, float3(tc.xy + float2(0, 1) * shadowMapSizeInv, cascadeIndex), 0, int2(col, row)).r;
                gather.y = sceneDepth > shadowMap.SampleLevel(SamplerPointClamp, float3(tc.xy + float2(1, 1) * shadowMapSizeInv, cascadeIndex), 0, int2(col, row)).r;
                gather.z = sceneDepth > shadowMap.SampleLevel(SamplerPointClamp, float3(tc.xy + float2(1, 0) * shadowMapSizeInv, cascadeIndex), 0, int2(col, row)).r;
                gather.w = sceneDepth > shadowMap.SampleLevel(SamplerPointClamp, float3(tc.xy + float2(0, 0) * shadowMapSizeInv, cascadeIndex), 0, int2(col, row)).r;
#else
                gather.x = sceneDepth < shadowMap.SampleLevel(SamplerPointClamp, float3(tc.xy + float2(0, 1) * shadowMapSizeInv, cascadeIndex), 0, int2(col, row)).r;
                gather.y = sceneDepth < shadowMap.SampleLevel(SamplerPointClamp, float3(tc.xy + float2(1, 1) * shadowMapSizeInv, cascadeIndex), 0, int2(col, row)).r;
                gather.z = sceneDepth < shadowMap.SampleLevel(SamplerPointClamp, float3(tc.xy + float2(1, 0) * shadowMapSizeInv, cascadeIndex), 0, int2(col, row)).r;
                gather.w = sceneDepth < shadowMap.SampleLevel(SamplerPointClamp, float3(tc.xy + float2(0, 0) * shadowMapSizeInv, cascadeIndex), 0, int2(col, row)).r;
#endif

                v1[(col + FS_2) / 2] = gather;
#endif
            }
            else
                v1[(col + FS_2) / 2] = 0.0f;

            if (col == -FS_2)
            {
                s.x += (1.0f - fc.y) * (v1[0].w * (CSMFilterWeights[row + FS_2][col + FS_2]
                                     - CSMFilterWeights[row + FS_2][col + FS_2] * fc.x)
                                     + v1[0].z * (fc.x * (CSMFilterWeights[row + FS_2][col + FS_2]
                                     - CSMFilterWeights[row + FS_2][col + FS_2 + 1])
                                     + CSMFilterWeights[row + FS_2][col + FS_2 + 1]));
                s.y += fc.y * (v1[0].x * (CSMFilterWeights[row + FS_2][col + FS_2]
                                     - CSMFilterWeights[row + FS_2][col + FS_2] * fc.x)
                                     + v1[0].y * (fc.x * (CSMFilterWeights[row + FS_2][col + FS_2]
                                     - CSMFilterWeights[row + FS_2][col + FS_2 + 1])
                                     +  CSMFilterWeights[row + FS_2][col + FS_2 + 1]));
                if(row > -FS_2)
                {
                    s.z += (1.0f - fc.y) * (v0[0].x * (CSMFilterWeights[row + FS_2 - 1][col + FS_2]
                                           - CSMFilterWeights[row + FS_2 - 1][col + FS_2] * fc.x)
                                           + v0[0].y * (fc.x * (CSMFilterWeights[row + FS_2 - 1][col + FS_2]
                                           - CSMFilterWeights[row + FS_2 - 1][col + FS_2 + 1])
                                           + CSMFilterWeights[row + FS_2 - 1][col + FS_2 + 1]));
                    s.w += fc.y * (v1[0].w * (CSMFilterWeights[row + FS_2 - 1][col + FS_2]
                                        - CSMFilterWeights[row + FS_2 - 1][col + FS_2] * fc.x)
                                        + v1[0].z * (fc.x * (CSMFilterWeights[row + FS_2 - 1][col + FS_2]
                                        - CSMFilterWeights[row + FS_2 - 1][col + FS_2 + 1])
                                        + CSMFilterWeights[row + FS_2 - 1][col + FS_2 + 1]));
                }
            }
            else if (col == FS_2)
            {
                s.x += (1 - fc.y) * (v1[FS_2].w * (fc.x * (CSMFilterWeights[row + FS_2][col + FS_2 - 1]
                                     - CSMFilterWeights[row + FS_2][col + FS_2]) + CSMFilterWeights[row + FS_2][col + FS_2])
                                     + v1[FS_2].z * fc.x * CSMFilterWeights[row + FS_2][col + FS_2]);
                s.y += fc.y * (v1[FS_2].x * (fc.x * (CSMFilterWeights[row + FS_2][col + FS_2 - 1]
                                     - CSMFilterWeights[row + FS_2][col + FS_2] ) + CSMFilterWeights[row + FS_2][col + FS_2])
                                     + v1[FS_2].y * fc.x * CSMFilterWeights[row + FS_2][col + FS_2]);
                if(row > -FS_2) {
                    s.z += (1 - fc.y) * (v0[FS_2].x * (fc.x * (CSMFilterWeights[row + FS_2 - 1][col + FS_2 - 1]
                                        - CSMFilterWeights[row + FS_2 - 1][col + FS_2])
                                        + CSMFilterWeights[row + FS_2 - 1][col + FS_2])
                                        + v0[FS_2].y * fc.x * CSMFilterWeights[row + FS_2 - 1][col + FS_2]);
                    s.w += fc.y * (v1[FS_2].w * (fc.x * (CSMFilterWeights[row + FS_2 - 1][col + FS_2 - 1]
                                        - CSMFilterWeights[row + FS_2 - 1][col + FS_2])
                                        + CSMFilterWeights[row + FS_2 - 1][col + FS_2])
                                        + v1[FS_2].z * fc.x * CSMFilterWeights[row + FS_2 - 1][col + FS_2]);
                }
            }
            else
            {
                s.x += (1 - fc.y) * (v1[(col + FS_2) / 2].w * (fc.x * (CSMFilterWeights[row + FS_2][col + FS_2 - 1]
                                    - CSMFilterWeights[row + FS_2][col + FS_2 + 0] ) + CSMFilterWeights[row + FS_2][col + FS_2 + 0])
                                    + v1[(col + FS_2) / 2].z * (fc.x * (CSMFilterWeights[row + FS_2][col + FS_2 - 0]
                                    - CSMFilterWeights[row + FS_2][col + FS_2 + 1]) + CSMFilterWeights[row + FS_2][col + FS_2 + 1]));
                s.y += fc.y * (v1[(col + FS_2) / 2].x * (fc.x * (CSMFilterWeights[row + FS_2][col + FS_2-1]
                                    - CSMFilterWeights[row + FS_2][col + FS_2 + 0]) + CSMFilterWeights[row + FS_2][col + FS_2 + 0])
                                    + v1[(col + FS_2) / 2].y * (fc.x * (CSMFilterWeights[row + FS_2][col + FS_2 - 0]
                                    - CSMFilterWeights[row + FS_2][col + FS_2 + 1]) + CSMFilterWeights[row + FS_2][col + FS_2 + 1]));
                if(row > -FS_2) {
                    s.z += (1 - fc.y) * (v0[(col + FS_2) / 2].x * (fc.x * (CSMFilterWeights[row + FS_2 - 1][col + FS_2 - 1]
                                            - CSMFilterWeights[row + FS_2 - 1][col + FS_2 + 0]) + CSMFilterWeights[row + FS_2 - 1][col + FS_2 + 0])
                                            + v0[(col + FS_2) / 2].y * (fc.x * (CSMFilterWeights[row + FS_2 - 1][col + FS_2 - 0]
                                            - CSMFilterWeights[row + FS_2 - 1][col + FS_2 + 1]) + CSMFilterWeights[row + FS_2 - 1][col + FS_2 + 1]));
                    s.w += fc.y * (v1[(col + FS_2) / 2].w * (fc.x * (CSMFilterWeights[row + FS_2 - 1][col + FS_2 - 1]
                                            - CSMFilterWeights[row + FS_2 - 1][col + FS_2 + 0]) + CSMFilterWeights[row + FS_2 - 1][col + FS_2 + 0])
                                            + v1[(col + FS_2) / 2].z * (fc.x * (CSMFilterWeights[row + FS_2 - 1][col + FS_2 - 0]
                                            - CSMFilterWeights[row + FS_2 - 1][col + FS_2 + 1]) + CSMFilterWeights[row + FS_2 - 1][col + FS_2 + 1]));
                }
            }

            if (row != FS_2)
                v0[(col + FS_2) / 2] = v1[(col + FS_2) / 2].xy;
        }
    }

    return dot(s, 1.0f) / CSMFilterWeightsSum;
    
#endif
}

// Helper function for SampleShadowMapOptimizedPCF
float SampleShadowMap(Texture2DArray shadowMap, float2 baseUv, float u, float v, float2 shadowMapSizeInv, uint cascadeIndex, float depth)
{
    float2 uv = baseUv + float2(u, v) * shadowMapSizeInv;
#if FLAX_REVERSE_Z
    return 1 - shadowMap.SampleCmpLevelZero(ShadowSamplerPCF, float3(uv, cascadeIndex), depth);
#else
    return shadowMap.SampleCmpLevelZero(ShadowSamplerPCF, float3(uv, cascadeIndex), depth);
#endif
}

// The method used in The Witness
float SampleShadowMapOptimizedPCF(Texture2DArray shadowMap, float2 shadowMapSize, float sceneDepth, float2 shadowPos, uint cascadeIndex)
{
    float2 uv = shadowPos.xy * shadowMapSize; // 1 unit - 1 texel
    float2 shadowMapSizeInv = 1.0f / shadowMapSize;

    float2 baseUv;
    baseUv.x = floor(uv.x + 0.5);
    baseUv.y = floor(uv.y + 0.5);
    float s = (uv.x + 0.5 - baseUv.x);
    float t = (uv.y + 0.5 - baseUv.y);
    baseUv -= float2(0.5, 0.5);
    baseUv *= shadowMapSizeInv;

    float sum = 0;

#if FilterSizeCSM == 2

#if FLAX_REVERSE_Z
    return 1 - shadowMap.SampleCmpLevelZero(ShadowSamplerPCF, float3(shadowPos.xy, cascadeIndex), sceneDepth);
#else
    return shadowMap.SampleCmpLevelZero(ShadowSamplerPCF, float3(shadowPos.xy, cascadeIndex), sceneDepth);
#endif

#elif FilterSizeCSM == 3

    float uw0 = (3 - 2 * s);
    float uw1 = (1 + 2 * s);

    float u0 = (2 - s) / uw0 - 1;
    float u1 = s / uw1 + 1;

    float vw0 = (3 - 2 * t);
    float vw1 = (1 + 2 * t);

    float v0 = (2 - t) / vw0 - 1;
    float v1 = t / vw1 + 1;

    sum += uw0 * vw0 * SampleShadowMap(shadowMap, baseUv, u0, v0, shadowMapSizeInv, cascadeIndex, sceneDepth);
    sum += uw1 * vw0 * SampleShadowMap(shadowMap, baseUv, u1, v0, shadowMapSizeInv, cascadeIndex, sceneDepth);
    sum += uw0 * vw1 * SampleShadowMap(shadowMap, baseUv, u0, v1, shadowMapSizeInv, cascadeIndex, sceneDepth);
    sum += uw1 * vw1 * SampleShadowMap(shadowMap, baseUv, u1, v1, shadowMapSizeInv, cascadeIndex, sceneDepth);

    return sum * 1.0f / 16;

#elif FilterSizeCSM == 5

    float uw0 = (4 - 3 * s);
    float uw1 = 7;
    float uw2 = (1 + 3 * s);

    float u0 = (3 - 2 * s) / uw0 - 2;
    float u1 = (3 + s) / uw1;
    float u2 = s / uw2 + 2;

    float vw0 = (4 - 3 * t);
    float vw1 = 7;
    float vw2 = (1 + 3 * t);

    float v0 = (3 - 2 * t) / vw0 - 2;
    float v1 = (3 + t) / vw1;
    float v2 = t / vw2 + 2;

    sum += uw0 * vw0 * SampleShadowMap(shadowMap, baseUv, u0, v0, shadowMapSizeInv, cascadeIndex, sceneDepth);
    sum += uw1 * vw0 * SampleShadowMap(shadowMap, baseUv, u1, v0, shadowMapSizeInv, cascadeIndex, sceneDepth);
    sum += uw2 * vw0 * SampleShadowMap(shadowMap, baseUv, u2, v0, shadowMapSizeInv, cascadeIndex, sceneDepth);

    sum += uw0 * vw1 * SampleShadowMap(shadowMap, baseUv, u0, v1, shadowMapSizeInv, cascadeIndex, sceneDepth);
    sum += uw1 * vw1 * SampleShadowMap(shadowMap, baseUv, u1, v1, shadowMapSizeInv, cascadeIndex, sceneDepth);
    sum += uw2 * vw1 * SampleShadowMap(shadowMap, baseUv, u2, v1, shadowMapSizeInv, cascadeIndex, sceneDepth);

    sum += uw0 * vw2 * SampleShadowMap(shadowMap, baseUv, u0, v2, shadowMapSizeInv, cascadeIndex, sceneDepth);
    sum += uw1 * vw2 * SampleShadowMap(shadowMap, baseUv, u1, v2, shadowMapSizeInv, cascadeIndex, sceneDepth);
    sum += uw2 * vw2 * SampleShadowMap(shadowMap, baseUv, u2, v2, shadowMapSizeInv, cascadeIndex, sceneDepth);

    return sum * 1.0f / 144;

#else // FilterSizeCSM == 7

    float uw0 = (5 * s - 6);
    float uw1 = (11 * s - 28);
    float uw2 = -(11 * s + 17);
    float uw3 = -(5 * s + 1);

    float u0 = (4 * s - 5) / uw0 - 3;
    float u1 = (4 * s - 16) / uw1 - 1;
    float u2 = -(7 * s + 5) / uw2 + 1;
    float u3 = -s / uw3 + 3;

    float vw0 = (5 * t - 6);
    float vw1 = (11 * t - 28);
    float vw2 = -(11 * t + 17);
    float vw3 = -(5 * t + 1);

    float v0 = (4 * t - 5) / vw0 - 3;
    float v1 = (4 * t - 16) / vw1 - 1;
    float v2 = -(7 * t + 5) / vw2 + 1;
    float v3 = -t / vw3 + 3;

    sum += uw0 * vw0 * SampleShadowMap(shadowMap, baseUv, u0, v0, shadowMapSizeInv, cascadeIndex, sceneDepth);
    sum += uw1 * vw0 * SampleShadowMap(shadowMap, baseUv, u1, v0, shadowMapSizeInv, cascadeIndex, sceneDepth);
    sum += uw2 * vw0 * SampleShadowMap(shadowMap, baseUv, u2, v0, shadowMapSizeInv, cascadeIndex, sceneDepth);
    sum += uw3 * vw0 * SampleShadowMap(shadowMap, baseUv, u3, v0, shadowMapSizeInv, cascadeIndex, sceneDepth);

    sum += uw0 * vw1 * SampleShadowMap(shadowMap, baseUv, u0, v1, shadowMapSizeInv, cascadeIndex, sceneDepth);
    sum += uw1 * vw1 * SampleShadowMap(shadowMap, baseUv, u1, v1, shadowMapSizeInv, cascadeIndex, sceneDepth);
    sum += uw2 * vw1 * SampleShadowMap(shadowMap, baseUv, u2, v1, shadowMapSizeInv, cascadeIndex, sceneDepth);
    sum += uw3 * vw1 * SampleShadowMap(shadowMap, baseUv, u3, v1, shadowMapSizeInv, cascadeIndex, sceneDepth);

    sum += uw0 * vw2 * SampleShadowMap(shadowMap, baseUv, u0, v2, shadowMapSizeInv, cascadeIndex, sceneDepth);
    sum += uw1 * vw2 * SampleShadowMap(shadowMap, baseUv, u1, v2, shadowMapSizeInv, cascadeIndex, sceneDepth);
    sum += uw2 * vw2 * SampleShadowMap(shadowMap, baseUv, u2, v2, shadowMapSizeInv, cascadeIndex, sceneDepth);
    sum += uw3 * vw2 * SampleShadowMap(shadowMap, baseUv, u3, v2, shadowMapSizeInv, cascadeIndex, sceneDepth);

    sum += uw0 * vw3 * SampleShadowMap(shadowMap, baseUv, u0, v3, shadowMapSizeInv, cascadeIndex, sceneDepth);
    sum += uw1 * vw3 * SampleShadowMap(shadowMap, baseUv, u1, v3, shadowMapSizeInv, cascadeIndex, sceneDepth);
    sum += uw2 * vw3 * SampleShadowMap(shadowMap, baseUv, u2, v3, shadowMapSizeInv, cascadeIndex, sceneDepth);
    sum += uw3 * vw3 * SampleShadowMap(shadowMap, baseUv, u3, v3, shadowMapSizeInv, cascadeIndex, sceneDepth);

    return sum * (1.0f / 2704);

#endif
}

// Samples the shadow from the shadow map cascade
float SampleShadowCascade(Texture2DArray shadowMap, float2 shadowMapSize, float sceneDepth, float2 shadowPosition, uint cascadeIndex)
{
    float shadow = SampleShadowMapFixedSizePCF(shadowMap, shadowMapSize, sceneDepth, shadowPosition, cascadeIndex);
    //float shadow = SampleShadowMapOptimizedPCF(shadowMap, shadowMapSize, sceneDepth, shadowPosition, cascadeIndex);
    return shadow;
}

// Samples the shadow for the given directional light (cascaded shadow map sampling)
float SampleShadow(LightData light, LightShadowData shadow, Texture2DArray shadowMap, float3 worldPosition, float viewDepth)
{
    // Create a blend factor which is one before and at the fade plane
    float fade = saturate((viewDepth - shadow.CascadeSplits[shadow.NumCascades - 1] + shadow.FadeDistance) / shadow.FadeDistance);
    BRANCH
    if (fade >= 1.0)
    {
        return 1;
    }

    // Figure out which cascade to sample from
    uint cascadeIndex = 0;
    for (uint i = 0; i < shadow.NumCascades - 1; i++)
    {
        if (viewDepth > shadow.CascadeSplits[i])
            cascadeIndex = i + 1;
    }

    // Project into shadow space
    float4 shadowPosition = mul(float4(worldPosition, 1.0f), shadow.ShadowVP[cascadeIndex]);
    shadowPosition.xy /= shadowPosition.w;

#if FLAX_REVERSE_Z
    // If reverse z is enabled, revert the sign of shadow bias
    shadowPosition.z += shadow.Bias;
#else
    shadowPosition.z -= shadow.Bias;
#endif

    // Sample shadow
    float result = SampleShadowCascade(shadowMap, shadow.ShadowMapSize, shadowPosition.z, shadowPosition.xy, cascadeIndex);

    // Increase the sharpness for higher cascades to match the filter radius
    const float SharpnessScale[MaxNumCascades] = { 1.0f, 1.5f, 3.0f, 3.5f };
    float sharpness = shadow.Sharpness * SharpnessScale[cascadeIndex];

#if CSM_BLENDING
    // Sample the next cascade, and blend between the two results to smooth the transition
    const float BlendThreshold = 0.1f;
    float nextSplit = shadow.CascadeSplits[cascadeIndex];
    float splitSize = cascadeIndex == 0 ? nextSplit : nextSplit - shadow.CascadeSplits[cascadeIndex - 1];
    float splitDist = (nextSplit - viewDepth) / splitSize;
    BRANCH
    if (splitDist <= BlendThreshold && cascadeIndex != shadow.NumCascades - 1)
    {
        // Find the position of this pixel in light space of next cascade
        shadowPosition = mul(float4(worldPosition, 1.0f), shadow.ShadowVP[cascadeIndex + 1]);
        shadowPosition.xy /= shadowPosition.w;

#if FLAX_REVERSE_Z
        // If reverse z is enabled, revert the sign of shadow bias
        shadowPosition.z += shadow.Bias;
#else
        shadowPosition.z -= shadow.Bias;
#endif

        // Sample next cascade and blur result
        float nextSplitShadow = SampleShadowCascade(shadowMap, shadow.ShadowMapSize, shadowPosition.z, shadowPosition.xy, cascadeIndex + 1);
        float lerpAmount = smoothstep(0.0f, BlendThreshold, splitDist);
        lerpAmount = splitDist / BlendThreshold;
        result = lerp(nextSplitShadow, result, lerpAmount);

        // Blur sharpness as well
        sharpness = lerp(shadow.Sharpness * SharpnessScale[cascadeIndex + 1], sharpness, lerpAmount);
    }
#endif

    // Apply shadow fade and sharpness
    result = saturate((result - 0.5) * sharpness + 0.5);
    result = lerp(1.0f, result, (1 - fade) * shadow.Fade);
    return result;
}

// Samples the shadow for the given directional light (cascaded shadow map sampling) for the material surface (supports subsurface shadowing)
float SampleShadow(LightData light, LightShadowData shadow, Texture2DArray shadowMap, GBufferSample gBuffer, out float subsurfaceShadow)
{
    subsurfaceShadow = 1;

    // Create a blend factor which is one before and at the fade plane
    float viewDepth = gBuffer.ViewPos.z;
    float fade = saturate((viewDepth - shadow.CascadeSplits[shadow.NumCascades - 1] + shadow.FadeDistance) / shadow.FadeDistance);
    BRANCH
    if (fade >= 1.0)
    {
        return 1;
    }

    // Figure out which cascade to sample from
    uint cascadeIndex = 0;
    for (uint i = 0; i < shadow.NumCascades - 1; i++)
    {
        if (viewDepth > shadow.CascadeSplits[i])
            cascadeIndex = i + 1;
    }

#if defined(USE_GBUFFER_CUSTOM_DATA)
    // Subsurface shadowing
    BRANCH
    if (IsSubsurfaceMode(gBuffer.ShadingModel))
    {
        // Get subsurface material info
        float opacity = gBuffer.CustomData.a;

        // Project into shadow space
        float4 shadowPosition = mul(float4(gBuffer.WorldPos, 1.0f), shadow.ShadowVP[cascadeIndex]);
        shadowPosition.xy /= shadowPosition.w;

#if FLAX_REVERSE_Z
        // If reverse z is enabled, revert the sign of shadow bias
        shadowPosition.z += shadow.Bias;
#else
        shadowPosition.z -= shadow.Bias;
#endif

        // Sample shadow map (single hardware sample with hardware filtering)
        float shadowMapDepth = shadowMap.SampleLevel(SamplerLinearClamp, float3(shadowPosition.xy, cascadeIndex), 0).r;
        subsurfaceShadow = CalculateSubsurfaceOcclusion(opacity, shadowPosition.z, shadowMapDepth);

        // Apply shadow fade
        subsurfaceShadow = lerp(1.0f, subsurfaceShadow, (1 - fade) * shadow.Fade);
    }
#endif

    float3 samplePosWS = gBuffer.WorldPos;

#if !LIGHTING_NO_DIRECTIONAL
    // Skip if surface is in a full shadow
    float NoL = dot(gBuffer.Normal, light.Direction);
    BRANCH
    if (NoL <= 0)
    {
        return 0;
    }

    // Apply normal offset bias
    samplePosWS += GetShadowPositionOffset(shadow.NormalOffsetScale, NoL, gBuffer.Normal);
#endif

    // Sample shadow
    return SampleShadow(light, shadow, shadowMap, samplePosWS, viewDepth);
}

// Samples the shadow for the given spot light (PCF shadow map sampling)
float SampleShadow(LightData light, LightShadowData shadow, Texture2D shadowMap, float3 worldPosition)
{
    float3 toLight = light.Position - worldPosition;
    float toLightLength = length(toLight);
    float3 L = toLight / toLightLength;
#if LIGHTING_NO_DIRECTIONAL
    float dirCheck = 1.0f;
#else
    float dirCheck = dot(-light.Direction, L);
#endif

    // Skip pixels outside of the light influence
    BRANCH
    if (toLightLength > light.Radius || dirCheck < 0)
    {
        return 1;
    }

    // Negate direction and use normalized value
    toLight = -L;

    // Project into shadow space
    float4 shadowPosition = mul(float4(worldPosition, 1.0f), shadow.ShadowVP[0]);

#if FLAX_REVERSE_Z
    // If reverse z is enabled, revert the sign of shadow bias
    shadowPosition.z += shadow.Bias;
#else
    shadowPosition.z -= shadow.Bias;
#endif

    shadowPosition.xyz /= shadowPosition.w;

    float2 shadowMapUVs = shadowPosition.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f);

#if FilterSizeSpot == 2

    // Use single hardware sample with filtering
#if FLAX_REVERSE_Z
    float result = 1 - shadowMap.SampleCmpLevelZero(ShadowSamplerPCF, shadowMapUVs, shadowPosition.z);
#else
    float result = shadowMap.SampleCmpLevelZero(ShadowSamplerPCF, shadowMapUVs, shadowPosition.z);
#endif

#else

    float3 sideVector = normalize(cross(toLight, float3(0, 0, 1)));
    float3 upVector = cross(sideVector, toLight);

    float shadowMapSizeInv = 1.0f / shadow.ShadowMapSize.x;
    sideVector *= shadowMapSizeInv;
    upVector *= shadowMapSizeInv;

    // Use PCF filter
    float result = 0;
    UNROLL
    for(int i = 0; i < FilterSizeCube; i++)
    {
        float2 samplePos = shadowMapUVs + sideVector.xy * PCFDiscSamples[i].x + upVector.xy * PCFDiscSamples[i].y;
#if FLAX_REVERSE_Z
        result += shadowMap.SampleCmpLevelZero(ShadowSamplerPCF, samplePos, shadowPosition.z);
#else
        result += shadowMap.SampleCmpLevelZero(ShadowSamplerPCF, samplePos, shadowPosition.z);
#endif
    }
    result *= (1.0f / FilterSizeCube);

#endif

    // Apply shadow fade and sharpness
    result = saturate((result - 0.5) * shadow.Sharpness + 0.5);
    result = lerp(1.0f, result, shadow.Fade);

    return result;
}

// Samples the shadow for the given spot light (PCF shadow map sampling) for the material surface (supports subsurface shadowing)
float SampleShadow(LightData light, LightShadowData shadow, Texture2D shadowMap, GBufferSample gBuffer, out float subsurfaceShadow)
{
    subsurfaceShadow = 1;
    float3 toLight = light.Position - gBuffer.WorldPos;
    float toLightLength = length(toLight);
    float3 L = toLight / toLightLength;
#if LIGHTING_NO_DIRECTIONAL
    float dirCheck = 1.0f;
#else
    float dirCheck = dot(-light.Direction, L);
#endif

    // Skip pixels outside of the light influence
    BRANCH
    if (toLightLength > light.Radius || dirCheck < 0)
    {
        return 1;
    }

#if defined(USE_GBUFFER_CUSTOM_DATA)
    // Subsurface shadowing
    BRANCH
    if (IsSubsurfaceMode(gBuffer.ShadingModel))
    {
        // Get subsurface material info
        float opacity = gBuffer.CustomData.a;

        // Project into shadow space
        float4 shadowPosition = mul(float4(gBuffer.WorldPos, 1.0f), shadow.ShadowVP[0]);

#if FLAX_REVERSE_Z
        // If reverse z is enabled, revert the sign of shadow bias
        shadowPosition.z += shadow.Bias;
#else
        shadowPosition.z -= shadow.Bias;
#endif

        shadowPosition.xyz /= shadowPosition.w;

        // Sample shadow map (use single hardware sample with filtering)
        float shadowMapDepth = shadowMap.SampleLevel(SamplerLinearClamp, shadowPosition.xy * float2(0.5f, -0.5f) + float2(0.5f, 0.5f), 0).r;
        subsurfaceShadow = CalculateSubsurfaceOcclusion(opacity, shadowPosition.z, shadowMapDepth);

        // Apply shadow fade
        subsurfaceShadow = lerp(1.0f, subsurfaceShadow, shadow.Fade);
    }
#endif
    
    float3 samplePosWS = gBuffer.WorldPos;

#if !LIGHTING_NO_DIRECTIONAL
    // Skip if surface is in a full shadow
    float NoL = dot(gBuffer.Normal, L);
    BRANCH
    if (NoL <= 0)
    {
        return 0;
    }

    // Apply normal offset bias
    samplePosWS += GetShadowPositionOffset(shadow.NormalOffsetScale, NoL, gBuffer.Normal);
#endif

    // Sample shadow
    return SampleShadow(light, shadow, shadowMap, samplePosWS);
}

// Samples the shadow for the given point light (PCF shadow map sampling)
float SampleShadow(LightData light, LightShadowData shadow, TextureCube<float> shadowMap, float3 worldPosition)
{
    float3 toLight = light.Position - worldPosition;
    float toLightLength = length(toLight);
    float3 L = toLight / toLightLength;

    // Skip pixels outside of the light influence
    BRANCH
    if (toLightLength > light.Radius)
    {
        return 1;
    }

    // Negate direction and use normalized value
    toLight = -L;

    // Figure out which cube face we're sampling from
    int cubeFaceIndex = GetCubeFaceIndex(toLight);

    // Project into shadow space
    float4 shadowPosition = mul(float4(worldPosition, 1.0f), shadow.ShadowVP[cubeFaceIndex]);

#if FLAX_REVERSE_Z
    // If reverse z is enabled, revert the sign of shadow bias
    shadowPosition.z += shadow.Bias;
#else
    shadowPosition.z -= shadow.Bias;
#endif

    shadowPosition.xyz /= shadowPosition.w;

#if FilterSizeCube == 2

    // Use single hardware sample with filtering
#if FLAX_REVERSE_Z
    float result = 1 - shadowMap.SampleCmpLevelZero(ShadowSamplerPCF, toLight, shadowPosition.z);
#else
    float result = shadowMap.SampleCmpLevelZero(ShadowSamplerPCF, toLight, shadowPosition.z);
#endif

#else

    float3 sideVector = normalize(cross(toLight, float3(0, 0, 1)));
    float3 upVector = cross(sideVector, toLight);

    float shadowMapSizeInv = 1.0f / shadow.ShadowMapSize.x;
    sideVector *= shadowMapSizeInv;
    upVector *= shadowMapSizeInv;

    // Use PCF filter
    float result = 0;
    UNROLL
    for (int i = 0; i < FilterSizeCube; i++)
    {
        float3 cubeSamplePos = toLight + sideVector * PCFDiscSamples[i].x + upVector * PCFDiscSamples[i].y;
#if FLAX_REVERSE_Z
        result += 1 - shadowMap.SampleCmpLevelZero(ShadowSamplerPCF, cubeSamplePos, shadowPosition.z);
#else
        result += shadowMap.SampleCmpLevelZero(ShadowSamplerPCF, cubeSamplePos, shadowPosition.z);
#endif
    }
    result *= (1.0f / FilterSizeCube);

#endif

    // Apply shadow fade and sharpness
    result = saturate((result - 0.5) * shadow.Sharpness + 0.5);
    result = lerp(1.0f, result, shadow.Fade);

    return result;
}

// Samples the shadow for the given point light (PCF shadow map sampling) for the material surface (supports subsurface shadowing)
float SampleShadow(LightData light, LightShadowData shadow, TextureCube<float> shadowMap, GBufferSample gBuffer, out float subsurfaceShadow)
{
    subsurfaceShadow = 1;
    float3 toLight = light.Position - gBuffer.WorldPos;
    float toLightLength = length(toLight);
    float3 L = toLight / toLightLength;

    // Skip pixels outside of the light influence
    BRANCH
    if (toLightLength > light.Radius)
    {
        return 1;
    }

    // Negate direction and use normalized value
    toLight = -L;

    // Figure out which cube face we're sampling from
    int cubeFaceIndex = GetCubeFaceIndex(toLight);

#if defined(USE_GBUFFER_CUSTOM_DATA)
    // Subsurface shadowing
    BRANCH
    if (IsSubsurfaceMode(gBuffer.ShadingModel))
    {
        // Get subsurface material info
        float opacity = gBuffer.CustomData.a;

        // Project into shadow space
        float4 shadowPosition = mul(float4(gBuffer.WorldPos, 1.0f), shadow.ShadowVP[cubeFaceIndex]);
        
#if FLAX_REVERSE_Z
        // If reverse z is enabled, revert the sign of shadow bias
        shadowPosition.z += shadow.Bias;
#else
        shadowPosition.z -= shadow.Bias;
#endif

        shadowPosition.xyz /= shadowPosition.w;

        // Sample shadow map (use single hardware sample with filtering)
        float shadowMapDepth = shadowMap.SampleLevel(SamplerLinearClamp, toLight, 0).r;
        subsurfaceShadow = CalculateSubsurfaceOcclusion(opacity, shadowPosition.z, shadowMapDepth);

        // Apply shadow fade
        subsurfaceShadow = lerp(1.0f, subsurfaceShadow, shadow.Fade);
    }
#endif
    
    float3 samplePosWS = gBuffer.WorldPos;

#if !LIGHTING_NO_DIRECTIONAL
    // Skip if surface is in a full shadow
    float NoL = dot(gBuffer.Normal, L);
    BRANCH
    if (NoL <= 0)
    {
        return 0;
    }

    // Apply normal offset bias
    samplePosWS += GetShadowPositionOffset(shadow.NormalOffsetScale, NoL, gBuffer.Normal);
#endif

    // Sample shadow
    return SampleShadow(light, shadow, shadowMap, samplePosWS);
}

#endif
