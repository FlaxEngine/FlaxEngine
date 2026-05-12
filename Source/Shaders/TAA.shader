// Copyright (c) Wojciech Figat. All rights reserved.

// Implementation based on:
// "Temporal Reprojection Anti-Aliasing in INSIDE", Lasse Jon Fuglsang Pedersen at GDC 2026
// Source: https://github.com/playdeadgames/temporal, MIT Licence, 2015 Playdead

// Configs
#define UNJITTER_INPUT 1
#define UNJITTER_NEIGHBORHOOD 0
#define UNJITTER_VELOCITY_DEPTH 0
#define MINMAX_3X3 1
//#define MINMAX_3X3_ROUNDED 1
//#define MINMAX_4TAP_VARYING 1
#define DEBUG_LUMINANCE_DIFF 0
#define DEBUG_MOTION 0
#define DEBUG_VELOCITY_REJECTION 0

#define NO_GBUFFER_SAMPLING
#define NEED_DEPTH_VELOCITY (MINMAX_4TAP_VARYING)
#if NEED_DEPTH_VELOCITY
#define VelocityDepth float3
#else
#define VelocityDepth float2
#endif

#include "./Flax/Common.hlsl"
#include "./Flax/GBuffer.hlsl"
#include "./Flax/Noise.hlsl"
#include "./Flax/Temporal.hlsl"

META_CB_BEGIN(0, Data)
float2 ScreenSizeInv;
float2 JitterInv;
float2 MotionScale;
float StationaryBlending;
float MotionBlending;
float3 QuantizationError;
float Sharpness;
GBufferData GBuffer;
META_CB_END

Texture2D Input : register(t0);
Texture2D InputHistory : register(t1);
Texture2D MotionVectors : register(t2);
Texture2D Depth : register(t3);

// Samples nearby pixels and returns (uv, raw depth) of the closest one to camera.
float3 FindClosestDepth3x3(float2 uv)
{
    float2 du = float2(ScreenSizeInv.x, 0.0);
    float2 dv = float2(0.0, ScreenSizeInv.y);

    float3 dtl = float3(-1, -1, SAMPLE_RT_DEPTH(Depth, uv - dv - du));
    float3 dtc = float3( 0, -1, SAMPLE_RT_DEPTH(Depth, uv - dv));
    float3 dtr = float3( 1, -1, SAMPLE_RT_DEPTH(Depth, uv - dv + du));

    float3 dml = float3(-1, 0, SAMPLE_RT_DEPTH(Depth, uv - du));
    float3 dmc = float3( 0, 0, SAMPLE_RT_DEPTH(Depth, uv).x);
    float3 dmr = float3( 1, 0, SAMPLE_RT_DEPTH(Depth, uv + du));

    float3 dbl = float3(-1, 1, SAMPLE_RT_DEPTH(Depth, uv + dv - du));
    float3 dbc = float3( 0, 1, SAMPLE_RT_DEPTH(Depth, uv + dv));
    float3 dbr = float3( 1, 1, SAMPLE_RT_DEPTH(Depth, uv + dv + du));

    float3 dmin = dtl;
#if REVERSE_Z
    #define FIND_MIN(a, b) a < b
#else
    #define FIND_MIN(a, b) a > b
#endif
    if (FIND_MIN(dmin.z, dtc.z)) dmin = dtc;
    if (FIND_MIN(dmin.z, dtr.z)) dmin = dtr;

    if (FIND_MIN(dmin.z, dml.z)) dmin = dml;
    if (FIND_MIN(dmin.z, dmc.z)) dmin = dmc;
    if (FIND_MIN(dmin.z, dmr.z)) dmin = dmr;

    if (FIND_MIN(dmin.z, dbl.z)) dmin = dbl;
    if (FIND_MIN(dmin.z, dbc.z)) dmin = dbc;
    if (FIND_MIN(dmin.z, dbr.z)) dmin = dbr;
#undef FIND_MIN

    return float3(uv + ScreenSizeInv.xy * dmin.xy, dmin.z);
}

// Samples nearby pixels and returns (velocity, uv) of the pixel with largest velocity.
float4 FindLargestVelocity(float2 uv, int range)
{
    float4 result = float4(0, 0, uv.x, uv.y);
    float vLargest = 0.0f;
    for (int y = -range; y <= range; y++)
    {
        for (int x = -range; x <= range; x++)
        {
            float2 vUV = uv + ScreenSizeInv * float2(x, y);
            float2 v = SAMPLE_RT_LINEAR(MotionVectors, vUV).xy;
            float vSize = dot(v, v);
            if (vSize > vLargest)
            {
                result = float4(v, vUV);
                vLargest = vSize;
            }
        }
    }
    return result;
}

VelocityDepth SampleVelocityDepth(float2 uv)
{
#if UNJITTER_VELOCITY_DEPTH
    uv -= JitterInv.xy;
#endif
    VelocityDepth velocityDepth;

#if QUALITY >= 2

    // 3x3 search for the largest velocity
    float4 largestVelocity = FindLargestVelocity(uv, 3);
    velocityDepth.xy = largestVelocity.xy;
#if NEED_DEPTH_VELOCITY
    velocityDepth.z = SAMPLE_RT_DEPTH(Depth, largestVelocity.zw);
#endif

#elif QUALITY >= 1

    // 3x3 search of the closest depth
    float3 closestDepth = FindClosestDepth3x3(uv);
    velocityDepth.xy = SAMPLE_RT_LINEAR(MotionVectors, closestDepth.xy).xy;
#if NEED_DEPTH_VELOCITY
    velocityDepth.z = closestDepth.z;
#endif

#else // QUALITY == 0

    // Current sample
    velocityDepth.xy = SAMPLE_RT_LINEAR(MotionVectors, uv).xy;
#if NEED_DEPTH_VELOCITY
    velocityDepth.z = SAMPLE_RT_DEPTH(Depth, uv);
#endif

#endif

#if NEED_DEPTH_VELOCITY
    // Linearize depth
    velocityDepth.z = LinearizeZ(GBuffer, velocityDepth.z);
#endif

    return velocityDepth;
}

// Pixel Shader for Temporal Anti-Aliasing
META_PS(true, FEATURE_LEVEL_ES2)
META_PERMUTATION_1(QUALITY=0)
META_PERMUTATION_1(QUALITY=1)
META_PERMUTATION_1(QUALITY=2)
float4 PS(Quad_VS2PS input) : SV_Target0
{
    // Sample velocity and depth
    VelocityDepth velocityDepth = SampleVelocityDepth(input.TexCoord);

    // Sample image and history
#if UNJITTER_INPUT
    float4 current = SAMPLE_RT(Input, input.TexCoord - JitterInv.xy);
#else
    float4 current = SAMPLE_RT(Input, input.TexCoord);
#endif
    float4 history = SAMPLE_RT_LINEAR(InputHistory, input.TexCoord - velocityDepth.xy);

    // Calculate min-max of current pixel neighbourhood
#if UNJITTER_NEIGHBORHOOD
    float2 uv = input.TexCoord - JitterInv.xy;
#else
    float2 uv = input.TexCoord;
#endif
#if MINMAX_3X3 || MINMAX_3X3_ROUNDED
    float2 du = float2(ScreenSizeInv.x, 0.0);
    float2 dv = float2(0.0, ScreenSizeInv.y);

    float4 ctl = SAMPLE_RT(Input, uv - dv - du);
    float4 ctc = SAMPLE_RT(Input, uv - dv);
    float4 ctr = SAMPLE_RT(Input, uv - dv + du);
    float4 cml = SAMPLE_RT(Input, uv - du);
    float4 cmc = SAMPLE_RT(Input, uv);
    float4 cmr = SAMPLE_RT(Input, uv + du);
    float4 cbl = SAMPLE_RT(Input, uv + dv - du);
    float4 cbc = SAMPLE_RT(Input, uv + dv);
    float4 cbr = SAMPLE_RT(Input, uv + dv + du);

    float4 cMin = min(ctl, min(ctc, min(ctr, min(cml, min(cmc, min(cmr, min(cbl, min(cbc, cbr))))))));
    float4 cMax = max(ctl, max(ctc, max(ctr, max(cml, max(cmc, max(cmr, max(cbl, max(cbc, cbr))))))));
    float4 cAvg = (ctl + ctc + ctr + cml + cmc + cmr + cbl + cbc + cbr) / 9.0f;
    float4 corners = 4.0f * (ctl + cbr) - 2.0f * current;

#if MINMAX_3X3_ROUNDED
    float4 cMin5 = min(ctc, min(cml, min(cmc, min(cmr, cbc))));
    float4 cMax5 = max(ctc, max(cml, max(cmc, max(cmr, cbc))));
    float4 cAvg5 = (ctc + cml + cmc + cmr + cbc) / 5.0;
    cMin = 0.5 * (cMin + cMin5);
    cMax = 0.5 * (cMax + cMax5);
    cAvg = 0.5 * (cAvg + cAvg5);
#endif
#elif MINMAX_4TAP_VARYING
    const float SubpixelThreshold = 0.5;
    const float GatherBase = 0.5;
    const float GatherSubpixelMotion = 0.1666;

    float velocityMagnitude = length(velocityDepth.xy / ScreenSizeInv.xy) * velocityDepth.z;
    float subpixelMotion = saturate(SubpixelThreshold / (velocityMagnitude + TAA_EPSILON));
    float minMaxSupport = GatherBase + GatherSubpixelMotion * subpixelMotion;

    float2 ssOffset01 = minMaxSupport * float2(-ScreenSizeInv.x, ScreenSizeInv.y);
    float2 ssOffset11 = minMaxSupport * float2(ScreenSizeInv.x, ScreenSizeInv.y);
    float4 c00 = SAMPLE_RT_LINEAR(Input, uv - ssOffset11);
    float4 c10 = SAMPLE_RT_LINEAR(Input, uv - ssOffset01);
    float4 c01 = SAMPLE_RT_LINEAR(Input, uv + ssOffset01);
    float4 c11 = SAMPLE_RT_LINEAR(Input, uv + ssOffset11);
    float4 corners = 4.0f * (c00 + c11) - 2.0f * current;

    float4 cMin = min(c00, min(c10, min(c01, c11)));
    float4 cMax = max(c00, max(c10, max(c01, c11)));
    float4 cAvg = (c00 + c10 + c01 + c11) / 4.0f;
#endif

    // Apply sharpening
    current += (current - (corners * 0.166667)) * Sharpness;
    current = clamp(current, 0, HDR_CLAMP_MAX);

    // Clamp current sample to neighbourhood pixels nearby colors to reduce ghosting artifacts
    history = ClipAAB(cMin.xyz, cMax.xyz, clamp(cAvg, cMin, cMax), history);
    //history = clamp(history, cMin, cMax);

    // Calculate history weight from unbiased luminance diff
    float historyBlend = TemporalHistoryWeight(current, history, MotionBlending);
#if DEBUG_LUMINANCE_DIFF
    return historyBlend.xxxx;
#endif

    // Higher history blend when there is no motion
    float motion = saturate(length(velocityDepth.xy) * 1000.0f);
#if DEBUG_MOTION
    return motion.xxxx;
#endif
    historyBlend = lerp(StationaryBlending, historyBlend, motion);

    // Lower history blend when motion goes outside the view
    const float velocityMin = 2.0f;
    const float velocityMax = 20.0f;
    const float velocityRange = velocityMax - velocityMin;
    float velocityRejection = clamp(length(velocityDepth.xy * MotionScale) - velocityMin, 0.0, velocityRange) / velocityRange;
#if DEBUG_VELOCITY_REJECTION
    return velocityRejection.xxxx;
#endif
    historyBlend = lerp(historyBlend, 0.0f, velocityRejection);

    // Blend current frame with history
    float4 color = lerp(current, history, historyBlend);

    // Apply quantization error to reduce yellowish artifacts due to R11G11B10 format
    float noise = rand2dTo1d(input.TexCoord);
    color.rgb = QuantizeColor(color.rgb, noise, QuantizationError);

    return color;
}
