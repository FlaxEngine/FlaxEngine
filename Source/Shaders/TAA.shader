// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"

META_CB_BEGIN(0, Data)
float2 ScreenSizeInv;
float2 JitterInv;
float Sharpness;
float StationaryBlending;
float MotionBlending;
float Dummy0;
META_CB_END

Texture2D Input : register(t0);
Texture2D InputHistory : register(t1);
Texture2D MotionVectors : register(t2);
Texture2D Depth : register(t3);

// [Pedersen, 2016, "Temporal Reprojection Anti-Aliasing in INSIDE"]
float4 ClipToAABB(float4 color, float4 minimum, float4 maximum)
{
	float4 center = (maximum + minimum) * 0.5;
	float4 extents = (maximum - minimum) * 0.5;
	float4 shift = color - center;
    float4 absUnit = abs(shift / max(extents, 0.0001));
    float maxUnit = max(max(absUnit.x, absUnit.y), absUnit.z);
	return maxUnit > 1.0 ? center + (shift / maxUnit) : color;
}

// Pixel Shader for Temporal Anti-Aliasing
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS(Quad_VS2PS input) : SV_Target0
{
	// Find the closest pixel in 3x3 neighborhood
	float bestDepth = 1;
	float2 bestUV = float2(0, 0);
	float4 neighborhoodMin = 100000;
	float4 neighborhoodMax = -10000;
	float4 current;
	float4 neighborhoodSum = 0;
	for (int x = -1; x <= 1; ++x)
	{
		for (int y = -1; y <= 1; ++y)
		{
			float2 sampleUV = input.TexCoord + float2(x, y) * ScreenSizeInv;

			float4 neighbor = SAMPLE_RT_LINEAR(Input, sampleUV);
			neighborhoodMin = min(neighborhoodMin, neighbor);
			neighborhoodMax = max(neighborhoodMax, neighbor);
			if (x == 0 && y == 0)
				current = neighbor;
			neighborhoodSum += neighbor;

			float depth = SAMPLE_RT(Depth, sampleUV).r;
			if (depth < bestDepth)
			{
				bestDepth = depth;
				bestUV = sampleUV;
			}
		}
	}
	float2 velocity = SAMPLE_RT_LINEAR(MotionVectors, bestUV).xy;
	float velocityLength = length(velocity);
	float2 prevUV = input.TexCoord - velocity;

	// Apply sharpening
	float4 neighborhoodAvg = neighborhoodSum / 9.0;
	current += (current - neighborhoodAvg) * Sharpness;

	// Sample history by clamp it to the nearby colors range to reduce artifacts
	float4 history = SAMPLE_RT_LINEAR(InputHistory, prevUV);
	float lumaOffset = abs(Luminance(neighborhoodAvg.rgb) - Luminance(current.rgb));
	float aabbMargin = lerp(4.0, 0.25, saturate(velocityLength * 100.0)) * lumaOffset;
	history = ClipToAABB(history, neighborhoodMin - aabbMargin, neighborhoodMax + aabbMargin);
	//history = clamp(history, neighborhoodMin, neighborhoodMax);

	// Calculate history blending factor
	float motion = saturate(velocityLength * 1000.0f);
	float blendfactor = any(abs(prevUV * 2 - 1) >= 1.0f) ? 0.0f : lerp(StationaryBlending, MotionBlending, motion);

	// Perform linear accumulation of the previous samples with a current one
	float4 color = lerp(current, history, blendfactor);
	color = clamp(color, 0, HDR_CLAMP_MAX);

	return color;
}
