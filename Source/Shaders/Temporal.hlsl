// Copyright (c) Wojciech Figat. All rights reserved.

#ifndef __TEMPORAL__
#define __TEMPORAL__

#include "./Flax/Common.hlsl"

#define TAA_EPSILON 0.000001f

// Calculates history weight from unbiased luminance diff
// [Timothy Lottes, 2011, "TSSAA (Temporal Super-Sampling AA)"]
float TemporalHistoryWeight(float4 current, float4 history, float motionBlending = 0.85f)
{
    float currentLum = Luminance(current.rgb);
    float historyLum = Luminance(history.rgb);
    float unbiasedDiff = abs(currentLum - historyLum) / max(currentLum, max(historyLum, 0.2f));
    float unbiasedWeight = 1.0 - unbiasedDiff;
    float unbiasedWeightSqr = unbiasedWeight * unbiasedWeight;
    float historyBlend = lerp(motionBlending, min(motionBlending + 0.2f, 0.97f), unbiasedWeightSqr);
#if defined(DEBUG_LUMINANCE_DIFF) && DEBUG_LUMINANCE_DIFF
    return unbiasedWeightSqr;
#endif
    return historyBlend;
}

// Clamps sample color to neighbourhood pixels bounds
// [Pedersen, 2016, "Temporal Reprojection Anti-Aliasing in INSIDE"]
float4 ClipToAABB(float4 color, float4 minimum, float4 maximum)
{
	float4 center = (maximum + minimum) * 0.5;
	float4 extents = (maximum - minimum) * 0.5;
	float4 shift = color - center;
    float4 absUnit = abs(shift / max(extents, 0.0001));
    float maxUnit = max(max(absUnit.x, absUnit.y), absUnit.z);
	return maxUnit > 1.0 ? center + (shift / maxUnit) : color;
}

float4 ClipAAB(float3 aabbMin, float3 aabbMax, float4 p, float4 q)
{
    // only clips towards aabb center
    float3 pClip = 0.5 * (aabbMax + aabbMin);
    float3 eClip = 0.5 * (aabbMax - aabbMin) + TAA_EPSILON;

    float4 vClip = q - float4(pClip, p.w);
    float3 vUnit = vClip.xyz / eClip;
    float3 aUnit = abs(vUnit);
    float maUnit = max(aUnit.x, max(aUnit.y, aUnit.z));

    if (maUnit > 1.0)
        return float4(pClip, p.w) + vClip / maUnit;
    else
        return q; // point inside aabb
}

#endif
