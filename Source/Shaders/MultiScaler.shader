// Copyright (c) Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"
#include "./Flax/Gather.hlsl"

META_CB_BEGIN(0, Data)
float2 TexelSize;
float2 Padding;
META_CB_END

// Use linear sampling (less texture fetches required)
#define SAMPLE_BLUR(rt, texCoord) SAMPLE_RT_LINEAR(rt, texCoord)

Texture2D Input : register(t0);

// Pixel Shader for depth buffer downscale (to half res)
META_PS(true, FEATURE_LEVEL_ES2)
META_PERMUTATION_1(OUTPUT_DEPTH=0)
META_PERMUTATION_1(OUTPUT_DEPTH=1)
META_PERMUTATION_1(HZB_CLOSEST=2)
float PS_HalfDepth(Quad_VS2PS input) 
#if OUTPUT_DEPTH
    : SV_Depth
#else
    : SV_Target0
#endif
{
    // Load 4 depth values (2x2 quad)
	float4 depths = TextureGatherDepth(Input, input.TexCoord);

#if REVERSE_Z
#if HZB_CLOSEST
	return max(depths.x, max(depths.y, max(depths.z, depths.w)));
#else
	return min(depths.x, min(depths.y, min(depths.z, depths.w)));
#endif
#else
#if HZB_CLOSEST
	return min(depths.x, min(depths.y, min(depths.z, depths.w)));
#else
	return max(depths.x, max(depths.y, max(depths.z, depths.w)));
#endif
#endif
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

	float4 color = SAMPLE_BLUR(Input, input.TexCoord) * weights[0];

	UNROLL
	for (int i = 1; i < 2; i++)
	{
#if CONVOLVE_VERTICAL
	float2 texCoordOffset = float2(offsets[i], 0) * TexelSize;
#else
	float2 texCoordOffset = float2(0, offsets[i]) * TexelSize;
#endif

		color += (SAMPLE_BLUR(Input, input.TexCoord + texCoordOffset)
				+ SAMPLE_BLUR(Input, input.TexCoord - texCoordOffset))
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

	float4 color = SAMPLE_BLUR(Input, input.TexCoord) * weights[0];

	UNROLL
	for (int i = 1; i < 3; i++)
	{
#if CONVOLVE_VERTICAL
	float2 texCoordOffset = float2(offsets[i], 0) * TexelSize;
#else
	float2 texCoordOffset = float2(0, offsets[i]) * TexelSize;
#endif

		color += (SAMPLE_BLUR(Input, input.TexCoord + texCoordOffset)
				+ SAMPLE_BLUR(Input, input.TexCoord - texCoordOffset))
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

	float4 color = SAMPLE_BLUR(Input, input.TexCoord) * weights[0];

	UNROLL
	for (int i = 1; i < 4; i++)
	{
#if CONVOLVE_VERTICAL
	float2 texCoordOffset = float2(offsets[i], 0) * TexelSize;
#else
	float2 texCoordOffset = float2(0, offsets[i]) * TexelSize;
#endif

		color += (SAMPLE_BLUR(Input, input.TexCoord + texCoordOffset)
				+ SAMPLE_BLUR(Input, input.TexCoord - texCoordOffset))
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

#ifdef _PS_BilateralUpscale

#include "./Flax/GBufferCommon.hlsl"

Texture2D Depths : register(t1);
Texture2D Normals : register(t2);

// Pixel Shader for upscaling image with bilateral filtering (depth and normal weighting)
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_BilateralUpscale(Quad_VS2PS input) : SV_Target0
{
    const float epsilon = 0.0001f;

    float2 inputSize = floor(1.0f / TexelSize);
    float2 baseUV = floor(input.TexCoord * inputSize - 0.5f) / inputSize + 0.5f * TexelSize;
    float2 bilinear = (input.TexCoord - baseUV) * inputSize;
    float4 weights = float4((1 - bilinear.y) * (1 - bilinear.x), (1 - bilinear.y) * bilinear.x, bilinear.y * (1 - bilinear.x), bilinear.y * bilinear.x);

    float4 values00 = SAMPLE_RT_LINEAR(Input, baseUV);
    float4 values10 = SAMPLE_RT_LINEAR(Input, baseUV + float2(TexelSize.x, 0));
    float4 values01 = SAMPLE_RT_LINEAR(Input, baseUV + float2(0, TexelSize.y));
    float4 values11 = SAMPLE_RT_LINEAR(Input, baseUV + TexelSize);

    float depth00 = SAMPLE_RT_DEPTH(Depths, baseUV);
    float depth10 = SAMPLE_RT_DEPTH(Depths, baseUV + float2(TexelSize.x, 0));
    float depth01 = SAMPLE_RT_DEPTH(Depths, baseUV + float2(0, TexelSize.y));
    float depth11 = SAMPLE_RT_DEPTH(Depths, baseUV + TexelSize);

    float minDepth = min(min(min(depth00, depth10), depth01), depth11);
    float maxDepth = max(max(max(depth00, depth10), depth01), depth11);
    if (maxDepth / minDepth > 1.021f)
    {
        float depth = SAMPLE_RT_DEPTH(Depths, input.TexCoord);
        weights *= 1.0f / (abs(float4(depth00, depth10, depth01, depth11) - depth.xxxx) + epsilon);
    }

    float3 normal00 = DecodeNormal(SAMPLE_RT_LINEAR(Normals, baseUV).rgb);
    float3 normal10 = DecodeNormal(SAMPLE_RT_LINEAR(Normals, baseUV + float2(TexelSize.x, 0)).rgb);
    float3 normal01 = DecodeNormal(SAMPLE_RT_LINEAR(Normals, baseUV + float2(0, TexelSize.y)).rgb);
    float3 normal11 = DecodeNormal(SAMPLE_RT_LINEAR(Normals, baseUV + TexelSize).rgb);

    float3 normal = DecodeNormal(SAMPLE_RT(Normals, input.TexCoord).rgb);
    float normalPower = 2;
    weights.x *= min(pow(saturate(dot(normal, normal00)), normalPower), 1.0);
    weights.y *= min(pow(saturate(dot(normal, normal10)), normalPower), 1.0);
    weights.z *= min(pow(saturate(dot(normal, normal01)), normalPower), 1.0);
    weights.w *= min(pow(saturate(dot(normal, normal11)), normalPower), 1.0);

    values00 *= weights.x;
    values10 *= weights.y;
    values01 *= weights.z;
    values11 *= weights.w;

    return (values00 + values10 + values01 + values11) / (dot(weights, 1) + epsilon);
}

#endif
