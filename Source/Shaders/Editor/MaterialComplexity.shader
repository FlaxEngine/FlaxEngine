// Copyright (c) Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"

Texture2D Image : register(t0);

META_PS(true, FEATURE_LEVEL_ES2)
float4 PS(Quad_VS2PS input) : SV_Target
{
	const float4 colors[4] = {
		float4(0, 0.97f, 0.12f, 1),
		float4(0.2f, 0.2f, 0.7f, 1),
		float4(1, 0, 0, 1),
		float4(1, 0.95f, 0.95f, 1)
	};
	const float weights[4] = {
		0.0f,
		0.5f,
		0.8f,
		1.0f
	};

	float complexity = saturate(Image.Sample(SamplerPointClamp, input.TexCoord).r);

	float4 color;
	if (complexity < weights[1])
		color = lerp(colors[0], colors[1], complexity / weights[1]);
	else if (complexity < weights[2])
		color = lerp(colors[1], colors[2], (complexity - weights[1]) / (weights[2] - weights[1]));
	else
		color = lerp(colors[2], colors[3], (complexity - weights[2]) / (weights[3] - weights[2]));
	return color;
}
