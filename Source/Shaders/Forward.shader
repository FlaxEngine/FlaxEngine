// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"

Texture2D Input : register(t0);
Texture2D Distortion : register(t1);

// Pixel shader for applying distortion pass results to the combined frame
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS_ApplyDistortion(Quad_VS2PS input) : SV_Target
{
	// Sample accumulated distortion offset
	half4 accumDist = Distortion.Sample(SamplerPointClamp, input.TexCoord);

	// Offset = [R-B,G-A]
	half2 distOffset = (accumDist.rg - accumDist.ba);

	static const half InvDistortionScaleBias = 1 / 4.0f;
	distOffset *= InvDistortionScaleBias;

	float2 newTexCoord = input.TexCoord + distOffset;

	// If we're about to sample outside the valid area, set to 0 distortion
	FLATTEN
	if (newTexCoord.x < 0 || newTexCoord.x > 1 || newTexCoord.y < 0 || newTexCoord.y > 1)
	{
		newTexCoord = input.TexCoord;
	}

	// Sample screen using offset coords
	return Input.SampleLevel(SamplerPointClamp, newTexCoord, 0);
}
