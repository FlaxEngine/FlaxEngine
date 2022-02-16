// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Math.h"
#include "Engine/Core/Formatting.h"
#include "Engine/Core/Templates.h"

/// <summary>
/// Two-components vector (32 bit integer type).
/// </summary>
API_STRUCT() struct FLAXENGINE_API Int2
{
DECLARE_SCRIPTING_TYPE_MINIMAL(Int2);
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
        };

        // Raw values
        int32 Raw[2];
    };

public:

    // Vector with all components equal 0
    static const Int2 Zero;

    // Vector with all components equal 1
    static const Int2 One;

    // A minimum Int2
    static const Int2 Minimum;

    // A maximum Int2
    static const Int2 Maximum;

public:

    /// <summary>
    /// Empty constructor.
    /// </summary>
    Int2()
    {
    }

    // Init
    // @param xy Value to assign to the all components
    Int2(int32 xy)
        : X(xy)
        , Y(xy)
    {
    }

    // Init
    // @param x X component value
    // @param y Y component value
    Int2(int32 x, int32 y)
        : X(x)
        , Y(y)
    {
    }

    // Init
    // @param xyz Int3 to use X and Y components
    Int2(const Int3& xyz);

    // Init
    // @param xyzw Int4 to use X and Y components
    Int2(const Int4& xyzw);
    
    // Init
    // @param xy Vector2 to use X and Y components
    explicit Int2(const Vector2& xy);

    // Init
    // @param xyz Vector3 to use X and Y components
    explicit Int2(const Vector3& xyz);

    // Init
    // @param xyzw Vector4 to use X and Y components
    explicit Int2(const Vector4& xyzw);
    
public:

    String ToString() const;

public:

    // Arithmetic operators with Int2

    Int2 operator+(const Int2& b) const
    {
        return Add(*this, b);
    }

    Int2 operator-(const Int2& b) const
    {
        return Subtract(*this, b);
    }

    Int2 operator*(const Int2& b) const
    {
        return Multiply(*this, b);
    }

    Int2 operator/(const Int2& b) const
    {
        return Divide(*this, b);
    }

    Int2 operator-() const
    {
        return Int2(-X, -Y);
    }

    // op= operators with Int2

    Int2& operator+=(const Int2& b)
    {
        *this = Add(*this, b);
        return *this;
    }

    Int2& operator-=(const Int2& b)
    {
        *this = Subtract(*this, b);
        return *this;
    }

    Int2& operator*=(const Int2& b)
    {
        *this = Multiply(*this, b);
        return *this;
    }

    Int2& operator/=(const Int2& b)
    {
        *this = Divide(*this, b);
        return *this;
    }

    // Arithmetic operators with int32

    Int2 operator+(int32 b) const
    {
        return Add(*this, b);
    }

    Int2 operator-(int32 b) const
    {
        return Subtract(*this, b);
    }

    Int2 operator*(int32 b) const
    {
        return Multiply(*this, b);
    }

    Int2 operator/(int32 b) const
    {
        return Divide(*this, b);
    }

    // op= operators with int32

    Int2& operator+=(int32 b)
    {
        *this = Add(*this, b);
        return *this;
    }

    Int2& operator-=(int32 b)
    {
        *this = Subtract(*this, b);
        return *this;
    }

    Int2& operator*=(int32 b)
    {
        *this = Multiply(*this, b);
        return *this;
    }

    Int2& operator/=(int32 b)
    {
        *this = Divide(*this, b);
        return *this;
    }

    // Comparison operators

    bool operator==(const Int2& b) const
    {
        return X == b.X && Y == b.Y;
    }

    bool operator!=(const Int2& b) const
    {
        return X != b.X || Y != b.Y;
    }

    bool operator>(const Int2& b) const
    {
        return X > b.X && Y > b.Y;
    }

    bool operator>=(const Int2& b) const
    {
        return X >= b.X && Y >= b.Y;
    }

    bool operator<(const Int2& b) const
    {
        return X < b.X && Y < b.Y;
    }

    bool operator<=(const Int2& b) const
    {
        return X <= b.X && Y <= b.Y;
    }

public:

    static void Add(const Int2& a, const Int2& b, Int2& result)
    {
        result.X = a.X + b.X;
        result.Y = a.Y + b.Y;
    }

    static Int2 Add(const Int2& a, const Int2& b)
    {
        Int2 result;
        Add(a, b, result);
        return result;
    }

    static void Subtract(const Int2& a, const Int2& b, Int2& result)
    {
        result.X = a.X - b.X;
        result.Y = a.Y - b.Y;
    }

    static Int2 Subtract(const Int2& a, const Int2& b)
    {
        Int2 result;
        Subtract(a, b, result);
        return result;
    }

    static Int2 Multiply(const Int2& a, const Int2& b)
    {
        return Int2(a.X * b.X, a.Y * b.Y);
    }

    static Int2 Multiply(const Int2& a, int32 b)
    {
        return Int2(a.X * b, a.Y * b);
    }

    static Int2 Divide(const Int2& a, const Int2& b)
    {
        return Int2(a.X / b.X, a.Y / b.Y);
    }

    static Int2 Divide(const Int2& a, int32 b)
    {
        return Int2(a.X / b, a.Y / b);
    }

    /// <summary>
    /// Gets a value indicting whether this vector is zero.
    /// </summary>
    /// <returns> True if the vector is zero, otherwise false.</returns>
    bool IsZero() const
    {
        return X == 0 && Y == 0;
    }

    /// <summary>
    /// Gets a value indicting whether any vector component is zero.
    /// </summary>
    /// <returns> True if a component is zero, otherwise false.</returns>
    bool IsAnyZero() const
    {
        return X == 0 || Y == 0;
    }

    /// <summary>
    /// Gets a value indicting whether this vector is one.
    /// </summary>
    /// <returns> True if the vector is one, otherwise false.</returns>
    bool IsOne() const
    {
        return X == 1 && Y == 1;
    }
    
    /// <summary>
    /// Calculates a vector with values being opposite to values of that vector
    /// </summary>
    /// <returns>Negative vector</returns>
    Int2 GetNegative() const
    {
        return Int2(-X, -Y);
    }
    
    /// <summary>
    /// Returns average arithmetic of all the components
    /// </summary>
    /// <returns>Average arithmetic of all the components</returns>
    float AverageArithmetic() const
    {
        return (X + Y) * 0.5f;
    }

    /// <summary>
    /// Gets sum of all vector components values
    /// </summary>
    /// <returns>Sum of X, Y, Z and W</returns>
    int32 SumValues() const
    {
        return X + Y;
    }

    /// <summary>
    /// Returns minimum value of all the components
    /// </summary>
    /// <returns>Minimum value</returns>
    int32 MinValue() const
    {
        return Math::Min(X, Y);
    }

    /// <summary>
    /// Returns maximum value of all the components
    /// </summary>
    /// <returns>Maximum value</returns>
    int32 MaxValue() const
    {
        return Math::Max(X, Y);
    }

    
    // Returns a vector containing the smallest components of the specified vectors
    // @param a The first source vector
    // @param b The second source vector
    static Int2 Min(const Int2& a, const Int2& b)
    {
        return Int2(a.X < b.X ? a.X : b.X, a.Y < b.Y ? a.Y : b.Y);
    }

    // Returns a vector containing the largest components of the specified vectors
    // @param a The first source vector
    // @param b The second source vector
    static Int2 Max(const Int2& a, const Int2& b)
    {
        return Int2(a.X > b.X ? a.X : b.X, a.Y > b.Y ? a.Y : b.Y);
    }

    // Returns a vector containing the smallest components of the specified vectors
    // @param a The first source vector
    // @param b The second source vector
    // @param result When the method completes, contains an new vector composed of the smallest components of the source vectors
    static void Min(const Int2& a, const Int2& b, Int2& result)
    {
        result = Int2(a.X < b.X ? a.X : b.X, a.Y < b.Y ? a.Y : b.Y);
    }

    // Returns a vector containing the largest components of the specified vectors
    // @param a The first source vector
    // @param b The second source vector
    // @param result When the method completes, contains an new vector composed of the largest components of the source vectors
    static void Max(const Int2& a, const Int2& b, Int2& result)
    {
        result = Int2(a.X > b.X ? a.X : b.X, a.Y > b.Y ? a.Y : b.Y);
    }
};

template<>
struct TIsPODType<Int2>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(Int2, "X:{0} Y:{1}", v.X, v.Y);
