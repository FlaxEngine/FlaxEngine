// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"
#include "./Flax/GammaCorrectionCommon.hlsl"

#define USE_TONEMAP 0

META_CB_BEGIN(0, Data)
float4 ScreenSize;// x-width, y-height, z-1/width, w-1/height
float4 TaaJitterStrength; // x, y, x/width, y/height
float4 FinalBlendParameters; // x: static, y: dynamic, z: motion amplification, w; sharpness
META_CB_END

Texture2D Input : register(t0);
Texture2D InputHistory : register(t1);
Texture2D MotionVectors : register(t2);
Texture2D Depth : register(t3);

float3 Fetch(Texture2D tex, float2 coords)
{
	return tex.SampleLevel(SamplerLinearClamp, coords, 0).rgb;
}

float3 Map(float3 x)
{
#if USE_TONEMAP
	return FastTonemap(x);
#else
	return x;
#endif
}

float3 Unmap(float3 x)
{
#if USE_TONEMAP
	return FastTonemapInvert(x);
#else
	return x;
#endif
}

float3 ClipToAABB(float3 color, float3 minimum, float3 maximum)
{
	// Note: only clips towards aabb center (but fast!)
	float3 center  = 0.5 * (maximum + minimum);
	float3 extents = 0.5 * (maximum - minimum);

	// This is actually `distance`, however the keyword is reserved
	float3 offset = color - center;

	float3 ts = abs(extents / max(offset, 0.0001));
	float t = saturate(min(min(ts.x, ts.y),  ts.z));
	return center + offset * t;
}

float2 GetClosestFragment(float2 uv)
{
	const float2 k = ScreenSize.zw;

	const float4 neighborhood = float4(
		SAMPLE_RT(Depth, uv - k).r,
		SAMPLE_RT(Depth, uv + float2(k.x, -k.y)).r,
		SAMPLE_RT(Depth, uv + float2(-k.x, k.y)).r,
		SAMPLE_RT(Depth, uv + k).r
	);

#if defined(REVERSED_Z)
	#define COMPARE_DEPTH(a, b) step(b, a)
#else
	#define COMPARE_DEPTH(a, b) step(a, b)
#endif

	float3 result = float3(0.0, 0.0, SAMPLE_RT(Depth, uv).r);
	result = lerp(result, float3(-1.0, -1.0, neighborhood.x), COMPARE_DEPTH(neighborhood.x, result.z));
	result = lerp(result, float3( 1.0, -1.0, neighborhood.y), COMPARE_DEPTH(neighborhood.y, result.z));
	result = lerp(result, float3(-1.0,  1.0, neighborhood.z), COMPARE_DEPTH(neighborhood.z, result.z));
	result = lerp(result, float3( 1.0,  1.0, neighborhood.w), COMPARE_DEPTH(neighborhood.w, result.z));

	return (uv + result.xy * k);
}

// Pixel Shader for Temporal Anti-Aliasing
META_PS(true, FEATURE_LEVEL_ES2)
META_PERMUTATION_1(IS_ORTHO=0)
META_PERMUTATION_1(IS_ORTHO=1)
void PS(Quad_VS2PS input, out float4 output : SV_Target0, out float4 outputHistory : SV_Target1)
{
	float2 jitter = TaaJitterStrength.zw;
	float2 texcoord = input.TexCoord;
	const float2 k = ScreenSize.zw;

#if IS_ORTHO
	float2 closest = texcoord;
#else
	float2 closest = GetClosestFragment(texcoord);
#endif

	// Sample velocity
	float2 velocity = MotionVectors.SampleLevel(SamplerLinearClamp, closest, 0).xy;

	// Sample color and surround
	float2 uv = texcoord - jitter;
	float3 color = Fetch(Input, uv);
	float3 topLeft = Fetch(Input, uv - k);
	float3 bottomRight = Fetch(Input, uv + k);
	float3 topRight = Fetch(Input, uv + float2(k.x, -k.y));
	float3 bottomLeft = Fetch(Input, uv + float2(-k.x, k.y));

	float3 corners = 4.0 * (topLeft + bottomRight) - 2.0 * color;

	// Sharpen output
	float sharpness = FinalBlendParameters.w;
	float3 blur = (topLeft + topRight + bottomLeft + bottomRight) * 0.25;
	color += (color - blur) * sharpness;
	color = clamp(color, 0.0, HDR_CLAMP_MAX);

	// Tonemap color
	float3 average = Map((corners + color) / 7.0);
	topLeft = Map(topLeft);
	bottomRight = Map(bottomRight);
	color = Map(color);

	// Sample history
	float3 history = Fetch(InputHistory, texcoord - velocity);
	history = Map(history);

	float colorLuma = Luminance(color);
	float averageLuma = Luminance(average);
	float velocityLength = length(velocity);
	float nudge = lerp(4.0, 0.25, velocityLength * 100.0) * abs(averageLuma - colorLuma);

	float3 minimum = min(bottomRight, topLeft) - nudge;
	float3 maximum = max(topLeft, bottomRight) + nudge;

	// Clip history sample
	history = ClipToAABB(history, minimum, maximum);

	// Blend color with history
	//float historyLuma = Luminance(history);
	//float weight = 1.0f - saturate(abs(colorLuma - historyLuma) / max(max(colorLuma, historyLuma), 0.2));
	float weight = saturate(velocityLength * FinalBlendParameters.z);
	float feedback = lerp(FinalBlendParameters.x, FinalBlendParameters.y, weight * weight);
	//feedback = lerp(feedback, 0.02, velocityWeight);
	color = Unmap(lerp(color, history, feedback));
	color = clamp(color, 0.0, HDR_CLAMP_MAX);

	output = float4(color, 1);
	outputHistory = output;

	//output = float4(1, 0, 0, 1) * feedback;
}
