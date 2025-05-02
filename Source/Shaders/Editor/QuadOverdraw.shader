// Copyright (c) Wojciech Figat. All rights reserved.

#include "./Flax/Common.hlsl"

// [Quad Overdraw implementation based on https://blog.selfshadow.com/2012/11/12/counting-quads/ by Stephen Hill]

Texture2D<uint> overdrawSRV : register(t0);

float4 ToColour(uint v)
{
    const uint nbColours = 10;
    const float4 colours[nbColours] =
    {
       float4(0,     0,   0, 255),
       float4(2,    147, 25, 255),
       float4(0,   255, 149, 255),
       float4(0,   255, 253, 255),
       float4(142, 250,   0, 255),
       float4(225, 251,   0, 255),
       float4(225, 147,   0, 255),
       float4(225,  38,   0, 255),
       float4(148,  17,   0, 255),
       float4(255, 255, 255, 255)
    };
    return colours[min(v, nbColours - 1)] / 255.0;
}

META_PS(true, FEATURE_LEVEL_ES2)
float4 PS(float4 svPos : SV_POSITION) : SV_Target
{
    uint2 quad = svPos.xy * 0.5;
    uint overdraw = overdrawSRV[quad];
    return ToColour(overdraw);
}
