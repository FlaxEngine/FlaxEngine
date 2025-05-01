// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Matrix.h"

/// <summary>
/// Helper matrix for optimized float3x4 package of transformation matrices.
/// </summary>
struct FLAXENGINE_API Matrix3x4
{
    union
    {
        float Values[3][4];
        float Raw[12];
    };

    void SetMatrix(const Matrix& m);
    void SetMatrixTranspose(const Matrix& m);
};

template<>
struct TIsPODType<Matrix3x4>
{
    enum { Value = true };
};
