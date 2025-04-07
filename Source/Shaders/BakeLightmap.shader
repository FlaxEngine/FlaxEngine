// Copyright (c) Wojciech Figat. All rights reserved.

#define USE_VERTEX_COLOR 0

#include "./Flax/Common.hlsl"
#include "./Flax/MaterialCommon.hlsl"
#include "./Flax/SH.hlsl"

// This config must match C++ code
#define HEMISPHERES_RESOLUTION 64
#define NUM_SH_TARGETS 3

META_CB_BEGIN(0, Data)
float4 LightmapArea;
float4x4 WorldMatrix;
float4x4 ToTangentSpace;
float FinalWeight;
uint TexelAdress;
uint AtlasSize;
float TerrainChunkSizeLOD0;
float4 HeightmapUVScaleBias;
float3 WorldInvScale;
float Dummy1;
META_CB_END

#define USED_TEXELS_BIAS 0.001f
#define BACKGROUND_TEXELS_MARK -1.0f

struct RenderCacheVSOutput
{
	float4 Position      : SV_Position;
	float3 WorldPosition : TEXCOORD0;
	float3 WorldNormal   : TEXCOORD1;
};

struct RenderCachePSOutput
{
	float4 WorldPosition : SV_Target0;
	float4 WorldNormal   : SV_Target1;
};

META_VS(true, FEATURE_LEVEL_SM5)
META_VS_IN_ELEMENT(POSITION, 0, R32G32B32_FLOAT,   0, 0,     PER_VERTEX, 0, true)
META_VS_IN_ELEMENT(TEXCOORD, 0, R16G16_FLOAT,      1, 0,     PER_VERTEX, 0, true)
META_VS_IN_ELEMENT(NORMAL,   0, R10G10B10A2_UNORM, 1, ALIGN, PER_VERTEX, 0, true)
META_VS_IN_ELEMENT(TANGENT,  0, R10G10B10A2_UNORM, 1, ALIGN, PER_VERTEX, 0, true)
META_VS_IN_ELEMENT(TEXCOORD, 1, R16G16_FLOAT,      1, ALIGN, PER_VERTEX, 0, true)
RenderCacheVSOutput VS_RenderCacheModel(ModelInput input)
{
	RenderCacheVSOutput output;

	// Calculate vertex world position
	output.WorldPosition = mul(float4(input.Position.xyz, 1), WorldMatrix).xyz;

	// Unpack and transform vertex tangent frame vectors to world space
	float3 normal = normalize(input.Normal.xyz * 2.0 - 1.0);
	output.WorldNormal = mul(normal, (float3x3)WorldMatrix);

	// Transform lightmap UV to clip-space
	float2 lightmapUV = input.LightmapUV * LightmapArea.zw + LightmapArea.xy;
	lightmapUV.y = 1.0 - lightmapUV.y;
	lightmapUV.xy = lightmapUV.xy * 2.0 - 1.0;
	output.Position = float4(lightmapUV, 0, 1);

	return output;
}

// Must match structure defined in TerrainManager.cpp
struct TerrainVertexInput
{
	float2 TexCoord : TEXCOORD0;
	float4 Morph    : TEXCOORD1;
};

#if defined(_VS_RenderCacheTerrain)

Texture2D Heightmap : register(t0);

// Removes the scale vector from the local to world transformation matrix
float3x3 RemoveScaleFromLocalToWorld(float3x3 localToWorld)
{
	localToWorld[0] *= WorldInvScale.x;
	localToWorld[1] *= WorldInvScale.y;
	localToWorld[2] *= WorldInvScale.z;
	return localToWorld;
}

float3x3 CalcTangentToWorld(float4x4 world, float3x3 tangentToLocal)
{
	float3x3 localToWorld = RemoveScaleFromLocalToWorld((float3x3)world);
	return mul(tangentToLocal, localToWorld); 
}

META_VS(true, FEATURE_LEVEL_SM5)
META_VS_IN_ELEMENT(TEXCOORD, 0, R32G32_FLOAT,   0, ALIGN, PER_VERTEX, 0, true)
META_VS_IN_ELEMENT(TEXCOORD, 1, R8G8B8A8_UNORM, 0, ALIGN, PER_VERTEX, 0, true)
RenderCacheVSOutput VS_RenderCacheTerrain(TerrainVertexInput input)
{
	RenderCacheVSOutput output;

	// Sample heightmap
	float2 heightmapUVs = input.TexCoord * HeightmapUVScaleBias.xy + HeightmapUVScaleBias.zw;
	float4 heightmapValue = Heightmap.SampleLevel(SamplerPointClamp, heightmapUVs, 0);
	bool isHole = (heightmapValue.b + heightmapValue.a) >= 1.9f;
	float height = (float)((int)(heightmapValue.x * 255.0) + ((int)(heightmapValue.y * 255) << 8)) / 65535.0;

	// Extract normal and the holes mask
	float2 normalTemp = float2(heightmapValue.b, heightmapValue.a) * 2.0f - 1.0f;
	float3 normal = float3(normalTemp.x, sqrt(1.0 - saturate(dot(normalTemp, normalTemp))), normalTemp.y);
	normal = normalize(normal);
	if (isHole)
	{
		normal = float3(0, 1, 0);
	}

	// Construct vertex position
	float2 positionXZ = input.TexCoord * TerrainChunkSizeLOD0;
	float3 position = float3(positionXZ.x, height, positionXZ.y);

	// Calculate vertex world position
	output.WorldPosition = mul(float4(position, 1), WorldMatrix).xyz;

	// Compute world space normal vector
	float3x3 tangentToLocal = CalcTangentBasisFromWorldNormal(normal);
	float3x3 tangentToWorld = CalcTangentToWorld(WorldMatrix, tangentToLocal);
	output.WorldNormal = tangentToWorld[2];

	// Transform lightmap UV to clip-space
	float2 lightmapUV = input.TexCoord * LightmapArea.zw + LightmapArea.xy;
	lightmapUV.y = 1.0 - lightmapUV.y;
	lightmapUV.xy = lightmapUV.xy * 2.0 - 1.0;
	output.Position = float4(lightmapUV, 0, 1);

	return output;
}

#endif

META_PS(true, FEATURE_LEVEL_SM5)
RenderCachePSOutput PS_RenderCache(RenderCacheVSOutput input)
{
	RenderCachePSOutput output;

	// Just pass interpolated values to the output render targets
	output.WorldPosition = float4(input.WorldPosition, 0);
	output.WorldNormal = float4(normalize(input.WorldNormal), 0);

	return output;
}

#if defined(_PS_BlurCache)

Texture2D<float4> WorldNormalsCache : register(t0);
Texture2D<float4> WorldPositionsCache : register(t1);

META_PS(true, FEATURE_LEVEL_SM5)
RenderCachePSOutput PS_BlurCache(Quad_VS2PS input)
{
	RenderCachePSOutput output;

	output.WorldNormal = WorldNormalsCache.SampleLevel(SamplerPointClamp, input.TexCoord, 0);
	output.WorldPosition = WorldPositionsCache.SampleLevel(SamplerPointClamp, input.TexCoord, 0);

	// TODO: check if we need to use that filter - this will add more hemispheres for rendering

	/*
	// Check if pixel isn't empty
	if(length(output.WorldNormal.xyz) > 0.1)
		return output;
	
	// Simple box filter (using only valid samples)
	const float blurRadius = 2.0f;
	float offset = 1.0f / AtlasSize;
	float weight = 0;
	float3 totalNormal = 0;
	float3 totalPosition = 0;
	for(float x = -blurRadius; x <= blurRadius; x++)
	{
		for(float y = -blurRadius; y <= blurRadius; y++)
		{
			float2 sampleUV = input.TexCoord + float2(x, y) * offset;
			float3 normal = WorldNormalsCache.SampleLevel(SamplerPointClamp, sampleUV, 0).xyz;
			float3 position = WorldPositionsCache.SampleLevel(SamplerPointClamp, sampleUV, 0).xyz;
			
			// Check if pixel isn't empty
			if(length(normal) > 0.1)
			{
				totalNormal += normal;
				totalPosition += position;
				weight++;
			}
		}
	}

	// Calculate avg values
	weight = 1.0 / max(weight, 1);
	output.WorldPosition = float4(totalPosition * weight, 0);
	output.WorldNormal = float4(normalize(totalNormal * weight), 0);
	*/
	return output;
}

#elif defined(_CS_Integrate)

Texture2D<float4> RadianceMap : register(t0);
RWBuffer<float4> OutputBuffer : register(u0);

// Shared memory for summing H-Basis coefficients for a row
groupshared float3 RowHBasis[HEMISPHERES_RESOLUTION][4];

// Performs the initial integration/weighting for each pixel and sums together all SH coefficients for a row.
// The integration is based on the "Projection from Cube Maps" section of Peter Pike Sloan's "Stupid Spherical Harmonics Tricks".
META_CS(true, FEATURE_LEVEL_SM5)
[numthreads(HEMISPHERES_RESOLUTION, 1, 1)]
void CS_Integrate(uint3 GroupID : SV_GroupID, uint3 GroupThreadID : SV_GroupThreadID)
{
	const int2 location = int2(GroupThreadID.x, GroupID.y);

	// Sample radiance
	float3 radiance = RadianceMap.Load(int3(location, 0)).rgb;

	// Calculate the location in [-1, 1] texture space
	float u =   (location.x / float(HEMISPHERES_RESOLUTION)) * 2.0f - 1.0f;
	float v = -((location.y / float(HEMISPHERES_RESOLUTION)) * 2.0f - 1.0f);

	// Calculate weight
	float squaredUVs = 1 + u * u + v * v;
	float weight = 4 / (sqrt(squaredUVs) * squaredUVs);

	// Extract direction from texel uv
	float3 dirVS = normalize(float3(u, v, 1.0f));
	float3 dirTS = mul(dirVS, (float3x3)ToTangentSpace);

	// Project onto SH
	float3 sh[9];
	ProjectOntoSH3(dirTS, radiance * weight, sh);

	// Convert to H-Basis
	float3 hBasis[4];
	ConvertSH3ToHBasis(sh, hBasis);

	// Store in shared memory
	RowHBasis[GroupThreadID.x][0] = hBasis[0];
	RowHBasis[GroupThreadID.x][1] = hBasis[1];
	RowHBasis[GroupThreadID.x][2] = hBasis[2];
	RowHBasis[GroupThreadID.x][3] = hBasis[3];
	GroupMemoryBarrierWithGroupSync();

	// Sum the coefficients for the row
	[unroll(HEMISPHERES_RESOLUTION)]
	for (uint s = HEMISPHERES_RESOLUTION / 2; s > 0; s >>= 1)
	{
		if (GroupThreadID.x < s)
		{
			RowHBasis[GroupThreadID.x][0] += RowHBasis[GroupThreadID.x + s][0];
			RowHBasis[GroupThreadID.x][1] += RowHBasis[GroupThreadID.x + s][1];
			RowHBasis[GroupThreadID.x][2] += RowHBasis[GroupThreadID.x + s][2];
			RowHBasis[GroupThreadID.x][3] += RowHBasis[GroupThreadID.x + s][3];
		}

		GroupMemoryBarrierWithGroupSync();
	}

	// Have the first thread write out to the output texture
	if (GroupThreadID.x == 0)
	{
		UNROLL
		for (uint i = 0; i < NUM_SH_TARGETS; i++)
		{
			float4 packed = float4(RowHBasis[0][0][i], RowHBasis[0][1][i], RowHBasis[0][2][i], RowHBasis[0][3][i]);
			OutputBuffer[GroupID.y + HEMISPHERES_RESOLUTION * i] = packed;
		}
	}
}

#elif defined(_CS_Reduction)

Buffer<float4> InputBuffer    : register(t0);
RWBuffer<float4> OutputBuffer : register(u0);

// Shared memory for reducing H-Basis coefficients
groupshared float4 ColumnHBasis[HEMISPHERES_RESOLUTION][3];

// Reduces H-basis to a 1x1 buffer
META_CS(true, FEATURE_LEVEL_SM5)
[numthreads(HEMISPHERES_RESOLUTION, 1, 1)]
void CS_Reduction(uint3 GroupID : SV_GroupID, uint3 GroupThreadID : SV_GroupThreadID)
{
	const int2 location = int2(GroupThreadID.x, GroupID.y);

	// Store in shared memory
	ColumnHBasis[location.x][location.y] = InputBuffer[location.x + HEMISPHERES_RESOLUTION * location.y];
	GroupMemoryBarrierWithGroupSync();

	// Sum the coefficients for the column
	[unroll(HEMISPHERES_RESOLUTION)]
	for (uint s = HEMISPHERES_RESOLUTION / 2; s > 0; s >>= 1)
	{
		if (GroupThreadID.x < s)
			ColumnHBasis[location.x][location.y] += ColumnHBasis[location.x + s][location.y];
		
		GroupMemoryBarrierWithGroupSync();
	}

	// Have the first thread write out to the output buffer
	if (GroupThreadID.x == 0 && GroupThreadID.z == 0)
	{
		float4 output = ColumnHBasis[location.x][location.y];

		// Note: we add some bias to indicate that this texel has been used
		output = output * FinalWeight + USED_TEXELS_BIAS;
		output = clamp(output, 0, 10000);
		OutputBuffer[TexelAdress + location.y] = output;
	}
}

#elif defined(_CS_BlurEmpty)

Buffer<float4> InputBuffer    : register(t0);
RWBuffer<float4> OutputBuffer : register(u0);

// Blur empty lightmap texels to reduce artifacts (blurs only holes and sets -1 to pixels that are not using lightmap - no data)
META_CS(true, FEATURE_LEVEL_SM5)
[numthreads(1, 1, 1)]
void CS_BlurEmpty(uint3 GroupID : SV_GroupID, uint3 GroupThreadID : SV_GroupThreadID)
{
	if (GroupID.x >= AtlasSize || GroupID.y > AtlasSize)
		return;
	const int2 location = int2(GroupID.x, GroupID.y);
	const uint texelAdress = (location.y * AtlasSize + location.x) * NUM_SH_TARGETS;

	// TODO: use more threads to sample lightmap and final therad make it blur

	// Simple box filter (using only valid samples)
	const int blurRadius = 2;
	float weight = 0;
	float4 total0 = 0;
	float4 total1 = 0;
	float4 total2 = 0;
	for (int x = -blurRadius; x <= blurRadius; x++)
	{
		for (int y = -blurRadius; y <= blurRadius; y++)
		{
			int2 sampleLocation = location + int2(x, y);
			uint sampleAdress = (clamp(sampleLocation.y, 0, AtlasSize - 1) * AtlasSize + clamp(sampleLocation.x, 0, AtlasSize - 1)) * NUM_SH_TARGETS;

			float4 sample0 = InputBuffer[sampleAdress + 0];
			float4 sample1 = InputBuffer[sampleAdress + 1];
			float4 sample2 = InputBuffer[sampleAdress + 2];

			if (any(sample0))
			{
				total0 += sample0 - USED_TEXELS_BIAS;
				total1 += sample1 - USED_TEXELS_BIAS;
				total2 += sample2 - USED_TEXELS_BIAS;
				weight++;
			}
		}
	}

	// Check if pixel has invalid value
	/*float4 lightmap0 = InputBuffer[texelAdress + 0];
	if (any(lightmap0))
	{
		// Discard sampling results
		total0 = lightmap0 - USED_TEXELS_BIAS;
		total1 = InputBuffer[texelAdress + 1] - USED_TEXELS_BIAS;
		total2 = InputBuffer[texelAdress + 2] - USED_TEXELS_BIAS;
	}
	else*/ if (weight > 0.0001f)
	{
		// Calculate avg values and save results
		weight = 1.0 / weight;
		total0 *= weight;
		total1 *= weight;
		total2 *= weight;
	}
	else
	{
		// No data - use wide gaussian blur for the background in 
		total0 = BACKGROUND_TEXELS_MARK;
		total1 = BACKGROUND_TEXELS_MARK;
		total2 = BACKGROUND_TEXELS_MARK;
	}

	// Save results
#if 1
	OutputBuffer[texelAdress + 0] = total0;
	OutputBuffer[texelAdress + 1] = total1;
	OutputBuffer[texelAdress + 2] = total2;
#else
	OutputBuffer[texelAdress + 0] = InputBuffer[texelAdress + 0];
	OutputBuffer[texelAdress + 1] = InputBuffer[texelAdress + 1];
	OutputBuffer[texelAdress + 2] = InputBuffer[texelAdress + 2];
#endif
}

#elif defined(_CS_Dilate)

Buffer<float4> InputBuffer : register(t0);
RWBuffer<float4> OutputBuffer : register(u0);

// Fills the empty lightmap texels with blurred data of the surroundings texels (uses only valid ones)
META_CS(true, FEATURE_LEVEL_SM5)
[numthreads(1, 1, 1)]
void CS_Dilate(uint3 GroupID : SV_GroupID, uint3 GroupThreadID : SV_GroupThreadID)
{
	if (GroupID.x >= AtlasSize || GroupID.y > AtlasSize)
		return;
	const int2 location = int2(GroupID.x, GroupID.y);
	const uint texelAdress = (location.y * AtlasSize + location.x) * NUM_SH_TARGETS;

	// Copy data
	float4 lightmap0 = InputBuffer[texelAdress + 0];
	float4 lightmap1 = InputBuffer[texelAdress + 1];
	float4 lightmap2 = InputBuffer[texelAdress + 2];
	OutputBuffer[texelAdress + 0] = lightmap0;
	OutputBuffer[texelAdress + 1] = lightmap1;
	OutputBuffer[texelAdress + 2] = lightmap2;

	// Check if pixel has valid value
	if (abs(lightmap0.r - BACKGROUND_TEXELS_MARK) > 0.001f)
		return;

	float total = 0;
	float4 total0 = 0;
	float4 total1 = 0;
	float4 total2 = 0;

	const int OffsetX[] = { -1,  0,  1, -1, 0, 1, -1, 0, 1, };
	const int OffsetY[] = { -1, -1, -1,  0, 0, 0,  1, 1, 1, };

	UNROLL
	for (int sampleIndex = 0; sampleIndex < 9; sampleIndex++)
	{
		int2 sampleLocation = location + int2(OffsetX[sampleIndex], OffsetY[sampleIndex]);
		if (sampleLocation.x >= 0 && sampleLocation.x < AtlasSize && sampleLocation.y >= 0 && sampleLocation.y < AtlasSize)
		{			
			uint sampleAdress = (sampleLocation.y * AtlasSize + sampleLocation.x) * NUM_SH_TARGETS;
			float4 sample0 = InputBuffer[sampleAdress + 0];
			float4 sample1 = InputBuffer[sampleAdress + 1];
			float4 sample2 = InputBuffer[sampleAdress + 2];

			// Use only valid texels
			if (abs(sample0.r - BACKGROUND_TEXELS_MARK) > 0.001f)
			{
				total0 += sample0;
				total1 += sample1;
				total2 += sample2;
				total++;
			}
		}
	}

	if (total > 0)
	{
		total = 1.0f / total;
		OutputBuffer[texelAdress + 0] = total0 * total;
		OutputBuffer[texelAdress + 1] = total1 * total;
		OutputBuffer[texelAdress + 2] = total2 * total;
	}
}

#elif defined(_CS_Finalize)

RWBuffer<float4> OutputBuffer : register(u0);

// Cleanups the lightmap data by removing the invalid texels to be just pure black
META_CS(true, FEATURE_LEVEL_SM5)
[numthreads(1, 1, 1)]
void CS_Finalize(uint3 GroupID : SV_GroupID, uint3 GroupThreadID : SV_GroupThreadID)
{
	if (GroupID.x >= AtlasSize || GroupID.y > AtlasSize)
		return;
	const int2 location = int2(GroupID.x, GroupID.y);
	const uint texelAdress = (location.y * AtlasSize + location.x) * NUM_SH_TARGETS;

	// Check if pixel has valid value
	if (abs(OutputBuffer[texelAdress].r - BACKGROUND_TEXELS_MARK) > 0.001f)
		return;

	// Make it black
	float4 clearColor = float4(0, 0, 0, 0);
	OutputBuffer[texelAdress + 0] = clearColor;
	OutputBuffer[texelAdress + 1] = clearColor;
	OutputBuffer[texelAdress + 2] = clearColor;
}

#endif
