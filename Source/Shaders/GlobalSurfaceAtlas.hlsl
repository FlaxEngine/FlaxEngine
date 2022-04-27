// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"
#include "./Flax/Collisions.hlsl"

// This must match C++
#define GLOBAL_SURFACE_ATLAS_CHUNKS_RESOLUTION 40 // Amount of chunks (in each direction) to split atlas draw distance for objects culling
#define GLOBAL_SURFACE_ATLAS_CHUNKS_GROUP_SIZE 4
#define GLOBAL_SURFACE_ATLAS_TILE_DATA_STRIDE 5 // Amount of float4s per-tile
#define GLOBAL_SURFACE_ATLAS_TILE_NORMAL_THRESHOLD 0.1f // Cut-off value for tiles transitions blending during sampling
#define GLOBAL_SURFACE_ATLAS_TILE_PROJ_PLANE_OFFSET 0.1f // Small offset to prevent clipping with the closest triangles (shifts near and far planes)

struct GlobalSurfaceTile
{
	float4 AtlasRectUV;
	float4x4 WorldToLocal;
	float3 ViewBoundsSize;
};

struct GlobalSurfaceObject
{
	float3 BoundsPosition;
	float BoundsRadius;
	float4x4 WorldToLocal;
	float3 Extent;
	uint TileOffsets[6];
	uint DataSize; // count of float4s for object+tiles
};

float4 LoadGlobalSurfaceAtlasObjectBounds(Buffer<float4> objects, uint objectAddress)
{
	// This must match C++
	return objects.Load(objectAddress + 0);
}

uint LoadGlobalSurfaceAtlasObjectDataSize(Buffer<float4> objects, uint objectAddress)
{
	// This must match C++
	return asuint(objects.Load(objectAddress + 1).w);
}

GlobalSurfaceObject LoadGlobalSurfaceAtlasObject(Buffer<float4> objects, uint objectAddress)
{
	// This must match C++
	float4 vector0 = objects.Load(objectAddress + 0);
	float4 vector1 = objects.Load(objectAddress + 1);
	float4 vector2 = objects.Load(objectAddress + 2);
	float4 vector3 = objects.Load(objectAddress + 3);
	float4 vector4 = objects.Load(objectAddress + 4);
	float4 vector5 = objects.Load(objectAddress + 5); // w unused
	GlobalSurfaceObject object = (GlobalSurfaceObject)0;
	object.BoundsPosition = vector0.xyz;
	object.BoundsRadius = vector0.w;
	object.WorldToLocal[0] = float4(vector2.xyz, 0.0f);
	object.WorldToLocal[1] = float4(vector3.xyz, 0.0f);
	object.WorldToLocal[2] = float4(vector4.xyz, 0.0f);
	object.WorldToLocal[3] = float4(vector2.w, vector3.w, vector4.w, 1.0f);
	object.Extent = vector5.xyz;
	uint vector1x = asuint(vector1.x);
	uint vector1y = asuint(vector1.y);
	uint vector1z = asuint(vector1.z);
	object.DataSize = asuint(vector1.w);
	object.TileOffsets[0] = vector1x & 0xffff;
	object.TileOffsets[1] = vector1x >> 16;
	object.TileOffsets[2] = vector1y & 0xffff;
	object.TileOffsets[3] = vector1y >> 16;
	object.TileOffsets[4] = vector1z & 0xffff;
	object.TileOffsets[5] = vector1z >> 16;
	return object;
}

GlobalSurfaceTile LoadGlobalSurfaceAtlasTile(Buffer<float4> objects, uint tileAddress)
{
	// This must match C++
	float4 vector0 = objects.Load(tileAddress + 0);
	float4 vector1 = objects.Load(tileAddress + 1);
	float4 vector2 = objects.Load(tileAddress + 2);
	float4 vector3 = objects.Load(tileAddress + 3);
	float4 vector4 = objects.Load(tileAddress + 4); // w unused
	GlobalSurfaceTile tile = (GlobalSurfaceTile)0;
	tile.AtlasRectUV = vector0.xyzw;
	tile.WorldToLocal[0] = float4(vector1.xyz, 0.0f);
	tile.WorldToLocal[1] = float4(vector2.xyz, 0.0f);
	tile.WorldToLocal[2] = float4(vector3.xyz, 0.0f);
	tile.WorldToLocal[3] = float4(vector1.w, vector2.w, vector3.w, 1.0f);
	tile.ViewBoundsSize = vector4.xyz;
	return tile;
}

// Global Surface Atlas data for a constant buffer
struct GlobalSurfaceAtlasData
{
	float3 ViewPos;
	float Padding0;
	float Padding1;
	float Resolution;
	float ChunkSize;
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
	float2 tileUV = saturate((tilePosition.xy / tile.ViewBoundsSize.xy) + 0.5f);
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
float4 SampleGlobalSurfaceAtlas(const GlobalSurfaceAtlasData data, ByteAddressBuffer chunks, Buffer<float4> culledObjects, Texture2D depth, Texture2D atlas, float3 worldPosition, float3 worldNormal)
{
	float4 result = float4(0, 0, 0, 0);
	float surfaceThreshold = 20.0f; // Additional threshold between object or tile size compared with input data (error due to SDF or LOD incorrect appearance)

	// Snap to the closest chunk to get culled objects
	uint3 chunkCoord = (uint3)clamp(floor((worldPosition - data.ViewPos) / data.ChunkSize + (GLOBAL_SURFACE_ATLAS_CHUNKS_RESOLUTION * 0.5f)), 0, GLOBAL_SURFACE_ATLAS_CHUNKS_RESOLUTION - 1);
	uint chunkAddress = (chunkCoord.z * (GLOBAL_SURFACE_ATLAS_CHUNKS_RESOLUTION * GLOBAL_SURFACE_ATLAS_CHUNKS_RESOLUTION) + chunkCoord.y * GLOBAL_SURFACE_ATLAS_CHUNKS_RESOLUTION + chunkCoord.x) * 4;
	uint objectsStart = chunks.Load(chunkAddress);
	if (objectsStart == 0)
	{
		// Empty chunk
		return result;
	}

	// Read objects counter
	float4 chunkHeader = culledObjects[objectsStart];
	objectsStart++;
	uint objectsCount = asuint(chunkHeader.x);

	// Loop over culled objects inside the chunk
	LOOP
	for (uint objectIndex = 0; objectIndex < objectsCount; objectIndex++)
	{
		// Cull point vs sphere
		uint objectAddress = objectsStart;
		float4 objectBounds = LoadGlobalSurfaceAtlasObjectBounds(culledObjects, objectAddress);
		uint objectSize = LoadGlobalSurfaceAtlasObjectDataSize(culledObjects, objectAddress);
		objectsStart += objectSize;
		if (distance(objectBounds.xyz, worldPosition) > objectBounds.w)
			continue;
		GlobalSurfaceObject object = LoadGlobalSurfaceAtlasObject(culledObjects, objectAddress);
		float3 localPosition = mul(float4(worldPosition, 1), object.WorldToLocal).xyz;
		float3 localExtent = object.Extent + surfaceThreshold;
		if (any(localPosition > localExtent) || any(localPosition < -localExtent))
			continue;

		// Sample tiles based on the directionality
		float3 localNormal = normalize(mul(worldNormal, (float3x3)object.WorldToLocal));
		float3 localNormalSq = localNormal * localNormal;
		uint tileOffset = object.TileOffsets[localNormal.x > 0.0f ? 0 : 1];
		if (localNormalSq.x > GLOBAL_SURFACE_ATLAS_TILE_NORMAL_THRESHOLD * GLOBAL_SURFACE_ATLAS_TILE_NORMAL_THRESHOLD && tileOffset != 0)
		{
			GlobalSurfaceTile tile = LoadGlobalSurfaceAtlasTile(culledObjects, objectAddress + tileOffset);
			result += SampleGlobalSurfaceAtlasTile(data, tile, depth, atlas, worldPosition, worldNormal, surfaceThreshold);
		}
		tileOffset = object.TileOffsets[localNormal.y > 0.0f ? 2 : 3];
		if (localNormalSq.y > GLOBAL_SURFACE_ATLAS_TILE_NORMAL_THRESHOLD * GLOBAL_SURFACE_ATLAS_TILE_NORMAL_THRESHOLD && tileOffset != 0)
		{
			GlobalSurfaceTile tile = LoadGlobalSurfaceAtlasTile(culledObjects, objectAddress + tileOffset);
			result += SampleGlobalSurfaceAtlasTile(data, tile, depth, atlas, worldPosition, worldNormal, surfaceThreshold);
		}
		tileOffset = object.TileOffsets[localNormal.z > 0.0f ? 4 : 5];
		if (localNormalSq.z > GLOBAL_SURFACE_ATLAS_TILE_NORMAL_THRESHOLD * GLOBAL_SURFACE_ATLAS_TILE_NORMAL_THRESHOLD && tileOffset != 0)
		{
			GlobalSurfaceTile tile = LoadGlobalSurfaceAtlasTile(culledObjects, objectAddress + tileOffset);
			result += SampleGlobalSurfaceAtlasTile(data, tile, depth, atlas, worldPosition, worldNormal, surfaceThreshold);
		}
	}

	// Normalize result
	result.rgb /= max(result.a, 0.0001f);

	return result;
}
