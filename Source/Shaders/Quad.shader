// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"

// Interpolants passed from the vertex shader to the PostFx material pixel shader
struct MaterialVertexOutput
{
	float4 Position      : SV_Position;
	float3 WorldPosition : TEXCOORD0;
	float2 TexCoord      : TEXCOORD1;
};

META_CB_BEGIN(0, Data)
float4 Color;
META_CB_END

// Vertex Shader for screen space quad rendering
META_VS(true, FEATURE_LEVEL_ES2)
META_VS_IN_ELEMENT(POSITION, 0, R32G32_FLOAT, 0, ALIGN, PER_VERTEX, 0, true)
META_VS_IN_ELEMENT(TEXCOORD, 0, R32G32_FLOAT, 0, ALIGN, PER_VERTEX, 0, true)
Quad_VS2PS VS(float2 Position : POSITION0, float2 TexCoord : TEXCOORD0)
{
	Quad_VS2PS output;

	output.Position = float4(Position, 0, 1);
	output.TexCoord = TexCoord;

	return output;
}

// Vertex Shader function for postFx materials rendering
META_VS(true, FEATURE_LEVEL_ES2)
META_VS_IN_ELEMENT(POSITION, 0, R32G32_FLOAT, 0, ALIGN, PER_VERTEX, 0, true)
META_VS_IN_ELEMENT(TEXCOORD, 0, R32G32_FLOAT, 0, ALIGN, PER_VERTEX, 0, true)
MaterialVertexOutput VS_PostFx(float2 Position : POSITION0, float2 TexCoord : TEXCOORD0)
{
	MaterialVertexOutput output;

	output.Position = float4(Position, 0, 1);
	output.WorldPosition = output.Position.xyz;
	output.TexCoord = TexCoord;

	return output;
}

#ifdef _PS_CopyLinear

Texture2D Source : register(t0);

// Pixel Shader for screen space quad rendering for image copy (linear sampling)
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_CopyLinear(Quad_VS2PS input) : SV_Target
{
	return Source.SampleLevel(SamplerLinearClamp, input.TexCoord, 0);
}

#endif

#ifdef _PS_Clear

// Pixel Shader for clearing a render target with a solid color
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_Clear(Quad_VS2PS input) : SV_Target
{
	return Color;
}

#endif
