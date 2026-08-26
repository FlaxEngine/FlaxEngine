// Copyright (c) Wojciech Figat. All rights reserved.

#ifndef __IES_PROFILE__
#define __IES_PROFILE__

// Calculate IES light profile from 1D texture
float ComputeLightProfileMultiplier(Texture2D tex, float3 worldPosition, float3 lightPosition, float3 lightDirection)
{
    float3 l = normalize(worldPosition - lightPosition);
    float d = dot(l, lightDirection);
    float angle = asin(d) / PI + 0.5f;
    return tex.SampleLevel(SamplerLinearClamp, float2(angle, 0), 0).r;
}

#endif
