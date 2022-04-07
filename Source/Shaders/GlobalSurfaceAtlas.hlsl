// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"
#include "./Flax/Collisions.hlsl"

// This must match C++
#define GLOBAL_SURFACE_ATLAS_OBJECT_SIZE (5 + 6 * 5) // Amount of float4s per-object
#define GLOBAL_SURFACE_ATLAS_TILE_NORMAL_THRESHOLD 0.3f // Cut-off value for tiles transitions blending during sampling

struct GlobalSurfaceTile
{
	float4 AtlasRect;
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
	tile.AtlasRect = vector0.xyzw;
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
	float3 Padding;
	uint ObjectsCount;
};

// Samples the Global Surface Atlas and returns the lighting (with opacity) at the given world location (and direction).
float4 SampleGlobalSurfaceAtlas(const GlobalSurfaceAtlasData data, Buffer<float4> objects, Texture2D depth, Texture2D atlas, float3 worldPosition, float3 worldNormal)
{
	float4 result = float4(0, 0, 0, 0);
	float surfaceThreshold = 10.0f; // Additional threshold between object or tile size compared with input data (error due to SDF or LOD incorrect appearance)
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
		float3 localNormal = normalize(mul(worldNormal, (float3x3)object.WorldToLocal));

		// Pick tiles to sample based on the directionality
		// TODO: sample 1/2/3 tiles with weight based on sample normal
		uint tileIndex = 2;

		GlobalSurfaceTile tile = LoadGlobalSurfaceAtlasTile(objects, objectIndex, tileIndex);

		// Tile normal weight based on the sampling angle
		float3 tileNormal = normalize(mul(worldNormal, (float3x3)tile.WorldToLocal));
		float normalWeight = saturate(dot(float3(0, 0, -1), tileNormal));
		normalWeight = (normalWeight - GLOBAL_SURFACE_ATLAS_TILE_NORMAL_THRESHOLD) / (1.0f - GLOBAL_SURFACE_ATLAS_TILE_NORMAL_THRESHOLD);
		if (normalWeight <= 0.0f)
			continue;

		// Get tile UV and depth at the world position
		float3 tilePosition = mul(float4(worldPosition, 1), tile.WorldToLocal).xyz;
		float tileDepth = tilePosition.z / tile.ViewBoundsSize.z;
		float2 tileUV = (tilePosition.xy / tile.ViewBoundsSize.xy) + 0.5f;
		tileUV.y = 1.0 - tileUV.y;
		float2 atlasCoord = tileUV * tile.AtlasRect.zw + tile.AtlasRect.xy;

		// Tile depth weight based on sample position occlusion
		// TODO: gather 4 depth samples to smooth weight (depth weight per-sample used late for bilinear weights)
		float tileZ = depth.Load(int3(atlasCoord, 0)).x;
		float depthThreshold = 2.0f * surfaceThreshold / tile.ViewBoundsSize.z;
		float depthWeight = 1.0f - saturate((abs(tileDepth - tileZ) - depthThreshold) / (0.5f * depthThreshold));
		if (depthWeight <= 0.0f)
			continue;

		// Sample atlas texture
		// TODO: separate GatherRed/Blue/Green with bilinear weights
		float4 color = atlas.Load(int3(atlasCoord, 0));

		// TODO: implement Global Surface Atlas sampling
		result = float4(color.rgb, 1);
	}
	return result;
}
