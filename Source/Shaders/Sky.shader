// Copyright (c) Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"
#include "./Flax/MaterialCommon.hlsl"
#include "./Flax/GBuffer.hlsl"
#include "./Flax/Common.hlsl"
#include "./Flax/AtmosphereFog.hlsl"

META_CB_BEGIN(0, Data)
float4x4 WorldViewProjection;
float4x4 InvViewProjection;
float3 ViewOffset;
float Padding;
GBufferData GBuffer;
AtmosphericFogData AtmosphericFog;
META_CB_END

DECLARE_GBUFFERDATA_ACCESS(GBuffer)

struct MaterialInput
{
	float4 Position : SV_Position;
	float4 ScreenPos : TEXCOORD0;
};

// Vertex Shader function for GBuffer Pass
META_VS(true, FEATURE_LEVEL_ES2)
META_VS_IN_ELEMENT(POSITION, 0, R32G32B32_FLOAT, 0, 0, PER_VERTEX, 0, true)
MaterialInput VS(ModelInput_PosOnly input)
{
	MaterialInput output;

	// Compute vertex position
	output.Position = mul(float4(input.Position.xyz, 1), WorldViewProjection);
	output.ScreenPos = output.Position;

	return output;
}

// Pixel Shader function for GBuffer Pass
META_PS(true, FEATURE_LEVEL_ES2)
GBufferOutput PS_Sky(MaterialInput input)
{
	GBufferOutput output;

    // Calculate view vector (unproject at the far plane)
	GBufferData gBufferData = GetGBufferData();
	float4 clipPos = float4(input.ScreenPos.xy / input.ScreenPos.w, 1.0, 1.0);
	clipPos = mul(clipPos, InvViewProjection);
	float3 worldPos = clipPos.xyz / clipPos.w;
    float3 viewVector = normalize(worldPos - gBufferData.ViewPos);

	// Sample atmosphere color
    float4 color = GetAtmosphericFog(AtmosphericFog, gBufferData.ViewFar, gBufferData.ViewPos + ViewOffset, viewVector, gBufferData.ViewFar, float3(0, 0, 0));

	// Pack GBuffer
	output.Light = color;
	output.RT0 = float4(0, 0, 0, 0);
	output.RT1 = float4(1, 0, 0, SHADING_MODEL_UNLIT);
	output.RT2 = float4(0, 0, 0, 0);
	output.RT3 = float4(0, 0, 0, 0);

	return output;
}
