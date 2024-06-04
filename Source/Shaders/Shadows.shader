// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#define USE_GBUFFER_CUSTOM_DATA

#include "./Flax/Common.hlsl"
#include "./Flax/GBuffer.hlsl"
#include "./Flax/MaterialCommon.hlsl"
#include "./Flax/ShadowsSampling.hlsl"

META_CB_BEGIN(0, PerLight)
GBufferData GBuffer;
LightData Light;
LightShadowData LightShadow;
float4x4 WVP;
float4x4 ViewProjectionMatrix;
float2 Dummy0;
float ContactShadowsDistance;
float ContactShadowsLength;
META_CB_END

DECLARE_GBUFFERDATA_ACCESS(GBuffer)
DECLARE_LIGHTSHADOWDATA_ACCESS(LightShadow);

#if CONTACT_SHADOWS

float RayCastScreenSpaceShadow(GBufferData gBufferData, GBufferSample gBuffer, float3 rayStartWS, float3 rayDirWS, float rayLength)
{
#if SHADOWS_QUALITY == 3
	const uint maxSteps = 16;
#elif SHADOWS_QUALITY == 3
	const uint maxSteps = 12;
#else
	const uint maxSteps = 8;
#endif
	float distanceFade = 1 - saturate(pow(length(gBuffer.WorldPos - gBufferData.ViewPos) / ContactShadowsDistance, 2));
	float maxShadowLength = gBufferData.InvProjectionMatrix[1][1] * gBuffer.ViewPos.z * rayLength * distanceFade;
	float4 rayStartCS = mul(float4(rayStartWS, 1), ViewProjectionMatrix);
	float4 rayEndCS = mul(float4(rayStartWS + rayDirWS * maxShadowLength, 1), ViewProjectionMatrix);
	float4 rayStepCS = (rayEndCS - rayStartCS) / maxSteps;
	float4 rayCS = rayStartCS + rayStepCS;
	float lightAmountMax = 0;
	for (uint step = 0; step < maxSteps; step++)
	{
		float3 rayUV = rayCS.xyz / rayCS.w;
		rayUV.xy = rayUV.xy * float2(0.5, -0.5) + float2(0.5, 0.5);
		float sceneDepth = SampleDepth(gBufferData, rayUV.xy) * gBufferData.ViewFar;
		float rayDepth = LinearizeZ(gBufferData, rayUV.z) * gBufferData.ViewFar * 0.998;
		float surfaceThickness = 0.035f + rayDepth * rayLength;
		float depthTestHardness = 0.005f;
		float lightAmount = saturate((rayDepth - sceneDepth) / depthTestHardness) * saturate((sceneDepth + surfaceThickness - rayDepth) / depthTestHardness);
		lightAmountMax = max(lightAmountMax, lightAmount);
		rayCS += rayStepCS;
	}
	return 1 - lightAmountMax;
}

#endif

// Vertex Shader for shadow volume model rendering
META_VS(true, FEATURE_LEVEL_ES2)
META_VS_IN_ELEMENT(POSITION, 0, R32G32B32_FLOAT, 0, 0, PER_VERTEX, 0, true)
Model_VS2PS VS_Model(ModelInput_PosOnly input)
{
	Model_VS2PS output;
	output.Position = mul(float4(input.Position.xyz, 1), WVP);
	output.ScreenPos = output.Position;
	return output;
}

#ifdef _PS_PointLight

TextureCube<float> ShadowMapPoint : register(t5);

// Pixel shader for point light shadow rendering
META_PS(true, FEATURE_LEVEL_ES2)
META_PERMUTATION_2(SHADOWS_QUALITY=0,CONTACT_SHADOWS=0)
META_PERMUTATION_2(SHADOWS_QUALITY=1,CONTACT_SHADOWS=0)
META_PERMUTATION_2(SHADOWS_QUALITY=2,CONTACT_SHADOWS=0)
META_PERMUTATION_2(SHADOWS_QUALITY=3,CONTACT_SHADOWS=0)
META_PERMUTATION_2(SHADOWS_QUALITY=0,CONTACT_SHADOWS=1)
META_PERMUTATION_2(SHADOWS_QUALITY=1,CONTACT_SHADOWS=1)
META_PERMUTATION_2(SHADOWS_QUALITY=2,CONTACT_SHADOWS=1)
META_PERMUTATION_2(SHADOWS_QUALITY=3,CONTACT_SHADOWS=1)
float4 PS_PointLight(Model_VS2PS input) : SV_Target0
{
	float shadow = 1;
	float subsurfaceShadow = 1;

	// Obtain texture coordinates corresponding to the current pixel
	float2 uv = (input.ScreenPos.xy / input.ScreenPos.w) * float2(0.5, -0.5) + float2(0.5, 0.5);

	// Sample GBuffer
	GBufferData gBufferData = GetGBufferData();
	GBufferSample gBuffer = SampleGBuffer(gBufferData, uv);

	// Sample shadow
	LightShadowData lightShadowData = GetLightShadowData();
	shadow = SampleShadow(Light, lightShadowData, ShadowMapPoint, gBuffer, subsurfaceShadow);

#if CONTACT_SHADOWS
	// Calculate screen-space contact shadow
	shadow *= RayCastScreenSpaceShadow(gBufferData, gBuffer, gBuffer.WorldPos, normalize(Light.Position - gBuffer.WorldPos), ContactShadowsLength);
#endif

	return float4(shadow, subsurfaceShadow, 1, 1);
}

#endif

#ifdef _PS_DirLight

Texture2DArray ShadowMapDir : register(t5);

// Pixel shader for directional light shadow rendering
META_PS(true, FEATURE_LEVEL_ES2)
META_PERMUTATION_3(SHADOWS_QUALITY=0,CSM_BLENDING=0,CONTACT_SHADOWS=0)
META_PERMUTATION_3(SHADOWS_QUALITY=1,CSM_BLENDING=0,CONTACT_SHADOWS=0)
META_PERMUTATION_3(SHADOWS_QUALITY=2,CSM_BLENDING=0,CONTACT_SHADOWS=0)
META_PERMUTATION_3(SHADOWS_QUALITY=3,CSM_BLENDING=0,CONTACT_SHADOWS=0)
META_PERMUTATION_3(SHADOWS_QUALITY=0,CSM_BLENDING=1,CONTACT_SHADOWS=0)
META_PERMUTATION_3(SHADOWS_QUALITY=1,CSM_BLENDING=1,CONTACT_SHADOWS=0)
META_PERMUTATION_3(SHADOWS_QUALITY=2,CSM_BLENDING=1,CONTACT_SHADOWS=0)
META_PERMUTATION_3(SHADOWS_QUALITY=3,CSM_BLENDING=1,CONTACT_SHADOWS=0)
META_PERMUTATION_3(SHADOWS_QUALITY=0,CSM_BLENDING=0,CONTACT_SHADOWS=1)
META_PERMUTATION_3(SHADOWS_QUALITY=1,CSM_BLENDING=0,CONTACT_SHADOWS=1)
META_PERMUTATION_3(SHADOWS_QUALITY=2,CSM_BLENDING=0,CONTACT_SHADOWS=1)
META_PERMUTATION_3(SHADOWS_QUALITY=3,CSM_BLENDING=0,CONTACT_SHADOWS=1)
META_PERMUTATION_3(SHADOWS_QUALITY=0,CSM_BLENDING=1,CONTACT_SHADOWS=1)
META_PERMUTATION_3(SHADOWS_QUALITY=1,CSM_BLENDING=1,CONTACT_SHADOWS=1)
META_PERMUTATION_3(SHADOWS_QUALITY=2,CSM_BLENDING=1,CONTACT_SHADOWS=1)
META_PERMUTATION_3(SHADOWS_QUALITY=3,CSM_BLENDING=1,CONTACT_SHADOWS=1)
float4 PS_DirLight(Quad_VS2PS input) : SV_Target0
{
	float shadow = 1;
	float subsurfaceShadow = 1;

	// Sample GBuffer
	GBufferData gBufferData = GetGBufferData();
	GBufferSample gBuffer = SampleGBuffer(gBufferData, input.TexCoord);

	// Sample shadow
	LightShadowData lightShadowData = GetLightShadowData();
	shadow = SampleShadow(Light, lightShadowData, ShadowMapDir, gBuffer, subsurfaceShadow);

#if CONTACT_SHADOWS
	// Calculate screen-space contact shadow
	shadow *= RayCastScreenSpaceShadow(gBufferData, gBuffer, gBuffer.WorldPos, Light.Direction, ContactShadowsLength);
#endif

	return float4(shadow, subsurfaceShadow, 1, 1);
}

#endif

#ifdef _PS_SpotLight

Texture2D ShadowMapSpot : register(t5);

// Pixel shader for spot light shadow rendering
META_PS(true, FEATURE_LEVEL_ES2)
META_PERMUTATION_2(SHADOWS_QUALITY=0,CONTACT_SHADOWS=0)
META_PERMUTATION_2(SHADOWS_QUALITY=1,CONTACT_SHADOWS=0)
META_PERMUTATION_2(SHADOWS_QUALITY=2,CONTACT_SHADOWS=0)
META_PERMUTATION_2(SHADOWS_QUALITY=3,CONTACT_SHADOWS=0)
META_PERMUTATION_2(SHADOWS_QUALITY=0,CONTACT_SHADOWS=1)
META_PERMUTATION_2(SHADOWS_QUALITY=1,CONTACT_SHADOWS=1)
META_PERMUTATION_2(SHADOWS_QUALITY=2,CONTACT_SHADOWS=1)
META_PERMUTATION_2(SHADOWS_QUALITY=3,CONTACT_SHADOWS=1)
float4 PS_SpotLight(Model_VS2PS input) : SV_Target0
{
	float shadow = 1;
	float subsurfaceShadow = 1;

	// Obtain texture coordinates corresponding to the current pixel
	float2 uv = (input.ScreenPos.xy / input.ScreenPos.w) * float2(0.5, -0.5) + float2(0.5, 0.5);

	// Sample GBuffer
	GBufferData gBufferData = GetGBufferData();
	GBufferSample gBuffer = SampleGBuffer(gBufferData, uv);

	// Sample shadow
	LightShadowData lightShadowData = GetLightShadowData();
	shadow = SampleShadow(Light, lightShadowData, ShadowMapSpot, gBuffer, subsurfaceShadow);

#if CONTACT_SHADOWS
	// Calculate screen-space contact shadow
	shadow *= RayCastScreenSpaceShadow(gBufferData, gBuffer, gBuffer.WorldPos, normalize(Light.Position - gBuffer.WorldPos), ContactShadowsLength);
#endif

	return float4(shadow, subsurfaceShadow, 1, 1);
}

#endif
