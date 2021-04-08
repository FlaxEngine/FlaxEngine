// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Math.h"
#include "Engine/Core/Formatting.h"
#include "Engine/Core/Templates.h"


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
        };

        // Raw values
        int32 Raw[3];
    };

public:

    // Vector with all components equal 0
    static const Int3 Zero;

    // Vector with all components equal 1
    static const Int3 One;

    // A minimum Int3
    static const Int3 Minimum;

    // A maximum Int3
    static const Int3 Maximum;

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

    // Init
    // @param v Int2 to use X and Y components
    // @param z Z component value
    Int3(const Int2& xy, int32 z);

    // Init
    // @param v Int4 to use X and Y components
    Int3(const Int4& xyzw);

    // Init
    // @param v Vector2 to use X and Y components
    // @param z Z component value
    explicit Int3(const Vector2& xy, int32 z);
    
    // Init
    // @param v Vector3 to use X, Y and Z components
    explicit Int3(const Vector3& xyz);

    // Init
    // @param v Vector4 to use X and Y components
    explicit Int3(const Vector4& xyzw);
    
public:

    String ToString() const;

public:

    // Arithmetic operators with Int2

    Int3 operator+(const Int3& b) const
    {
        return Add(*this, b);
    }

    Int3 operator-(const Int3& b) const
    {
        return Subtract(*this, b);
    }

    Int3 operator*(const Int3& b) const
    {
        return Multiply(*this, b);
    }

    Int3 operator/(const Int3& b) const
    {
        return Divide(*this, b);
    }

    Int3 operator-() const
    {
        return Int3(-X, -Y, -Z);
    }

    // op= operators with Int2

    Int3& operator+=(const Int3& b)
    {
        *this = Add(*this, b);
        return *this;
    }

    Int3& operator-=(const Int3& b)
    {
        *this = Subtract(*this, b);
        return *this;
    }

    Int3& operator*=(const Int3& b)
    {
        *this = Multiply(*this, b);
        return *this;
    }

    Int3& operator/=(const Int3& b)
    {
        *this = Divide(*this, b);
        return *this;
    }

    // Arithmetic operators with int32

    Int3 operator+(int32 b) const
    {
        return Add(*this, b);
    }

    Int3 operator-(int32 b) const
    {
        return Subtract(*this, b);
    }

    Int3 operator*(int32 b) const
    {
        return Multiply(*this, b);
    }

    Int3 operator/(int32 b) const
    {
        return Divide(*this, b);
    }

    // op= operators with int32

    Int3& operator+=(int32 b)
    {
        *this = Add(*this, b);
        return *this;
    }

    Int3& operator-=(int32 b)
    {
        *this = Subtract(*this, b);
        return *this;
    }

    Int3& operator*=(int32 b)
    {
        *this = Multiply(*this, b);
        return *this;
    }

    Int3& operator/=(int32 b)
    {
        *this = Divide(*this, b);
        return *this;
    }

    // Comparison operators

    bool operator==(const Int3& b) const
    {
        return X == b.X && Y == b.Y;
    }

    bool operator!=(const Int3& b) const
    {
        return X != b.X || Y != b.Y;
    }

    bool operator>(const Int3& b) const
    {
        return X > b.X && Y > b.Y;
    }

    bool operator>=(const Int3& b) const
    {
        return X >= b.X && Y >= b.Y;
    }

    bool operator<(const Int3& b) const
    {
        return X < b.X && Y < b.Y;
    }

    bool operator<=(const Int3& b) const
    {
        return X <= b.X && Y <= b.Y;
    }

public:

    static void Add(const Int3& a, const Int3& b, Int3& result)
    {
        result.X = a.X + b.X;
        result.Y = a.Y + b.Y;
        result.Z = a.Z + b.Z;
    }

    static Int3 Add(const Int3& a, const Int3& b)
    {
        Int3 result;
        Add(a, b, result);
        return result;
    }

    static void Subtract(const Int3& a, const Int3& b, Int3& result)
    {
        result.X = a.X - b.X;
        result.Y = a.Y - b.Y;
        result.Z = a.Z - b.Z;
    }

    static Int3 Subtract(const Int3& a, const Int3& b)
    {
        Int3 result;
        Subtract(a, b, result);
        return result;
    }

    static Int3 Multiply(const Int3& a, const Int3& b)
    {
        return Int3(a.X * b.X, a.Y * b.Y, a.Z * b.Z);
    }

    static Int3 Multiply(const Int3& a, int32 b)
    {
        return Int3(a.X * b, a.Y * b, a.Z * b);
    }

    static Int3 Divide(const Int3& a, const Int3& b)
    {
        return Int3(a.X / b.X, a.Y / b.Y, a.Z / b.Z);
    }

    static Int3 Divide(const Int3& a, int32 b)
    {
        return Int3(a.X / b, a.Y / b, a.Z / b);
    }

public:

    /// <summary>
    /// Gets a value indicting whether this vector is zero.
    /// </summary>
    /// <returns> True if the vector is zero, otherwise false.</returns>
    bool IsZero() const
    {
        return X == 0 && Y == 0 && Z == 0;
    }

    /// <summary>
    /// Gets a value indicting whether any vector component is zero.
    /// </summary>
    /// <returns> True if a component is zero, otherwise false.</returns>
    bool IsAnyZero() const
    {
        return X == 0 || Y == 0 || Z == 0;
    }

    /// <summary>
    /// Gets a value indicting whether this vector is one.
    /// </summary>
    /// <returns> True if the vector is one, otherwise false.</returns>
    bool IsOne() const
    {
        return X == 1 && Y == 1 && Z == 1;
    }
    
    /// <summary>
    /// Calculates a vector with values being opposite to values of that vector
    /// </summary>
    /// <returns>Negative vector</returns>
    Int3 GetNegative() const
    {
        return Int3(-X, -Y, -Z);
    }
    
    /// <summary>
    /// Returns average arithmetic of all the components
    /// </summary>
    /// <returns>Average arithmetic of all the components</returns>
    float AverageArithmetic() const
    {
        return (X + Y + Z) / 3.0f;
    }

    /// <summary>
    /// Gets sum of all vector components values
    /// </summary>
    /// <returns>Sum of X, Y, Z and W</returns>
    int32 SumValues() const
    {
        return X + Y + Z;
    }

    /// <summary>
    /// Returns minimum value of all the components
    /// </summary>
    /// <returns>Minimum value</returns>
    int32 MinValue() const
    {
        return Math::Min(X, Y, Z);
    }

    /// <summary>
    /// Returns maximum value of all the components
    /// </summary>
    /// <returns>Maximum value</returns>
    int32 MaxValue() const
    {
        return Math::Max(X, Y, Z);
    }
    
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
    static void Max(const Int3& a, const Int3& b, Int3& result)
    {
        result = Int3(a.X > b.X ? a.X : b.X, a.Y > b.Y ? a.Y : b.Y, a.Z > b.Z ? a.Z : b.Z);
    }

    // Returns a vector containing the smallest components of the specified vectors
    // @param a The first source vector
    // @param b The second source vector
    // @param result When the method completes, contains an new vector composed of the smallest components of the source vectors
    static void Min(const Int3& a, const Int3& b, Int3 result)
    {
        result = Int3(a.X < b.X ? a.X : b.X, a.Y < b.Y ? a.Y : b.Y, a.Z < b.Z ? a.Z : b.Z);
    }
};

template<>
struct TIsPODType<Int3>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(Int3, "X:{0} Y:{1} Z:{2}", v.X, v.Y, v.Z);
