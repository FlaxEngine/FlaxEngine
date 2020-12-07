// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"
#include "./Flax/BRDF.hlsl"
#include "./Flax/Random.hlsl"
#include "./Flax/MonteCarlo.hlsl"
#include "./Flax/LightingCommon.hlsl"
#include "./Flax/GBuffer.hlsl"
#include "./Flax/ReflectionsCommon.hlsl"
#include "./Flax/BRDF.hlsl"

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
float NoTemporalEffect;
float Intensity;
float FadeOutDistance;

float3 Dummy0;
float InvFadeDistance;

float4x4 ViewMatrix;
float4x4 ViewProjectionMatrix;

META_CB_END

DECLARE_GBUFFERDATA_ACCESS(GBuffer)

Texture2D Texture0 : register(t4);
Texture2D Texture1 : register(t5);
Texture2D Texture2 : register(t6);

float max2(float2 v)
{
	return max(v.x, v.y);
}

float2 RandN2(float2 pos, float2 random)
{
	return frac(sin(dot(pos.xy + random, float2(12.9898, 78.233))) * float2(43758.5453, 28001.8384));
}

// 1:-1 to 0:1
float2 ClipToUv(float2 clipPos)
{
	return clipPos * float2(0.5, -0.5) + float2(0.5, 0.5);
}

// go into clip space (-1:1 from bottom/left to up/right)
float3 ProjectWorldToClip(float3 wsPos)
{
	float4 clipPos = mul(float4(wsPos, 1), ViewProjectionMatrix);
	return clipPos.xyz / clipPos.w;
}

// go into UV space. (0:1 from top/left to bottom/right)
float3 ProjectWorldToUv(float3 wsPos)
{
	float3 clipPos = ProjectWorldToClip(wsPos);
	return float3(ClipToUv(clipPos.xy), clipPos.z);
}

float4 TangentToWorld(float3 N, float4 H)
{
	float3 upVector = abs(N.z) < 0.999 ? float3(0.0, 0.0, 1.0) : float3(1.0, 0.0, 0.0);
	float3 T = normalize(cross(upVector, N));
	float3 B = cross(N, T);
	return float4((T * H.x) + (B * H.y) + (N * H.z), H.w);
}

float RayAttenBorder(float2 pos, float value)
{
	float borderDist = min(1.0 - max(pos.x, pos.y), min(pos.x, pos.y));
	return saturate(borderDist > value ? 1.0 : borderDist / value);
}

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
float4 PS_RayTracePass(Quad_VS2PS input) : SV_Target0
{
	float2 uv = input.TexCoord;

	// Sample GBuffer
	GBufferData gBufferData = GetGBufferData();
	GBufferSample gBuffer = SampleGBuffer(gBufferData, uv);

	// Reject invalid pixels
	if (gBuffer.ShadingModel == SHADING_MODEL_UNLIT || gBuffer.Roughness > RoughnessFade || gBuffer.ViewPos.z > FadeOutDistance)
		return 0;

	// Calculate view space normal vector
	float3 normalVS = mul(gBuffer.Normal, (float3x3)ViewMatrix);

	// Randomize it a little
	float2 jitter = RandN2(uv, TemporalTime);
	float2 Xi = jitter;
	Xi.y = lerp(Xi.y, 0.0, BRDFBias);

	float4 H = TangentToWorld(gBuffer.Normal, ImportanceSampleGGX(Xi, gBuffer.Roughness));
	if (NoTemporalEffect)
		H.xyz = gBuffer.Normal;

	// Calculate normalized view space reflection vector
	float3 reflectVS = normalize(reflect(gBuffer.ViewPos, normalVS));
	if (gBuffer.ViewPos.z < 1.0 && reflectVS.z < 0.4)
		return 0;

	float3 viewWS = normalize(gBuffer.WorldPos - GBuffer.ViewPos);
	float3 reflectWS = reflect(viewWS, H.xyz);

	float3 startWS = gBuffer.WorldPos + gBuffer.Normal * WorldAntiSelfOcclusionBias;
	float3 startUV = ProjectWorldToUv(startWS);
	float3 endUV = ProjectWorldToUv(startWS + reflectWS);

	float3 rayUV = endUV - startUV;
	rayUV *= RayTraceStep / max2(abs(rayUV.xy));
	float3 startUv = startUV + rayUV * 2;

	float3 currOffset = startUv;
	float3 rayStep = rayUV * 2;

	// Calculate number of samples
	float3 samplesToEdge = ((sign(rayStep.xyz) * 0.5 + 0.5) - currOffset.xyz) / rayStep.xyz;
	samplesToEdge.x = min(samplesToEdge.x, min(samplesToEdge.y, samplesToEdge.z)) * 1.05f;
	float numSamples = min(MaxTraceSamples, samplesToEdge.x);
	rayStep *= samplesToEdge.x / numSamples;

	// Calculate depth difference error
	float depthDiffError = 1.3f * abs(rayStep.z);

	// Ray trace
	float currSampleIndex = 0;
	float currSample, depthDiff;
	LOOP
	while (currSampleIndex < numSamples)
	{
		// Sample depth buffer and calculate depth difference
		currSample = SampleZ(currOffset.xy);
		depthDiff = currOffset.z - currSample;

		// Check intersection
		if (depthDiff >= 0)
		{
			if (depthDiff < depthDiffError)
			{
				break;
			}
			else
			{
				currOffset -= rayStep;
				rayStep *= 0.5;
			}
		}

		// Move forward
		currOffset += rayStep;
		currSampleIndex++;
	}

	// Check if has valid result after ray tracing
	if (currSampleIndex >= numSamples)
	{
		// All samples done but no result
		return 0;
	}

	float2 hitUV = currOffset.xy;

	// Fade rays close to screen edge
	const float fadeStart = 0.9f;
	const float fadeEnd = 1.0f;
	const float fadeDiffRcp = 1.0f / (fadeEnd - fadeStart);
	float2 boundary = abs(hitUV - float2(0.5f, 0.5f)) * 2.0f;
	float fadeOnBorder = 1.0f - saturate((boundary.x - fadeStart) * fadeDiffRcp);
	fadeOnBorder *= 1.0f - saturate((boundary.y - fadeStart) * fadeDiffRcp);
	fadeOnBorder = smoothstep(0.0f, 1.0f, fadeOnBorder);
	fadeOnBorder *= RayAttenBorder(hitUV, EdgeFadeFactor);

	// Fade rays on high roughness
	float roughnessFade = saturate((RoughnessFade - gBuffer.Roughness) * 20);

	// Fade on distance
	float distanceFade = saturate((FadeOutDistance - gBuffer.ViewPos.z) * InvFadeDistance);

	// Output: xy: hitUV, z: hitMask, w: unused
	return float4(hitUV, fadeOnBorder * roughnessFade * distanceFade * Intensity, 0);
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
	// Texture0 - color buffer (rgb color, with mip maps chain or without)
	// Texture1 - ray trace buffer (xy: hitUV, z: hitMask)

	// Early out for pixels with no hit result
	if (Texture1.SampleLevel(SamplerLinearClamp, uv, 0).z <= 0.001)
		return 0;

	// Sample GBuffer
	GBufferData gBufferData = GetGBufferData();
	GBufferSample gBuffer = SampleGBuffer(gBufferData, uv);

	// Calculate view vector
	float3 viewVector = normalize(gBufferData.ViewPos - gBuffer.WorldPos);

	// Randomize it a little
	float2 random = RandN2(uv, TemporalTime);
	float2 blueNoise = random.xy * 2.0 - 1.0;
	float2x2 offsetRotationMatrix = float2x2(blueNoise.x, blueNoise.y, -blueNoise.y, blueNoise.x);

	float NdotV = saturate(dot(gBuffer.Normal, viewVector));
	float coneTangent = lerp(0.0, gBuffer.Roughness * 5 * (1.0 - BRDFBias), pow(NdotV, 1.5) * sqrt(gBuffer.Roughness));

	// Resolve samples
	float4 result = 0.0;
	UNROLL
	for (int i = 0; i < RESOLVE_SAMPLES; i++)
	{
		float2 offsetUV = Offsets[i] * SSRtexelSize;
		offsetUV =  mul(offsetRotationMatrix, offsetUV);

		// "uv" is the location of the current (or "local") pixel. We want to resolve the local pixel using
		// intersections spawned from neighbouring pixels. The neighbouring pixel is this one:
		float2 neighborUv = uv + offsetUV;

		// Now we fetch the intersection point
		float4 hitPacked = Texture1.SampleLevel(SamplerLinearClamp, neighborUv, 0);
		float2 hitUv = hitPacked.xy;
		float hitMask = hitPacked.z;

		float intersectionCircleRadius = coneTangent * length(hitUv - uv);
		float mip = clamp(log2(intersectionCircleRadius * TraceSizeMax), 0.0, MaxColorMiplevel);

		float3 sampleColor = Texture0.SampleLevel(SamplerLinearClamp, hitUv, mip).rgb;
#if SSR_REDUCE_HIGHLIGHTS
		sampleColor /= 1 + Luminance(sampleColor);
#endif

		result.rgb += sampleColor;
		result.a += hitMask;
	}

	// Calculate final result value
	result /= RESOLVE_SAMPLES;
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
