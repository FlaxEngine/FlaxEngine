// Copyright (c) Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"
#include "./Flax/Math.hlsl"
#include "./Flax/Noise.hlsl"
#include "./Flax/GlobalSignDistanceField.hlsl"
#include "./Flax/TerrainCommon.hlsl"

#define GLOBAL_SDF_RASTERIZE_MODEL_MAX_COUNT 28
#define GLOBAL_SDF_RASTERIZE_HEIGHTFIELD_MAX_COUNT 2
#define GLOBAL_SDF_RASTERIZE_GROUP_SIZE 8
#define GLOBAL_SDF_MIP_GROUP_SIZE 4

struct ObjectRasterizeData
{
	float4x3 WorldToVolume;
	float4x3 VolumeToWorld;
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
float2 Padding20;
float MipMaxDistanceLoad;
float MipMaxDistanceStore;
uint MipTexResolution;
uint MipCoordScale;
uint MipTexOffsetX;
uint MipMipOffsetX;
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

float CombineSDF(float oldSdf, float newSdf)
{
    // Use distance closer to 0
    if (oldSdf < 0 && newSdf < 0)
        return max(oldSdf, newSdf);
    return min(oldSdf, newSdf);
}

#if defined(_CS_RasterizeModel) || defined(_CS_RasterizeHeightfield)

RWTexture3D<snorm float> GlobalSDFTex : register(u0);
StructuredBuffer<ObjectRasterizeData> ObjectsBuffer : register(t0);

#endif

#if defined(_CS_RasterizeModel)

Texture3D<float> ObjectsTextures[GLOBAL_SDF_RASTERIZE_MODEL_MAX_COUNT] : register(t1);

float DistanceToModelSDF(float minDistance, ObjectRasterizeData modelData, Texture3D<float> modelSDFTex, float3 worldPos)
{
    // Object scaling is the length of the rows
    float4x4 volumeToWorld = ToMatrix4x4(modelData.VolumeToWorld);
    float3 volumeToWorldScale = float3(length(volumeToWorld[0]), length(volumeToWorld[1]), length(volumeToWorld[2]));
    float volumeScale = min(volumeToWorldScale.x, min(volumeToWorldScale.y, volumeToWorldScale.z));

	// Compute SDF volume UVs and distance in world-space to the volume bounds
	float3 volumePos = mul(float4(worldPos, 1), ToMatrix4x4(modelData.WorldToVolume)).xyz;
	float3 volumeUV = volumePos * modelData.VolumeToUVWMul + modelData.VolumeToUVWAdd;
	float3 volumePosClamped = clamp(volumePos, -modelData.VolumeLocalBoundsExtent, modelData.VolumeLocalBoundsExtent);
	float3 worldPosClamped = mul(float4(volumePosClamped, 1), volumeToWorld).xyz;
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
    UNROLL_N(GLOBAL_SDF_RASTERIZE_MODEL_MAX_COUNT)
	for (uint i = 0; i < ObjectsCount; i++)
	{
		ObjectRasterizeData objectData = ObjectsBuffer[Objects[i / 4][i % 4]];
		float objectDistance = DistanceToModelSDF(minDistance, objectData, ObjectsTextures[i], voxelWorldPos);
		minDistance = CombineSDF(minDistance, objectDistance);
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
#if defined(PLATFORM_PS4) || defined(PLATFORM_PS5)
    // TODO: fix shader compilation error
#else
	uint3 voxelCoord = ChunkCoord + DispatchThreadId;
	float3 voxelWorldPos = voxelCoord * CascadeCoordToPosMul + CascadeCoordToPosAdd;
	voxelCoord.x += CascadeIndex * CascadeResolution;
	float minDistance = MaxDistance * GlobalSDFTex[voxelCoord];
	float thickness = -300.0f;
    UNROLL_N(GLOBAL_SDF_RASTERIZE_HEIGHTFIELD_MAX_COUNT)
	for (uint i = 0; i < ObjectsCount; i++)
	{
		ObjectRasterizeData objectData = ObjectsBuffer[Objects[i / 4][i % 4]];

		// Convert voxel world-space position into heightfield local-space position and get heightfield UV
		float4x4 worldToLocal = ToMatrix4x4(objectData.WorldToVolume);
		float3 volumePos = mul(float4(voxelWorldPos, 1), worldToLocal).xyz;

        // Sample heightfield around the voxel location (heightmap uses point sampler)
        Texture2D<float4> heightmap = ObjectsTextures[i];
        float4 localToUV = float4(objectData.VolumeToUVWMul.xz, objectData.VolumeToUVWAdd.xz);
#if 1
        float3 n00, n10, n01, n11;
        bool h00, h10, h01, h11;
        float offset = CascadeVoxelSize;
        float3 p00 = SampleHeightmap(heightmap, volumePos + float3(-offset, 0, 0), localToUV, n00, h00, objectData.MipOffset);
        float3 p10 = SampleHeightmap(heightmap, volumePos + float3(+offset, 0, 0), localToUV, n10, h10, objectData.MipOffset);
        float3 p01 = SampleHeightmap(heightmap, volumePos + float3(0, 0, -offset), localToUV, n01, h01, objectData.MipOffset);
        float3 p11 = SampleHeightmap(heightmap, volumePos + float3(0, 0, +offset), localToUV, n11, h11, objectData.MipOffset);

        // Calculate average sample (linear interpolation)
        float3 heightfieldPosition = (p00 + p10 + p01 + p11) * 0.25f;
        float3 heightfieldNormal = (n00 + n10 + n01 + n11) * 0.25f;
        heightfieldNormal = normalize(heightfieldNormal);
        bool isHole = h00 || h10 || h01 || h11;
#else
        float3 heightfieldNormal;
        bool isHole;
        float3 heightfieldPosition = SampleHeightmap(heightmap, volumePos, localToUV, heightfieldNormal, isHole, objectData.MipOffset);
#endif

        // Skip holes and pixels outside the heightfield
	    if (isHole)
		    continue;

        // Transform to world-space
	    float4x4 localToWorld = ToMatrix4x4(objectData.VolumeToWorld);
	    heightfieldPosition = mul(float4(heightfieldPosition, 1), localToWorld).xyz;
	    // TODO: rotate normal vector
	    //heightfieldNormal = normalize(float3(localToWorld[0].y, localToWorld[1].y, localToWorld[2].y));
	    //heightfieldNormal = float3(0, 1, 0);

		// Calculate distance from voxel center to the heightfield
		float objectDistance = dot(heightfieldNormal, voxelWorldPos - heightfieldPosition);
        //objectDistance += (1.0f - saturate(dot(heightfieldNormal, float3(0, 1, 0)))) * -50.0f;
		if (objectDistance < thickness * 0.5f)
			objectDistance = thickness - objectDistance;
		minDistance = CombineSDF(minDistance, objectDistance);
	}
	GlobalSDFTex[voxelCoord] = clamp(minDistance / MaxDistance, -1, 1);
#endif
}

#endif

#if defined(_CS_ClearChunk)

RWTexture3D<snorm float> GlobalSDFTex : register(u0);

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

RWTexture3D<snorm float> GlobalSDFMip : register(u0);
Texture3D<snorm float> GlobalSDFTex : register(t0);

float SampleSDF(uint3 voxelCoordMip, int3 offset)
{
	// Sample SDF
	voxelCoordMip = (uint3)clamp((int3)(voxelCoordMip * MipCoordScale) + offset, int3(0, 0, 0), (int3)(MipTexResolution - 1));
	voxelCoordMip.x += MipTexOffsetX;
	float result = GlobalSDFTex[voxelCoordMip].r;
    if (result >= GLOBAL_SDF_MIN_VALID)
        return MipMaxDistanceStore; // No valid distance so use the limit
    result *= MipMaxDistanceLoad; // Decode normalized distance to world-units

	// Extend by distance to the sampled texel location
	float distanceToVoxel = length((float3)offset) * CascadeVoxelSize * ((float)CascadeResolution / (float)MipTexResolution);
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

	voxelCoordMip.x += MipMipOffsetX;
	GlobalSDFMip[voxelCoordMip] = clamp(minDistance / MipMaxDistanceStore, -1, 1);
}

#endif

#ifdef _PS_Debug

Texture3D<snorm float> GlobalSDFTex : register(t0);
Texture3D<snorm float> GlobalSDFMip : register(t1);

// Pixel shader for Global SDF debug drawing
META_PS(true, FEATURE_LEVEL_SM5)
META_FLAG(DevelopmentOnly)
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
    trace.WorldPosition += rand3dTo3d(float3(input.TexCoord, viewRay.x)); // Noise a bit
    float stepScale = lerp(rand3dTo1d(trace.WorldPosition), 0.7f, 0.9f); // Boost precision
	GlobalSDFHit hit = RayTraceGlobalSDF(GlobalSDF, GlobalSDFTex, GlobalSDFMip, trace, 0.0f, stepScale);

	// Debug draw
	float3 color = saturate(hit.StepsCount / 50.0f).xxx;
	if (hit.IsHit())
    {
        // Normalize step count back to default scale (higher precision is just for preview)
        hit.StepsCount *= stepScale;

#if 1
        float3 hitPosition = hit.GetHitPosition(trace);
        float hitSDF;
        float3 hitNormal = SampleGlobalSDFGradient(GlobalSDF, GlobalSDFTex, GlobalSDFMip, hitPosition, hitSDF, hit.HitCascade);
#if 1
        // Composite step count with SDF normals
		//color.rgb *= saturate(normalize(hitNormal) * 0.5f + 0.7f) + 0.3f;
		color = lerp(normalize(hitNormal) * 0.5f + 0.5f, 1 - color, saturate(hit.StepsCount / 80.0f));
#else
		// Debug draw SDF normals
		color = normalize(hitNormal) * 0.5f + 0.5f;
#endif
#else
        // Heatmap with step count
        if (hit.StepsCount > 40)
            color = float3(saturate(hit.StepsCount / 80.0f), 0, 0);
        else if (hit.StepsCount > 20)
            color = float3(saturate(hit.StepsCount / 40.0f).xx, 0);
        else
            color = float3(0, saturate(hit.StepsCount / 20.0f), 0);
#endif
    }
    else
    {
        // Bluish sky
		color.rg *= 0.4f;
    }
	return float4(color, 1);
}

#endif

#ifdef _PS_Overdraw

#define NO_GBUFFER_SAMPLING
#include "./Flax/GBuffer.hlsl"
#include "./Flax/Noise.hlsl"
#include "./Flax/Outlines.hlsl"

META_CB_BEGIN(2, OverdrawData)
GBufferData GBuffer;
float3 CascadeBoundsMin;
float CascadeBoundsResolution;
float3 CascadeBoundsMax;
float Padding30;
uint4 ChunksBuffer[512/4]; // (MaxVolumeRes/ChunkRes)^3 = (256/32)^3
META_CB_END

Texture2D Depth : register(t0);
Texture2D GBuffer0 : register(t1);

float4 GetHeatmap(float value)
{
    const float4 colors[4] = {
        float4(0, 0.97f, 0.12f, 1),
        float4(0.2f, 0.2f, 0.7f, 1),
        float4(1, 0.57f, 0, 1),
        float4(1, 0, 0, 1)
    };
    const float weights[4] = { 0.0f, 0.5f, 0.8f, 1.0f };

    float4 color;
    if (value < weights[1])
        color = lerp(colors[0], colors[1], value / weights[1]);
    else if (value < weights[2])
        color = lerp(colors[1], colors[2], (value - weights[1]) / (weights[2] - weights[1]));
    else
        color = lerp(colors[2], colors[3], (value - weights[2]) / (weights[3] - weights[2]));
    return color;
}

// Pixel shader for Global SDF overdraw drawing
META_PS(true, FEATURE_LEVEL_SM5)
META_FLAG(DevelopmentOnly)
float4 PS_Overdraw(Quad_VS2PS input) : SV_Target
{
    // Use over-exposed diffuse as a base for scene identification
    float3 diffuse = SAMPLE_RT(GBuffer0, input.TexCoord).rgb;
    diffuse = saturate(Luminance(diffuse.rgb).xxx * 0.5f + 0.6f);

    // Make depth-based outlines
    float baseDepth = SAMPLE_RT_DEPTH(Depth, input.TexCoord);
    if (DEPTH_01(baseDepth) > 0.999999f) return 0; // Skip background
    float outline = DepthOutline(Depth, input.TexCoord);

    // Get position inside chunk used by the pixel position
    float3 worldPos = GetWorldPos(GBuffer, input.TexCoord, baseDepth);
    worldPos += rand3dTo3d(float3(input.TexCoord, diffuse.x)) * 0.1f; // Noise a bit to reduce z-fighting at chunk borders
    float3 cascadeBoundsSize = CascadeBoundsMax - CascadeBoundsMin;
    float3 cascadePos = (worldPos - CascadeBoundsMin) / cascadeBoundsSize;
    cascadePos = floor(cascadePos * CascadeBoundsResolution) / CascadeBoundsResolution;
    if (any(cascadePos < 0) || any(cascadePos >= 1)) return float4(0, 0, 0, 0); // Skip pixels outside the current cascade
    //return float4(cascadePos, 1);

    // Read complexity of the Global SDF rasterization at the pixel position
    cascadePos *= CascadeBoundsResolution;
    uint i = (uint)(cascadePos.z * CascadeBoundsResolution * CascadeBoundsResolution + cascadePos.y * CascadeBoundsResolution + cascadePos.x);
    float complexity = (float)ChunksBuffer[i / 4][i % 4] / 255.0f;

    // Mix complexity with depth/diffuse to improve scene readability
    return float4(diffuse * outline, 1) * GetHeatmap(complexity);
}

#endif
