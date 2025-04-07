// Copyright (c) Wojciech Figat. All rights reserved.

#define NO_GBUFFER_SAMPLING

#include "./Flax/Common.hlsl"
#include "./Flax/GBufferCommon.hlsl"
#include "./Flax/GBuffer.hlsl"
#include "./Flax/ExponentialHeightFog.hlsl"

// Disable Volumetric Fog if is not supported
#if VOLUMETRIC_FOG && !CAN_USE_COMPUTE_SHADER
#define VOLUMETRIC_FOG 0
#endif

META_CB_BEGIN(0, Data)
GBufferData GBuffer;
ExponentialHeightFogData ExponentialHeightFog;
META_CB_END

DECLARE_GBUFFERDATA_ACCESS(GBuffer)

Texture2D Depth : register(t0);
#if VOLUMETRIC_FOG
Texture3D IntegratedLightScattering : register(t1);
#endif

// Get world space position at given pixel coordinate
float3 GetWorldPos(float2 uv)
{
	float depth = SAMPLE_RT(Depth, uv).r;
	GBufferData gBufferData = GetGBufferData();
	float3 viewPos = GetViewPos(gBufferData, uv, depth);
	return mul(float4(viewPos, 1), gBufferData.InvViewMatrix).xyz;
}

float4 CalculateCombinedFog(float3 posWS, float sceneDepth, float3 volumeUV)
{
	float skipDistance = 0;

#if VOLUMETRIC_FOG
	skipDistance = max(ExponentialHeightFog.VolumetricFogMaxDistance - 100, 0);
#endif

	float4 fog = GetExponentialHeightFog(ExponentialHeightFog, posWS, GBuffer.ViewPos, skipDistance);

#if VOLUMETRIC_FOG
	float4 volumetricFog = IntegratedLightScattering.SampleLevel(SamplerLinearClamp, volumeUV, 0);
	fog = float4(volumetricFog.rgb + fog.rgb * volumetricFog.a, volumetricFog.a * fog.a);
#endif

	return fog;
}

META_PS(true, FEATURE_LEVEL_ES2)
META_PERMUTATION_1(VOLUMETRIC_FOG=0)
META_PERMUTATION_1(VOLUMETRIC_FOG=1)
float4 PS_Fog(Quad_VS2PS input) : SV_Target0
{
	// Calculate pixel world space position
	float3 posWS = GetWorldPos(input.TexCoord);
	float3 viewVector = posWS - GBuffer.ViewPos;
	float sceneDepth = length(viewVector);

	// Calculate volumetric fog coordinates
	float depthSlice = sceneDepth / ExponentialHeightFog.VolumetricFogMaxDistance;
	float3 volumeUV = float3(input.TexCoord, depthSlice);

	// Debug code
#if VOLUMETRIC_FOG && 0
	volumeUV = posWS / 1000;
	if (!all(volumeUV >= 0 && volumeUV <= 1))
		return 0;

	return float4(IntegratedLightScattering.SampleLevel(SamplerLinearClamp, volumeUV, 0).rgb, 1);
	//return float4(volumeUV, 1);
	//return float4(posWS / 100, 1);
#endif

	// Calculate fog color
	float4 fog = CalculateCombinedFog(posWS, sceneDepth, volumeUV);

	return fog;
}
