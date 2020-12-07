// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"

META_CB_BEGIN(0, Data)

float2 TexelSize;
float2 Padding;

META_CB_END

// Use linear sampling (less texture fetches required)
#define SAMPLE(rt, texCoord) SAMPLE_RT_LINEAR(rt, texCoord)

Texture2D Input : register(t0);

// Pixel Shader for depth buffer downscale (to half res)
META_PS(true, FEATURE_LEVEL_ES2)
float PS_HalfDepth(Quad_VS2PS input) : SV_Depth
{
#if CAN_USE_GATHER
	float4 depths = Input.GatherRed(SamplerPointClamp, input.TexCoord);
#else
	float4 depths;
	depths.x = Input.SampleLevel(SamplerPointClamp, input.TexCoord + float2(0, 1) * TexelSize, 0).r;
	depths.y = Input.SampleLevel(SamplerPointClamp, input.TexCoord + float2(1, 1) * TexelSize, 0).r;
	depths.z = Input.SampleLevel(SamplerPointClamp, input.TexCoord + float2(1, 0) * TexelSize, 0).r;
	depths.w = Input.SampleLevel(SamplerPointClamp, input.TexCoord + float2(0, 0) * TexelSize, 0).r;
#endif

	return max(depths.x, max(depths.y, max(depths.z, depths.w))) + 0.0001f;
}

// Pixel Shader for 5-tap gaussian blur
META_PS(true, FEATURE_LEVEL_ES2)
META_PERMUTATION_1(CONVOLVE_VERTICAL=0)
META_PERMUTATION_1(CONVOLVE_VERTICAL=1)
float4 PS_Blur5(Quad_VS2PS input) : SV_Target0
{
	const float offsets[2] = {
		0.00000000,
		1.33333333
    };
	const float weights[2] = {
		0.29411765,
		0.35294118
	};

	float4 color = SAMPLE(Input, input.TexCoord) * weights[0];

	UNROLL
	for (int i = 1; i < 2; i++)
	{
#if CONVOLVE_VERTICAL
	float2 texCoordOffset = float2(offsets[i], 0) * TexelSize;
#else
	float2 texCoordOffset = float2(0, offsets[i]) * TexelSize;
#endif

		color += (SAMPLE(Input, input.TexCoord + texCoordOffset)
				+ SAMPLE(Input, input.TexCoord - texCoordOffset))
				* weights[i];
	}

	return color;
}

// Pixel Shader for 9-tap Gaussian blur
META_PS(true, FEATURE_LEVEL_ES2)
META_PERMUTATION_1(CONVOLVE_VERTICAL=0)
META_PERMUTATION_1(CONVOLVE_VERTICAL=1)
float4 PS_Blur9(Quad_VS2PS input) : SV_Target0
{	
	const float offsets[3] = {
		0.00000000,
		1.38461538,
		3.23076923
    };
	const float weights[3] = {
		0.22702703,
		0.31621622,
		0.07027027
	};

	float4 color = SAMPLE(Input, input.TexCoord) * weights[0];

	UNROLL
	for (int i = 1; i < 3; i++)
	{
#if CONVOLVE_VERTICAL
	float2 texCoordOffset = float2(offsets[i], 0) * TexelSize;
#else
	float2 texCoordOffset = float2(0, offsets[i]) * TexelSize;
#endif

		color += (SAMPLE(Input, input.TexCoord + texCoordOffset)
				+ SAMPLE(Input, input.TexCoord - texCoordOffset))
				* weights[i];
	}

	return color;
}

// Pixel Shader for 13-tap gaussian blur
META_PS(true, FEATURE_LEVEL_ES2)
META_PERMUTATION_1(CONVOLVE_VERTICAL=0)
META_PERMUTATION_1(CONVOLVE_VERTICAL=1)
float4 PS_Blur13(Quad_VS2PS input) : SV_Target0
{	
	const float offsets[4] = {
		0.00000000,
		1.41176471,
		3.29411765,
		5.17647059
    };
	const float weights[4] = {
		0.19648255,
		0.29690696,
		0.09447040,
		0.01038136
	};

	float4 color = SAMPLE(Input, input.TexCoord) * weights[0];

	UNROLL
	for (int i = 1; i < 4; i++)
	{
#if CONVOLVE_VERTICAL
	float2 texCoordOffset = float2(offsets[i], 0) * TexelSize;
#else
	float2 texCoordOffset = float2(0, offsets[i]) * TexelSize;
#endif

		color += (SAMPLE(Input, input.TexCoord + texCoordOffset)
				+ SAMPLE(Input, input.TexCoord - texCoordOffset))
				* weights[i];
	}

	return color;
}
