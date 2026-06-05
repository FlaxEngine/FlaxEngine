// Copyright (c) Wojciech Figat. All rights reserved.

#ifndef __GATHER__
#define __GATHER__

#include "./Flax/Common.hlsl"

float4 TextureGatherRed(Texture2D tex, SamplerState sam, float2 uv)
{
#if CAN_USE_GATHER
	return tex.GatherRed(sam, uv);
#else
    float x = tex.Sample(sam, uv, int2(0, 1)).x;
    float y = tex.Sample(sam, uv, int2(1, 1)).x;
    float z = tex.Sample(sam, uv, int2(1, 0)).x;
    float w = tex.Sample(sam, uv, int2(0, 0)).x;
    return float4(x, y, z, w);
#endif
}

float4 TextureGatherRed(Texture2DArray tex, SamplerState sam, float3 uv)
{
#if CAN_USE_GATHER
	return tex.GatherRed(sam, uv);
#else
    float x = tex.Sample(sam, uv, int2(0, 1)).x;
    float y = tex.Sample(sam, uv, int2(1, 1)).x;
    float z = tex.Sample(sam, uv, int2(1, 0)).x;
    float w = tex.Sample(sam, uv, int2(0, 0)).x;
    return float4(x, y, z, w);
#endif
}

float4 TextureGatherRed(Texture2D tex, SamplerState sam, float2 uv, int2 offset)
{
#if CAN_USE_GATHER
	return tex.GatherRed(sam, uv, offset);
#else
    float x = tex.Sample(sam, uv, offset + int2(0, 1)).x;
    float y = tex.Sample(sam, uv, offset + int2(1, 1)).x;
    float z = tex.Sample(sam, uv, offset + int2(1, 0)).x;
    float w = tex.Sample(sam, uv, offset + int2(0, 0)).x;
    return float4(x, y, z, w);
#endif
}

float4 TextureGatherRed(Texture2D<float> tex, SamplerState sam, float2 uv)
{
#if CAN_USE_GATHER
	return tex.GatherRed(sam, uv);
#else
    float x = tex.Sample(sam, uv, int2(0, 1)).x;
    float y = tex.Sample(sam, uv, int2(1, 1)).x;
    float z = tex.Sample(sam, uv, int2(1, 0)).x;
    float w = tex.Sample(sam, uv, int2(0, 0)).x;
    return float4(x, y, z, w);
#endif
}

float4 TextureGatherRed(Texture2D<float> tex, SamplerState sam, float2 uv, int2 offset)
{
#if CAN_USE_GATHER
	return tex.GatherRed(sam, uv, offset);
#else
    float x = tex.Sample(sam, uv, offset + int2(0, 1)).x;
    float y = tex.Sample(sam, uv, offset + int2(1, 1)).x;
    float z = tex.Sample(sam, uv, offset + int2(1, 0)).x;
    float w = tex.Sample(sam, uv, offset + int2(0, 0)).x;
    return float4(x, y, z, w);
#endif
}

float4 TextureGatherGreen(Texture2D tex, SamplerState sam, float2 uv)
{
#if CAN_USE_GATHER
	return tex.GatherGreen(sam, uv);
#else
    float x = tex.Sample(sam, uv, int2(0, 1)).g;
    float y = tex.Sample(sam, uv, int2(1, 1)).g;
    float z = tex.Sample(sam, uv, int2(1, 0)).g;
    float w = tex.Sample(sam, uv, int2(0, 0)).g;
    return float4(x, y, z, w);
#endif
}

float4 TextureGatherBlue(Texture2D tex, SamplerState sam, float2 uv)
{
#if CAN_USE_GATHER
	return tex.GatherBlue(sam, uv);
#else
    float x = tex.Sample(sam, uv, int2(0, 1)).b;
    float y = tex.Sample(sam, uv, int2(1, 1)).b;
    float z = tex.Sample(sam, uv, int2(1, 0)).b;
    float w = tex.Sample(sam, uv, int2(0, 0)).b;
    return float4(x, y, z, w);
#endif
}

float4 TextureGatherDepth(Texture2D tex, float2 uv)
{
#if defined(WGSL)
    // WebGPU doesn't allow to sample depth texture with regular sampler, need to use Load instead of Sample and get texture size for UV to pixel coordinate conversion
    uint2 size;
    tex.GetDimensions(size.x, size.y);
    uint2 coord = (uint2)((float2)size * uv - 0.5f);
    float x = tex.Load(uint3(coord + uint2(0, 1), 0)).x;
    float y = tex.Load(uint3(coord + uint2(1, 1), 0)).x;
    float z = tex.Load(uint3(coord + uint2(1, 0), 0)).x;
    float w = tex.Load(uint3(coord + uint2(0, 0), 0)).x;
    return float4(x, y, z, w);
#else
    return TextureGatherRed(tex, SamplerPointClamp, uv);
#endif
}

#endif
