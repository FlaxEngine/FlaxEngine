// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/Platform.h"
#include "Engine/Core/Formatting.h"
#include "Engine/Core/Templates.h"
#include "Math.h"

struct Vector2;
struct Vector3;
struct Color;
struct Matrix;
struct Rectangle;
class String;

/// <summary>
/// Represents a four dimensional mathematical vector.
/// </summary>
API_STRUCT() struct FLAXENGINE_API Vector4
{
DECLARE_SCRIPTING_TYPE_MINIMAL(Vector4);
public:

    union
    {
        struct
        {
            /// <summary>
            /// The X component.
            /// </summary>
            API_FIELD() float X;

            /// <summary>
            /// The Y component.
            /// </summary>
            API_FIELD() float Y;

            /// <summary>
            /// The Z component.
            /// </summary>
            API_FIELD() float Z;

            /// <summary>
            /// The W component.
            /// </summary>
            API_FIELD() float W;
        };

        /// <summary>
        /// The raw vector values (in xyzw order).
        /// </summary>
        float Raw[4];
    };

public:

    // Vector with all components equal 0
    static const Vector4 Zero;

    // Vector with all components equal 1
    static const Vector4 One;

    // Vector X=1, Y=0, Z=0, W=0
    static const Vector4 UnitX;

    // Vector X=0, Y=1, Z=0, W=0
    static const Vector4 UnitY;

    // Vector X=0, Y=0, Z=1, W=0
    static const Vector4 UnitZ;

    // Vector X=0, Y=0, Z=0, W=1
    static const Vector4 UnitW;

    // A minimum Vector4
    static const Vector4 Minimum;

    // A maximum Vector4
    static const Vector4 Maximum;

public:

    /// <summary>
    /// Empty constructor.
    /// </summary>
    Vector4()
    {
    }

    // Init
    // @param xyzw Value to assign to the all components
    Vector4(float xyzw)
        : X(xyzw)
        , Y(xyzw)
        , Z(xyzw)
        , W(xyzw)
    {
    }

    // Init
    // @param xyzw Values to assign
    explicit Vector4(float xyzw[4]);

    // Init
    // @param x X component value
    // @param y Y component value
    // @param z Z component value
    // @param w W component value
    Vector4(float x, float y, float z, float w)
        : X(x)
        , Y(y)
        , Z(z)
        , W(w)
    {
    }

    // Init
    // @param xy X and Y values in the vector
    // @param z Z component value
    // @param w W component value
    Vector4(const Vector2& xy, float z, float w);

    // Init
    // @param xy X and Y values in the vector
    // @param zw Z and W values in the vector
    // @param z Z component value
    // @param w W component value
    Vector4(const Vector2& xy, const Vector2& zw);

    // Init
    // @param xyz X, Y and Z values in the vector
    // @param w W component value
    Vector4(const Vector3& xyz, float w);

    // Init
    // @param color Int4 value
    explicit Vector4(const Int4& xyzw);

    // Init
    // @param color Color value
    explicit Vector4(const Color& color);

    // Init
    // @param rect Rectangle value
    explicit Vector4(const Rectangle& rect);

public:

    String ToString() const;

public:

    // Gets a value indicting whether this vector is zero
    bool IsZero() const
    {
        return Math::IsZero(X) && Math::IsZero(Y) && Math::IsZero(Z) && Math::IsZero(W);
    }

    // Gets a value indicting whether any vector component is zero
    bool IsAnyZero() const
    {
        return Math::IsZero(X) || Math::IsZero(Y) || Math::IsZero(Z) || Math::IsZero(W);
    }

    // Gets a value indicting whether this vector is one
    bool IsOne() const
    {
        return Math::IsOne(X) && Math::IsOne(Y) && Math::IsOne(Z) && Math::IsOne(W);
    }

    /// <summary>
    /// Calculates a vector with values being absolute values of that vector
    /// </summary>
    /// <returns>Absolute vector</returns>
    Vector4 GetAbsolute() const
    {
        return Vector4(Math::Abs(X), Math::Abs(Y), Math::Abs(Z), Math::Abs(W));
    }

    /// <summary>
    /// Calculates a vector with values being opposite to values of that vector
    /// </summary>
    /// <returns>Negative vector</returns>
    Vector4 GetNegative() const
    {
        return Vector4(-X, -Y, -Z, -W);
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
    float SumValues() const
    {
        return X + Y + Z + W;
    }

    /// <summary>
    /// Returns minimum value of all the components
    /// </summary>
    /// <returns>Minimum value</returns>
    float MinValue() const
    {
        return Math::Min(X, Y, Z, W);
    }

    /// <summary>
    /// Returns maximum value of all the components
    /// </summary>
    /// <returns>Maximum value</returns>
    float MaxValue() const
    {
        return Math::Max(X, Y, Z, W);
    }

    /// <summary>
    /// Returns true if vector has one or more components is not a number (NaN)
    /// </summary>
    /// <returns>True if one or more components is not a number (NaN)</returns>
    bool IsNaN() const
    {
        return isnan(X) || isnan(Y) || isnan(Z) || isnan(W);
    }

    /// <summary>
    /// Returns true if vector has one or more components equal to +/- infinity
    /// </summary>
    /// <returns>True if one or more components equal to +/- infinity</returns>
    bool IsInfinity() const
    {
        return isinf(X) || isinf(Y) || isinf(Z) || isinf(W);
    }

    /// <summary>
    /// Returns true if vector has one or more components equal to +/- infinity or NaN
    /// </summary>
    /// <returns>True if one or more components equal to +/- infinity or NaN</returns>
    bool IsNanOrInfinity() const
    {
        return IsInfinity() || IsNaN();
    }

public:

    // Arithmetic operators with Vector4
    inline Vector4 operator+(const Vector4& b) const
    {
        return Add(*this, b);
    }

    inline Vector4 operator-(const Vector4& b) const
    {
        return Subtract(*this, b);
    }

    inline Vector4 operator*(const Vector4& b) const
    {
        return Multiply(*this, b);
    }

    inline Vector4 operator/(const Vector4& b) const
    {
        return Divide(*this, b);
    }

    // op= operators with Vector4
    inline Vector4& operator+=(const Vector4& b)
    {
        *this = Add(*this, b);
        return *this;
    }

    inline Vector4& operator-=(const Vector4& b)
    {
        *this = Subtract(*this, b);
        return *this;
    }

    inline Vector4& operator*=(const Vector4& b)
    {
        *this = Multiply(*this, b);
        return *this;
    }

    inline Vector4& operator/=(const Vector4& b)
    {
        *this = Divide(*this, b);
        return *this;
    }

    // Arithmetic operators with float
    inline Vector4 operator+(float b) const
    {
        return Add(*this, b);
    }

    inline Vector4 operator-(float b) const
    {
        return Subtract(*this, b);
    }

    inline Vector4 operator*(float b) const
    {
        return Multiply(*this, b);
    }

    inline Vector4 operator/(float b) const
    {
        return Divide(*this, b);
    }

    // op= operators with float
    inline Vector4& operator+=(float b)
    {
        *this = Add(*this, b);
        return *this;
    }

    inline Vector4& operator-=(float b)
    {
        *this = Subtract(*this, b);
        return *this;
    }

    inline Vector4& operator*=(float b)
    {
        *this = Multiply(*this, b);
        return *this;
    }

    inline Vector4& operator/=(float b)
    {
        *this = Divide(*this, b);
        return *this;
    }

    // Comparison
    inline bool operator==(const Vector4& b) const
    {
        return X == b.X && Y == b.Y && Z == b.Z && W == b.W;
    }

    inline bool operator!=(const Vector4& b) const
    {
        return X != b.X || Y != b.Y || Z != b.Z || W != b.W;
    }

    inline bool operator>(const Vector4& b) const
    {
        return X > b.X && Y > b.Y && Z > b.Z && W > b.W;
    }

    inline bool operator>=(const Vector4& b) const
    {
        return X >= b.X && Y >= b.Y && Z >= b.Z && W >= b.W;
    }

    inline bool operator<(const Vector4& b) const
    {
        return X < b.X && Y < b.Y && Z < b.Z && W < b.W;
    }

    inline bool operator<=(const Vector4& b) const
    {
        return X <= b.X && Y <= b.Y && Z <= b.Z && W <= b.W;
    }

public:

    static bool NearEqual(const Vector4& a, const Vector4& b)
    {
        return Math::NearEqual(a.X, b.X) && Math::NearEqual(a.Y, b.Y) && Math::NearEqual(a.Z, b.Z) && Math::NearEqual(a.W, b.W);
    }

    static bool NearEqual(const Vector4& a, const Vector4& b, float epsilon)
    {
        return Math::NearEqual(a.X, b.X, epsilon) && Math::NearEqual(a.Y, b.Y, epsilon) && Math::NearEqual(a.Z, b.Z, epsilon) && Math::NearEqual(a.W, b.W, epsilon);
    }

public:

    static void Add(const Vector4& a, const Vector4& b, Vector4& result)
    {
        result.X = a.X + b.X;
        result.Y = a.Y + b.Y;
        result.Z = a.Z + b.Z;
        result.W = a.W + b.W;
    }

    static Vector4 Add(const Vector4& a, const Vector4& b)
    {
        Vector4 result;
        Add(a, b, result);
        return result;
    }

    static void Subtract(const Vector4& a, const Vector4& b, Vector4& result)
    {
        result.X = a.X - b.X;
        result.Y = a.Y - b.Y;
        result.Z = a.Z - b.Z;
        result.W = a.W - b.W;
    }

    static Vector4 Subtract(const Vector4& a, const Vector4& b)
    {
        Vector4 result;
        Subtract(a, b, result);
        return result;
    }

    static Vector4 Multiply(const Vector4& a, const Vector4& b)
    {
        return Vector4(a.X * b.X, a.Y * b.Y, a.Z * b.Z, a.W * b.W);
    }

    static Vector4 Multiply(const Vector4& a, float b)
    {
        return Vector4(a.X * b, a.Y * b, a.Z * b, a.W * b);
    }

    static Vector4 Divide(const Vector4& a, const Vector4& b)
    {
        return Vector4(a.X / b.X, a.Y / b.Y, a.Z / b.Z, a.W / b.W);
    }

    static Vector4 Divide(const Vector4& a, float b)
    {
        return Vector4(a.X / b, a.Y / b, a.Z / b, a.W / b);
    }

    static Vector4 Floor(const Vector4& v);
    static Vector4 Frac(const Vector4& v);
    static Vector4 Round(const Vector4& v);
    static Vector4 Ceil(const Vector4& v);

public:

    // Restricts a value to be within a specified range
    // @param value The value to clamp
    // @param min The minimum value,
    // @param max The maximum value
    // @returns Clamped value
    static Vector4 Clamp(const Vector4& value, const Vector4& min, const Vector4& max);

    // Restricts a value to be within a specified range
    // @param value The value to clamp
    // @param min The minimum value,
    // @param max The maximum value
    // @param result When the method completes, contains the clamped value
    static void Clamp(const Vector4& value, const Vector4& min, const Vector4& max, Vector4& result);

    // Performs a linear interpolation between two vectors
    // @param start Start vector
    // @param end End vector
    // @param amount Value between 0 and 1 indicating the weight of end
    // @param result When the method completes, contains the linear interpolation of the two vectors
    static void Lerp(const Vector4& start, const Vector4& end, float amount, Vector4& result)
    {
        result.X = Math::Lerp(start.X, end.X, amount);
        result.Y = Math::Lerp(start.Y, end.Y, amount);
        result.Z = Math::Lerp(start.Z, end.Z, amount);
        result.W = Math::Lerp(start.W, end.W, amount);
    }

    // <summary>
    // Performs a linear interpolation between two vectors.
    // </summary>
    // @param start Start vector,
    // @param end End vector,
    // @param amount Value between 0 and 1 indicating the weight of @paramref end"/>,
    // @returns The linear interpolation of the two vectors
    static Vector4 Lerp(const Vector4& start, const Vector4& end, float amount)
    {
        Vector4 result;
        Lerp(start, end, amount, result);
        return result;
    }

    static Vector4 Transform(const Vector4& v, const Matrix& m);
};

inline Vector4 operator+(float a, const Vector4& b)
{
    return b + a;
}

inline Vector4 operator-(float a, const Vector4& b)
{
    return Vector4(a) - b;
}

inline Vector4 operator*(float a, const Vector4& b)
{
    return b * a;
}

inline Vector4 operator/(float a, const Vector4& b)
{
    return Vector4(a) / b;
}

namespace Math
{
    FORCE_INLINE static bool NearEqual(const Vector4& a, const Vector4& b)
    {
        return Vector4::NearEqual(a, b);
    }
}

template<>
struct TIsPODType<Vector4>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(Vector4, "X:{0} Y:{1} Z:{2} W:{3}", v.X, v.Y, v.Z, v.W);
