// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"
#include "./Flax/Collisions.hlsl"

// This must match C++
#define GLOBAL_SURFACE_ATLAS_OBJECT_STRIDE (16 * 1)

struct GlobalSurfaceTile
{
	uint Index;
	uint2 AtlasCoord;
	bool Enabled;
};

struct GlobalSurfaceObject
{
	float3 BoundsPosition;
	float BoundsRadius;
	float3x3 InvRotation;
	float3 BoundsMin;
	float3 BoundsMax;
	GlobalSurfaceTile Tiles[6];
};

float4 LoadGlobalSurfaceAtlasObjectBounds(Buffer<float4> objects, uint objectIndex)
{
	// This must match C++
	const uint objectStart = objectIndex * GLOBAL_SURFACE_ATLAS_OBJECT_STRIDE;
	return objects.Load(objectStart);
}

GlobalSurfaceObject LoadGlobalSurfaceAtlasObject(Buffer<float4> objects, uint objectIndex)
{
	// This must match C++
	const uint objectStart = objectIndex * GLOBAL_SURFACE_ATLAS_OBJECT_STRIDE;
	float4 vector0 = objects.Load(objectStart + 0);
	GlobalSurfaceObject object = (GlobalSurfaceObject)0;
	object.BoundsPosition = vector0.xyz;
	object.BoundsRadius = vector0.w;
	// TODO: InvRotation
	// TODO: BoundsMin
	// TODO: BoundsMax
	// TODO: Tiles
	return object;
}

// Global Surface Atlas data for a constant buffer
struct GlobalSurfaceAtlasData
{
	float3 Padding;
	uint ObjectsCount;
};

// Samples the Global Surface Atlas and returns the lighting (with opacity) at the given world location (and direction).
float4 SampleGlobalSurfaceAtlas(const GlobalSurfaceAtlasData data, Texture3D<float4> atlas, Buffer<float4> objects, float3 worldPosition, float3 worldNormal)
{
	float4 result = float4(0, 0, 0, 0);
	// TODO: add grid culling to object for faster lookup
	LOOP
	for (uint objectIndex = 0; objectIndex < data.ObjectsCount && result.a <= 0.0f; objectIndex++)
	{
		// Cull point vs sphere
		float4 objectBounds = LoadGlobalSurfaceAtlasObjectBounds(objects, objectIndex);
		if (distance(objectBounds.xyz, worldPosition) > objectBounds.w)
			continue;
		GlobalSurfaceObject object = LoadGlobalSurfaceAtlasObject(objects, objectIndex);

		// TODO: project worldPosition and worldNormal into object-space
		// TODO: select 1, 2 or 3 tiles from object that match normal vector
		// TODO: sample tiles with weight based on sample normal (reject tile if projected UVs are outside 0-1 range)

		// TODO: implement Global Surface Atlas sampling
		result = float4((objectIndex + 1) / data.ObjectsCount, 0, 0, 1);
	}
	return result;
}
