// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#ifndef __IES_PROFILE__
#define __IES_PROFILE__

// Apply 1D IES light profile texture
float ComputeLightProfileMultiplier(Texture2D tex, float3 worldPosition, float3 lightPosition, float3 lightDirection)
{
	float3 negLightDirection = normalize(worldPosition - lightPosition);

	// -1..1
	float dotProd = dot(negLightDirection, lightDirection);
	// -PI..PI (this distortion could be put into the texture but not without quality loss or more memory)
	float angle = asin(dotProd);
	// 0..1
	float normAngle = angle / PI + 0.5f;

	return tex.SampleLevel(SamplerLinearClamp, float2(normAngle, 0), 0).r;
}

#endif
