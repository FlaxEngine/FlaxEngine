// Copyright (c) Wojciech Figat. All rights reserved.

#define MATERIAL 1
#define USE_VERTEX_COLOR 1

#include "./Flax/Common.hlsl"
#include "./Flax/MaterialCommon.hlsl"

META_CB_BEGIN(0, Data)
float4x4 ViewProjectionMatrix;
float4x4 WorldMatrix;
META_CB_END

struct VertexOutput
{
	float4 Position    : SV_Position;
	float4 VertexColor : COLOR;
};

struct PixelInput
{
	float4 Position    : SV_Position;
	float4 VertexColor : COLOR;
};

META_VS(true, FEATURE_LEVEL_ES2)
META_VS_IN_ELEMENT(POSITION, 0, R32G32B32_FLOAT,   0, 0,     PER_VERTEX, 0, true)
META_VS_IN_ELEMENT(TEXCOORD, 0, R16G16_FLOAT,      1, 0,     PER_VERTEX, 0, true)
META_VS_IN_ELEMENT(NORMAL,   0, R10G10B10A2_UNORM, 1, ALIGN, PER_VERTEX, 0, true)
META_VS_IN_ELEMENT(TANGENT,  0, R10G10B10A2_UNORM, 1, ALIGN, PER_VERTEX, 0, true)
META_VS_IN_ELEMENT(TEXCOORD, 1, R16G16_FLOAT,      1, ALIGN, PER_VERTEX, 0, true)
META_VS_IN_ELEMENT(COLOR,    0, R8G8B8A8_UNORM,    2, 0,     PER_VERTEX, 0, true)
VertexOutput VS(ModelInput input)
{
	VertexOutput output;
	float3 worldPosition = mul(float4(input.Position.xyz, 1), WorldMatrix).xyz;
	output.Position = mul(float4(worldPosition, 1), ViewProjectionMatrix);
	output.VertexColor = input.Color;
	return output;
}

META_PS(true, FEATURE_LEVEL_ES2)
void PS(in PixelInput input, out float4 Light : SV_Target0, out float4 RT0 : SV_Target1, out float4 RT1 : SV_Target2, out float4 RT2 : SV_Target3)
{
	Light = float4(input.VertexColor.rgb, 0);
	RT0 = float4(0, 0, 0, 0);
	RT1 = float4(float3(0, 0, 1) * 0.5 + 0.5, SHADING_MODEL_LIT * (1.0 / 3.0));
	RT2 = float4(0.4, 0, 0.5, 0);
}
