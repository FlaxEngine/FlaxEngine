// Copyright (c) Wojciech Figat. All rights reserved.

@0// Global Illumination: Defines
#define USE_GI 1
@1// Global Illumination: Includes
#include "./Flax/GI/DDGI.hlsl"
#include "./Flax/Lighting/LightingCommon.hlsl"
@2// Global Illumination: Constants
DDGIData DDGI;
@3// Global Illumination: Resources
Texture2D<snorm float4> ProbesData : register(t__SRV__);
Texture2D<float4> ProbesDistance : register(t__SRV__);
Texture2D<float4> ProbesIrradiance : register(t__SRV__);
Texture2D<float4> ProbesRadiance : register(t__SRV__);
@4// Global Illumination: Utilities
float4 GetGlobalIlluminationLighting(GBufferSample gBuffer)
{
    float3 irradiance = SampleDDGIIrradiance(DDGI, ProbesData, ProbesDistance, ProbesIrradiance, gBuffer.WorldPos, gBuffer.Normal);
	float3 diffuseColor = GetDiffuseColor(gBuffer);
	float3 diffuse = Diffuse_Lambert(diffuseColor);
	return float4(diffuse * irradiance, saturate(length(irradiance)));
}

float4 GetGlobalIlluminationSpecular(GBufferSample gBuffer)
{
    float3 specular = SampleDDGISpecular(DDGI, ProbesData, ProbesDistance, ProbesRadiance, gBuffer.WorldPos, gBuffer.Normal, gBuffer.Roughness);
    float useRadiance = DDGI.BlendOrigin[0].w; // 1 or 0
    return float4(specular, useRadiance);
}

@5// Global Illumination: Shaders
