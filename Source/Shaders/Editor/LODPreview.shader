// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#define MATERIAL 1

#include "./Flax/Common.hlsl"
#include "./Flax/MaterialCommon.hlsl"

META_CB_BEGIN(0, Data)
float4x4 ViewProjectionMatrix;
float4x4 WorldMatrix;
float4 Color;
float3 WorldInvScale;
float LODDitherFactor;
META_CB_END

struct VertexOutput
{
	float4 Position    : SV_Position;
	float3 WorldNormal : TEXCOORD0;
};

struct PixelInput
{
	float4 Position    : SV_Position;
	float3 WorldNormal : TEXCOORD0;
};

float3x3 RemoveScaleFromLocalToWorld(float3x3 localToWorld)
{
	localToWorld[0] *= WorldInvScale.x;
	localToWorld[1] *= WorldInvScale.y;
	localToWorld[2] *= WorldInvScale.z;
	return localToWorld;
}

float3x3 CalcTangentToWorld(float4x4 world, float3x3 tangentToLocal)
{
	float3x3 localToWorld = RemoveScaleFromLocalToWorld((float3x3)world);
	return mul(tangentToLocal, localToWorld); 
}

float4 PerformFakeLighting(float3 n, float4 c)
{
	c.rgb *= saturate(abs(dot(n, float3(0, 1, 0))) + 0.5f);
	return c;
}

void ClipLODTransition(float4 svPosition, float ditherFactor)
{
	if (abs(ditherFactor) > 0.001)
	{
		float randGrid = cos(dot(floor(svPosition.xy), float2(347.83452793, 3343.28371863)));
		float randGridFrac = frac(randGrid * 1000.0);
		half mask = (ditherFactor < 0.0) ? (ditherFactor + 1.0 > randGridFrac) : (ditherFactor < randGridFrac);
		clip(mask - 0.001);
	}
}

META_VS(true, FEATURE_LEVEL_ES2)
META_VS_IN_ELEMENT(POSITION, 0, R32G32B32_FLOAT,   0, 0,     PER_VERTEX, 0, true)
META_VS_IN_ELEMENT(TEXCOORD, 0, R16G16_FLOAT,      1, 0,     PER_VERTEX, 0, true)
META_VS_IN_ELEMENT(NORMAL,   0, R10G10B10A2_UNORM, 1, ALIGN, PER_VERTEX, 0, true)
META_VS_IN_ELEMENT(TANGENT,  0, R10G10B10A2_UNORM, 1, ALIGN, PER_VERTEX, 0, true)
META_VS_IN_ELEMENT(TEXCOORD, 1, R16G16_FLOAT,      1, ALIGN, PER_VERTEX, 0, true)
VertexOutput VS(ModelInput input)
{
	float bitangentSign = input.Tangent.w ? -1.0f : +1.0f;
	float3 normal = input.Normal.xyz * 2.0 - 1.0;
	float3 tangent = input.Tangent.xyz * 2.0 - 1.0;
	float3 bitangent = cross(normal, tangent) * bitangentSign;
	float3x3 tangentToLocal = float3x3(tangent, bitangent, normal);
	float3x3 tangentToWorld = CalcTangentToWorld(WorldMatrix, tangentToLocal);
	float3 worldPosition = mul(float4(input.Position.xyz, 1), WorldMatrix).xyz;

	VertexOutput output;
	output.Position = mul(float4(worldPosition, 1), ViewProjectionMatrix);
	output.WorldNormal = tangentToWorld[2];
	return output;
}

META_PS(true, FEATURE_LEVEL_ES2)
void PS(in PixelInput input, out float4 Light : SV_Target0, out float4 RT0 : SV_Target1, out float4 RT1 : SV_Target2, out float4 RT2 : SV_Target3)
{
	ClipLODTransition(input.Position, LODDitherFactor);
	Light = PerformFakeLighting(input.WorldNormal, Color);
	RT0 = float4(0, 0, 0, 0);
	RT1 = float4(input.WorldNormal * 0.5 + 0.5, SHADING_MODEL_LIT * (1.0 / 3.0));
	RT2 = float4(0.4, 0, 0.5, 0);
}
