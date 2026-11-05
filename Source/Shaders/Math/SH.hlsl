// Copyright (c) Wojciech Figat. All rights reserved.

#ifndef __SH__
#define __SH__

// The SH coefficients for the projection of a function that maps directions to scalar values
struct ThreeBandSHVector
{
    half4 V0;
    half4 V1;
    half V2;
};

ThreeBandSHVector SHBasisFunction3(half3 v)
{
    ThreeBandSHVector result;

    result.V0.x = 0.282095f;
    result.V0.y = -0.488603f * v.y;
    result.V0.z = 0.488603f * v.z;
    result.V0.w = -0.488603f * v.x;

    result.V1.x = 1.092548f * v.x * v.y;
    result.V1.y = -1.092548f * v.y * v.z;
    result.V1.z = 0.315392f * (3.0f * v.z - 1.0f);
    result.V1.w = -1.092548f * v.x * v.z;
    result.V2 = 0.546274f * (v.x - v.y);

    return result;
}

// Projects a direction onto SH and convolves with a cosine kernel to compute irradiance
void ProjectOntoSH3(in float3 n, in float3 color, out float3 sh[9])
{
    // Cosine kernel
    const float A0 = 3.141593f;
    const float A1 = 2.095395f;
    const float A2 = 0.785398f;

    // Band 0
    sh[0] = 0.282095f * A0 * color;

    // Band 1
    sh[1] = 0.488603f * n.y * A1 * color;
    sh[2] = 0.488603f * n.z * A1 * color;
    sh[3] = 0.488603f * n.x * A1 * color;

    // Band 2
    sh[4] = 1.092548f * n.x * n.y * A2 * color;
    sh[5] = 1.092548f * n.y * n.z * A2 * color;
    sh[6] = 0.315392f * (3.0f * n.z * n.z - 1.0f) * A2 * color;
    sh[7] = 1.092548f * n.x * n.z * A2 * color;
    sh[8] = 0.546274f * (n.x * n.x - n.y * n.y) * A2 * color;
}

// Converts from 3-band SH to 2-band H-Basis
// [Ralf Habel and Michael Wimmer, "Efficient Irradiance Normal Mapping"]
void ConvertSH3ToHBasis(in float3 sh[9], out float3 hBasis[4])
{
    const float rt2 = sqrt(2.0f);
    const float rt32 = sqrt(3.0f / 2.0f);
    const float rt52 = sqrt(5.0f / 2.0f);
    const float rt152 = sqrt(15.0f / 2.0f);
    const float convMatrix[4 * 9] =
    {
        // @formatter:off
        1.0f / rt2, 0, 0.5f * rt32, 0, 0, 0, 0, 0, 0,
        0, 1.0f / rt2, 0, 0, 0, (3.0f / 8.0f) * rt52, 0, 0, 0,
        0, 0, 1.0f / (2.0f * rt2), 0, 0, 0, 0.25f * rt152, 0, 0,
        0, 0, 0, 1.0f / rt2, 0, 0, 0, (3.0f / 8.0f) * rt52, 0
        // @formatter:on
    };

    UNROLL
    for (uint row = 0; row < 4; row++)
    {
        hBasis[row] = 0.0f;
        UNROLL
        for (uint col = 0; col < 9; col++)
            hBasis[row] += convMatrix[row * 9 + col] * sh[col];
    }
}

#endif
