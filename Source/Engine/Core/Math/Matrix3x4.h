// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Matrix.h"

/// <summary>
/// Helper matrix for optimized float3x4 package of transformation matrices.
/// </summary>
struct FLAXENGINE_API Matrix3x4
{
    float M[3][4];

    void SetMatrix(const Matrix& m)
    {
        const float* src = m.Raw;
        float* dst = &M[0][0];
        dst[0] = src[0];
        dst[1] = src[1];
        dst[2] = src[2];
        dst[3] = src[3];
        dst[4] = src[4];
        dst[5] = src[5];
        dst[6] = src[6];
        dst[7] = src[7];
        dst[8] = src[8];
        dst[9] = src[9];
        dst[10] = src[10];
        dst[11] = src[11];
    }

    void SetMatrixTranspose(const Matrix& m)
    {
        const float* src = m.Raw;
        float* dst = &M[0][0];
        dst[0] = src[0];
        dst[1] = src[4];
        dst[2] = src[8];
        dst[3] = src[12];
        dst[4] = src[1];
        dst[5] = src[5];
        dst[6] = src[9];
        dst[7] = src[13];
        dst[8] = src[2];
        dst[9] = src[6];
        dst[10] = src[10];
        dst[11] = src[14];
    }
};

template<>
struct TIsPODType<Matrix3x4>
{
    enum { Value = true };
};
