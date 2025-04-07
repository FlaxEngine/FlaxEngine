// Copyright (c) Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"
#include "./Flax/BRDF.hlsl"
#include "./Flax/MonteCarlo.hlsl"
#include "./Flax/SH.hlsl"

META_CB_BEGIN(0, Data)
float2 Dummy0;
int CubeFace;
float SourceMipIndex;
META_CB_END

TextureCube Cube : register(t0);

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
			filteredColor += Cube.SampleLevel(SamplerLinearClamp, L, SourceMipIndex) * NoL;
			weight += NoL;
		}
	}

	return filteredColor / max(weight, 0.001);
}
