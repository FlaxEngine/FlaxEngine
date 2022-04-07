// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"
#include "./Flax/Math.hlsl"
#include "./Flax/GlobalSurfaceAtlas.hlsl"
#include "./Flax/GlobalSignDistanceField.hlsl"

META_CB_BEGIN(0, Data)
float3 ViewWorldPos;
float ViewNearPlane;
float3 Padding00;
float ViewFarPlane;
float4 ViewFrustumWorldRays[4];
GlobalSDFData GlobalSDF;
GlobalSurfaceAtlasData GlobalSurfaceAtlas;
META_CB_END

// Vertex shader for Global Surface Atlas software clearing
META_VS(true, FEATURE_LEVEL_SM5)
META_VS_IN_ELEMENT(POSITION, 0, R16G16_FLOAT, 0, ALIGN, PER_VERTEX, 0, true)
float4 VS_Clear(float2 Position : POSITION0) : SV_Position
{
	return float4(Position, 1, 1);
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
