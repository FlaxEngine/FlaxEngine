// Copyright (c) Wojciech Figat. All rights reserved.

#ifndef __TEMPORAL__
#define __TEMPORAL__

#include "./Flax/Common.hlsl"

// [Pedersen, 2016, "Temporal Reprojection Anti-Aliasing in INSIDE"]
float4 ClipToAABB(float4 color, float4 minimum, float4 maximum)
{
	float4 center = (maximum + minimum) * 0.5;
	float4 extents = (maximum - minimum) * 0.5;
	float4 shift = color - center;
    float4 absUnit = abs(shift / max(extents, 0.0001));
    float maxUnit = max(max(absUnit.x, absUnit.y), absUnit.z);
	return maxUnit > 1.0 ? center + (shift / maxUnit) : color;
}

#endif
