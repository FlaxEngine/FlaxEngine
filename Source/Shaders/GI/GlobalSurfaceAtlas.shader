// Copyright (c) Wojciech Figat. All rights reserved.

// Diffuse-only lighting
#define LIGHTING_NO_SPECULAR 1

#include "./Flax/Common.hlsl"
#include "./Flax/Math.hlsl"
#include "./Flax/LightingCommon.hlsl"
#include "./Flax/GlobalSignDistanceField.hlsl"
#include "./Flax/GI/GlobalSurfaceAtlas.hlsl"
#include "./Flax/GI/DDGI.hlsl"

META_CB_BEGIN(0, Data)
float3 ViewWorldPos;
float ViewNearPlane;
float SkyboxIntensity;
uint CulledObjectsCapacity;
float LightShadowsStrength;
float ViewFarPlane;
float4 ViewFrustumWorldRays[4];
GlobalSDFData GlobalSDF;
GlobalSurfaceAtlasData GlobalSurfaceAtlas;
DDGIData DDGI;
LightData Light;
META_CB_END

struct AtlasVertexInput
{
	float2 Position : POSITION0;
	float2 TileUV : TEXCOORD0;
	uint TileAddress : TEXCOORD1;
};

struct AtlasVertexOutput
{
	float4 Position : SV_Position;
	float2 TileUV : TEXCOORD0;
	nointerpolation uint TileAddress : TEXCOORD1;
};

// Vertex shader for Global Surface Atlas rendering (custom vertex buffer to render per-tile)
META_VS(true, FEATURE_LEVEL_SM5)
AtlasVertexOutput VS_Atlas(AtlasVertexInput input)
{
	AtlasVertexOutput output;
	output.Position = float4(input.Position, 1, 1);
	output.TileUV = input.TileUV;
	output.TileAddress = input.TileAddress;
	return output;
}

// Pixel shader for Global Surface Atlas software clearing
META_PS(true, FEATURE_LEVEL_SM5)
void PS_Clear(out float4 Light : SV_Target0, out float4 RT0 : SV_Target1, out float4 RT1 : SV_Target2, out float4 RT2 : SV_Target3)
{
	Light = float4(0, 0, 0, 0);
	RT0 = float4(0, 0, 0, 0);
	RT1 = float4(0, 0, 0, 0);
	RT2 = float4(1, 0, 0, 0);
}

#ifdef _PS_ClearLighting

Buffer<float4> GlobalSurfaceAtlasObjects : register(t4);
Texture2D Texture : register(t7);

// Pixel shader for Global Surface Atlas clearing
META_PS(true, FEATURE_LEVEL_SM5)
float4 PS_ClearLighting(AtlasVertexOutput input) : SV_Target
{
	GlobalSurfaceTile tile = LoadGlobalSurfaceAtlasTile(GlobalSurfaceAtlasObjects, input.TileAddress);
	float2 atlasUV = input.TileUV * tile.AtlasRectUV.zw + tile.AtlasRectUV.xy;
	return Texture.Sample(SamplerPointClamp, atlasUV);
}

#endif

#ifdef _PS_Lighting

#include "./Flax/GBuffer.hlsl"
#include "./Flax/Matrix.hlsl"
#include "./Flax/Lighting.hlsl"

// GBuffer+Depth at 0-3 slots
Buffer<float4> GlobalSurfaceAtlasObjects : register(t4);
#if INDIRECT_LIGHT
Texture2D<snorm float4> ProbesData : register(t5);
Texture2D<float4> ProbesDistance : register(t6);
Texture2D<float4> ProbesIrradiance : register(t7);
#else
Texture3D<snorm float> GlobalSDFTex : register(t5);
Texture3D<snorm float> GlobalSDFMip : register(t6);
#endif

// Pixel shader for Global Surface Atlas shading
META_PS(true, FEATURE_LEVEL_SM5)
META_PERMUTATION_1(RADIAL_LIGHT=0)
META_PERMUTATION_1(RADIAL_LIGHT=1)
META_PERMUTATION_1(INDIRECT_LIGHT=1)
float4 PS_Lighting(AtlasVertexOutput input) : SV_Target
{
	// Load current tile info
	GlobalSurfaceTile tile = LoadGlobalSurfaceAtlasTile(GlobalSurfaceAtlasObjects, input.TileAddress);
	float2 atlasUV = input.TileUV * tile.AtlasRectUV.zw + tile.AtlasRectUV.xy;

	// Load GBuffer sample from atlas
    float4 gBuffer0 = SAMPLE_RT(GBuffer0, atlasUV);
    float4 gBuffer1 = SAMPLE_RT(GBuffer1, atlasUV);
    GBufferSample gBuffer = (GBufferSample)0;
    gBuffer.Normal = DecodeNormal(gBuffer1.rgb);
    gBuffer.ShadingModel = (int)(gBuffer1.a * 3.999);
    gBuffer.Color = gBuffer0.rgb;
    gBuffer.Roughness = 1.0f;
    gBuffer.AO = gBuffer0.a;
	BRANCH
	if (gBuffer.ShadingModel == SHADING_MODEL_UNLIT)
	{
		// Skip unlit pixels
		discard;
		return 0;
	}

	// Reconstruct world-space position manually (from uv+depth within a tile)
	float tileDepth = SampleZ(atlasUV);
	float3 tileSpacePos = float3(input.TileUV.x - 0.5f, 0.5f - input.TileUV.y, tileDepth);
	float3 gBufferTilePos = tileSpacePos * tile.ViewBoundsSize;
	float4x4 tileLocalToWorld = Inverse(tile.WorldToLocal);
	gBuffer.WorldPos = mul(float4(gBufferTilePos, 1), tileLocalToWorld).xyz;

	// Boost material diffuse color to improve GI
	//gBuffer.Color *= 1.1f;

#if INDIRECT_LIGHT
    // Sample irradiance
    float3 irradiance = SampleDDGIIrradiance(DDGI, ProbesData, ProbesDistance, ProbesIrradiance, gBuffer.WorldPos, gBuffer.Normal);
    irradiance *= Light.Radius; // Cached BounceIntensity / IndirectLightingIntensity

    // Calculate lighting
    float3 diffuseColor = GetDiffuseColor(gBuffer);
    diffuseColor = min(diffuseColor, 0.9f); // Nothing reflects diffuse like perfectly in the real world (ensure to have energy loss at each light bounce)
    float3 diffuse = Diffuse_Lambert(diffuseColor);
    float4 light = float4(diffuse * irradiance * gBuffer.AO, 1);
#else
	// Calculate shadowing
	float3 L = Light.Direction;
#if RADIAL_LIGHT
	float3 toLight = Light.Position - gBuffer.WorldPos;
	float toLightDst = length(toLight);
	if (toLightDst >= Light.Radius)
	{
		// Skip texels outside the light influence range
		discard;
		return 0;
	}
	L = toLight / toLightDst;
#else
	float toLightDst = GLOBAL_SDF_WORLD_SIZE;
#endif
	float4 shadowMask = 1;
	if (Light.ShadowsBufferAddress != 0)
	{
		float NoL = dot(gBuffer.Normal, L);
		float shadowBias = 10.0f;
		float bias = 2 * shadowBias * saturate(1 - NoL) + shadowBias;
		BRANCH
		if (NoL > 0)
		{
			// Shot a ray from texel into the light to see if there is any occluder
			GlobalSDFTrace trace;
			trace.Init(gBuffer.WorldPos + gBuffer.Normal * shadowBias, L, bias, toLightDst - bias);
			GlobalSDFHit hit = RayTraceGlobalSDF(GlobalSDF, GlobalSDFTex, GlobalSDFMip, trace, 2.0f);
			shadowMask = hit.IsHit() ? LightShadowsStrength : 1;
		}
		else
		{
			shadowMask = 0;
		}
	}

	// Calculate lighting
#if RADIAL_LIGHT
	bool isSpotLight = Light.SpotAngles.x > -2.0f;
#else
	bool isSpotLight = false;
#endif
	float4 light = GetLighting(ViewWorldPos, Light, gBuffer, shadowMask, RADIAL_LIGHT, isSpotLight);
#endif

	return light;
}

#endif

#if defined(_CS_CullObjects)

#include "./Flax/Collisions.hlsl"

RWByteAddressBuffer RWGlobalSurfaceAtlasChunks : register(u0);
RWByteAddressBuffer RWGlobalSurfaceAtlasCulledObjects : register(u1);
Buffer<float4> GlobalSurfaceAtlasObjects : register(t0);
Buffer<uint> GlobalSurfaceAtlasObjectsList : register(t1);

#define GLOBAL_SURFACE_ATLAS_SHARED_CULL_SIZE 511 // Limit of objects that can be culled for a whole group of 4x4x4 threads (64 chunks)

groupshared uint SharedCulledObjectsCount;
groupshared uint SharedCulledObjects[GLOBAL_SURFACE_ATLAS_SHARED_CULL_SIZE];

// Compute shader for culling objects into chunks
META_CS(true, FEATURE_LEVEL_SM5)
[numthreads(GLOBAL_SURFACE_ATLAS_CHUNKS_GROUP_SIZE, GLOBAL_SURFACE_ATLAS_CHUNKS_GROUP_SIZE, GLOBAL_SURFACE_ATLAS_CHUNKS_GROUP_SIZE)]
void CS_CullObjects(uint3 DispatchThreadId : SV_DispatchThreadID, uint3 GroupId : SV_GroupID, uint3 GroupThreadId : SV_GroupThreadID)
{
	uint3 chunkCoord = DispatchThreadId;
	uint chunkAddress = (chunkCoord.z * (GLOBAL_SURFACE_ATLAS_CHUNKS_RESOLUTION * GLOBAL_SURFACE_ATLAS_CHUNKS_RESOLUTION) + chunkCoord.y * GLOBAL_SURFACE_ATLAS_CHUNKS_RESOLUTION + chunkCoord.x) * 4;
	float3 chunkMin = GlobalSurfaceAtlas.ViewPos + (chunkCoord - (GLOBAL_SURFACE_ATLAS_CHUNKS_RESOLUTION * 0.5f)) * GlobalSurfaceAtlas.ChunkSize;
	float3 chunkMax = chunkMin + GlobalSurfaceAtlas.ChunkSize.xxx;
	uint groupIndex = (GroupThreadId.z * GLOBAL_SURFACE_ATLAS_CHUNKS_GROUP_SIZE + GroupThreadId.y) * GLOBAL_SURFACE_ATLAS_CHUNKS_GROUP_SIZE + GroupThreadId.x;
	float3 groupMin = GlobalSurfaceAtlas.ViewPos + (GroupId * GLOBAL_SURFACE_ATLAS_CHUNKS_GROUP_SIZE - (GLOBAL_SURFACE_ATLAS_CHUNKS_RESOLUTION * 0.5f)) * GlobalSurfaceAtlas.ChunkSize;
	float3 groupMax = groupMin + (GlobalSurfaceAtlas.ChunkSize * GLOBAL_SURFACE_ATLAS_CHUNKS_GROUP_SIZE).xxx;

    // Clear shared memory
	if (groupIndex == 0)
	{
        SharedCulledObjectsCount = 0;
	}
	GroupMemoryBarrierWithGroupSync();

    // Shared culling of all objects by all threads for a whole group
	LOOP
	for (uint objectIndex = groupIndex; objectIndex < GlobalSurfaceAtlas.ObjectsCount; objectIndex += GLOBAL_SURFACE_ATLAS_CHUNKS_GROUP_SIZE * GLOBAL_SURFACE_ATLAS_CHUNKS_GROUP_SIZE * GLOBAL_SURFACE_ATLAS_CHUNKS_GROUP_SIZE)
	{
        uint objectAddress = GlobalSurfaceAtlasObjectsList.Load(objectIndex);
		float4 objectBounds = LoadGlobalSurfaceAtlasObjectBounds(GlobalSurfaceAtlasObjects, objectAddress);
		if (BoxIntersectsSphere(groupMin, groupMax, objectBounds.xyz, objectBounds.w))
		{
            uint sharedIndex;
            InterlockedAdd(SharedCulledObjectsCount, 1, sharedIndex);
            if (sharedIndex < GLOBAL_SURFACE_ATLAS_SHARED_CULL_SIZE)
                SharedCulledObjects[sharedIndex] = objectAddress;
		}
	}
	GroupMemoryBarrierWithGroupSync();

    // Cull objects from the shared buffer against active thread's chunk
    uint objectsCount = 0;
	LOOP
	for (uint i = 0; i < SharedCulledObjectsCount; i++)
	{
        uint objectAddress = SharedCulledObjects[i];
		float4 objectBounds = LoadGlobalSurfaceAtlasObjectBounds(GlobalSurfaceAtlasObjects, objectAddress);
		if (BoxIntersectsSphere(chunkMin, chunkMax, objectBounds.xyz, objectBounds.w))
        {
            objectsCount++;
        }
	}
    if (objectsCount == 0)
    {
        // Empty chunk
        RWGlobalSurfaceAtlasChunks.Store(chunkAddress, 0);
        return;
    }

	// Allocate object data size in the buffer
	uint objectsStart;
	uint objectsSize = objectsCount + 1; // Include objects count before actual objects data
	RWGlobalSurfaceAtlasCulledObjects.InterlockedAdd(0, objectsSize, objectsStart); // Counter at 0
	if (objectsStart + objectsSize > CulledObjectsCapacity)
	{
		// Not enough space in the buffer
		RWGlobalSurfaceAtlasChunks.Store(chunkAddress, 0);
		return;
	}

	// Write object data start
	RWGlobalSurfaceAtlasChunks.Store(chunkAddress, objectsStart);

	// Write objects count before actual objects indices
	RWGlobalSurfaceAtlasCulledObjects.Store(objectsStart * 4, objectsCount);

	// Copy objects data in this chunk (cull from the shared buffer)
    LOOP
	for (uint i = 0; i < SharedCulledObjectsCount; i++)
    {
        uint objectAddress = SharedCulledObjects[i];
		float4 objectBounds = LoadGlobalSurfaceAtlasObjectBounds(GlobalSurfaceAtlasObjects, objectAddress);
		if (BoxIntersectsSphere(chunkMin, chunkMax, objectBounds.xyz, objectBounds.w))
        {
            objectsStart++;
            RWGlobalSurfaceAtlasCulledObjects.Store(objectsStart * 4, objectAddress);
        }
    }
}

#endif

#ifdef _PS_Debug

Texture3D<snorm float> GlobalSDFTex : register(t0);
Texture3D<snorm float> GlobalSDFMip : register(t1);
ByteAddressBuffer GlobalSurfaceAtlasChunks : register(t2);
ByteAddressBuffer GlobalSurfaceAtlasCulledObjects : register(t3);
Buffer<float4> GlobalSurfaceAtlasObjects : register(t4);
Texture2D GlobalSurfaceAtlasTex : register(t5);
Texture2D GlobalSurfaceAtlasDepth : register(t6);
TextureCube Skybox : register(t7);

// Pixel shader for Global Surface Atlas debug drawing
META_PS(true, FEATURE_LEVEL_SM5)
META_PERMUTATION_1(GLOBAL_SURFACE_ATLAS_DEBUG_MODE=0)
META_PERMUTATION_1(GLOBAL_SURFACE_ATLAS_DEBUG_MODE=1)
float4 PS_Debug(Quad_VS2PS input) : SV_Target
{
#if 0
	// Preview Global Surface Atlas texture
	return float4(GlobalSurfaceAtlasTex.SampleLevel(SamplerLinearClamp, input.TexCoord, 0).rgb, 1);
#endif

	// Shot a ray from camera into the Global SDF
	GlobalSDFTrace trace;
	float3 viewRay = lerp(lerp(ViewFrustumWorldRays[3], ViewFrustumWorldRays[0], input.TexCoord.x), lerp(ViewFrustumWorldRays[2], ViewFrustumWorldRays[1], input.TexCoord.x), 1 - input.TexCoord.y).xyz;
	viewRay = normalize(viewRay - ViewWorldPos);
	trace.Init(ViewWorldPos, viewRay, ViewNearPlane, ViewFarPlane);
	trace.NeedsHitNormal = true;
	GlobalSDFHit hit = RayTraceGlobalSDF(GlobalSDF, GlobalSDFTex, GlobalSDFMip, trace);

    float3 color;
	if (hit.IsHit())
	{
        // Sample Global Surface Atlas at the hit location
        float surfaceThreshold = GetGlobalSurfaceAtlasThreshold(GlobalSDF, hit);
        color = SampleGlobalSurfaceAtlas(GlobalSurfaceAtlas, GlobalSurfaceAtlasChunks, GlobalSurfaceAtlasCulledObjects, GlobalSurfaceAtlasObjects, GlobalSurfaceAtlasDepth, GlobalSurfaceAtlasTex, hit.GetHitPosition(trace), -viewRay, surfaceThreshold).rgb;
	    //color = hit.HitNormal * 0.5f + 0.5f;
    }
    else
    {
        // Sample skybox
        float3 skybox = Skybox.SampleLevel(SamplerLinearClamp, viewRay, 0).rgb;
        float3 sky = float3(0.4f, 0.4f, 1.0f) * saturate(hit.StepsCount / 80.0f);
        color = lerp(sky, skybox, SkyboxIntensity);
    }
    return float4(color, 1);
}

#endif
