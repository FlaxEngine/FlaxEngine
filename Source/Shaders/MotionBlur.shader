// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#define NO_GBUFFER_SAMPLING

#include "./Flax/Common.hlsl"
#include "./Flax/GBuffer.hlsl"

META_CB_BEGIN(0, Data)

GBufferData GBuffer;

// Camera Motion Vectors
float4x4 CurrentVP;
float4x4 PreviousVP;
float4 TemporalAAJitter;

META_CB_END

DECLARE_GBUFFERDATA_ACCESS(GBuffer)

Texture2D Input0 : register(t0);
Texture2D Input1 : register(t1);
Texture2D Input2 : register(t2);

// Pixel shader for camera motion vectors
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_CameraMotionVectors(Quad_VS2PS input) : SV_Target
{
	// Get the pixel world space position
	float deviceDepth = SAMPLE_RT(Input0, input.TexCoord).r;
	GBufferData gBufferData = GetGBufferData();
	float4 worldPos = float4(GetWorldPos(gBufferData, input.TexCoord, deviceDepth), 1);

	float4 prevClipPos = mul(worldPos, PreviousVP);
	float4 curClipPos = mul(worldPos, CurrentVP);
	float2 prevHPos = prevClipPos.xy / prevClipPos.w;
	float2 curHPos = curClipPos.xy / curClipPos.w;

	// Revert temporal jitter offset
	prevHPos -= TemporalAAJitter.zw;
	curHPos -= TemporalAAJitter.xy;

	// Clip Space -> UV Space
	float2 vPosPrev = prevHPos.xy * 0.5f + 0.5f;
	float2 vPosCur = curHPos.xy * 0.5f + 0.5f;
	vPosPrev.y = 1.0 - vPosPrev.y;
	vPosCur.y = 1.0 - vPosCur.y;

	return float4(vPosCur - vPosPrev, 0, 1);
}
