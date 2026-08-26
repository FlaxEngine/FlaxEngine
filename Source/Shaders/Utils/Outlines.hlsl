// Copyright (c) Wojciech Figat. All rights reserved.

#ifndef __OUTLINES__
#define __OUTLINES__

#include "./Flax/Common.hlsl"

float DepthOutlineHelper(Texture2D depthBuffer, float baseDepth, float2 uv, float2 offset, float2 depthSizeInv)
{
    float neighborDepth = SAMPLE_RT_DEPTH(depthBuffer, uv + offset * depthSizeInv);
    return DEPTH_DIFF(baseDepth, neighborDepth) * 1000.0f;
}

float DepthOutline(Texture2D depthBuffer, float2 uv)
{
    float baseDepth = SAMPLE_RT_DEPTH(depthBuffer, uv);
    float2 depthSize;
    depthBuffer.GetDimensions(depthSize.x, depthSize.y);
    float2 depthSizeInv = 1.0f / depthSize;
    float outline = DepthOutlineHelper(depthBuffer, baseDepth, uv, float2(1, 0), depthSizeInv);
    outline += DepthOutlineHelper(depthBuffer, baseDepth, uv, float2(0, 1), depthSizeInv);
    outline += DepthOutlineHelper(depthBuffer, baseDepth, uv, float2(-1, 0), depthSizeInv);
    outline += DepthOutlineHelper(depthBuffer, baseDepth, uv, float2(0, -1), depthSizeInv);
    outline = 1 - saturate(outline);
    return outline;
}

#endif
