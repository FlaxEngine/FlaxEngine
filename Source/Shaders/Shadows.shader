// Copyright (c) Wojciech Figat. All rights reserved.

#define USE_GBUFFER_CUSTOM_DATA
#define SHADOWS_CSM_DITHERING 1

#include "./Flax/Common.hlsl"
#include "./Flax/GBuffer.hlsl"
#include "./Flax/MaterialCommon.hlsl"
#include "./Flax/ShadowsSampling.hlsl"

META_CB_BEGIN(0, PerLight)
GBufferData GBuffer;
LightData Light;
float4x4 WVP;
float4x4 ViewProjectionMatrix;
float Dummy0;
float TemporalTime;
float ContactShadowsDistance;
float ContactShadowsLength;
META_CB_END

Buffer<float4> ShadowsBuffer : register(t5);
Texture2D<float> ShadowMap : register(t6);

DECLARE_GBUFFERDATA_ACCESS(GBuffer)

#if CONTACT_SHADOWS

float RayCastScreenSpaceShadow(GBufferData gBufferData, GBufferSample gBuffer, float3 rayStartWS, float3 rayDirWS, float rayLength)
{
#if SHADOWS_QUALITY == 3
	const uint maxSteps = 16;
#elif SHADOWS_QUALITY == 2
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
		float rayDepth = (gBufferData.ViewInfo.w / (rayUV.z - gBufferData.ViewInfo.z)) * gBufferData.ViewFar * 0.998;
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
	// Obtain texture coordinates corresponding to the current pixel
	float2 uv = (input.ScreenPos.xy / input.ScreenPos.w) * float2(0.5, -0.5) + float2(0.5, 0.5);

	// Sample GBuffer
	GBufferData gBufferData = GetGBufferData();
	GBufferSample gBuffer = SampleGBuffer(gBufferData, uv);

	// Sample shadow
    ShadowSample shadow = SamplePointLightShadow(Light, ShadowsBuffer, ShadowMap, gBuffer);

#if CONTACT_SHADOWS && SHADOWS_QUALITY > 0
	// Calculate screen-space contact shadow
	shadow.SurfaceShadow *= RayCastScreenSpaceShadow(gBufferData, gBuffer, gBuffer.WorldPos, normalize(Light.Position - gBuffer.WorldPos), ContactShadowsLength);
#endif

	return GetShadowMask(shadow);
}

// Pixel shader for directional light shadow rendering
META_PS(true, FEATURE_LEVEL_ES2)
META_PERMUTATION_3(SHADOWS_QUALITY=0,CONTACT_SHADOWS=0,SHADOWS_CSM_BLENDING=0)
META_PERMUTATION_3(SHADOWS_QUALITY=1,CONTACT_SHADOWS=0,SHADOWS_CSM_BLENDING=0)
META_PERMUTATION_3(SHADOWS_QUALITY=2,CONTACT_SHADOWS=0,SHADOWS_CSM_BLENDING=0)
META_PERMUTATION_3(SHADOWS_QUALITY=3,CONTACT_SHADOWS=0,SHADOWS_CSM_BLENDING=0)
META_PERMUTATION_3(SHADOWS_QUALITY=0,CONTACT_SHADOWS=1,SHADOWS_CSM_BLENDING=0)
META_PERMUTATION_3(SHADOWS_QUALITY=1,CONTACT_SHADOWS=1,SHADOWS_CSM_BLENDING=0)
META_PERMUTATION_3(SHADOWS_QUALITY=2,CONTACT_SHADOWS=1,SHADOWS_CSM_BLENDING=0)
META_PERMUTATION_3(SHADOWS_QUALITY=3,CONTACT_SHADOWS=1,SHADOWS_CSM_BLENDING=0)
META_PERMUTATION_3(SHADOWS_QUALITY=0,CONTACT_SHADOWS=0,SHADOWS_CSM_BLENDING=1)
META_PERMUTATION_3(SHADOWS_QUALITY=1,CONTACT_SHADOWS=0,SHADOWS_CSM_BLENDING=1)
META_PERMUTATION_3(SHADOWS_QUALITY=2,CONTACT_SHADOWS=0,SHADOWS_CSM_BLENDING=1)
META_PERMUTATION_3(SHADOWS_QUALITY=3,CONTACT_SHADOWS=0,SHADOWS_CSM_BLENDING=1)
META_PERMUTATION_3(SHADOWS_QUALITY=0,CONTACT_SHADOWS=1,SHADOWS_CSM_BLENDING=1)
META_PERMUTATION_3(SHADOWS_QUALITY=1,CONTACT_SHADOWS=1,SHADOWS_CSM_BLENDING=1)
META_PERMUTATION_3(SHADOWS_QUALITY=2,CONTACT_SHADOWS=1,SHADOWS_CSM_BLENDING=1)
META_PERMUTATION_3(SHADOWS_QUALITY=3,CONTACT_SHADOWS=1,SHADOWS_CSM_BLENDING=1)
float4 PS_DirLight(Quad_VS2PS input) : SV_Target0
{
	// Sample GBuffer
	GBufferData gBufferData = GetGBufferData();
	GBufferSample gBuffer = SampleGBuffer(gBufferData, input.TexCoord);

	// Sample shadow
    ShadowSample shadow = SampleDirectionalLightShadow(Light, ShadowsBuffer, ShadowMap, gBuffer, TemporalTime);

#if CONTACT_SHADOWS && SHADOWS_QUALITY > 0
	// Calculate screen-space contact shadow
	shadow.SurfaceShadow *= RayCastScreenSpaceShadow(gBufferData, gBuffer, gBuffer.WorldPos, Light.Direction, ContactShadowsLength);
#endif

	return GetShadowMask(shadow);
}

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
	// Obtain texture coordinates corresponding to the current pixel
	float2 uv = (input.ScreenPos.xy / input.ScreenPos.w) * float2(0.5, -0.5) + float2(0.5, 0.5);

	// Sample GBuffer
	GBufferData gBufferData = GetGBufferData();
	GBufferSample gBuffer = SampleGBuffer(gBufferData, uv);

	// Sample shadow
    ShadowSample shadow = SampleSpotLightShadow(Light, ShadowsBuffer, ShadowMap, gBuffer);

#if CONTACT_SHADOWS && SHADOWS_QUALITY > 0
	// Calculate screen-space contact shadow
	shadow.SurfaceShadow *= RayCastScreenSpaceShadow(gBufferData, gBuffer, gBuffer.WorldPos, normalize(Light.Position - gBuffer.WorldPos), ContactShadowsLength);
#endif

	return GetShadowMask(shadow);
}
