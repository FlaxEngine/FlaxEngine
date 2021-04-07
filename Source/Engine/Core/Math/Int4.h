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
API_STRUCT() struct FLAXENGINE_API Int4
{
DECLARE_SCRIPTING_TYPE_MINIMAL(Int4);
public:

    union
    {
        struct
        {
            /// <summary>
            /// The X component.
            /// </summary>
            API_FIELD() int32 X;

            /// <summary>
            /// The Y component.
            /// </summary>
            API_FIELD() int32 Y;

            /// <summary>
            /// The Z component.
            /// </summary>
            API_FIELD() int32 Z;

            /// <summary>
            /// The W component.
            /// </summary>
            API_FIELD() int32 W;
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
    // @param v Int2 to use X and Y components
    // @param z Z component value
    // @param w W component value
    Int4(const Int2& xy, float z, float w);

    // Init
    // @param v Int3 to use X , Y and Z components
    // @param w W component value
    Int4(const Int3& xyz, float w);

    // Init
    // @param v Vector2 to use X and Y components
    // @param z Z component value
    // @param w W component value
    explicit Int4(const Vector2& xy, float z, float w);

    // Init
    // @param v Vector3 to use X , Y and Z components
    // @param w W component value
    explicit Int4(const Vector3& xyz, float w);
    
    // Init
    // @param v Vector to use X, Y, Z and W components
    explicit Int4(const Vector4& xyzw);

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

    // Returns a vector containing the largest components of the specified vectors
    // @param a The first source vector
    // @param b The second source vector
    static Int4 Max(const Int4& a, const Int4& b)
    {
        return Int4(a.X > b.X ? a.X : b.X, a.Y > b.Y ? a.Y : b.Y, a.Z > b.Z ? a.Z : b.Z, a.W > b.W ? a.W : b.W);
    }

    // Returns a vector containing the smallest components of the specified vectors
    // @param a The first source vector
    // @param b The second source vector
    static Int4 Min(const Int4& a, const Int4& b)
    {
        return Int4(a.X < b.X ? a.X : b.X, a.Y < b.Y ? a.Y : b.Y, a.Z < b.Z ? a.Z : b.Z, a.W < b.W ? a.W : b.W);
    }

    // Returns a vector containing the largest components of the specified vectors
    // @param a The first source vector
    // @param b The second source vector
    // @param result When the method completes, contains an new vector composed of the largest components of the source vectors
    static void Max(const Int4& a, const Int4& b, Int4* result)
    {
        *result = Int4(a.X > b.X ? a.X : b.X, a.Y > b.Y ? a.Y : b.Y, a.Z > b.Z ? a.Z : b.Z, a.W > b.W ? a.W : b.W);
    }

    // Returns a vector containing the smallest components of the specified vectors
    // @param a The first source vector
    // @param b The second source vector
    // @param result When the method completes, contains an new vector composed of the smallest components of the source vectors
    static void Min(const Int4& a, const Int4& b, Int4* result)
    {
        *result = Int4(a.X < b.X ? a.X : b.X, a.Y < b.Y ? a.Y : b.Y, a.Z < b.Z ? a.Z : b.Z, a.W < b.W ? a.W : b.W);
    }
};

template<>
struct TIsPODType<Int4>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(Int4, "X:{0} Y:{1} Z:{2} W:{3}", v.X, v.Y, v.Z, v.W);
