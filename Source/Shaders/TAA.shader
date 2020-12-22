// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"

// Pixel Shader for Temporal Anti-Aliasing
META_PS(true, FEATURE_LEVEL_ES2)
float4 PS(Quad_VS2PS input) : SV_Target0
{
	return float4(0, 0, 0, 0);
}
