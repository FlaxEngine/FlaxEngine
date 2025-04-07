// Copyright (c) Wojciech Figat. All rights reserved.

@0// Global Illumination: Defines
#define USE_GI 1
@1// Global Illumination: Includes
#include "./Flax/GI/DDGI.hlsl"
#include "./Flax/LightingCommon.hlsl"
@2// Global Illumination: Constants
DDGIData DDGI;
@3// Global Illumination: Resources
Texture2D<snorm float4> ProbesState : register(t__SRV__);
Texture2D<float4> ProbesDistance : register(t__SRV__);
Texture2D<float4> ProbesIrradiance : register(t__SRV__);
@4// Global Illumination: Utilities
float4 GetGlobalIlluminationLighting(GBufferSample gBuffer)
{
	float3 irradiance = SampleDDGIIrradiance(DDGI, ProbesState, ProbesDistance, ProbesIrradiance, gBuffer.WorldPos, gBuffer.Normal);
	float3 diffuseColor = GetDiffuseColor(gBuffer);
	float3 diffuse = Diffuse_Lambert(diffuseColor);
	return float4(diffuse * irradiance, saturate(length(irradiance)));
}

@5// Global Illumination: Shaders
