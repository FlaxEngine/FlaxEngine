// Copyright (c) Wojciech Figat. All rights reserved.

#ifndef __VOLUMETRIC_FOG__
#define __VOLUMETRIC_FOG__

#include "./Flax/Noise.hlsl"

#define VOLUMETRIC_FOG_GRID_Z_LINEAR 1

// Structure that contains information about volumetric fog
struct VolumetricFogData
{
    float4 GridSliceParameters;
    float2 ScreenSize;
    float2 VolumeTexelSize; // Scaled for dithering
};

float GetDepthFromSlice(float4 gridSliceParameters, float zSlice)
{
#if VOLUMETRIC_FOG_GRID_Z_LINEAR
	return zSlice * gridSliceParameters.x;
#else
    return (exp2(zSlice / gridSliceParameters.z) - gridSliceParameters.y) / gridSliceParameters.x;
#endif
}

float GetSliceFromDepth(float4 gridSliceParameters, float sceneDepth)
{
#if VOLUMETRIC_FOG_GRID_Z_LINEAR
    return sceneDepth * gridSliceParameters.y;
#else
	return (log2(sceneDepth * gridSliceParameters.x + gridSliceParameters.y) * gridSliceParameters.z);
#endif
}

float4 SampleVolumetricFog(Texture3D volumetricFogTexture, VolumetricFogData volumetricFogData, float3 viewVector, float2 uv, float4 temporalAAJitter = 0)
{
    // Project view vector to get 3D frustum UVW coordinates
    float sceneDepth = length(viewVector);
    float zSlice = GetSliceFromDepth(volumetricFogData.GridSliceParameters, sceneDepth) * volumetricFogData.GridSliceParameters.w;
    float3 volumeUV = float3(uv, zSlice);

    // Dither to reduce banding artifacts
    float2 noiseUV = volumeUV.xy + temporalAAJitter.xy;
    float2 noise = rand2dTo2d(noiseUV * volumetricFogData.ScreenSize) * 2.0f - 1.0f;
    volumeUV.xy += noise * volumetricFogData.VolumeTexelSize;

    // Sample 3D texture
    return volumetricFogTexture.SampleLevel(SamplerLinearClamp, volumeUV, 0);
}

float4 CombineVolumetricFog(float4 fog, float4 volumetricFog)
{
    return float4(volumetricFog.rgb + fog.rgb * volumetricFog.a, volumetricFog.a * fog.a);
}

#endif
