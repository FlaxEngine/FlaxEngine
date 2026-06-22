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
uint FrameIndexMod8;
float TemporalTime;
float ContactShadowsDistance;
float ContactShadowsLength;
META_CB_END

Buffer<float4> ShadowsBuffer : register(t5);
Texture2D<float> ShadowMap : register(t6);

DECLARE_GBUFFERDATA_ACCESS(GBuffer)

#if CONTACT_SHADOWS

#include "./Flax/Noise.hlsl"

float RayCastScreenSpaceShadow(GBufferData gBufferData, GBufferSample gBuffer, float3 rayStartWS, float3 rayDirWS, float rayLength, float dither = 0.5f)
{
    uint2 depthSize;
    Depth.GetDimensions(depthSize.x, depthSize.y);
#if SHADOWS_QUALITY == 3
    const uint maxSteps = 16;
#elif SHADOWS_QUALITY == 2
    const uint maxSteps = 12;
#else
    const uint maxSteps = 8;
#endif

    // Build start and end points of the trace
    float distanceFade = 1 - saturate(pow(length(gBuffer.WorldPos - gBufferData.ViewPos) / ContactShadowsDistance, 2));
    float maxShadowLength = gBufferData.InvProjectionMatrix[1][1] * gBuffer.ViewPos.z * rayLength * distanceFade;
    float4 rayStartCS = PROJECT_POINT(float4(rayStartWS, 1), ViewProjectionMatrix);
    float4 rayDirCS = PROJECT_POINT(float4(rayDirWS * maxShadowLength, 0), ViewProjectionMatrix);
    float4 rayEndCS = rayStartCS + rayDirCS;
    float3 rayStart = rayStartCS.xyz / rayStartCS.w;
    float3 rayEnd = rayEndCS.xyz / rayEndCS.w;

    // Use initial ray step that isn't sub-pixel to avoid in low-resolution
    float3 raySize = rayEnd - rayStart;
    float2 rayStepDst = abs(raySize).xy * depthSize;
    float rayStepDstMin = min(rayStepDst.x, rayStepDst.y);
    float3 rayStepMin = raySize / max(min(maxSteps, rayStepDstMin), 1);
    float3 rayStep = raySize / maxSteps;
    float3 ray = rayStart + rayStepMin * (dither * 2 + 1.0f);

    // Sample over the ray
    float lightAmountMax = 0;
    for (uint step = 0; step < maxSteps; step++)
    {
        float2 rayUV = ProjectClipToUV(ray.xy);
        float sceneDepth = SampleDepth(gBufferData, rayUV) * gBufferData.ViewFar;
        float rayDepth = LinearizeZ(gBufferData, ray.z) * gBufferData.ViewFar * 0.998;
        float surfaceThickness = 0.035f + rayDepth * rayLength;
        float depthTestHardness = 0.005f;
        float lightAmount = saturate((rayDepth - sceneDepth) / depthTestHardness) * saturate((sceneDepth + surfaceThickness - rayDepth) / depthTestHardness);
        lightAmountMax = max(lightAmountMax, lightAmount);
        ray += rayStep;
    }

    return 1 - lightAmountMax * distanceFade;
}

#endif

// Vertex Shader for shadow volume model rendering
META_VS(true, FEATURE_LEVEL_ES2)
META_VS_IN_ELEMENT(POSITION, 0, R32G32B32_FLOAT, 0, 0, PER_VERTEX, 0, true)
Model_VS2PS VS_Model(ModelInput_PosOnly input)
{
	Model_VS2PS output;
	output.Position = PROJECT_POINT(float4(input.Position.xyz, 1), WVP);
	output.ScreenPos = output.Position;
	return output;
}

// Pixel shader for directional light shadow rendering
META_PS(true, FEATURE_LEVEL_ES2)
META_PERMUTATION_3(SHADOWS_QUALITY=0,CONTACT_SHADOWS=0,SHADOWS_CSM_BLENDING=0)
META_PERMUTATION_3(SHADOWS_QUALITY=1,CONTACT_SHADOWS=0,SHADOWS_CSM_BLENDING=0)
META_PERMUTATION_3(SHADOWS_QUALITY=2,CONTACT_SHADOWS=0,SHADOWS_CSM_BLENDING=0)
META_PERMUTATION_3(SHADOWS_QUALITY=3,CONTACT_SHADOWS=0,SHADOWS_CSM_BLENDING=0)
META_PERMUTATION_3(SHADOWS_QUALITY=0,CONTACT_SHADOWS=0,SHADOWS_CSM_BLENDING=0)
META_PERMUTATION_3(SHADOWS_QUALITY=1,CONTACT_SHADOWS=1,SHADOWS_CSM_BLENDING=0)
META_PERMUTATION_3(SHADOWS_QUALITY=2,CONTACT_SHADOWS=1,SHADOWS_CSM_BLENDING=0)
META_PERMUTATION_3(SHADOWS_QUALITY=3,CONTACT_SHADOWS=1,SHADOWS_CSM_BLENDING=0)
META_PERMUTATION_3(SHADOWS_QUALITY=0,CONTACT_SHADOWS=0,SHADOWS_CSM_BLENDING=1)
META_PERMUTATION_3(SHADOWS_QUALITY=1,CONTACT_SHADOWS=0,SHADOWS_CSM_BLENDING=1)
META_PERMUTATION_3(SHADOWS_QUALITY=2,CONTACT_SHADOWS=0,SHADOWS_CSM_BLENDING=1)
META_PERMUTATION_3(SHADOWS_QUALITY=3,CONTACT_SHADOWS=0,SHADOWS_CSM_BLENDING=1)
META_PERMUTATION_3(SHADOWS_QUALITY=0,CONTACT_SHADOWS=0,SHADOWS_CSM_BLENDING=1)
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

#if CONTACT_SHADOWS
    // Calculate screen-space contact shadow
    float dither = InterleavedGradientNoise(input.Position.xy, FrameIndexMod8);
    float contactShadow = RayCastScreenSpaceShadow(gBufferData, gBuffer, gBuffer.WorldPos, Light.Direction, ContactShadowsLength, dither);
    shadow.SurfaceShadow = min(shadow.SurfaceShadow, contactShadow);
#endif

	return GetShadowMask(shadow);
}

// Pixel shader for point/spot light shadow rendering
META_PS(true, FEATURE_LEVEL_ES2)
META_PERMUTATION_3(SHADOWS_QUALITY=0,CONTACT_SHADOWS=0,LIGHT_TYPE=0) // Point light
META_PERMUTATION_3(SHADOWS_QUALITY=1,CONTACT_SHADOWS=0,LIGHT_TYPE=0)
META_PERMUTATION_3(SHADOWS_QUALITY=2,CONTACT_SHADOWS=0,LIGHT_TYPE=0)
META_PERMUTATION_3(SHADOWS_QUALITY=3,CONTACT_SHADOWS=0,LIGHT_TYPE=0)
META_PERMUTATION_3(SHADOWS_QUALITY=0,CONTACT_SHADOWS=0,LIGHT_TYPE=0)
META_PERMUTATION_3(SHADOWS_QUALITY=1,CONTACT_SHADOWS=1,LIGHT_TYPE=0)
META_PERMUTATION_3(SHADOWS_QUALITY=2,CONTACT_SHADOWS=1,LIGHT_TYPE=0)
META_PERMUTATION_3(SHADOWS_QUALITY=3,CONTACT_SHADOWS=1,LIGHT_TYPE=0)
META_PERMUTATION_3(SHADOWS_QUALITY=0,CONTACT_SHADOWS=0,LIGHT_TYPE=1) // Spot light
META_PERMUTATION_3(SHADOWS_QUALITY=1,CONTACT_SHADOWS=0,LIGHT_TYPE=1)
META_PERMUTATION_3(SHADOWS_QUALITY=2,CONTACT_SHADOWS=0,LIGHT_TYPE=1)
META_PERMUTATION_3(SHADOWS_QUALITY=3,CONTACT_SHADOWS=0,LIGHT_TYPE=1)
META_PERMUTATION_3(SHADOWS_QUALITY=0,CONTACT_SHADOWS=0,LIGHT_TYPE=1)
META_PERMUTATION_3(SHADOWS_QUALITY=1,CONTACT_SHADOWS=1,LIGHT_TYPE=1)
META_PERMUTATION_3(SHADOWS_QUALITY=2,CONTACT_SHADOWS=1,LIGHT_TYPE=1)
META_PERMUTATION_3(SHADOWS_QUALITY=3,CONTACT_SHADOWS=1,LIGHT_TYPE=1)
float4 PS_LocalLight(Model_VS2PS input) : SV_Target0
{
	// Sample GBuffer
	float2 uv = ProjectClipToUV(input.ScreenPos.xy / input.ScreenPos.w);
	GBufferData gBufferData = GetGBufferData();
	GBufferSample gBuffer = SampleGBuffer(gBufferData, uv);

	// Sample shadow
#if LIGHT_TYPE == 0
    ShadowSample shadow = SamplePointLightShadow(Light, ShadowsBuffer, ShadowMap, gBuffer);
#elif LIGHT_TYPE == 1
    ShadowSample shadow = SampleSpotLightShadow(Light, ShadowsBuffer, ShadowMap, gBuffer);
#else
    ShadowSample shadow = (ShadowSample)0;
#endif

#if CONTACT_SHADOWS
    // Calculate screen-space contact shadow
    float dither = InterleavedGradientNoise(input.ScreenPos.xy, FrameIndexMod8);
    float contactShadow = RayCastScreenSpaceShadow(gBufferData, gBuffer, gBuffer.WorldPos, normalize(Light.Position - gBuffer.WorldPos), ContactShadowsLength, dither);
    shadow.SurfaceShadow = min(shadow.SurfaceShadow, contactShadow);
#endif

	return GetShadowMask(shadow);
}
