// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
#ifndef USE_PER_VIEW_CONSTANTS
#define USE_PER_VIEW_CONSTANTS 0
#endif
#ifndef USE_PER_DRAW_CONSTANTS
#define USE_PER_DRAW_CONSTANTS 0
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

// Object properties
struct ObjectData
{
    float4x4 WorldMatrix;
    float4x4 PrevWorldMatrix;
    float3 GeometrySize;
    float WorldDeterminantSign;
    float LODDitherFactor;
    float PerInstanceRandom;
    float4 LightmapArea;
};

float2 UnpackHalf2(uint xy)
{
    return float2(f16tof32(xy & 0xffff), f16tof32(xy >> 16));
}

// Loads the object data from the global buffer
ObjectData LoadObject(Buffer<float4> objectsBuffer, uint objectIndex)
{
    // This must match ShaderObjectData::Store
    objectIndex *= 8;
    ObjectData object = (ObjectData)0;
    float4 vector0 = objectsBuffer.Load(objectIndex + 0);
    float4 vector1 = objectsBuffer.Load(objectIndex + 1);
    float4 vector2 = objectsBuffer.Load(objectIndex + 2);
    float4 vector3 = objectsBuffer.Load(objectIndex + 3);
    float4 vector4 = objectsBuffer.Load(objectIndex + 4);
    float4 vector5 = objectsBuffer.Load(objectIndex + 5);
    float4 vector6 = objectsBuffer.Load(objectIndex + 6);
    float4 vector7 = objectsBuffer.Load(objectIndex + 7);
    object.WorldMatrix[0] = float4(vector0.xyz, 0.0f);
    object.WorldMatrix[1] = float4(vector1.xyz, 0.0f);
    object.WorldMatrix[2] = float4(vector2.xyz, 0.0f);
    object.WorldMatrix[3] = float4(vector0.w, vector1.w, vector2.w, 1.0f);
    object.PrevWorldMatrix[0] = float4(vector3.xyz, 0.0f);
    object.PrevWorldMatrix[1] = float4(vector4.xyz, 0.0f);
    object.PrevWorldMatrix[2] = float4(vector5.xyz, 0.0f);
    object.PrevWorldMatrix[3] = float4(vector3.w, vector4.w, vector5.w, 1.0f);
    object.GeometrySize = vector6.xyz;
    object.PerInstanceRandom = vector6.w;
    object.WorldDeterminantSign = vector7.x;
    object.LODDitherFactor = vector7.y;
    object.LightmapArea.xy = UnpackHalf2(asuint(vector7.z));
    object.LightmapArea.zw = UnpackHalf2(asuint(vector7.w));
    return object;
}

#ifndef LoadObjectFromCB
// Loads the object data from the constant buffer into the variable
#define LoadObjectFromCB(var) \
    var = (ObjectData)0; \
    var.WorldMatrix = ToMatrix4x4(WorldMatrix); \
    var.PrevWorldMatrix = ToMatrix4x4(PrevWorldMatrix); \
    var.GeometrySize = GeometrySize; \
    var.PerInstanceRandom = PerInstanceRandom; \
    var.WorldDeterminantSign = WorldDeterminantSign; \
    var.LODDitherFactor = LODDitherFactor; \
    var.LightmapArea = LightmapArea;
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
    float3 WorldDisplacement;
    float Mask;
    float TessellationMultiplier;
#if USE_CUSTOM_VERTEX_INTERPOLATORS
	float4 CustomVSToPS[CUSTOM_VERTEX_INTERPOLATORS_COUNT];
#endif
};

// Secondary constant buffer (with per-view constants at slot 1)
#if USE_PER_VIEW_CONSTANTS
cbuffer ViewData : register(b1)
{
    float4x4 ViewMatrix;
    float4x4 ViewProjectionMatrix;
    float4x4 PrevViewProjectionMatrix;
    float4x4 MainViewProjectionMatrix;
    float4 MainScreenSize;
    float3 ViewPos;
    float ViewFar;
    float3 ViewDir;
    float TimeParam;
    float4 ViewInfo;
    float4 ScreenSize;
    float4 TemporalAAJitter;
    float3 LargeWorldsChunkIndex;
    float LargeWorldsChunkSize;
};
#endif

// Draw pipeline constant buffer (with per-draw constants at slot 2)
#if USE_PER_DRAW_CONSTANTS
cbuffer DrawData : register(b2)
{
    float3 DrawPadding;
    uint DrawObjectIndex;
};
#endif

struct ModelInput
{
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
    float4 Normal : NORMAL;
    float4 Tangent : TANGENT;
    float2 LightmapUV : TEXCOORD1;
#if USE_VERTEX_COLOR
	half4 Color : COLOR;
#endif
#if USE_INSTANCING
	uint ObjectIndex : ATTRIBUTE0;
#endif
};

struct ModelInput_PosOnly
{
    float3 Position : POSITION;
#if USE_INSTANCING
	uint ObjectIndex : ATTRIBUTE0;
#endif
};

struct ModelInput_Skinned
{
    float3 Position : POSITION;
    float2 TexCoord : TEXCOORD0;
    float4 Normal : NORMAL;
    float4 Tangent : TANGENT;
    uint4 BlendIndices : BLENDINDICES;
    float4 BlendWeights : BLENDWEIGHT;
};

struct Model_VS2PS
{
    float4 Position : SV_Position;
    float4 ScreenPos : TEXCOORD0;
};

struct GBufferOutput
{
    float4 Light : SV_Target0;
    float4 RT0 : SV_Target1;
    float4 RT1 : SV_Target2;
    float4 RT2 : SV_Target3;
    float4 RT3 : SV_Target4;
};

float3x3 CalcTangentBasis(float3 normal, float3 pos, float2 uv)
{
    // References:
    // http://www.thetenthplanet.de/archives/1180
    // https://zhangdoa.com/posts/normal-and-normal-mapping
    float3 dp1 = ddx(pos);
    float3 dp2 = ddy(pos);
    float2 duv1 = ddx(uv);
    float2 duv2 = ddy(uv);
    float3 dp2perp = cross(dp2, normal);
    float3 dp1perp = cross(normal, dp1);
    float3 tangent = normalize(dp2perp * duv1.x + dp1perp * duv2.x);
    float3 bitangent = normalize(dp2perp * duv1.y + dp1perp * duv2.y);
    return float3x3(tangent, bitangent, normal);
}

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
    float3 a = 2.0404 * albedo - 0.3324;
    float3 b = -4.7951 * albedo + 0.6417;
    float3 c = 2.7552 * albedo + 0.6903;
    return max(visibility, ((visibility * a + b) * visibility + c) * visibility);
}

float2 Flipbook(float2 uv, float frame, float2 sizeXY, float2 flipXY = 0.0f)
{
    float2 frameXY = float2((uint)frame % (uint)sizeXY.y, (uint)frame / (uint)sizeXY.x);
    float2 flipFrameXY = sizeXY - frameXY - float2(1, 1);
    frameXY = lerp(frameXY, flipFrameXY, flipXY);
    return (uv + frameXY) / sizeXY;
}

// Calculates the world-position offset to stabilize tiling (eg. via triplanar mapping) due to Large Worlds view origin offset.
float3 GetLargeWorldsTileOffset(float tileSize)
{
#if USE_PER_VIEW_CONSTANTS
    return LargeWorldsChunkIndex * fmod(LargeWorldsChunkSize, tileSize);
#else
    return float3(0, 0, 0);
#endif
}

#endif
