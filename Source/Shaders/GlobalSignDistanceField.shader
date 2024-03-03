// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"
#include "./Flax/Math.hlsl"
#include "./Flax/GlobalSignDistanceField.hlsl"

#define GLOBAL_SDF_RASTERIZE_MODEL_MAX_COUNT 28
#define GLOBAL_SDF_RASTERIZE_HEIGHTFIELD_MAX_COUNT 2
#define GLOBAL_SDF_RASTERIZE_GROUP_SIZE 8
#define GLOBAL_SDF_MIP_GROUP_SIZE 4

struct ObjectRasterizeData
{
	float4x4 WorldToVolume; // TODO: use 3x4 matrix
	float4x4 VolumeToWorld; // TODO: use 3x4 matrix
	float3 VolumeToUVWMul;
	float MipOffset;
	float3 VolumeToUVWAdd;
	float DecodeMul;
	float3 VolumeLocalBoundsExtent;
	float DecodeAdd;
};

META_CB_BEGIN(0, Data)
float3 ViewWorldPos;
float ViewNearPlane;
float3 Padding00;
float ViewFarPlane;
float4 ViewFrustumWorldRays[4];
GlobalSDFData GlobalSDF;
META_CB_END

META_CB_BEGIN(1, ModelsRasterizeData)
int3 ChunkCoord;
float MaxDistance;
float3 CascadeCoordToPosMul;
uint ObjectsCount;
float3 CascadeCoordToPosAdd;
int CascadeResolution;
int CascadeIndex;
float CascadeVoxelSize;
int CascadeMipResolution;
int CascadeMipFactor;
uint4 Objects[GLOBAL_SDF_RASTERIZE_MODEL_MAX_COUNT / 4];
uint GenerateMipTexResolution;
uint GenerateMipCoordScale;
uint GenerateMipTexOffsetX;
uint GenerateMipMipOffsetX;
META_CB_END

float CombineDistanceToSDF(float sdf, float distanceToSDF)
{
	// Simple sum (aprox)
	//return sdf + distanceToSDF; 

	// Negative distinace inside the SDF
	if (sdf <= 0 && distanceToSDF <= 0) return sdf;

	// Worst-case scenario with triangle edge (C^2 = A^2 + B^2)
	return sqrt(Square(max(sdf, 0)) + Square(distanceToSDF)); 
}

#if defined(_CS_RasterizeModel) || defined(_CS_RasterizeHeightfield)

RWTexture3D<float> GlobalSDFTex : register(u0);
StructuredBuffer<ObjectRasterizeData> ObjectsBuffer : register(t0);

#endif

#if defined(_CS_RasterizeModel)

Texture3D<float> ObjectsTextures[GLOBAL_SDF_RASTERIZE_MODEL_MAX_COUNT] : register(t1);

float DistanceToModelSDF(float minDistance, ObjectRasterizeData modelData, Texture3D<float> modelSDFTex, float3 worldPos)
{
    // Object scaling is the length of the rows
    float3 volumeToWorldScale = float3(length(modelData.VolumeToWorld[0]), length(modelData.VolumeToWorld[1]), length(modelData.VolumeToWorld[2]));
    float volumeScale = min(volumeToWorldScale.x, min(volumeToWorldScale.y, volumeToWorldScale.z));

	// Compute SDF volume UVs and distance in world-space to the volume bounds
	float3 volumePos = mul(float4(worldPos, 1), modelData.WorldToVolume).xyz;
	float3 volumeUV = volumePos * modelData.VolumeToUVWMul + modelData.VolumeToUVWAdd;
	float3 volumePosClamped = clamp(volumePos, -modelData.VolumeLocalBoundsExtent, modelData.VolumeLocalBoundsExtent);
	float3 worldPosClamped = mul(float4(volumePosClamped, 1), modelData.VolumeToWorld).xyz;
	float distanceToVolume = distance(worldPos, worldPosClamped);
	if (distanceToVolume < 0.01f)
	    distanceToVolume = length((volumePos - volumePosClamped) * volumeToWorldScale);
    distanceToVolume = max(distanceToVolume, 0);

	// Skip sampling SDF if there is already a better result
	BRANCH if (minDistance <= distanceToVolume) return distanceToVolume;

	// Sample SDF
#if defined(PLATFORM_PS4) || defined(PLATFORM_PS5)
    float volumeDistance = 0; // TODO: fix shader compilation error
#else
	float volumeDistance = modelSDFTex.SampleLevel(SamplerLinearClamp, volumeUV, modelData.MipOffset).x * modelData.DecodeMul + modelData.DecodeAdd;
#endif
	volumeDistance *= volumeScale; // Apply uniform instance scale (non-uniform is not supported)

	// Combine distance to the volume with distance to the surface inside the model
	float result = CombineDistanceToSDF(volumeDistance, distanceToVolume);
	if (distanceToVolume > 0)
	{
		// Prevent negative distance outside the model
		result = max(distanceToVolume, result);
	}
	return result;
}

// Compute shader for rasterizing model SDF into Global SDF
META_CS(true, FEATURE_LEVEL_SM5)
META_PERMUTATION_1(READ_SDF=0)
META_PERMUTATION_1(READ_SDF=1)
[numthreads(GLOBAL_SDF_RASTERIZE_GROUP_SIZE, GLOBAL_SDF_RASTERIZE_GROUP_SIZE, GLOBAL_SDF_RASTERIZE_GROUP_SIZE)]
void CS_RasterizeModel(uint3 DispatchThreadId : SV_DispatchThreadID)
{
	uint3 voxelCoord = ChunkCoord + DispatchThreadId;
	float3 voxelWorldPos = voxelCoord * CascadeCoordToPosMul + CascadeCoordToPosAdd;
	voxelCoord.x += CascadeIndex * CascadeResolution;
	float minDistance = MaxDistance;
#if READ_SDF
	minDistance *= GlobalSDFTex[voxelCoord];
#endif
	for (uint i = 0; i < ObjectsCount; i++)
	{
		ObjectRasterizeData objectData = ObjectsBuffer[Objects[i / 4][i % 4]];
		float objectDistance = DistanceToModelSDF(minDistance, objectData, ObjectsTextures[i], voxelWorldPos);
		minDistance = min(minDistance, objectDistance);
	}
	GlobalSDFTex[voxelCoord] = clamp(minDistance / MaxDistance, -1, 1);
}

#endif

#if defined(_CS_RasterizeHeightfield)

Texture2D<float4> ObjectsTextures[GLOBAL_SDF_RASTERIZE_HEIGHTFIELD_MAX_COUNT] : register(t1);

// Compute shader for rasterizing heightfield into Global SDF
META_CS(true, FEATURE_LEVEL_SM5)
[numthreads(GLOBAL_SDF_RASTERIZE_GROUP_SIZE, GLOBAL_SDF_RASTERIZE_GROUP_SIZE, GLOBAL_SDF_RASTERIZE_GROUP_SIZE)]
void CS_RasterizeHeightfield(uint3 DispatchThreadId : SV_DispatchThreadID)
{
	uint3 voxelCoord = ChunkCoord + DispatchThreadId;
	float3 voxelWorldPos = voxelCoord * CascadeCoordToPosMul + CascadeCoordToPosAdd;
	voxelCoord.x += CascadeIndex * CascadeResolution;
	float minDistance = MaxDistance * GlobalSDFTex[voxelCoord];
	float thickness = CascadeVoxelSize * -8;
	for (uint i = 0; i < ObjectsCount; i++)
	{
		ObjectRasterizeData objectData = ObjectsBuffer[Objects[i / 4][i % 4]];

		// Convert voxel world-space position into heightfield local-space position and get heightfield UV
		float3 volumePos = mul(float4(voxelWorldPos, 1), objectData.WorldToVolume).xyz;
		float3 volumeUV = volumePos * objectData.VolumeToUVWMul + objectData.VolumeToUVWAdd;
		float2 heightfieldUV = float2(volumeUV.x, volumeUV.z);

		// Sample the heightfield
#if defined(PLATFORM_PS4) || defined(PLATFORM_PS5)
        float4 heightmapValue = 0; // TODO: fix shader compilation error
#else
		float4 heightmapValue = ObjectsTextures[i].SampleLevel(SamplerLinearClamp, heightfieldUV, objectData.MipOffset);
#endif
		bool isHole = (heightmapValue.b + heightmapValue.a) >= 1.9f;
		if (isHole || any(heightfieldUV < 0.0f) || any(heightfieldUV > 1.0f))
			continue;
		float height = (float)((int)(heightmapValue.x * 255.0) + ((int)(heightmapValue.y * 255) << 8)) / 65535.0;
		float2 positionXZ = volumePos.xz;
		float3 position = float3(positionXZ.x, height, positionXZ.y);
		float3 heightfieldPosition = mul(float4(position, 1), objectData.VolumeToWorld).xyz;
		float3 heightfieldNormal = normalize(float3(objectData.VolumeToWorld[0].y, objectData.VolumeToWorld[1].y, objectData.VolumeToWorld[2].y));

		// Calculate distance from voxel center to the heightfield
		float objectDistance = dot(heightfieldNormal, voxelWorldPos - heightfieldPosition);
		if (objectDistance < thickness)
			objectDistance = thickness - objectDistance;
		minDistance = min(minDistance, objectDistance);
	}
	GlobalSDFTex[voxelCoord] = clamp(minDistance / MaxDistance, -1, 1);
}

#endif

#if defined(_CS_ClearChunk)

RWTexture3D<float> GlobalSDFTex : register(u0);

// Compute shader for clearing Global SDF chunk
META_CS(true, FEATURE_LEVEL_SM5)
[numthreads(GLOBAL_SDF_RASTERIZE_GROUP_SIZE, GLOBAL_SDF_RASTERIZE_GROUP_SIZE, GLOBAL_SDF_RASTERIZE_GROUP_SIZE)]
void CS_ClearChunk(uint3 DispatchThreadId : SV_DispatchThreadID)
{
	uint3 voxelCoord = ChunkCoord + DispatchThreadId;
	voxelCoord.x += CascadeIndex * CascadeResolution;
	GlobalSDFTex[voxelCoord] = 1.0f;
}

#endif

#if defined(_CS_GenerateMip)

RWTexture3D<float> GlobalSDFMip : register(u0);
Texture3D<float> GlobalSDFTex : register(t0);

float SampleSDF(uint3 voxelCoordMip, int3 offset)
{
	// Sample SDF
	voxelCoordMip = (uint3)clamp((int3)(voxelCoordMip * GenerateMipCoordScale) + offset, int3(0, 0, 0), (int3)(GenerateMipTexResolution - 1));
	voxelCoordMip.x += GenerateMipTexOffsetX;
	float result = GlobalSDFTex[voxelCoordMip].r;

	// Extend by distance to the sampled texel location
	float distanceInWorldUnits = length((float3)offset) * (MaxDistance / (float)GenerateMipTexResolution);
	float distanceToVoxel = distanceInWorldUnits / MaxDistance;
	result = CombineDistanceToSDF(result, distanceToVoxel);

	return result;
}

// Compute shader for generating mip for Global SDF (uses flood fill algorithm)
META_CS(true, FEATURE_LEVEL_SM5)
[numthreads(GLOBAL_SDF_MIP_GROUP_SIZE, GLOBAL_SDF_MIP_GROUP_SIZE, GLOBAL_SDF_MIP_GROUP_SIZE)]
void CS_GenerateMip(uint3 DispatchThreadId : SV_DispatchThreadID)
{
	uint3 voxelCoordMip = DispatchThreadId;
	float minDistance = SampleSDF(voxelCoordMip, int3(0, 0, 0));

	// Find the distance to the closest surface by sampling the nearby area (flood fill)
	minDistance = min(minDistance, SampleSDF(voxelCoordMip, int3(1, 0, 0)));
	minDistance = min(minDistance, SampleSDF(voxelCoordMip, int3(0, 1, 0)));
	minDistance = min(minDistance, SampleSDF(voxelCoordMip, int3(0, 0, 1)));
	minDistance = min(minDistance, SampleSDF(voxelCoordMip, int3(-1, 0, 0)));
	minDistance = min(minDistance, SampleSDF(voxelCoordMip, int3(0, -1, 0)));
	minDistance = min(minDistance, SampleSDF(voxelCoordMip, int3(0, 0, -1)));

	voxelCoordMip.x += GenerateMipMipOffsetX;
	GlobalSDFMip[voxelCoordMip] = minDistance;
}

#endif

#ifdef _PS_Debug

Texture3D<float> GlobalSDFTex : register(t0);
Texture3D<float> GlobalSDFMip : register(t1);

// Pixel shader for Global SDF debug drawing
META_PS(true, FEATURE_LEVEL_SM5)
float4 PS_Debug(Quad_VS2PS input) : SV_Target
{
#if 0
	// Preview Global SDF slice
	float zSlice = 0.6f;
	float mip = 0;
	uint cascade = 0;
	float distance01 = GlobalSDFTex.SampleLevel(SamplerLinearClamp, float3(input.TexCoord, zSlice), mip).x;
	//float distance01 = GlobalSDFTex.SampleLevel(SamplerLinearClamp, float3((input.TexCoord.x + cascade) / (float)GlobalSDF.CascadesCount, input.TexCoord.y, zSlice), mip).x;
	//float distance01 = GlobalSDFMip.SampleLevel(SamplerLinearClamp, float3(input.TexCoord, zSlice), mip).x;
	float distance = distance01 * GlobalSDF.CascadePosDistance[cascade].w;
	if (abs(distance) < 1)
		return float4(1, 0, 0, 1);
	if (distance01 < 0)
		return float4(0, 0, 1 - distance01, 1);
	return float4(0, 1 - distance01, 0, 1);
#endif

#if 1
    // Debug negative SDF (inside geometry)
    float viewSDF = SampleGlobalSDF(GlobalSDF, GlobalSDFTex, GlobalSDFMip, ViewWorldPos);
    if (viewSDF < 0)
		return float4(float3(0.7, 0.4, 0.1) * saturate(viewSDF * -0.005f), 1);
#endif

	// Shot a ray from camera into the Global SDF
	GlobalSDFTrace trace;
	float3 viewRay = lerp(lerp(ViewFrustumWorldRays[3], ViewFrustumWorldRays[0], input.TexCoord.x), lerp(ViewFrustumWorldRays[2], ViewFrustumWorldRays[1], input.TexCoord.x), 1 - input.TexCoord.y).xyz;
	viewRay = normalize(viewRay - ViewWorldPos);
	trace.Init(ViewWorldPos, viewRay, ViewNearPlane, ViewFarPlane);
	GlobalSDFHit hit = RayTraceGlobalSDF(GlobalSDF, GlobalSDFTex, GlobalSDFMip, trace);

	// Debug draw
	float3 color = saturate(hit.StepsCount / 80.0f).xxx;
	if (!hit.IsHit())
		color.rg *= 0.4f;
#if 0
	else
	{
		// Debug draw SDF normals
		float dst;
		color.rgb = normalize(SampleGlobalSDFGradient(GlobalSDF, GlobalSDFTex, hit.GetHitPosition(trace), dst)) * 0.5f + 0.5f;
	}
#endif
	return float4(color, 1);
}

#endif
