// Copyright (c) Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"
#include "./Flax/LightingCommon.hlsl"
#include "./Flax/ReflectionsCommon.hlsl"
#include "./Flax/SSR.hlsl"
#include "./Flax/GBuffer.hlsl"
#include "./Flax/GlobalSignDistanceField.hlsl"
#include "./Flax/GI/GlobalSurfaceAtlas.hlsl"

// Enable/disable luminance filter to reduce reflections highlights
#define SSR_REDUCE_HIGHLIGHTS 1

// Enable/disable blurring SSR during sampling results and mixing with reflections buffer
#define SSR_MIX_BLUR 1

META_CB_BEGIN(0, Data)

GBufferData GBuffer;

float MaxColorMiplevel;
float TraceSizeMax;
float MaxTraceSamples;
float RoughnessFade;

float2 SSRtexelSize;
float TemporalTime;
float BRDFBias;

float WorldAntiSelfOcclusionBias;
float EdgeFadeFactor;
float TemporalResponse;
float TemporalScale;

float RayTraceStep;
float TemporalEffect;
float Intensity;
float FadeOutDistance;

float4x4 ViewMatrix;
float4x4 ViewProjectionMatrix;

GlobalSDFData GlobalSDF;
GlobalSurfaceAtlasData GlobalSurfaceAtlas;

META_CB_END

DECLARE_GBUFFERDATA_ACCESS(GBuffer)

Texture2D Texture0 : register(t4);
Texture2D Texture1 : register(t5);
Texture2D Texture2 : register(t6);
#if USE_GLOBAL_SURFACE_ATLAS
Texture3D<snorm float> GlobalSDFTex : register(t7);
Texture3D<snorm float> GlobalSDFMip : register(t8);
ByteAddressBuffer GlobalSurfaceAtlasChunks : register(t9);
ByteAddressBuffer RWGlobalSurfaceAtlasCulledObjects : register(t10);
Buffer<float4> GlobalSurfaceAtlasObjects : register(t11);
Texture2D GlobalSurfaceAtlasDepth : register(t12);
Texture2D GlobalSurfaceAtlasTex : register(t13);
#endif

// Pixel Shader for screen space reflections rendering - combine pass
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_CombinePass(Quad_VS2PS input) : SV_Target0
{
	// Inputs:
	// Texture0 - light buffer
	// Texture1 - reflections buffer
	// Texture2 - PreIntegratedGF

	// Sample GBuffer and light buffer
	GBufferData gBufferData = GetGBufferData();
	GBufferSample gBuffer = SampleGBuffer(gBufferData, input.TexCoord);
	float4 light = SAMPLE_RT(Texture0, input.TexCoord);

	// Check if can light pixel
	BRANCH
	if (gBuffer.ShadingModel != SHADING_MODEL_UNLIT)
	{
		// Sample reflections buffer
		float3 reflections = SAMPLE_RT(Texture1, input.TexCoord).rgb;

		// Calculate specular color
		float3 specularColor = GetSpecularColor(gBuffer);
		if (gBuffer.Metalness < 0.001)
			specularColor = 0.04f * gBuffer.Specular;

		// Calculate reflection color
		float3 V = normalize(gBufferData.ViewPos - gBuffer.WorldPos);
		float NoV = saturate(dot(gBuffer.Normal, V));
		light.rgb += reflections * EnvBRDF(Texture2, specularColor, gBuffer.Roughness, NoV) * gBuffer.AO;
	}

	return light;
}

// Pixel Shader for screen space reflections rendering - ray trace pass
META_PS(true, FEATURE_LEVEL_ES2)
META_PERMUTATION_1(USE_GLOBAL_SURFACE_ATLAS=0)
META_PERMUTATION_1(USE_GLOBAL_SURFACE_ATLAS=1)
float4 PS_RayTracePass(Quad_VS2PS input) : SV_Target0
{
	// Inputs:
	// Texture0 - color buffer (rgb color, with mip maps chain or without)
    // SRV 7-8 Global SDF
    // SRV 9-13 Global Surface Atlas

    // Base layer color with reflections from probes but empty alpha so SSR blur will have valid bacground values to smooth with
    float4 base = float4(Texture0.SampleLevel(SamplerLinearClamp, input.TexCoord, 0).rgb, 0);

	// Sample GBuffer
	GBufferData gBufferData = GetGBufferData();
	GBufferSample gBuffer = SampleGBuffer(gBufferData, input.TexCoord);

    // Reject invalid pixels
    if (gBuffer.ShadingModel == SHADING_MODEL_UNLIT || gBuffer.Roughness > RoughnessFade || gBuffer.ViewPos.z > FadeOutDistance)
        return base;

	// Trace depth buffer to find intersection
	float3 screenHit = TraceScreenSpaceReflection(input.TexCoord, gBuffer, Depth, gBufferData.ViewPos, ViewMatrix, ViewProjectionMatrix, RayTraceStep, MaxTraceSamples, TemporalEffect, TemporalTime, WorldAntiSelfOcclusionBias, BRDFBias, FadeOutDistance, RoughnessFade, EdgeFadeFactor);
	float4 result = base;
	if (screenHit.z > 0)
	{
	    float3 viewVector = normalize(gBufferData.ViewPos - gBuffer.WorldPos);
        float NdotV = saturate(dot(gBuffer.Normal, viewVector));
        float coneTangent = lerp(0.0, gBuffer.Roughness * 5 * (1.0 - BRDFBias), pow(NdotV, 1.5) * sqrt(gBuffer.Roughness));
        float intersectionCircleRadius = coneTangent * length(screenHit.xy - input.TexCoord);
        float mip = clamp(log2(intersectionCircleRadius * TraceSizeMax), 0.0, MaxColorMiplevel);
        float3 sampleColor = Texture0.SampleLevel(SamplerLinearClamp, screenHit.xy, mip).rgb;
        result = float4(sampleColor, screenHit.z);
        if (screenHit.z >= REFLECTIONS_HIT_THRESHOLD)
            return result;
	}

    // Fallback to Global SDF and Global Surface Atlas tracing
#if USE_GLOBAL_SURFACE_ATLAS && CAN_USE_GLOBAL_SURFACE_ATLAS
	// Calculate reflection direction (the same TraceScreenSpaceReflection)
    float3 reflectWS = ScreenSpaceReflectionDirection(input.TexCoord, gBuffer, gBufferData.ViewPos, TemporalEffect, TemporalTime, BRDFBias);

    GlobalSDFTrace sdfTrace;
    float maxDistance = GLOBAL_SDF_WORLD_SIZE;
    sdfTrace.Init(gBuffer.WorldPos, reflectWS, 0.0f, maxDistance);
    GlobalSDFHit sdfHit = RayTraceGlobalSDF(GlobalSDF, GlobalSDFTex, GlobalSDFMip, sdfTrace, 2.0f);
    if (sdfHit.IsHit())
    {
        float3 hitPosition = sdfHit.GetHitPosition(sdfTrace);
        float surfaceThreshold = GetGlobalSurfaceAtlasThreshold(GlobalSDF, sdfHit);
        float4 surfaceAtlas = SampleGlobalSurfaceAtlas(GlobalSurfaceAtlas, GlobalSurfaceAtlasChunks, RWGlobalSurfaceAtlasCulledObjects, GlobalSurfaceAtlasObjects, GlobalSurfaceAtlasDepth, GlobalSurfaceAtlasTex, hitPosition, -reflectWS, surfaceThreshold);
        result = lerp(surfaceAtlas, float4(result.rgb, 1), result.a);
    }
#endif

	return result;
}

#ifndef RESOLVE_SAMPLES
#define RESOLVE_SAMPLES 1
#endif

// Pixel Shader for screen space reflections rendering - resolve pass
META_PS(true, FEATURE_LEVEL_ES2)
META_PERMUTATION_1(RESOLVE_SAMPLES=1)
META_PERMUTATION_1(RESOLVE_SAMPLES=2)
META_PERMUTATION_1(RESOLVE_SAMPLES=4)
META_PERMUTATION_1(RESOLVE_SAMPLES=8)
float4 PS_ResolvePass(Quad_VS2PS input) : SV_Target0
{
	static const float2 Offsets[8] =
	{
		float2( 0,  0),
		float2( 2, -2),
		float2(-2, -2),
		float2( 0,  2),
		float2(-2,  0),
		float2( 0, -2),
		float2( 2,  0),
		float2( 2,  2),
	};

	float2 uv = input.TexCoord;

	// Inputs:
	// Texture0 - ray trace buffer (xy: HDR color, z: weight)

	// Sample GBuffer
	GBufferData gBufferData = GetGBufferData();
	GBufferSample gBuffer = SampleGBuffer(gBufferData, uv);
    if (gBuffer.ShadingModel == SHADING_MODEL_UNLIT)
        return 0;

	// Randomize it a little
	float2 random = RandN2(uv + TemporalTime);
	float2 blueNoise = random.xy * 2.0 - 1.0;
	float2x2 offsetRotationMatrix = float2x2(blueNoise.x, blueNoise.y, -blueNoise.y, blueNoise.x);

	// Resolve samples
	float4 result = 0.0;
	UNROLL
	for (int i = 0; i < RESOLVE_SAMPLES; i++)
	{
		float2 offsetUV = Offsets[i] * SSRtexelSize;
		offsetUV =  mul(offsetRotationMatrix, offsetUV);
		float4 value = Texture0.SampleLevel(SamplerLinearClamp, uv + offsetUV, 0);
#if SSR_REDUCE_HIGHLIGHTS
		value.rgb /= 1 + Luminance(value.rgb);
#endif
		result += value;
	}

	// Calculate final result value
	result /= RESOLVE_SAMPLES;
	result.a *= Intensity;
#if SSR_REDUCE_HIGHLIGHTS
	result.rgb /= 1 - Luminance(result.rgb);
#endif

	//return float4(abs(Luminance(result) - Luminance(saturate(result))).xxx, 1);
	//return saturate(result);
	return result;
}

// Pixel Shader for screen space reflections rendering - temporal pass
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_TemporalPass(Quad_VS2PS input) : SV_Target0
{
	// Inputs:
	// Texture0 - resolved SSR reflections buffer
	// Texture1 - prev frame temporal SSR buffer
	// Texture2 - motion vectors

	float2 uv = input.TexCoord;

	// Sample velocity
	float2 velocity = Texture2.SampleLevel(SamplerLinearClamp, uv, 0).xy;

	// Prepare
	float2 prevUV = uv - velocity;
	float4 current = Texture0.SampleLevel(SamplerLinearClamp, uv, 0);
	float4 previous = Texture1.SampleLevel(SamplerLinearClamp, prevUV, 0);
	float2 du = float2(SSRtexelSize.x, 0.0);
	float2 dv = float2(0.0, SSRtexelSize.y);

	// Sample pixels around
	float4 currentTopLeft = Texture0.SampleLevel(SamplerLinearClamp, uv.xy - dv - du, 0);
	float4 currentTopCenter = Texture0.SampleLevel(SamplerLinearClamp, uv.xy - dv, 0);
	float4 currentTopRight = Texture0.SampleLevel(SamplerLinearClamp, uv.xy - dv + du, 0);
	float4 currentMiddleLeft = Texture0.SampleLevel(SamplerLinearClamp, uv.xy - du, 0);
	float4 currentMiddleCenter = Texture0.SampleLevel(SamplerLinearClamp, uv.xy, 0);
	float4 currentMiddleRight = Texture0.SampleLevel(SamplerLinearClamp, uv.xy + du, 0);
	float4 currentBottomLeft = Texture0.SampleLevel(SamplerLinearClamp, uv.xy + dv - du, 0);
	float4 currentBottomCenter = Texture0.SampleLevel(SamplerLinearClamp, uv.xy + dv, 0);
	float4 currentBottomRight = Texture0.SampleLevel(SamplerLinearClamp, uv.xy + dv + du, 0);

	float4 currentMin = min(currentTopLeft, min(currentTopCenter, min(currentTopRight, min(currentMiddleLeft, min(currentMiddleCenter, min(currentMiddleRight, min(currentBottomLeft, min(currentBottomCenter, currentBottomRight))))))));
	float4 currentMax = max(currentTopLeft, max(currentTopCenter, max(currentTopRight, max(currentMiddleLeft, max(currentMiddleCenter, max(currentMiddleRight, max(currentBottomLeft, max(currentBottomCenter, currentBottomRight))))))));

	float scale = TemporalScale;

	float4 center = (currentMin + currentMax) * 0.5f;
	currentMin = (currentMin - center) * scale + center;
	currentMax = (currentMax - center) * scale + center;

	previous = clamp(previous, currentMin, currentMax);
	current = clamp(current, 0, HDR_CLAMP_MAX);

	float response = TemporalResponse * (1 - length(velocity) * 8);
	return lerp(current, previous, saturate(response));
}

// Pixel Shader for screen space reflections rendering - mix pass
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_MixPass(Quad_VS2PS input) : SV_Target0
{
	// Inputs:
	// Texture0 - final SSR reflections buffer

	float4 ssr = Texture0.SampleLevel(SamplerLinearClamp, input.TexCoord, 0);

#if SSR_MIX_BLUR

	ssr += Texture0.SampleLevel(SamplerLinearClamp, input.TexCoord + float2(0, SSRtexelSize.y), 0);
	ssr += Texture0.SampleLevel(SamplerLinearClamp, input.TexCoord - float2(0, SSRtexelSize.y), 0);
	ssr += Texture0.SampleLevel(SamplerLinearClamp, input.TexCoord + float2(SSRtexelSize.x, 0), 0);
	ssr += Texture0.SampleLevel(SamplerLinearClamp, input.TexCoord - float2(SSRtexelSize.x, 0), 0);
	ssr *= (1.0f / 5.0f);

#endif

	return ssr;
}
