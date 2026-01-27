// Copyright (c) Wojciech Figat. All rights reserved.

#ifndef __VOLUMETRIC_FOG__
#define __VOLUMETRIC_FOG__

#define VOLUMETRIC_FOG_GRID_Z_LINEAR 1

float GetDepthFromSlice(float3 gridSliceParameters, float zSlice)
{
#if VOLUMETRIC_FOG_GRID_Z_LINEAR
	return zSlice * gridSliceParameters.x;
#else
    return (exp2(zSlice / gridSliceParameters.z) - gridSliceParameters.y) / gridSliceParameters.x;
#endif
}

float GetSliceFromDepth(float3 gridSliceParameters, float sceneDepth)
{
#if VOLUMETRIC_FOG_GRID_Z_LINEAR
    return sceneDepth * gridSliceParameters.y;
#else
	return (log2(sceneDepth * gridSliceParameters.x + gridSliceParameters.y) * gridSliceParameters.z);
#endif
}

float4 SampleVolumetricFog(Texture3D volumetricFogTexture, float3 viewVector, float maxDistance, float2 uv)
{
    float sceneDepth = length(viewVector);
    float zSlice = sceneDepth / maxDistance;
    // TODO: use GetSliceFromDepth instead to handle non-linear depth distributions
    float3 volumeUV = float3(uv, zSlice);
    return volumetricFogTexture.SampleLevel(SamplerLinearClamp, volumeUV, 0);
}

float4 CombineVolumetricFog(float4 fog, float4 volumetricFog)
{
    return float4(volumetricFog.rgb + fog.rgb * volumetricFog.a, volumetricFog.a * fog.a);
}

#endif
