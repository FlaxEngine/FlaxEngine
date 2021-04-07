// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Math.h"
#include "Engine/Core/Formatting.h"
#include "Engine/Core/Templates.h"

struct Vector2;
struct Vector3;
struct Vector4;
struct Int2;

/// <summary>
/// Three-components vector (32 bit integer type).
/// </summary>
API_STRUCT() struct FLAXENGINE_API Int3
{
DECLARE_SCRIPTING_TYPE_MINIMAL(Int3);
public:

    union
    {
        struct
        {
            // X component
            int32 X;

            // Y component
            int32 Y;

            // Y component
            int32 Z;
        };

        // Raw values
        int32 Raw[3];
    };

public:

    // Vector with all components equal 0
    static const Int3 Zero;

    // Vector with all components equal 1
    static const Int3 One;

public:

    /// <summary>
    /// Empty constructor.
    /// </summary>
    Int3()
    {
    }

    // Init
    // @param xy Value to assign to the all components
    Int3(int32 xyz)
        : X(xyz)
        , Y(xyz)
        , Z(xyz)
    {
    }

    // Init
    // @param x X component value
    // @param y Y component value
    // @param z Z component value
    Int3(int32 x, int32 y, int32 z)
        : X(x)
        , Y(y)
        , Z(z)
    {
    }

    Int3(const Int2& xy, float z);

    Int3(const Int4& xyzw);
    
    explicit Int3(const Vector2& xy, float z);
    
    // Init
    // @param v Vector to use X, Y and Z components
    explicit Int3(const Vector3& xyz);

    explicit Int3(const Vector4& xyzw);
    
public:

    String ToString() const;

public:

    // Returns a vector containing the largest components of the specified vectors
    // @param a The first source vector
    // @param b The second source vector
    static Int3 Max(const Int3& a, const Int3& b)
    {
        return Int3(a.X > b.X ? a.X : b.X, a.Y > b.Y ? a.Y : b.Y, a.Z > b.Z ? a.Z : b.Z);
    }

    // Returns a vector containing the smallest components of the specified vectors
    // @param a The first source vector
    // @param b The second source vector
    static Int3 Min(const Int3& a, const Int3& b)
    {
        return Int3(a.X < b.X ? a.X : b.X, a.Y < b.Y ? a.Y : b.Y, a.Z < b.Z ? a.Z : b.Z);
    }

    // Returns a vector containing the largest components of the specified vectors
    // @param a The first source vector
    // @param b The second source vector
    // @param result When the method completes, contains an new vector composed of the largest components of the source vectors
    static void Max(const Int3& a, const Int3& b, Int3* result)
    {
        *result = Int3(a.X > b.X ? a.X : b.X, a.Y > b.Y ? a.Y : b.Y, a.Z > b.Z ? a.Z : b.Z);
    }

    // Returns a vector containing the smallest components of the specified vectors
    // @param a The first source vector
    // @param b The second source vector
    // @param result When the method completes, contains an new vector composed of the smallest components of the source vectors
    static void Min(const Int3& a, const Int3& b, Int3* result)
    {
        *result = Int3(a.X < b.X ? a.X : b.X, a.Y < b.Y ? a.Y : b.Y, a.Z < b.Z ? a.Z : b.Z);
    }
};

template<>
struct TIsPODType<Int3>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(Int3, "X:{0} Y:{1} Z:{2}", v.X, v.Y, v.Z);
