// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"
#include "./Flax/Collisions.hlsl"

// This must match C++
#define GLOBAL_SURFACE_ATLAS_OBJECT_SIZE 5 // Amount of float4s per-object

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
	float4x4 WorldToLocal;
	float3 Extent;
	GlobalSurfaceTile Tiles[6];
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
	float4 vector4 = objects.Load(objectStart + 4);
	GlobalSurfaceObject object = (GlobalSurfaceObject)0;
	object.BoundsPosition = vector0.xyz;
	object.BoundsRadius = vector0.w;
	object.WorldToLocal[0] = float4(vector1.xyz, 0.0f);
	object.WorldToLocal[1] = float4(vector2.xyz, 0.0f);
	object.WorldToLocal[2] = float4(vector3.xyz, 0.0f);
	object.WorldToLocal[3] = float4(vector1.w, vector2.w, vector3.w, 1.0f);
	object.Extent = vector4.xyz;
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
float4 SampleGlobalSurfaceAtlas(const GlobalSurfaceAtlasData data, Texture2D atlas, Buffer<float4> objects, float3 worldPosition, float3 worldNormal)
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
		float3 localPosition = mul(float4(worldPosition, 1), object.WorldToLocal).xyz;
		object.Extent += 10.0f; // TODO: why SDF is so enlarged compared to actual bounds?
		if (any(localPosition > object.Extent) || any(localPosition < -object.Extent))
			continue;
		float3 localNormal = normalize(mul(worldNormal, (float3x3)object.WorldToLocal));

		// TODO: select 1, 2 or 3 tiles from object that match normal vector
		// TODO: sample tiles with weight based on sample normal (reject tile if projected UVs are outside 0-1 range)

		// TODO: implement Global Surface Atlas sampling
		result = float4((float)(objectIndex + 1) / (float)data.ObjectsCount, 0, 0, 1);
	}
	return result;
}
