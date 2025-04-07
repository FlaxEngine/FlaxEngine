// Copyright (c) Wojciech Figat. All rights reserved.

#define USE_GBUFFER_CUSTOM_DATA

#include "./Flax/Common.hlsl"
#include "./Flax/GBuffer.hlsl"
#include "./Flax/BRDF.hlsl"

META_CB_BEGIN(0, Data)
GBufferData GBuffer;
float3 Dummy0;
int ViewMode;
META_CB_END

DECLARE_GBUFFERDATA_ACCESS(GBuffer)

// View modes
#define View_Mode_Default 0
#define View_Mode_Fast 1
#define View_Mode_Diffuse 2
#define View_Mode_Normals 3
#define View_Mode_Emissive 4
#define View_Mode_Depth 5
#define View_Mode_AmbientOcclusion 6
#define View_Mode_Metalness 7
#define View_Mode_Rougness 8
#define View_Mode_Specular 9
#define View_Mode_SpecularColor 10
#define View_Mode_ShadingModel 11
#define View_Mode_LightsBuffer 12
#define View_Mode_Reflections 13
#define View_Mode_Wireframe 14
#define View_Mode_MotionVectors 15
#define View_Mode_SubsurfaceColor 16
#define View_Mode_Unlit 17

float3 GetShadingModelColor(GBufferSample gBuffer)
{
	switch (gBuffer.ShadingModel)
	{
		case SHADING_MODEL_UNLIT: return float3(1.0f, 0.44f, 0.929f);
		case SHADING_MODEL_LIT: return float3(0.768f, 0.988f, 0.388f);
		case SHADING_MODEL_SUBSURFACE: return float3(0.478f, 0.086, 0.098);
		case SHADING_MODEL_FOLIAGE: return float3(0.182f, 0.711, 0.235);
	}
	return 1;
}

// Pixel shader for debug view
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_DebugView(Quad_VS2PS input) : SV_Target
{
	float3 result = float3(0, 0, 0);
	GBufferData gBufferData = GetGBufferData();
	GBufferSample gBuffer = SampleGBuffer(gBufferData, input.TexCoord);
	switch (ViewMode)
	{
		case View_Mode_Diffuse: result = gBuffer.Color; break;
		case View_Mode_Normals: result = gBuffer.Normal * 0.5 + 0.5; break;
		case View_Mode_Depth: result = gBuffer.ViewPos.z / GBuffer.ViewFar; break;
		case View_Mode_AmbientOcclusion: result = gBuffer.AO; break;
		case View_Mode_Metalness: result = gBuffer.Metalness; break;
		case View_Mode_Rougness: result = gBuffer.Roughness; break;
		case View_Mode_Specular: result = gBuffer.Specular; break;
		case View_Mode_SpecularColor: result = GetSpecularColor(gBuffer); break;
		case View_Mode_ShadingModel: result = GetShadingModelColor(gBuffer); break;
		case View_Mode_SubsurfaceColor: result = gBuffer.CustomData.rgb; break;
		case View_Mode_Unlit: result = gBuffer.Color * gBuffer.AO; break;
	}
	return float4(result, 1);
}
