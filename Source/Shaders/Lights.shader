// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#define USE_GBUFFER_CUSTOM_DATA
#define SDF_SKYLIGHT_SHADOW_RAYS_COUNT 40

#include "./Flax/Common.hlsl"
#include "./Flax/MaterialCommon.hlsl"
#include "./Flax/LightingCommon.hlsl"
#include "./Flax/IESProfile.hlsl"
#include "./Flax/GBuffer.hlsl"
#include "./Flax/Lighting.hlsl"
#include "./Flax/GlobalSignDistanceField.hlsl"
#include "./Flax/MonteCarlo.hlsl"
#include "./Flax/Random.hlsl"

// Per light data
META_CB_BEGIN(0, PerLight)
LightData Light;
float4x4 WVP;
META_CB_END

// Per frame data
META_CB_BEGIN(1, PerFrame)
GBufferData GBuffer;
GlobalSDFData GlobalSDF;
META_CB_END


DECLARE_GBUFFERDATA_ACCESS(GBuffer)

// Rendered shadow
Texture2D Shadow : register(t5);
Texture2D IESTexture : register(t6);
TextureCube CubeImage : register(t7);

#if USE_SDF_SKYLIGHT_SHADOWS
Texture3D<float> GlobalSDFTex : register(t8);
Texture3D<float> GlobalSDFMip : register(t9);
#endif

// Vertex Shader for models rendering
META_VS(true, FEATURE_LEVEL_ES2)
META_VS_IN_ELEMENT(POSITION, 0, R32G32B32_FLOAT, 0, 0, PER_VERTEX, 0, true)
Model_VS2PS VS_Model(ModelInput_PosOnly input)
{
	Model_VS2PS output;
	output.Position = mul(float4(input.Position.xyz, 1), WVP);
	output.ScreenPos = output.Position;
	return output;
}

// Pixel shader for directional light rendering
META_PS(true, FEATURE_LEVEL_ES2)
META_PERMUTATION_1(LIGHTING_NO_SPECULAR=0)
META_PERMUTATION_1(LIGHTING_NO_SPECULAR=1)
void PS_Directional(Quad_VS2PS input, out float4 output : SV_Target0)
{
	output = 0;

	// Sample GBuffer
	GBufferData gBufferData = GetGBufferData();
	GBufferSample gBuffer = SampleGBuffer(gBufferData, input.TexCoord);

	// Check if cannot shadow pixel
	BRANCH
	if (gBuffer.ShadingModel == SHADING_MODEL_UNLIT)
	{
		discard;
		return;
	}

	// Sample shadow mask
	float4 shadowMask = 1;
	if (Light.ShadowsBufferAddress != 0)
	{
		shadowMask = SAMPLE_RT(Shadow, input.TexCoord);
	}

	// Calculate lighting
	output = GetLighting(gBufferData.ViewPos, Light, gBuffer, shadowMask, false, false);
}

// Pixel shader for point light rendering
META_PS(true, FEATURE_LEVEL_ES2)
META_PERMUTATION_2(LIGHTING_NO_SPECULAR=0, USE_IES_PROFILE=0)
META_PERMUTATION_2(LIGHTING_NO_SPECULAR=1, USE_IES_PROFILE=0)
META_PERMUTATION_2(LIGHTING_NO_SPECULAR=0, USE_IES_PROFILE=1)
META_PERMUTATION_2(LIGHTING_NO_SPECULAR=1, USE_IES_PROFILE=1)
void PS_Point(Model_VS2PS input, out float4 output : SV_Target0)
{
	output = 0;

	// Obtain UVs corresponding to the current pixel
	float2 uv = (input.ScreenPos.xy / input.ScreenPos.w) * float2(0.5, -0.5) + float2(0.5, 0.5);

	// Sample GBuffer
	GBufferData gBufferData = GetGBufferData();
	GBufferSample gBuffer = SampleGBuffer(gBufferData, uv);

	// Check if cannot shadow pixel
	BRANCH
	if (gBuffer.ShadingModel == SHADING_MODEL_UNLIT)
	{
		discard;
		return;
	}

	// Sample shadow mask
	float4 shadowMask = 1;
	if (Light.ShadowsBufferAddress != 0)
	{
		shadowMask = SAMPLE_RT(Shadow, uv);
	}

	// Calculate lighting
	output = GetLighting(gBufferData.ViewPos, Light, gBuffer, shadowMask, true, false);

	// Apply IES texture
#if USE_IES_PROFILE
	output *= ComputeLightProfileMultiplier(IESTexture, gBuffer.WorldPos, Light.Position, -Light.Direction);
#endif
}

// Pixel shader for spot light rendering
META_PS(true, FEATURE_LEVEL_ES2)
META_PERMUTATION_2(LIGHTING_NO_SPECULAR=0, USE_IES_PROFILE=0)
META_PERMUTATION_2(LIGHTING_NO_SPECULAR=1, USE_IES_PROFILE=0)
META_PERMUTATION_2(LIGHTING_NO_SPECULAR=0, USE_IES_PROFILE=1)
META_PERMUTATION_2(LIGHTING_NO_SPECULAR=1, USE_IES_PROFILE=1)
void PS_Spot(Model_VS2PS input, out float4 output : SV_Target0)
{
	output = 0;

	// Obtain UVs corresponding to the current pixel
	float2 uv = (input.ScreenPos.xy / input.ScreenPos.w) * float2(0.5, -0.5) + float2(0.5, 0.5);

	// Sample GBuffer
	GBufferData gBufferData = GetGBufferData();
	GBufferSample gBuffer = SampleGBuffer(gBufferData, uv);

	// Check if cannot shadow pixel
	BRANCH
	if (gBuffer.ShadingModel == SHADING_MODEL_UNLIT)
	{
		discard;
		return;
	}

	// Sample shadow mask
	float4 shadowMask = 1;
	if (Light.ShadowsBufferAddress != 0)
	{
		shadowMask = SAMPLE_RT(Shadow, uv);
	}

	// Calculate lighting
	output = GetLighting(gBufferData.ViewPos, Light, gBuffer, shadowMask, true, true);

	// Apply IES texture
#if USE_IES_PROFILE
	output *= ComputeLightProfileMultiplier(IESTexture, gBuffer.WorldPos, Light.Position, -Light.Direction);
#endif
}

// Pixel shader for sky light rendering
META_PS(true, FEATURE_LEVEL_ES2)
META_PERMUTATION_1(USE_SDF_SKYLIGHT_SHADOWS = 0)
META_PERMUTATION_1(USE_SDF_SKYLIGHT_SHADOWS = 1)
float4 PS_Sky(Model_VS2PS input) : SV_Target0
{
	float4 output = 0;

	// Obtain UVs corresponding to the current pixel
	float2 uv = (input.ScreenPos.xy / input.ScreenPos.w) * float2(0.5, -0.5) + float2(0.5, 0.5);

	// Sample GBuffer
	GBufferData gBufferData = GetGBufferData();
	GBufferSample gBuffer = SampleGBuffer(gBufferData, uv);

	// Check if can light pixel
	if (gBuffer.ShadingModel != SHADING_MODEL_UNLIT)
	{
		output = GetSkyLightLighting(Light, gBuffer, CubeImage);
	}

	// SDF soft-shadows implementation reference: https://iquilezles.org/articles/rmshadows/
#if USE_SDF_SKYLIGHT_SHADOWS
	// Matrix used to sample directions from the hemisphere around the normal
	float3x3 rotMat = MakeRotationMatrix(gBuffer.Normal);
	float maxDistance = 100;
	float selfOcclusionBias = GlobalSDF.CascadeVoxelSize[0];
	float totalVis = 0;
	for (int i = 0; i < SDF_SKYLIGHT_SHADOW_RAYS_COUNT; i++){
#if SDF_SKYLIGHT_SHADOW_RAYS_COUNT <= 1
		// Trace along the normal
		float3 dir = gBuffer.Normal;
#else
		// Trace along a random direction on the hemisphere
		float3 dir = mul(CosineSampleHemisphere(RandN2(uv)).xyz, rotMat);
#endif
		GlobalSDFTrace sdfTrace;
		sdfTrace.Init(gBuffer.WorldPos + gBuffer.Normal * selfOcclusionBias, dir, 0.0f, maxDistance);
		GlobalSDFHit sdfHit = RayTraceGlobalSDF(GlobalSDF, GlobalSDFTex, GlobalSDFMip, sdfTrace);
		// Assume that the gbuffer normal is normalized, so that hit time is hit distance
		float hitDistance = sdfHit.HitTime >= 0 ? sdfHit.HitTime : maxDistance;
		totalVis += hitDistance / maxDistance;
	}
    
	// TODO: use a AO factor to replace hardcoded `3`
	output *= min(3 * totalVis / SDF_SKYLIGHT_SHADOW_RAYS_COUNT, 1);
#endif

	return output;
}
