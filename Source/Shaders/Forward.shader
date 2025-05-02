// Copyright (c) Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"

Texture2D Input : register(t0);
Texture2D Distortion : register(t1);

// Pixel shader for applying distortion pass results to the combined frame
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_ApplyDistortion(Quad_VS2PS input) : SV_Target
{
	float4 accumDist = Distortion.Sample(SamplerPointClamp, input.TexCoord);
	float2 distOffset = (accumDist.rg - accumDist.ba) / 4.0f;
	float2 newTexCoord = input.TexCoord + distOffset;

	// Clamp around screen
	FLATTEN
	if (newTexCoord.x < 0 || newTexCoord.x > 1 || newTexCoord.y < 0 || newTexCoord.y > 1)
	{
		newTexCoord = input.TexCoord;
	}

	return Input.SampleLevel(SamplerPointClamp, newTexCoord, 0);
}
