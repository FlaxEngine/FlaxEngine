// Copyright (c) Wojciech Figat. All rights reserved.

#ifndef __GUI_COMMON__
#define __GUI_COMMON__

#include "./Flax/Common.hlsl"

#define CLIPPING_ENABLE 1

struct Render2DVertex
{
    float2 Position : POSITION0;
    float2 TexCoord : TEXCOORD0;
    float4 Color : COLOR0;
    float4 CustomDataAndClipOrigin : TEXCOORD1; // x-per-geometry type, y-features mask, zw-clip origin
    float4 ClipExtents : TEXCOORD2;
};

struct VS2PS
{
    float4 Position : SV_Position;
    float4 Color : COLOR0;
    float2 TexCoord : TEXCOORD0;
    float2 CustomData : TEXCOORD1;
    float4 ClipExtents : TEXCOORD2;
    float4 ClipOriginAndPos : TEXCOORD3;
};

float cross2(float2 a, float2 b)
{
    return a.x * b.y - a.y * b.x;
}

// Given a point p and a parallelogram defined by point a and vectors b and c, determines in p is inside the parallelogram. Returns a 4-vector that can be used with the clip instruction.
float4 PointInParallelogram(float2 p, float2 a, float4 bc)
{
    float2 o = p - a;
    float invD = 1 / cross2(bc.xy, bc.zw);
    float2 t = (o.x * bc.yw - o.y * bc.xz) * float2(-invD, invD);
    return float4(t, float2(1, 1) - t);
}

void PerformClipping(float2 clipOrigin, float2 windowPos, float4 clipExtents)
{
#if CLIPPING_ENABLE
    // Clip pixels which are outside the clipping rect
    float4 clipTest = PointInParallelogram(windowPos, clipOrigin, clipExtents);
    clip(clipTest);
#endif
}

void PerformClipping(VS2PS input)
{
    PerformClipping(input.ClipOriginAndPos.xy, input.ClipOriginAndPos.zw, input.ClipExtents);
}

float SampleFont(Texture2D font, float2 uv)
{
    return font.Sample(SamplerLinearClamp, uv).r;
}

float GetFontMSDFMedian(Texture2D font, float2 uv)
{
    float4 msd = font.Sample(SamplerLinearClamp, uv);
    return max(min(msd.r, msd.g), min(max(msd.r, msd.g), msd.b));
}

float GetFontMSDFMedian(float4 msd)
{
    return max(min(msd.r, msd.g), min(max(msd.r, msd.g), msd.b));
}

float GetFontMSDFPixelRange(Texture2D font, float2 uv)
{
    uint width, height;
    font.GetDimensions(width, height);
    float pxRange = 4.0f; // Must match C++ code
    float unitRange = float2(pxRange, pxRange) / float2(width, height);

    float2 dx = ddx(uv);
    float2 dy = ddy(uv);
    float2 screenTexSize = rsqrt(dx * dx + dy * dy);
    return max(0.5f * dot(screenTexSize, unitRange), 1.0f);
}

float SampleFontMSDF(Texture2D font, float2 uv)
{
    float sd = GetFontMSDFMedian(font, uv);
    float screenPxRange = GetFontMSDFPixelRange(font, uv);
    float screenPxDist = screenPxRange * (sd - 0.5f);
    return saturate(screenPxDist + 0.5f);
}

float SampleFontMSDFOutline(Texture2D font, float2 uv, float thickness)
{
    float4 msd = font.Sample(SamplerLinearClamp, uv);
    float sd = max(min(msd.r, msd.g), min(max(msd.r, msd.g), msd.b));
    float screenPxRange = GetFontMSDFPixelRange(font, uv);
    float thick = clamp(thickness, 0.0, screenPxRange * 0.5 - 1.0) / screenPxRange;
    float outline = saturate((min(sd, msd.a) - 0.5 + thick) * screenPxRange + 0.5);
    outline *= 1 - saturate(screenPxRange * (sd - 0.5f) + 0.5f);
    return outline;
}

#endif
