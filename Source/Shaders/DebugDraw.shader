// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"

META_CB_BEGIN(0, Data)
float4x4 ViewProjection;
float3 Padding;
bool EnableDepthTest;
META_CB_END

struct VS2PS
{
	float4 Position : SV_Position;
	float4 Color    : TEXCOORD0;
};

Texture2D SceneDepthTexture : register(t0);

META_VS(true, FEATURE_LEVEL_ES2)
META_VS_IN_ELEMENT(POSITION, 0, R32G32B32_FLOAT, 0, ALIGN, PER_VERTEX, 0, true)
META_VS_IN_ELEMENT(COLOR,    0, R8G8B8A8_UNORM,  0, ALIGN, PER_VERTEX, 0, true)
VS2PS VS(float3 Position : POSITION, float4 Color : COLOR)
{
	VS2PS output;
	output.Position = mul(float4(Position, 1), ViewProjection);
	output.Color = Color;
	return output;
}

void PerformDepthTest(float4 svPosition)
{
	// Depth test manually if compositing editor primitives
	FLATTEN
	if (EnableDepthTest)
	{
		float sceneDepthDeviceZ = SceneDepthTexture.Load(int3(svPosition.xy, 0)).r;
		float interpolatedDeviceZ = svPosition.z;
		clip(sceneDepthDeviceZ - interpolatedDeviceZ);
	}
}

float4 PerformFakeLighting(float4 svPosition, float4 c)
{
	// Reconstruct view normal and calculate faked lighting
	float depth = svPosition.z * 10000;
	float3 n = normalize(float3(-ddx(depth), -ddy(depth), 1.0f));
	c.rgb *= saturate(abs(dot(n, float3(0, 1, 0))) + 0.5f);
	return c;
}

META_PS(true, FEATURE_LEVEL_ES2)
float4 PS(VS2PS input) : SV_Target
{
	return PerformFakeLighting(input.Position, input.Color);
}

META_PS(true, FEATURE_LEVEL_ES2)
float4 PSUnlit(VS2PS input) : SV_Target
{
	return input.Color;
}

META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_DepthTest(VS2PS input) : SV_Target
{
	PerformDepthTest(input.Position);
	return PerformFakeLighting(input.Position, input.Color);
}

META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_DepthTestUnlit(VS2PS input) : SV_Target
{
	PerformDepthTest(input.Position);
	return input.Color;
}
