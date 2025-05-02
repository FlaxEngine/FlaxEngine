// Copyright (c) Wojciech Figat. All rights reserved.

#define MATERIAL 1
#define MATERIAL_TEXCOORDS 4

#include "./Flax/Common.hlsl"
#include "./Flax/MaterialCommon.hlsl"

META_CB_BEGIN(0, Data)
float4x4 ViewProjectionMatrix;
float4x4 WorldMatrix;
float4 LightmapArea;
float3 WorldInvScale;
float LightmapTexelsPerWorldUnit;
float3 Dummy0;
float LightmapSize;
META_CB_END

Texture2D GridTexture : register(t0);

struct VertexOutput
{
	float4 Position : SV_Position;
	float3 WorldPosition : TEXCOORD0;
	float2 LightmapUV : TEXCOORD1;
	float3 WorldNormal : TEXCOORD2;
};

struct PixelInput
{
	float4 Position : SV_Position;
	float3 WorldPosition : TEXCOORD0;
	float2 LightmapUV : TEXCOORD1;
	float3 WorldNormal : TEXCOORD2;
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

	VertexOutput output;
	output.WorldPosition = mul(float4(input.Position.xyz, 1), WorldMatrix).xyz;
	output.Position = mul(float4(output.WorldPosition.xyz, 1), ViewProjectionMatrix);
	output.LightmapUV = input.LightmapUV * LightmapArea.zw + LightmapArea.xy;
	output.WorldNormal = tangentToWorld[2];
	return output;
}

META_PS(true, FEATURE_LEVEL_ES2)
void PS(in PixelInput input, out float4 Light : SV_Target0, out float4 RT0 : SV_Target1, out float4 RT1 : SV_Target2, out float4 RT2 : SV_Target3)
{
	// Inputs
	float3 worldPosition = input.WorldPosition;
	float minDensity = 0.0;
	float bestDensity = 0.21;
	float maxDensity = 0.82;
	float texelScale = 1;
	float2 lightmapUVs = input.LightmapUV * LightmapSize / LightmapTexelsPerWorldUnit;

	// Calculate lightmap texels density
	float worldArea = max(length(cross(ddx(worldPosition), ddy(worldPosition))), 0.0000001);
	float2 a = ddx(lightmapUVs);
	float2 b = ddy(lightmapUVs);
	float2 c = a.xy * b.yx;
	float lightmapArea = abs(c.x - c.y);
	float density = lightmapArea / worldArea;
	density = clamp(density, minDensity, maxDensity);

	// Color based on lightmap texels density
	float3 color;
	float3 minColor = float3(235/255.0, 52/255.0, 67/255.0);
	float3 bestColor = float3(51/255.0, 235/255.0, 70/255.0);
	float3 maxColor = float3(52/255.0, 149/255.0, 235/255.0);
    if (LightmapSize < 0.0f)
    {
        color = float3(52/255.0, 229/255.0, 235/255.0); // No lightmap
    }
    else if (density < bestDensity)
	{
		color = lerp(minColor, bestColor, (density - minDensity) / (bestDensity - minDensity));
	}
	else
	{
		color = lerp(bestColor, maxColor, (density - bestDensity) / (maxDensity - bestDensity));
	}

	// Apply grid color
	color *= GridTexture.Sample(SamplerLinearWrap, input.LightmapUV / LightmapArea.zw * 10).g;

	// Outputs
	Light = float4(color, 0);
	RT0 = float4(0, 0, 0, 0);
	RT1 = float4(input.WorldNormal * 0.5 + 0.5, SHADING_MODEL_LIT * (1.0 / 3.0));
	RT2 = float4(0.4, 0, 0.5, 0);
}
