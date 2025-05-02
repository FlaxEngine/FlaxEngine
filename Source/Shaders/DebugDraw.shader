// Copyright (c) Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"

META_CB_BEGIN(0, Data)
float4x4 ViewProjection;
float2 Padding;
float ClipPosZBias;
bool EnableDepthTest;
META_CB_END

struct VS2PS
{
	float4 Position : SV_Position;
	float4 Color    : TEXCOORD0;
};

Texture2D SceneDepthTexture : register(t0);

META_VS(true, FEATURE_LEVEL_ES2)
VS2PS VS(float3 Position : POSITION, float4 Color : COLOR)
{
	VS2PS output;
	output.Position = mul(float4(Position, 1), ViewProjection);
	output.Position.z += ClipPosZBias;
	output.Color = Color;
	return output;
}

META_PS(true, FEATURE_LEVEL_ES2)
META_PERMUTATION_2(USE_DEPTH_TEST=0,USE_FAKE_LIGHTING=0)
META_PERMUTATION_2(USE_DEPTH_TEST=0,USE_FAKE_LIGHTING=1)
META_PERMUTATION_2(USE_DEPTH_TEST=1,USE_FAKE_LIGHTING=0)
META_PERMUTATION_2(USE_DEPTH_TEST=1,USE_FAKE_LIGHTING=1)
float4 PS(VS2PS input) : SV_Target
{
#if USE_DEPTH_TEST
	// Depth test manually if compositing editor primitives
	FLATTEN
	if (EnableDepthTest)
	{
		float sceneDepthDeviceZ = SceneDepthTexture.Load(int3(input.Position.xy, 0)).r;
		float interpolatedDeviceZ = input.Position.z;
		clip(sceneDepthDeviceZ - interpolatedDeviceZ);
	}
#endif

    float4 color = input.Color;
#if USE_FAKE_LIGHTING
	// Reconstruct view normal and calculate faked lighting
	float depth = input.Position.z * 10000;
	float3 n = normalize(float3(-ddx(depth), -ddy(depth), 1.0f));
	color.rgb *= saturate(abs(dot(n, float3(0, 1, 0))) + 0.5f);
#endif

    return color;
}
