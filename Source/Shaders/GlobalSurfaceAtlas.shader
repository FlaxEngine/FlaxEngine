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

#ifdef _PS_Debug

Texture3D<float> GlobalSDFTex[4] : register(t0);
Texture3D<float> GlobalSDFMip[4] : register(t4);
Texture2D GlobalSurfaceAtlasTex : register(t8);
//Buffer<float4> GlobalSurfaceAtlasObjects  : register(t9);

// Pixel shader for Global Surface Atlas debug drawing
META_PS(true, FEATURE_LEVEL_SM5)
float4 PS_Debug(Quad_VS2PS input) : SV_Target
{
#if 1
	// Preview Global Surface Atlas texture
	float4 texSample = GlobalSurfaceAtlasTex.SampleLevel(SamplerLinearClamp, input.TexCoord, 0);
	return float4(texSample.rgb, 1);
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

	// TODO: debug draw Surface Cache
	//float4 surfaceColor = SampleGlobalSurfaceAtlas(GlobalSurfaceAtlas, GlobalSurfaceAtlas, GlobalSurfaceAtlasObjects, hit.GetHitPosition(trace), -viewRay);

	// Debug draw SDF normals
	float3 color = hit.HitNormal * 0.5f + 0.5f;
	return float4(color, 1);
}

#endif
