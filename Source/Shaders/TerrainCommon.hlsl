// Copyright (c) Wojciech Figat. All rights reserved.

#ifndef __TERRAIN_COMMON__
#define __TERRAIN_COMMON__

#include "./Flax/Common.hlsl"

float DecodeHeightmapHeight(float4 value)
{
    return (float)((int)(value.x * 255.0) + ((int)(value.y * 255) << 8)) / 65535.0;
}

float3 DecodeHeightmapNormal(float4 value, out bool isHole)
{
	isHole = (value.b + value.a) >= 1.9f;
    float2 normalTemp = float2(value.b, value.a) * 2.0f - 1.0f;
    float3 normal = float3(normalTemp.x, sqrt(1.0 - saturate(dot(normalTemp, normalTemp))), normalTemp.y);
    return normalize(normal);
}

float SampleHeightmap(Texture2D<float4> heightmap, float2 uv, float mipOffset = 0.0f)
{
	float4 value = heightmap.SampleLevel(SamplerPointClamp, uv, mipOffset);
    return DecodeHeightmapHeight(value);
}

float SampleHeightmap(Texture2D<float4> heightmap, float2 uv, out float3 normal, out bool isHole, float mipOffset = 0.0f)
{
	float4 value = heightmap.SampleLevel(SamplerPointClamp, uv, mipOffset);
    normal = DecodeHeightmapNormal(value, isHole);
    return DecodeHeightmapHeight(value);
}

float3 SampleHeightmap(Texture2D<float4> heightmap, float3 localPosition, float4 localToUV, out float3 normal, out bool isHole, float mipOffset = 0.0f)
{
    // Sample heightmap
	float2 uv = localPosition.xz * localToUV.xy + localToUV.zw;
	float4 value = heightmap.SampleLevel(SamplerPointClamp, uv, mipOffset);

    // Decode heightmap
    normal = DecodeHeightmapNormal(value, isHole);
	float height = DecodeHeightmapHeight(value);;
	float3 position = float3(localPosition.x, height, localPosition.z);

    // UVs outside the heightmap are empty
    isHole = isHole || any(uv < 0.0f) || any(uv > 1.0f);

    return position;
}

#endif
