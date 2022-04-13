// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

// Diffuse-only lighting
#define NO_SPECULAR

#include "./Flax/Common.hlsl"
#include "./Flax/Math.hlsl"
#include "./Flax/GlobalSurfaceAtlas.hlsl"
#include "./Flax/GlobalSignDistanceField.hlsl"
#include "./Flax/LightingCommon.hlsl"

META_CB_BEGIN(0, Data)
float3 ViewWorldPos;
float ViewNearPlane;
float3 Padding00;
float ViewFarPlane;
float4 ViewFrustumWorldRays[4];
GlobalSDFData GlobalSDF;
GlobalSurfaceAtlasData GlobalSurfaceAtlas;
LightData Light;
META_CB_END

struct AtlasVertexIput
{
	float2 Position : POSITION0;
	float2 TileUV : TEXCOORD0;
	uint2 Index : TEXCOORD1;
};

struct AtlasVertexOutput
{
	float4 Position : SV_Position;
	float2 TileUV : TEXCOORD0;
	nointerpolation uint2 Index : TEXCOORD1;
};

// Vertex shader for Global Surface Atlas rendering (custom vertex buffer to render per-tile)
META_VS(true, FEATURE_LEVEL_SM5)
META_VS_IN_ELEMENT(POSITION, 0, R16G16_FLOAT, 0, ALIGN, PER_VERTEX, 0, true)
META_VS_IN_ELEMENT(TEXCOORD, 0, R16G16_FLOAT, 0, ALIGN, PER_VERTEX, 0, true)
META_VS_IN_ELEMENT(TEXCOORD, 1, R16G16_UINT,  0, ALIGN, PER_VERTEX, 0, true)
AtlasVertexOutput VS_Atlas(AtlasVertexIput input)
{
	AtlasVertexOutput output;
	output.Position = float4(input.Position, 1, 1);
	output.TileUV = input.TileUV;
	output.Index = input.Index;
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

#ifdef _PS_DirectLighting

#include "./Flax/GBuffer.hlsl"
#include "./Flax/Matrix.hlsl"
#include "./Flax/Lighting.hlsl"

// GBuffer+Depth at 0-3 slots
Buffer<float4> GlobalSurfaceAtlasObjects : register(t4);

// Pixel shader for Global Surface Atlas shading with direct light contribution
META_PS(true, FEATURE_LEVEL_SM5)
META_PERMUTATION_1(RADIAL_LIGHT=0)
META_PERMUTATION_1(RADIAL_LIGHT=1)
float4 PS_DirectLighting(AtlasVertexOutput input) : SV_Target
{
	// Load current tile info
	//GlobalSurfaceObject object = LoadGlobalSurfaceAtlasObject(GlobalSurfaceAtlasObjects, input.Index.x);
	GlobalSurfaceTile tile = LoadGlobalSurfaceAtlasTile(GlobalSurfaceAtlasObjects, input.Index.x, input.Index.y);
	float2 atlasUV = input.TileUV * tile.AtlasRectUV.zw + tile.AtlasRectUV.xy;

	// Load GBuffer sample from atlas
	GBufferData gBufferData = (GBufferData)0;
	GBufferSample gBuffer = SampleGBuffer(gBufferData, atlasUV);

	// Skip unlit pixels
	BRANCH
	if (gBuffer.ShadingModel == SHADING_MODEL_UNLIT)
	{
		discard;
		return 0;
	}

	// Reconstruct world-space position manually (from uv+depth within a tile)
	float tileDepth = SampleZ(atlasUV);
	//float tileNear = -GLOBAL_SURFACE_ATLAS_TILE_PROJ_PLANE_OFFSET;
	//float tileFar = tile.ViewBoundsSize.z + 2 * GLOBAL_SURFACE_ATLAS_TILE_PROJ_PLANE_OFFSET;
	//gBufferData.ViewInfo.zw = float2(tileFar / (tileFar - tileNear), (-tileFar * tileNear) / (tileFar - tileNear) / tileFar);
	//gBufferData.ViewInfo.zw = float2(1, 0);
	//float tileLinearDepth = LinearizeZ(gBufferData, tileDepth);
	float3 tileSpacePos = float3(input.TileUV.x - 0.5f, 0.5f - input.TileUV.y, tileDepth);
	float3 gBufferTilePos = tileSpacePos * tile.ViewBoundsSize;
	float4x4 tileLocalToWorld = Inverse(tile.WorldToLocal);
	gBuffer.WorldPos = mul(float4(gBufferTilePos, 1), tileLocalToWorld).xyz;

	float4 shadowMask = 1;
	BRANCH
	if (Light.CastShadows > 0)
	{
		// TODO: calculate shadow for the light (use Global SDF)
	}

	// Calculate lighting
#if RADIAL_LIGHT
	bool isSpotLight = Light.SpotAngles.x > -2.0f;
#else
	bool isSpotLight = false;
#endif
	float4 light = GetLighting(ViewWorldPos, Light, gBuffer, shadowMask, RADIAL_LIGHT, isSpotLight);

	return light;
}

#endif

#ifdef _PS_Debug

Texture3D<float> GlobalSDFTex[4] : register(t0);
Texture3D<float> GlobalSDFMip[4] : register(t4);
Buffer<float4> GlobalSurfaceAtlasObjects : register(t8);
Texture2D GlobalSurfaceAtlasDepth : register(t9);
Texture2D GlobalSurfaceAtlasTex : register(t10);

// Pixel shader for Global Surface Atlas debug drawing
META_PS(true, FEATURE_LEVEL_SM5)
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
	if (!hit.IsHit())
		return float4(float3(0.4f, 0.4f, 1.0f) * saturate(hit.StepsCount / 80.0f), 1);
	//return float4(hit.HitNormal * 0.5f + 0.5f, 1);

	// Sample Global Surface Atlas at the hit location
	float4 surfaceColor = SampleGlobalSurfaceAtlas(GlobalSurfaceAtlas, GlobalSurfaceAtlasObjects, GlobalSurfaceAtlasDepth, GlobalSurfaceAtlasTex, hit.GetHitPosition(trace), -viewRay);
	return float4(surfaceColor.rgb, 1);
}

#endif
