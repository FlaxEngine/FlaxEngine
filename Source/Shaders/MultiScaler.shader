// Copyright (c) Wojciech Figat. All rights reserved.

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

// Samples a texture with Catmull-Rom filtering, using 9 texture fetches instead of 16.
// See http://vec3.ca/bicubic-filtering-in-fewer-taps/ for more details
// The following code is licensed under the MIT license: https://gist.github.com/TheRealMJP/bc503b0b87b643d3505d41eab8b332ae
// Source: https://gist.github.com/TheRealMJP/c83b8c0f46b63f3a88a5986f4fa982b1
float4 SampleTextureCatmullRom(Texture2D tex, SamplerState linearSampler, float2 uv, float2 texelSize)
{
    // We're going to sample a a 4x4 grid of texels surrounding the target UV coordinate. We'll do this by rounding
    // down the sample location to get the exact center of our "starting" texel. The starting texel will be at
    // location [1, 1] in the grid, where [0, 0] is the top left corner.
    float2 samplePos = uv / texelSize;
    float2 texPos1 = floor(samplePos - 0.5f) + 0.5f;

    // Compute the fractional offset from our starting texel to our original sample location, which we'll
    // feed into the Catmull-Rom spline function to get our filter weights.
    float2 f = samplePos - texPos1;

    // Compute the Catmull-Rom weights using the fractional offset that we calculated earlier.
    // These equations are pre-expanded based on our knowledge of where the texels will be located,
    // which lets us avoid having to evaluate a piece-wise function.
    float2 w0 = f * (-0.5f + f * (1.0f - 0.5f * f));
    float2 w1 = 1.0f + f * f * (-2.5f + 1.5f * f);
    float2 w2 = f * (0.5f + f * (2.0f - 1.5f * f));
    float2 w3 = f * f * (-0.5f + 0.5f * f);

    // Work out weighting factors and sampling offsets that will let us use bilinear filtering to
    // simultaneously evaluate the middle 2 samples from the 4x4 grid.
    float2 w12 = w1 + w2;
    float2 offset12 = w2 / (w1 + w2);

    // Compute the final UV coordinates we'll use for sampling the texture
    float2 texPos0 = texPos1 - 1;
    float2 texPos3 = texPos1 + 2;
    float2 texPos12 = texPos1 + offset12;

    texPos0 *= texelSize;
    texPos3 *= texelSize;
    texPos12 *= texelSize;

    float4 result = 0.0f;
    result += tex.SampleLevel(linearSampler, float2(texPos0.x, texPos0.y), 0.0f) * w0.x * w0.y;
    result += tex.SampleLevel(linearSampler, float2(texPos12.x, texPos0.y), 0.0f) * w12.x * w0.y;
    result += tex.SampleLevel(linearSampler, float2(texPos3.x, texPos0.y), 0.0f) * w3.x * w0.y;

    result += tex.SampleLevel(linearSampler, float2(texPos0.x, texPos12.y), 0.0f) * w0.x * w12.y;
    result += tex.SampleLevel(linearSampler, float2(texPos12.x, texPos12.y), 0.0f) * w12.x * w12.y;
    result += tex.SampleLevel(linearSampler, float2(texPos3.x, texPos12.y), 0.0f) * w3.x * w12.y;

    result += tex.SampleLevel(linearSampler, float2(texPos0.x, texPos3.y), 0.0f) * w0.x * w3.y;
    result += tex.SampleLevel(linearSampler, float2(texPos12.x, texPos3.y), 0.0f) * w12.x * w3.y;
    result += tex.SampleLevel(linearSampler, float2(texPos3.x, texPos3.y), 0.0f) * w3.x * w3.y;

    return result;
}

// Pixel Shader for upscaling image
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_Upscale(Quad_VS2PS input) : SV_Target0
{
	// Catmull-Rom filtering with 9-taps
	return SampleTextureCatmullRom(Input, SamplerLinearClamp, input.TexCoord, TexelSize);
}
