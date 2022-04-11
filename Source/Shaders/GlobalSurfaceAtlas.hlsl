// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"
#include "./Flax/Collisions.hlsl"

// This must match C++
#define GLOBAL_SURFACE_ATLAS_OBJECT_SIZE (5 + 6 * 5) // Amount of float4s per-object
#define GLOBAL_SURFACE_ATLAS_TILE_NORMAL_THRESHOLD 0.1f // Cut-off value for tiles transitions blending during sampling

struct GlobalSurfaceTile
{
	float4 AtlasRectUV;
	float4x4 WorldToLocal;
	float3 ViewBoundsSize;
	bool Enabled;
};

struct GlobalSurfaceObject
{
	float3 BoundsPosition;
	float BoundsRadius;
	float4x4 WorldToLocal;
	float3 Extent;
};

float4 LoadGlobalSurfaceAtlasObjectBounds(Buffer<float4> objects, uint objectIndex)
{
	// This must match C++
	const uint objectStart = objectIndex * GLOBAL_SURFACE_ATLAS_OBJECT_SIZE;
	return objects.Load(objectStart);
}

GlobalSurfaceObject LoadGlobalSurfaceAtlasObject(Buffer<float4> objects, uint objectIndex)
{
	// This must match C++
	const uint objectStart = objectIndex * GLOBAL_SURFACE_ATLAS_OBJECT_SIZE;
	float4 vector0 = objects.Load(objectStart + 0);
	float4 vector1 = objects.Load(objectStart + 1);
	float4 vector2 = objects.Load(objectStart + 2);
	float4 vector3 = objects.Load(objectStart + 3);
	float4 vector4 = objects.Load(objectStart + 4); // w unused
	GlobalSurfaceObject object = (GlobalSurfaceObject)0;
	object.BoundsPosition = vector0.xyz;
	object.BoundsRadius = vector0.w;
	object.WorldToLocal[0] = float4(vector1.xyz, 0.0f);
	object.WorldToLocal[1] = float4(vector2.xyz, 0.0f);
	object.WorldToLocal[2] = float4(vector3.xyz, 0.0f);
	object.WorldToLocal[3] = float4(vector1.w, vector2.w, vector3.w, 1.0f);
	object.Extent = vector4.xyz;
	return object;
}

GlobalSurfaceTile LoadGlobalSurfaceAtlasTile(Buffer<float4> objects, uint objectIndex, uint tileIndex)
{
	// This must match C++
	const uint objectStart = objectIndex * GLOBAL_SURFACE_ATLAS_OBJECT_SIZE;
	const uint tileStart = objectStart + 5 + tileIndex * 5;
	float4 vector0 = objects.Load(tileStart + 0);
	float4 vector1 = objects.Load(tileStart + 1);
	float4 vector2 = objects.Load(tileStart + 2);
	float4 vector3 = objects.Load(tileStart + 3);
	float4 vector4 = objects.Load(tileStart + 4);
	GlobalSurfaceTile tile = (GlobalSurfaceTile)0;
	tile.AtlasRectUV = vector0.xyzw;
	tile.WorldToLocal[0] = float4(vector1.xyz, 0.0f);
	tile.WorldToLocal[1] = float4(vector2.xyz, 0.0f);
	tile.WorldToLocal[2] = float4(vector3.xyz, 0.0f);
	tile.WorldToLocal[3] = float4(vector1.w, vector2.w, vector3.w, 1.0f);
	tile.ViewBoundsSize = vector4.xyz;
	tile.Enabled = vector4.w > 0;
	return tile;
}

// Global Surface Atlas data for a constant buffer
struct GlobalSurfaceAtlasData
{
	float2 Padding;
	float Resolution;
	uint ObjectsCount;
};

float3 SampleGlobalSurfaceAtlasTex(Texture2D atlas, float2 atlasUV, float4 bilinearWeights)
{
	float4 sampleX = atlas.GatherRed(SamplerLinearClamp, atlasUV);
	float4 sampleY = atlas.GatherGreen(SamplerLinearClamp, atlasUV);
	float4 sampleZ = atlas.GatherBlue(SamplerLinearClamp, atlasUV);
	return float3(dot(sampleX, bilinearWeights), dot(sampleY, bilinearWeights), dot(sampleZ, bilinearWeights));
}

float4 SampleGlobalSurfaceAtlasTile(const GlobalSurfaceAtlasData data, GlobalSurfaceTile tile, Texture2D depth, Texture2D atlas, float3 worldPosition, float3 worldNormal, float surfaceThreshold)
{
	// Tile normal weight based on the sampling angle
	float3 tileNormal = normalize(mul(worldNormal, (float3x3)tile.WorldToLocal));
	float normalWeight = saturate(dot(float3(0, 0, -1), tileNormal));
	normalWeight = (normalWeight - GLOBAL_SURFACE_ATLAS_TILE_NORMAL_THRESHOLD) / (1.0f - GLOBAL_SURFACE_ATLAS_TILE_NORMAL_THRESHOLD);
	if (normalWeight <= 0.0f)
		return 0;

	// Get tile UV and depth at the world position
	float3 tilePosition = mul(float4(worldPosition, 1), tile.WorldToLocal).xyz;
	float tileDepth = tilePosition.z / tile.ViewBoundsSize.z;
	float2 tileUV = (tilePosition.xy / tile.ViewBoundsSize.xy) + 0.5f;
	tileUV.y = 1.0 - tileUV.y;
	float2 atlasUV = tileUV * tile.AtlasRectUV.zw + tile.AtlasRectUV.xy;

	// Calculate bilinear weights
	float2 bilinearWeightsUV = frac(atlasUV * data.Resolution + 0.5f);
	float4 bilinearWeights;
	bilinearWeights.x = (1.0 - bilinearWeightsUV.x) * (bilinearWeightsUV.y);
	bilinearWeights.y = (bilinearWeightsUV.x) * (bilinearWeightsUV.y);
	bilinearWeights.z = (bilinearWeightsUV.x) * (1 - bilinearWeightsUV.y);
	bilinearWeights.w = (1 - bilinearWeightsUV.x) * (1 - bilinearWeightsUV.y);

	// Tile depth weight based on sample position occlusion
	float4 tileZ = depth.Gather(SamplerLinearClamp, atlasUV, 0.0f);
	float depthThreshold = 2.0f * surfaceThreshold / tile.ViewBoundsSize.z;
	float4 depthVisibility = 1.0f;
	UNROLL
	for (uint i = 0; i < 4; i++)
	{
		depthVisibility[i] = 1.0f - saturate((abs(tileDepth - tileZ[i]) - depthThreshold) / (0.5f * depthThreshold));
		if (tileZ[i] >= 1.0f)
			depthVisibility[i] = 0.0f;
	}
	float sampleWeight = normalWeight * dot(depthVisibility, bilinearWeights);
	if (sampleWeight <= 0.0f)
		return 0;
	bilinearWeights = depthVisibility * bilinearWeights;
	//bilinearWeights = normalize(bilinearWeights);

	// Sample atlas texture
	float3 sampleColor = SampleGlobalSurfaceAtlasTex(atlas, atlasUV, bilinearWeights);

	//return float4(sampleWeight.xxx, sampleWeight);
	return float4(sampleColor.rgb * sampleWeight, sampleWeight);
	//return float4(normalWeight.xxx, sampleWeight);
}

// Samples the Global Surface Atlas and returns the lighting (with opacity) at the given world location (and direction).
float4 SampleGlobalSurfaceAtlas(const GlobalSurfaceAtlasData data, Buffer<float4> objects, Texture2D depth, Texture2D atlas, float3 worldPosition, float3 worldNormal)
{
	float4 result = float4(0, 0, 0, 0);
	float surfaceThreshold = 20.0f; // Additional threshold between object or tile size compared with input data (error due to SDF or LOD incorrect appearance)
	// TODO: add grid culling to object for faster lookup
	LOOP
	for (uint objectIndex = 0; objectIndex < data.ObjectsCount && result.a <= 0.0f; objectIndex++)
	{
		// Cull point vs sphere
		float4 objectBounds = LoadGlobalSurfaceAtlasObjectBounds(objects, objectIndex);
		if (distance(objectBounds.xyz, worldPosition) > objectBounds.w)
			continue;
		GlobalSurfaceObject object = LoadGlobalSurfaceAtlasObject(objects, objectIndex);
		float3 localPosition = mul(float4(worldPosition, 1), object.WorldToLocal).xyz;
		float3 localExtent = object.Extent + surfaceThreshold;
		if (any(localPosition > localExtent) || any(localPosition < -localExtent))
			continue;

		// Sample tiles based on the directionality
		// TODO: place enabled tiles mask in object data to skip reading disabled tiles
		float3 localNormal = normalize(mul(worldNormal, (float3x3)object.WorldToLocal));
		float3 localNormalSq = localNormal * localNormal;
		if (localNormalSq.x > GLOBAL_SURFACE_ATLAS_TILE_NORMAL_THRESHOLD * GLOBAL_SURFACE_ATLAS_TILE_NORMAL_THRESHOLD)
		{
			uint tileIndex = localNormal.x > 0.0f ? 0 : 1;
			GlobalSurfaceTile tile = LoadGlobalSurfaceAtlasTile(objects, objectIndex, tileIndex);
			result += SampleGlobalSurfaceAtlasTile(data, tile, depth, atlas, worldPosition, worldNormal, surfaceThreshold);
		}
		if (localNormalSq.y > GLOBAL_SURFACE_ATLAS_TILE_NORMAL_THRESHOLD * GLOBAL_SURFACE_ATLAS_TILE_NORMAL_THRESHOLD)
		{
			uint tileIndex = localNormal.y > 0.0f ? 2 : 3;
			GlobalSurfaceTile tile = LoadGlobalSurfaceAtlasTile(objects, objectIndex, tileIndex);
			result += SampleGlobalSurfaceAtlasTile(data, tile, depth, atlas, worldPosition, worldNormal, surfaceThreshold);
		}
		if (localNormalSq.z > GLOBAL_SURFACE_ATLAS_TILE_NORMAL_THRESHOLD * GLOBAL_SURFACE_ATLAS_TILE_NORMAL_THRESHOLD)
		{
			uint tileIndex = localNormal.z > 0.0f ? 4 : 5;
			GlobalSurfaceTile tile = LoadGlobalSurfaceAtlasTile(objects, objectIndex, tileIndex);
			result += SampleGlobalSurfaceAtlasTile(data, tile, depth, atlas, worldPosition, worldNormal, surfaceThreshold);
		}
	}

	// Normalize result
	result.rgb /= max(result.a, 0.0001f);

	return result;
}
