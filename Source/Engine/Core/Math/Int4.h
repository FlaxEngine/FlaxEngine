// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Math.h"
#include "Engine/Core/Formatting.h"
#include "Engine/Core/Templates.h"

struct Vector2;
struct Vector3;
struct Vector4;

/// <summary>
/// Four-components vector (32 bit integer type).
/// </summary>
API_STRUCT(InBuild) struct FLAXENGINE_API Int4
{
DECLARE_SCRIPTING_TYPE_MINIMAL(Int4);
public:

    union
    {
        struct
        {
            // X component
            int32 X;

            // Y component
            int32 Y;

            // Z component
            int32 Z;

            // W component
            int32 W;
        };

        // Raw values
        int32 Raw[4];
    };

public:

    // Vector with all components equal 0
    static const Int4 Zero;

    // Vector with all components equal 1
    static const Int4 One;

public:

    /// <summary>
    /// Empty constructor.
    /// </summary>
    Int4()
    {
    }

    // Init
    // @param xy Value to assign to the all components
    Int4(int32 xyzw)
        : X(xyzw)
        , Y(xyzw)
        , Z(xyzw)
        , W(xyzw)
    {
    }

    // Init
    // @param x X component value
    // @param y Y component value
    // @param z Z component value
    // @param w W component value
    Int4(int32 x, int32 y, int32 z, int32 w)
        : X(x)
        , Y(y)
        , Z(z)
        , W(w)
    {
    }

    // Init
    // @param v Vector to use X, Y, Z and W components
    explicit Int4(const Vector4& v);

public:

    String ToString() const;

public:

    /// <summary>
    /// Returns average arithmetic of all the components
    /// </summary>
    /// <returns>Average arithmetic of all the components</returns>
    float AverageArithmetic() const
    {
        return (X + Y + Z + W) * 0.25f;
    }

    /// <summary>
    /// Gets sum of all vector components values
    /// </summary>
    /// <returns>Sum of X, Y, Z and W</returns>
    int32 SumValues() const
    {
        return X + Y + Z + W;
    }

    /// <summary>
    /// Returns minimum value of all the components
    /// </summary>
    /// <returns>Minimum value</returns>
    int32 MinValue() const
    {
        return Math::Min(X, Y, Z, W);
    }

    /// <summary>
    /// Returns maximum value of all the components
    /// </summary>
    /// <returns>Maximum value</returns>
    int32 MaxValue() const
    {
        return Math::Max(X, Y, Z, W);
    }
};

template<>
struct TIsPODType<Int4>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(Int4, "X:{0} Y:{1} Z:{2} W:{3}", v.X, v.Y, v.Z, v.W);
