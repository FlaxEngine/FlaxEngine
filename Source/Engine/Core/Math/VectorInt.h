// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

#pragma once

#include "Math.h"
#include "Engine/Core/Formatting.h"
#include "Engine/Core/Templates.h"

struct Vector2;
struct Vector3;
struct Vector4;

/// <summary>
/// Two-components vector (32 bit integer type).
/// </summary>
API_STRUCT(InBuild) struct Int2
{
public:

    union
    {
        struct
        {
            // X component
            int32 X;

            // Y component
            int32 Y;
        };

        // Raw values
        int32 Raw[2];
    };

public:

    // Vector with all components equal 0
    static const Int2 Zero;

    // Vector with all components equal 1
    static const Int2 One;

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
    // @param v Vector to use X and Y components
    explicit Int2(const Vector2& v);

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

    static void Add(const Int2& a, const Int2& b, Int2* result)
    {
        result->X = a.X + b.X;
        result->Y = a.Y + b.Y;
    }

    static Int2 Add(const Int2& a, const Int2& b)
    {
        Int2 result;
        Add(a, b, &result);
        return result;
    }

    static void Subtract(const Int2& a, const Int2& b, Int2* result)
    {
        result->X = a.X - b.X;
        result->Y = a.Y - b.Y;
    }

    static Int2 Subtract(const Int2& a, const Int2& b)
    {
        Int2 result;
        Subtract(a, b, &result);
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

    // Creates vector from minimum components of two vectors
    static Int2 Min(const Int2& a, const Int2& b)
    {
        return Int2(a.X < b.X ? a.X : b.X, a.Y < b.Y ? a.Y : b.Y);
    }

    // Creates vector from maximum components of two vectors
    static Int2 Max(const Int2& a, const Int2& b)
    {
        return Int2(a.X > b.X ? a.X : b.X, a.Y > b.Y ? a.Y : b.Y);
    }
};

/// <summary>
/// Three-components vector (32 bit integer type).
/// </summary>
API_STRUCT(InBuild) struct Int3
{
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

    // Init
    // @param v Vector to use X, Y and Z components
    explicit Int3(const Vector3& v);

public:

    String ToString() const;

public:

    // Returns a vector containing the largest components of the specified vectors
    // @param a The first source vector
    // @param b The second source vector
    // @param result When the method completes, contains an new vector composed of the largest components of the source vectors
    static Int3 Max(const Int3& a, const Int3& b)
    {
        return Int3(a.X > b.X ? a.X : b.X, a.Y > b.Y ? a.Y : b.Y, a.Z > b.Z ? a.Z : b.Z);
    }

    // Returns a vector containing the smallest components of the specified vectors
    // @param a The first source vector
    // @param b The second source vector
    // @param result When the method completes, contains an new vector composed of the smallest components of the source vectors
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

/// <summary>
/// Four-components vector (32 bit integer type).
/// </summary>
API_STRUCT(InBuild) struct Int4
{
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
struct TIsPODType<Int2>
{
    enum { Value = true };
};

template<>
struct TIsPODType<Int3>
{
    enum { Value = true };
};

template<>
struct TIsPODType<Int4>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(Int2, "X:{0} Y:{1}", v.X, v.Y);

DEFINE_DEFAULT_FORMATTING(Int3, "X:{0} Y:{1} Z:{2}", v.X, v.Y, v.Z);

DEFINE_DEFAULT_FORMATTING(Int4, "X:{0} Y:{1} Z:{2} W:{3}", v.X, v.Y, v.Z, v.W);
