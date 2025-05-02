// Copyright (c) Wojciech Figat. All rights reserved.

#define USE_GBUFFER_CUSTOM_DATA

#include "./Flax/Common.hlsl"
#include "./Flax/MaterialCommon.hlsl"
#include "./Flax/LightingCommon.hlsl"
#include "./Flax/IESProfile.hlsl"
#include "./Flax/GBuffer.hlsl"
#include "./Flax/Lighting.hlsl"

// Per light data
META_CB_BEGIN(0, PerLight)
LightData Light;
float4x4 WVP;
META_CB_END

// Per frame data
META_CB_BEGIN(1, PerFrame)
GBufferData GBuffer;
META_CB_END

DECLARE_GBUFFERDATA_ACCESS(GBuffer)

// Rendered shadow
Texture2D Shadow : register(t5);
Texture2D IESTexture : register(t6);
TextureCube CubeImage : register(t7);

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

	return output;
}
