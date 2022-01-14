// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Core.h"
#include "Half.h"

/// <summary>
/// Half-precision 16 bit floating point number consisting of a sign bit, a 5 bit biased exponent, and a 10 bit mantissa.
/// </summary>
typedef Half Float16;

/// <summary>
/// Packed vector, layout: R:10 bytes, G:10 bytes, B:10 bytes, A:2 bytes, all values are stored as floats in range [0;1].
/// </summary>
struct FLAXENGINE_API Float1010102
{
    union
    {
        struct
        {
            uint32 X : 10; // 0/1023 to 1023/1023
            uint32 Y : 10; // 0/1023 to 1023/1023
            uint32 Z : 10; // 0/1023 to 1023/1023
            uint32 W : 2; //     0/3 to       3/3
        };

        uint32 Value;
    };

    Float1010102() = default;

    explicit Float1010102(uint32 packed)
        : Value(packed)
    {
    }

    Float1010102(float x, float y, float z, float w);
    Float1010102(const Vector3& v, float alpha = 0);
    explicit Float1010102(const float* values);

    operator uint32() const
    {
        return Value;
    }

    operator Vector4() const;

    Float1010102& operator=(const Float1010102& other)
    {
        Value = other.Value;
        return *this;
    }

    Float1010102& operator=(uint32 packed)
    {
        Value = packed;
        return *this;
    }

public:

    Vector3 ToVector3() const;
};

// The 3D vector is packed into 32 bits with 11/11/10 bits per floating-point component.
struct FLAXENGINE_API FloatR11G11B10
{
    union
    {
        struct
        {
            uint32 xm : 6; // x-mantissa
            uint32 xe : 5; // x-exponent
            uint32 ym : 6; // y-mantissa
            uint32 ye : 5; // y-exponent
            uint32 zm : 5; // z-mantissa
            uint32 ze : 5; // z-exponent
        };

        uint32 Value;
    };

    FloatR11G11B10() = default;

    explicit FloatR11G11B10(uint32 packed)
        : Value(packed)
    {
    }

    FloatR11G11B10(float x, float y, float z);
    FloatR11G11B10(const Vector3& v);
    FloatR11G11B10(const Vector4& v);
    FloatR11G11B10(const Color& v);
    explicit FloatR11G11B10(const float* values);

    operator uint32() const
    {
        return Value;
    }

    operator Vector3() const;

    FloatR11G11B10& operator=(const FloatR11G11B10& other)
    {
        Value = other.Value;
        return *this;
    }

    FloatR11G11B10& operator=(uint32 packed)
    {
        Value = packed;
        return *this;
    }

public:

    Vector3 ToVector3() const;
};

struct FLAXENGINE_API RG16UNorm
{
    uint16 X, Y;

    RG16UNorm(float x, float y)
    {
        X = (uint16)(x * MAX_uint16);
        Y = (uint16)(y * MAX_uint16);
    }

    Vector2 ToVector2() const;
};

struct FLAXENGINE_API RGBA16UNorm
{
    uint16 X, Y, Z, W;

    RGBA16UNorm(float x, float y, float z, float w)
    {
        X = (uint16)(x * MAX_uint16);
        Y = (uint16)(y * MAX_uint16);
        Z = (uint16)(z * MAX_uint16);
        W = (uint16)(w * MAX_uint16);
    }

    Vector4 ToVector4() const;
};
