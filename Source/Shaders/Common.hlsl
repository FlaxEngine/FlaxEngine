// Copyright (c) Wojciech Figat. All rights reserved.

#ifndef __COMMON__
#define __COMMON__

// Platform macros
#if !defined(DIRECTX)
#define DIRECTX 0
#endif
#if !defined(OPENGL)
#define OPENGL 0
#endif
#if !defined(VULKAN)
#define VULKAN 0
#endif
#if defined(PLATFORM_PS4)
#include "./FlaxPlatforms/PS4/Shaders/PS4Common.hlsl"
#endif
#if defined(PLATFORM_PS5)
#include "./FlaxPlatforms/PS5/Shaders/PS5Common.hlsl"
#endif

// Feature levels
#define FEATURE_LEVEL_ES2 0
#define FEATURE_LEVEL_ES3 1
#define FEATURE_LEVEL_ES3_1 2
#define FEATURE_LEVEL_SM4 3
#define FEATURE_LEVEL_SM5 4
#define FEATURE_LEVEL_SM6 5
#if !defined(FEATURE_LEVEL)
#error "Invalid platform defines"
#endif

// Meta macros used by shaders parser
#define META_VS(isVisible, minFeatureLevel)
#define META_VS_IN_ELEMENT(type, index, format, slot, offset, slotClass, stepRate, isVisible) // [Deprecated in v1.10]
#define META_HS(isVisible, minFeatureLevel)
#define META_HS_PATCH(inControlPoints)
#define META_DS(isVisible, minFeatureLevel)
#define META_GS(isVisible, minFeatureLevel)
#define META_PS(isVisible, minFeatureLevel)
#define META_CS(isVisible, minFeatureLevel)
#define META_FLAG(flag)
#define META_PERMUTATION_1(param0)
#define META_PERMUTATION_2(param0, param1)
#define META_PERMUTATION_3(param0, param1, param2)
#define META_PERMUTATION_4(param0, param1, param2, param3)
#define META_CB_BEGIN(index, name) cbuffer name : register(b##index) {
#define META_CB_END };

#define SHADING_MODEL_UNLIT 0
#define SHADING_MODEL_LIT 1
#define SHADING_MODEL_SUBSURFACE 2
#define SHADING_MODEL_FOLIAGE 3

// Detect feature level support
#if FEATURE_LEVEL >= FEATURE_LEVEL_SM5
#define CAN_USE_GATHER 1
#else
#define CAN_USE_GATHER 0
#endif
#if FEATURE_LEVEL >= FEATURE_LEVEL_SM5
#define CAN_USE_COMPUTE_SHADER 1
#else
#define CAN_USE_COMPUTE_SHADER 0
#endif
#if FEATURE_LEVEL >= FEATURE_LEVEL_SM4
#define CAN_USE_GEOMETRY_SHADER 1
#else
#define CAN_USE_GEOMETRY_SHADER 0
#endif
#if FEATURE_LEVEL >= FEATURE_LEVEL_SM5
#define CAN_USE_TESSELLATION 1
#else
#define CAN_USE_TESSELLATION 0
#endif

// Compiler attributes

#if DIRECTX || VULKAN

// Avoids flow control constructs.
#define UNROLL [unroll]

// Gives preference to flow control constructs.
#define LOOP [loop]

// Performs branching by using control flow instructions like jmp and label.
#define BRANCH [branch]

// Performs branching by using the cnd instructions.
#define FLATTEN [flatten]

#endif

// Compiler attribute fallback
#ifndef UNROLL
#define UNROLL
#endif
#ifndef LOOP
#define LOOP
#endif
#ifndef BRANCH
#define BRANCH
#endif
#ifndef FLATTEN
#define FLATTEN
#endif

#ifndef SamplerLinearClamp
// Static samplers
sampler SamplerLinearClamp : register(s0);
sampler SamplerPointClamp : register(s1);
sampler SamplerLinearWrap : register(s2);
sampler SamplerPointWrap : register(s3);
SamplerComparisonState ShadowSampler : register(s4);
SamplerComparisonState ShadowSamplerLinear : register(s5);
#endif

// General purpose macros
#define SAMPLE_RT(rt, texCoord) rt.SampleLevel(SamplerPointClamp, texCoord, 0)
#define SAMPLE_RT_LINEAR(rt, texCoord) rt.SampleLevel(SamplerLinearClamp, texCoord, 0)
#define HDR_CLAMP_MAX 65472.0
#define PI 3.1415926535897932

// Structure that contains information about GBuffer
struct GBufferData
{
    float4 ViewInfo; // x-1/Projection[0,0], y-1/Projection[1,1], z-(Far / (Far - Near), w-(-Far * Near) / (Far - Near) / Far)
    float4 ScreenSize; // x-Width, y-Height, z-1/Width, w-1/Height
    float3 ViewPos; // view position (in world space)
    float ViewFar; // view far plane distance (in world space)
    float4x4 InvViewMatrix; // inverse view matrix (4 rows by 4 columns)
    float4x4 InvProjectionMatrix; // inverse projection matrix (4 rows by 4 columns)
};

#ifdef PLATFORM_ANDROID
// #AdrenoVK_CB_STRUCT_MEMBER_ACCESS_BUG
#define DECLARE_GBUFFERDATA_ACCESS(uniformName) GBufferData Get##uniformName##Data() { GBufferData tmp; tmp.ViewInfo = uniformName.ViewInfo; tmp.ScreenSize = uniformName.ScreenSize; tmp.ViewPos = uniformName.ViewPos; tmp.ViewFar = uniformName.ViewFar; tmp.InvViewMatrix = uniformName.InvViewMatrix; tmp.InvProjectionMatrix = uniformName.InvProjectionMatrix; return tmp; }
#else
#define DECLARE_GBUFFERDATA_ACCESS(uniformName) GBufferData Get##uniformName##Data() { return uniformName; }
#endif

// Structure that contains information about atmosphere fog
struct AtmosphericFogData
{
    float AtmosphericFogDensityScale;
    float AtmosphericFogSunDiscScale;
    float AtmosphericFogDistanceScale;
    float AtmosphericFogGroundOffset;

    float AtmosphericFogAltitudeScale;
    float AtmosphericFogStartDistance;
    float AtmosphericFogPower;
    float AtmosphericFogDistanceOffset;

    float3 AtmosphericFogSunDirection;
    float AtmosphericFogSunPower;

    float3 AtmosphericFogSunColor;
    float AtmosphericFogDensityOffset;
};

// Packed env probe data
struct ProbeData
{
    float4 Data0; // x - Position.x,  y - Position.y,  z - Position.z,  w - unused
    float4 Data1; // x - Radius    ,  y - 1 / Radius,  z - Brightness,  w - unused
};

#define ProbePos Data0.xyz
#define ProbeRadius Data1.x
#define ProbeInvRadius Data1.y
#define ProbeBrightness Data1.z

struct Quad_VS2PS
{
    float4 Position : SV_Position;
    noperspective float2 TexCoord : TEXCOORD0;
};

struct Quad_VS2GS
{
    Quad_VS2PS Vertex;
    uint LayerIndex : TEXCOORD1;
};

struct Quad_GS2PS
{
    Quad_VS2PS Vertex;
    uint LayerIndex : SV_RenderTargetArrayIndex;
};

float Luminance(float3 color)
{
    return dot(color, float3(0.299f, 0.587f, 0.114f));
}

// Quaternion multiplication (http://mathworld.wolfram.com/Quaternion.html)
float4 QuatMultiply(float4 q1, float4 q2)
{
    return float4(q2.xyz * q1.w + q1.xyz * q2.w + cross(q1.xyz, q2.xyz), q1.w * q2.w - dot(q1.xyz, q2.xyz));
}

// Vector rotation with a quaternion (http://mathworld.wolfram.com/Quaternion.html)
float3 QuatRotateVector(float4 q, float3 v)
{
    float4 nq = q * float4(-1, -1, -1, 1);
    return QuatMultiply(q, QuatMultiply(float4(v, 0), nq)).xyz;
}

// Samples the unwrapped 3D texture (eg. volume texture of size 16x16x16 would be unwrapped to 256x16)
float4 SampleUnwrappedTexture3D(Texture2D tex, SamplerState s, float3 uvw, float size)
{
    float intW = floor(uvw.z * size - 0.5);
    half fracW = uvw.z * size - 0.5 - intW;
    float u = (uvw.x + intW) / size;
    float v = uvw.y;
    float4 rg0 = tex.Sample(s, float2(u, v));
    float4 rg1 = tex.Sample(s, float2(u + 1.0f / size, v));
    return lerp(rg0, rg1, fracW);
}

// Converts compact 4x3 object transformation matrix into a full 4x4 matrix.
float4x4 ToMatrix4x4(float4x3 m)
{
    return float4x4(float4(m[0].xyz, 0.0f), float4(m[1].xyz, 0.0f), float4(m[2].xyz, 0.0f), float4(m._m30, m._m31, m._m32, 1.0f));
}

#endif
