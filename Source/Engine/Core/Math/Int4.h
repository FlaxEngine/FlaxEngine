// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Math.h"
#include "Engine/Core/Formatting.h"
#include "Engine/Core/Templates.h"

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

    // A minimum Int4
    static const Int4 Minimum;

    // A maximum Int4
    static const Int4 Maximum;
    
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
    Int4(const Int2& xy, int32 z, int32 w);

    // Init
    // @param v Int3 to use X , Y and Z components
    // @param w W component value
    Int4(const Int3& xyz, int32 w);

    // Init
    // @param v Vector2 to use X and Y components
    // @param z Z component value
    // @param w W component value
    explicit Int4(const Vector2& xy, int32 z, int32 w);

    // Init
    // @param v Vector3 to use X , Y and Z components
    // @param w W component value
    explicit Int4(const Vector3& xyz, int32 w);
    
    // Init
    // @param v Vector to use X, Y, Z and W components
    explicit Int4(const Vector4& xyzw);

public:

    String ToString() const;

public:

    // Arithmetic operators with Int2

    Int4 operator+(const Int4& b) const
    {
        return Add(*this, b);
    }

    Int4 operator-(const Int4& b) const
    {
        return Subtract(*this, b);
    }

    Int4 operator*(const Int4& b) const
    {
        return Multiply(*this, b);
    }

    Int4 operator/(const Int4& b) const
    {
        return Divide(*this, b);
    }

    Int4 operator-() const
    {
        return Int4(-X, -Y, -Z, -W);
    }

    // op= operators with Int2

    Int4& operator+=(const Int4& b)
    {
        *this = Add(*this, b);
        return *this;
    }

    Int4& operator-=(const Int4& b)
    {
        *this = Subtract(*this, b);
        return *this;
    }

    Int4& operator*=(const Int4& b)
    {
        *this = Multiply(*this, b);
        return *this;
    }

    Int4& operator/=(const Int4& b)
    {
        *this = Divide(*this, b);
        return *this;
    }

    // Arithmetic operators with int32

    Int4 operator+(int32 b) const
    {
        return Add(*this, b);
    }

    Int4 operator-(int32 b) const
    {
        return Subtract(*this, b);
    }

    Int4 operator*(int32 b) const
    {
        return Multiply(*this, b);
    }

    Int4 operator/(int32 b) const
    {
        return Divide(*this, b);
    }

    // op= operators with int32

    Int4& operator+=(int32 b)
    {
        *this = Add(*this, b);
        return *this;
    }

    Int4& operator-=(int32 b)
    {
        *this = Subtract(*this, b);
        return *this;
    }

    Int4& operator*=(int32 b)
    {
        *this = Multiply(*this, b);
        return *this;
    }

    Int4& operator/=(int32 b)
    {
        *this = Divide(*this, b);
        return *this;
    }

    // Comparison operators

    bool operator==(const Int4& b) const
    {
        return X == b.X && Y == b.Y;
    }

    bool operator!=(const Int4& b) const
    {
        return X != b.X || Y != b.Y;
    }

    bool operator>(const Int4& b) const
    {
        return X > b.X && Y > b.Y;
    }

    bool operator>=(const Int4& b) const
    {
        return X >= b.X && Y >= b.Y;
    }

    bool operator<(const Int4& b) const
    {
        return X < b.X && Y < b.Y;
    }

    bool operator<=(const Int4& b) const
    {
        return X <= b.X && Y <= b.Y;
    }

public:

    static void Add(const Int4& a, const Int4& b, Int4& result)
    {
        result.X = a.X + b.X;
        result.Y = a.Y + b.Y;
        result.Z = a.Z + b.Z;
        result.W = a.W + b.W;
    }

    static Int4 Add(const Int4& a, const Int4& b)
    {
        Int4 result;
        Add(a, b, result);
        return result;
    }

    static void Subtract(const Int4& a, const Int4& b, Int4& result)
    {
        result.X = a.X - b.X;
        result.Y = a.Y - b.Y;
        result.Z = a.Z - b.Z;
        result.W = a.W - b.W;
    }

    static Int4 Subtract(const Int4& a, const Int4& b)
    {
        Int4 result;
        Subtract(a, b, result);
        return result;
    }

    static Int4 Multiply(const Int4& a, const Int4& b)
    {
        return Int4(a.X * b.X, a.Y * b.Y, a.Z * b.Z, a.W * b.W);
    }

    static Int4 Multiply(const Int4& a, int32 b)
    {
        return Int4(a.X * b, a.Y * b, a.Z * b, a.W * b);
    }

    static Int4 Divide(const Int4& a, const Int4& b)
    {
        return Int4(a.X / b.X, a.Y / b.Y, a.Z / b.Z, a.W / b.W);
    }

    static Int4 Divide(const Int4& a, int32 b)
    {
        return Int4(a.X / b, a.Y / b, a.Z / b, a.Y / b);
    }
    
public:

    /// <summary>
    /// Gets a value indicting whether this vector is zero.
    /// </summary>
    /// <returns> True if the vector is zero, otherwise false.</returns>
    bool IsZero() const
    {
        return X == 0 && Y == 0 && Z == 0 && W == 0;
    }

    /// <summary>
    /// Gets a value indicting whether any vector component is zero.
    /// </summary>
    /// <returns> True if a component is zero, otherwise false.</returns>
    bool IsAnyZero() const
    {
        return X == 0 || Y == 0 || Z == 0 || W == 0;
    }

    /// <summary>
    /// Gets a value indicting whether this vector is one.
    /// </summary>
    /// <returns> True if the vector is one, otherwise false.</returns>
    bool IsOne() const
    {
        return X == 1 && Y == 1 && Z == 1 && W == 1;
    }
    
    /// <summary>
    /// Calculates a vector with values being opposite to values of that vector
    /// </summary>
    /// <returns>Negative vector</returns>
    Int4 GetNegative() const
    {
        return Int4(-X, -Y, -Z, -W);
    }
    
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
