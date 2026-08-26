// Copyright (c) Wojciech Figat. All rights reserved.

#ifndef __OCTAHEDRAL__
#define __OCTAHEDRAL__

// Implementation based on:
// "A Survey of Efficient Representations for Independent Unit Vectors", Journal of Computer Graphics Tools (JCGT), vol. 3, no. 2, 1-30, 2014
// Zina H. Cigolle, Sam Donow, Daniel Evangelakos, Michael Mara, Morgan McGuire, and Quirin Meyer
// http://jcgt.org/published/0003/02/01/

float GetSignNotZero(float v)
{
    return v >= 0.0f ? 1.0f : -1.0f;
}

float2 GetSignNotZero(float2 v)
{
    return float2(GetSignNotZero(v.x), GetSignNotZero(v.y));
}

// Calculates octahedral coordinates (in range [-1; 1]) for direction vector
float2 GetOctahedralCoords(float3 direction)
{
    float2 uv = direction.xy * (1.0f / (abs(direction.x) + abs(direction.y) + abs(direction.z)));
    if (direction.z < 0.0f)
        uv = (1.0f - abs(uv.yx)) * GetSignNotZero(uv.xy);
    return uv;
}

// Calculates octahedral coordinates (in range [-1; 1]) for 2D texture (assuming 1 pixel border around)
float2 GetOctahedralCoords(uint2 texCoords, uint resolution)
{
    float2 uv = float2(texCoords.x % resolution, texCoords.y % resolution) + 0.5f;
    uv.xy /= float(resolution);
    return uv * 2.0f - 1.0f;
}

// Gets the direction vector from octahedral coordinates
float3 GetOctahedralDirection(float2 coords)
{
    float3 direction = float3(coords.x, coords.y, 1.0f - abs(coords.x) - abs(coords.y));
    if (direction.z < 0.0f)
        direction.xy = (1.0f - abs(direction.yx)) * GetSignNotZero(direction.xy);
    return normalize(direction);
}

#endif
