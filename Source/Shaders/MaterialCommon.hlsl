// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#ifndef __MATERIAL_COMMON__
#define __MATERIAL_COMMON__

// Material input types
#define MATERIAL_DOMAIN_SURFACE      0
#define MATERIAL_DOMAIN_POSTPROCESS  1
#define MATERIAL_DOMAIN_DECAL        2
#define MATERIAL_BLEND_OPAQUE        0
#define MATERIAL_BLEND_TRANSPARENT   1
#define MATERIAL_BLEND_ADDITIVE      2
#define MATERIAL_BLEND_MULTIPLY      3
#define MATERIAL_TESSELLATION_NONE   0
#define MATERIAL_TESSELLATION_FLAT   1
#define MATERIAL_TESSELLATION_PN     2
#define MATERIAL_TESSELLATION_PHONG  3

#define USE_CUSTOM_VERTEX_INTERPOLATORS (CUSTOM_VERTEX_INTERPOLATORS_COUNT > 0)

// Validate inputs
#ifndef MATERIAL
	#define MATERIAL 0
#endif
#ifndef MATERIAL_DOMAIN
	#define MATERIAL_DOMAIN MATERIAL_DOMAIN_SURFACE
#endif
#ifndef MATERIAL_BLEND
	#define MATERIAL_BLEND MATERIAL_BLEND_OPAQUE
#endif
#ifndef MATERIAL_SHADING_MODEL
	#define MATERIAL_SHADING_MODEL SHADING_MODEL_LIT
#endif
#ifndef USE_INSTANCING
	#define USE_INSTANCING 0
#endif
#ifndef USE_SKINNING
	#define USE_SKINNING 0
#endif
#ifndef USE_LIGHTMAP
	#define USE_LIGHTMAP 0
#endif
#ifndef USE_POSITION_OFFSET
	#define USE_POSITION_OFFSET 0
#endif
#ifndef USE_VERTEX_COLOR
	#define USE_VERTEX_COLOR 0
#endif
#ifndef USE_DISPLACEMENT
	#define USE_DISPLACEMENT 0
#endif
#ifndef USE_TESSELLATION
	#define USE_TESSELLATION 0
#endif
#ifndef USE_DITHERED_LOD_TRANSITION
	#define USE_DITHERED_LOD_TRANSITION 0
#endif
#ifndef MATERIAL_TESSELLATION
	#define MATERIAL_TESSELLATION MATERIAL_TESSELLATION_NONE
#endif
#ifndef MAX_TESSELLATION_FACTOR
	#define MAX_TESSELLATION_FACTOR 15
#endif
#ifndef PER_BONE_MOTION_BLUR
	#define PER_BONE_MOTION_BLUR 0
#endif

// Material properties
struct Material
{
	float3 Emissive;
	float Roughness;
	float3 Color;
	float AO;
	float3 WorldNormal;
	float Metalness;
	float3 TangentNormal;
	float Specular;
	float3 PositionOffset;
	float Opacity;
	float3 SubsurfaceColor;
	float Refraction;
	float Mask;
	float TessellationMultiplier;
	float3 WorldDisplacement;
#if USE_CUSTOM_VERTEX_INTERPOLATORS
	float4 CustomVSToPS[CUSTOM_VERTEX_INTERPOLATORS_COUNT];
#endif
};

struct ModelInput
{
	float3 Position   : POSITION;
	float2 TexCoord   : TEXCOORD0;
	float4 Normal     : NORMAL;
	float4 Tangent    : TANGENT;
	float2 LightmapUV : TEXCOORD1;
#if USE_VERTEX_COLOR
	half4 Color       : COLOR;
#endif

#if USE_INSTANCING
	float4 InstanceOrigin      : ATTRIBUTE0; // .w contains PerInstanceRandom
	float4 InstanceTransform1  : ATTRIBUTE1; // .w contains LODDitherFactor
	float3 InstanceTransform2  : ATTRIBUTE2;
	float3 InstanceTransform3  : ATTRIBUTE3;
	half4 InstanceLightmapArea : ATTRIBUTE4; 
#endif
};

struct ModelInput_PosOnly
{
	float3 Position   : POSITION;
	
#if USE_INSTANCING
	float4 InstanceOrigin      : ATTRIBUTE0; // .w contains PerInstanceRandom
	float4 InstanceTransform1  : ATTRIBUTE1; // .w contains LODDitherFactor
	float3 InstanceTransform2  : ATTRIBUTE2;
	float3 InstanceTransform3  : ATTRIBUTE3;
	half4 InstanceLightmapArea : ATTRIBUTE4; 
#endif
};

struct ModelInput_Skinned
{
	float3 Position     : POSITION;
	float2 TexCoord     : TEXCOORD0;
	float4 Normal       : NORMAL;
	float4 Tangent      : TANGENT;
	uint4 BlendIndices  : BLENDINDICES;
	float4 BlendWeights : BLENDWEIGHT;
	
#if USE_INSTANCING
	float4 InstanceOrigin      : ATTRIBUTE0; // .w contains PerInstanceRandom
	float4 InstanceTransform1  : ATTRIBUTE1; // .w contains LODDitherFactor
	float3 InstanceTransform2  : ATTRIBUTE2;
	float3 InstanceTransform3  : ATTRIBUTE3;
	half4 InstanceLightmapArea : ATTRIBUTE4; 
#endif
};

struct Model_VS2PS
{
	float4 Position  : SV_Position;
	float4 ScreenPos : TEXCOORD0;
};

struct GBufferOutput
{
	float4 Light : SV_Target0;
	float4 RT0   : SV_Target1;
	float4 RT1   : SV_Target2;
	float4 RT2   : SV_Target3;
	float4 RT3   : SV_Target4;
};

float3x3 CalcTangentBasisFromWorldNormal(float3 normal)
{
	float3 tangent = cross(normal, float3(1, 0, 0));
	float3 bitangent = cross(normal, tangent);
	return float3x3(tangent, bitangent, normal);
}

float3x3 CalcTangentBasis(float3 normal, float4 tangent)
{
	float3 bitangent = cross(normal, tangent.xyz) * tangent.w;
	return float3x3(tangent.xyz, bitangent, normal);
}

// [Jimenez et al. 2016, "Practical Realtime Strategies for Accurate Indirect Occlusion"]
float3 AOMultiBounce(float visibility, float3 albedo)
{
	float3 a =  2.0404 * albedo - 0.3324;
	float3 b = -4.7951 * albedo + 0.6417;
	float3 c =  2.7552 * albedo + 0.6903;
	return max(visibility, ((visibility * a + b) * visibility + c) * visibility);
}

#endif
