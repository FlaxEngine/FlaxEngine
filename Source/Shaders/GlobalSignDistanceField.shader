// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"
#include "./Flax/Math.hlsl"
#include "./Flax/GlobalSignDistanceField.hlsl"

#define GLOBAL_SDF_RASTERIZE_MODEL_MAX_COUNT 28
#define GLOBAL_SDF_RASTERIZE_GROUP_SIZE 8
#define GLOBAL_SDF_MIP_GROUP_SIZE 4

struct ModelRasterizeData
{
	float4x4 WorldToVolume; // TODO: use 3x4 matrix
	float4x4 VolumeToWorld; // TODO: use 3x4 matrix
	float3 VolumeToUVWMul;
	float MipOffset;
	float3 VolumeToUVWAdd;
	float MaxDistance;
	float3 VolumeLocalBoundsExtent;
	float Padding0;
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
int ModelsCount;
float3 CascadeCoordToPosAdd;
int CascadeResolution;
float2 Padding0;
int CascadeMipResolution;
int CascadeMipFactor;
uint4 Models[GLOBAL_SDF_RASTERIZE_MODEL_MAX_COUNT / 4];
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

#if defined(_CS_RasterizeModel)

RWTexture3D<float> GlobalSDFTex : register(u0);
StructuredBuffer<ModelRasterizeData> ModelsBuffer : register(t0);
Texture3D<float> ModelSDFTex[GLOBAL_SDF_RASTERIZE_MODEL_MAX_COUNT] : register(t1);

float DistanceToModelSDF(float minDistance, ModelRasterizeData modelData, Texture3D<float> modelSDFTex, float3 worldPos)
{
	// Compute SDF volume UVs and distance in world-space to the volume bounds
	float3 volumePos = mul(float4(worldPos, 1), modelData.WorldToVolume).xyz;
	float3 volumeUV = volumePos * modelData.VolumeToUVWMul + modelData.VolumeToUVWAdd;
	float3 volumePosClamped = clamp(volumePos, -modelData.VolumeLocalBoundsExtent, modelData.VolumeLocalBoundsExtent);
	float3 worldPosClamped = mul(float4(volumePosClamped, 1), modelData.VolumeToWorld).xyz;
	float distanceToVolume = distance(worldPos, worldPosClamped);

	// Skip sampling SDF if there is already a better result
	BRANCH if (minDistance <= distanceToVolume) return distanceToVolume;

	// Sample SDF
	float volumeDistance = modelSDFTex.SampleLevel(SamplerLinearClamp, volumeUV, modelData.MipOffset).x * modelData.MaxDistance;

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
void CS_RasterizeModel(uint3 GroupId : SV_GroupID, uint3 DispatchThreadId : SV_DispatchThreadID, uint3 GroupThreadId : SV_GroupThreadID)
{
	uint3 voxelCoord = ChunkCoord + DispatchThreadId;
	float3 voxelWorldPos = voxelCoord * CascadeCoordToPosMul + CascadeCoordToPosAdd;
	float minDistance = MaxDistance;
#if READ_SDF
	minDistance *= GlobalSDFTex[voxelCoord];
#endif
	for (int i = 0; i < ModelsCount; i++)
	{
		ModelRasterizeData modelData = ModelsBuffer[Models[i / 4][i % 4]];
		float modelDistance = DistanceToModelSDF(minDistance, modelData, ModelSDFTex[i], voxelWorldPos);
		minDistance = min(minDistance, modelDistance);
	}
	GlobalSDFTex[voxelCoord] = saturate(minDistance / MaxDistance);
}

#endif

#if defined(_CS_ClearChunk)

RWTexture3D<float> GlobalSDFTex : register(u0);

// Compute shader for clearing Global SDF chunk
META_CS(true, FEATURE_LEVEL_SM5)
[numthreads(GLOBAL_SDF_RASTERIZE_GROUP_SIZE, GLOBAL_SDF_RASTERIZE_GROUP_SIZE, GLOBAL_SDF_RASTERIZE_GROUP_SIZE)]
void CS_ClearChunk(uint3 GroupId : SV_GroupID, uint3 DispatchThreadId : SV_DispatchThreadID, uint3 GroupThreadId : SV_GroupThreadID)
{
	uint3 voxelCoord = ChunkCoord + DispatchThreadId;
	GlobalSDFTex[voxelCoord] = 1.0f;
}

#endif

#if defined(_CS_GenerateMip)

RWTexture3D<float> GlobalSDFMip : register(u0);
Texture3D<float> GlobalSDFTex : register(t0);

float SampleSDF(uint3 voxelCoordMip, int3 offset)
{
#if SAMPLE_MIP
	// Sampling Global SDF Mip
	float resolution = CascadeMipResolution;
#else
	// Sampling Global SDF Tex
	voxelCoordMip *= CascadeMipFactor;
	float resolution = CascadeResolution;
#endif

	// Sample SDF
	voxelCoordMip = (uint3)clamp((int3)voxelCoordMip + offset, 0, resolution - 1);
	float result = GlobalSDFTex[voxelCoordMip].r;

	// Extend by distance to the sampled texel location
	float distanceInWorldUnits = length(offset) * (MaxDistance / resolution);
	float distanceToVoxel = distanceInWorldUnits / MaxDistance;
	result = CombineDistanceToSDF(result, distanceToVoxel);

	return result;
}

// Compute shader for generating mip for Global SDF (uses flood fill algorithm)
META_CS(true, FEATURE_LEVEL_SM5)
META_PERMUTATION_1(SAMPLE_MIP=0)
META_PERMUTATION_1(SAMPLE_MIP=1)
[numthreads(GLOBAL_SDF_MIP_GROUP_SIZE, GLOBAL_SDF_MIP_GROUP_SIZE, GLOBAL_SDF_MIP_GROUP_SIZE)]
void CS_GenerateMip(uint3 GroupId : SV_GroupID, uint3 DispatchThreadId : SV_DispatchThreadID, uint3 GroupThreadId : SV_GroupThreadID)
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

	GlobalSDFMip[voxelCoordMip] = minDistance;
}

#endif

#ifdef _PS_Debug

Texture3D<float> GlobalSDFTex[4] : register(t0);
Texture3D<float> GlobalSDFMip[4] : register(t4);

// Pixel shader for Global SDF debug drawing
META_PS(true, FEATURE_LEVEL_SM5)
float4 PS_Debug(Quad_VS2PS input) : SV_Target
{
#if 0
	// Preview Global SDF slice
	float zSlice = 0.6f;
	float mip = 0;
	uint cascade = 0;
	float distance01 = GlobalSDFTex[cascade].SampleLevel(SamplerLinearClamp, float3(input.TexCoord, zSlice), mip).x;
	//float distance01 = GlobalSDFMip[cascade].SampleLevel(SamplerLinearClamp, float3(input.TexCoord, zSlice), mip).x;
	float distance = distance01 * GlobalSDF.CascadePosDistance[cascade].w;
	if (abs(distance) < 1)
		return float4(1, 0, 0, 1);
	if (distance01 < 0)
		return float4(0, 0, 1 - distance01, 1);
	return float4(0, 1 - distance01, 0, 1);
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
	return float4(color, 1);
}

#endif
