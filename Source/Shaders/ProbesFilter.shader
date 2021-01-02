// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"
#include "./Flax/BRDF.hlsl"
#include "./Flax/MonteCarlo.hlsl"
#include "./Flax/SH.hlsl"

META_CB_BEGIN(0, Data)
float2 Dummy0;
int CubeFace;
int SourceMipIndex;
float4 Sample01;
float4 Sample23;
float4 CoefficientMask0;
float4 CoefficientMask1;
float3 Dummy1;
float CoefficientMask2;
META_CB_END

TextureCube Cube : register(t0);
Texture2D Image : register(t1);

float4 SampleCubemap(float3 uv)
{
	return Cube.SampleLevel(SamplerLinearClamp, uv, SourceMipIndex);
}

float3 UvToCubeMapUv(float2 uv)
{
	float3 coords;
	if (CubeFace == 0)
	{
		coords = float3(1, -uv.y, -uv.x);
	}
	else if (CubeFace == 1)
	{
		coords = float3(-1, -uv.y, uv.x);
	}
	else if (CubeFace == 2)
	{
		coords = float3(uv.x, 1, uv.y);
	}
	else if (CubeFace == 3)
	{
		coords = float3(uv.x, -1, -uv.y);
	}
	else if (CubeFace == 4)
	{
		coords = float3(uv.x, -uv.y, 1);
	}
	else
	{
		coords = float3(-uv.x, -uv.y, -1);
	}
	return coords;
}

// Pixel Shader for filtring probe mip levels
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_FilterFace(Quad_VS2PS input) : SV_Target
{
	float2 uv = input.TexCoord * 2 - 1;
	float3 cubeCoordinates = UvToCubeMapUv(uv);

#define NUM_FILTER_SAMPLES 512

	float3 N = normalize(cubeCoordinates);
	float roughness = ProbeRoughnessFromMip(SourceMipIndex);

	float4 filteredColor = 0;
	float weight = 0;

	LOOP
	for (int i = 0; i < NUM_FILTER_SAMPLES; i++)
	{
		float2 E = Hammersley(i, NUM_FILTER_SAMPLES, 0);
		float3 H = TangentToWorld(ImportanceSampleGGX(E, roughness).xyz, N);
		float3 L = 2 * dot(N, H) * H - N;
		float NoL = saturate(dot(N, L));

		BRANCH
		if (NoL > 0)
		{
			filteredColor += SampleCubemap(L) * NoL;
			weight += NoL;
		}
	}

	return filteredColor / max(weight, 0.001);
}

// Pixel Shader for coping probe face
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_CopyFace(Quad_VS2PS input) : SV_Target
{
	return Image.SampleLevel(SamplerLinearClamp, input.TexCoord, 0);
}

// Pixel Shader for calculating Diffuse Irradiance
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_CalcDiffuseIrradiance(Quad_VS2PS input) : SV_Target
{
	float2 uv = input.TexCoord * 2 - 1;	
	float3 cubeCoordinates = normalize(UvToCubeMapUv(uv));
	float squaredUVs = 1 + dot(uv, uv);
	float weight = 4 / (sqrt(squaredUVs) * squaredUVs);

	ThreeBandSHVector shCoefficients = SHBasisFunction3(cubeCoordinates);
	float currentSHCoefficient = dot(shCoefficients.V0, CoefficientMask0) + dot(shCoefficients.V1, CoefficientMask1) + shCoefficients.V2 * CoefficientMask2;

	float3 radiance = SampleCubemap(cubeCoordinates).rgb;
	return float4(radiance * currentSHCoefficient * weight, weight);
}

// Pixel Shader for accumulating Diffuse Irradiance
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_AccDiffuseIrradiance(Quad_VS2PS input) : SV_Target
{
	float4 result = 0;
	{
		float2 uv = saturate(input.TexCoord + Sample01.xy) * 2 - 1;
		float3 cubeCoordinates = UvToCubeMapUv(uv);
		result += SampleCubemap(cubeCoordinates);
	}
	{
		float2 uv = saturate(input.TexCoord + Sample01.zw) * 2 - 1;
		float3 cubeCoordinates = UvToCubeMapUv(uv);
		result += SampleCubemap(cubeCoordinates);
	}
	{
		float2 uv = saturate(input.TexCoord + Sample23.xy) * 2 - 1;
		float3 cubeCoordinates = UvToCubeMapUv(uv);
		result += SampleCubemap(cubeCoordinates);
	}
	{
		float2 uv = saturate(input.TexCoord + Sample23.zw) * 2 - 1;
		float3 cubeCoordinates = UvToCubeMapUv(uv);
		result += SampleCubemap(cubeCoordinates);
	}
	return result / 4.0f;
}

// Pixel Shader for accumulating cube faces into one pixel
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_AccumulateCubeFaces(Quad_VS2PS input) : SV_Target
{
	float4 result = SampleCubemap(float3(1, 0, 0));
	result += SampleCubemap(float3(-1, 0, 0));
	result += SampleCubemap(float3(0, 1, 0));
	result += SampleCubemap(float3(0, -1, 0));
	result += SampleCubemap(float3(0, 0, 1));
	result += SampleCubemap(float3(0, 0, -1));
	return float4((4 * PI) * result.rgb / max(result.a, 0.00001f), 0);
}

// Pixel Shader for copying frame to cube face with setting lower hemisphere to black
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_CopyFrameLHB(Quad_VS2PS input) : SV_Target
{
	float mask = input.TexCoord.y < 0.5;// TODO: make is smooth (and branchless using function)
	return float4(Image.SampleLevel(SamplerLinearClamp, input.TexCoord, 0).xyz * mask * CoefficientMask2, 1);
}
