// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Math.h"
#include "Engine/Core/Formatting.h"
#include "Engine/Core/Templates.h"

struct Vector3;
struct Vector4;
struct Int2;
struct Int3;
struct Int4;
struct Color;
struct Matrix;

/// <summary>
/// Represents a two dimensional mathematical vector.
/// </summary>
API_STRUCT() struct FLAXENGINE_API Vector2
{
DECLARE_SCRIPTING_TYPE_MINIMAL(Vector2);
public:

    union
    {
        struct
        {
            /// <summary>
            /// The X component of the vector.
            /// </summary>
            API_FIELD() float X;

            /// <summary>
            /// The Y component of the vector.
            /// </summary>
            API_FIELD() float Y;
        };

        // Raw values
        float Raw[2];
    };

public:

    // Vector with all components equal 0
    static const Vector2 Zero;

    // Vector with all components equal 1
    static const Vector2 One;

    // Vector X=1, Y=0
    static const Vector2 UnitX;

    // Vector X=0, Y=1
    static const Vector2 UnitY;

    // A minimum Vector2
    static const Vector2 Minimum;

    // A maximum Vector2
    static const Vector2 Maximum;

public:

    /// <summary>
    /// Empty constructor.
    /// </summary>
    Vector2()
    {
    }

    // Init
    // @param xy Value to assign to the all components
    Vector2(float xy)
        : X(xy)
        , Y(xy)
    {
    }

    // Init
    // @param x X component value
    // @param y Y component value
    Vector2(float x, float y)
        : X(x)
        , Y(y)
    {
    }

    // Init
    // @param v Int2 to use X and Y components
    explicit Vector2(const Int2& xy);

    // Init
    // @param v Int3 to use X and Y components
    explicit Vector2(const Int3& xyz);

    // Init
    // @param v Int4 to use X and Y components
    explicit Vector2(const Int4& xyzw);
    
    // Init
    // @param v Vector3 to use X and Y components
    explicit Vector2(const Vector3& xyz);

    // Init
    // @param v Vector4 to use X and Y components
    explicit Vector2(const Vector4& xyzw);

    // Init
    // @param color Color value
    explicit Vector2(const Color& color);

public:

    String ToString() const;

public:

    // Arithmetic operators with Vector2
    Vector2 operator+(const Vector2& b) const
    {
        return Add(*this, b);
    }

    Vector2 operator-(const Vector2& b) const
    {
        return Subtract(*this, b);
    }

    Vector2 operator*(const Vector2& b) const
    {
        return Multiply(*this, b);
    }

    Vector2 operator/(const Vector2& b) const
    {
        return Divide(*this, b);
    }

    Vector2 operator-() const
    {
        return Vector2(-X, -Y);
    }

    // op= operators with Vector2
    Vector2& operator+=(const Vector2& b)
    {
        *this = Add(*this, b);
        return *this;
    }

    Vector2& operator-=(const Vector2& b)
    {
        *this = Subtract(*this, b);
        return *this;
    }

    Vector2& operator*=(const Vector2& b)
    {
        *this = Multiply(*this, b);
        return *this;
    }

    Vector2& operator/=(const Vector2& b)
    {
        *this = Divide(*this, b);
        return *this;
    }

    // Arithmetic operators with float
    Vector2 operator+(float b) const
    {
        return Add(*this, b);
    }

    Vector2 operator-(float b) const
    {
        return Subtract(*this, b);
    }

    Vector2 operator*(float b) const
    {
        return Multiply(*this, b);
    }

    Vector2 operator/(float b) const
    {
        return Divide(*this, b);
    }

    // op= operators with float
    Vector2& operator+=(float b)
    {
        *this = Add(*this, b);
        return *this;
    }

    Vector2& operator-=(float b)
    {
        *this = Subtract(*this, b);
        return *this;
    }

    Vector2& operator*=(float b)
    {
        *this = Multiply(*this, b);
        return *this;
    }

    Vector2& operator/=(float b)
    {
        *this = Divide(*this, b);
        return *this;
    }

    // Comparison operators
    bool operator==(const Vector2& b) const
    {
        return X == b.X && Y == b.Y;
    }

    bool operator!=(const Vector2& b) const
    {
        return X != b.X || Y != b.Y;
    }

    bool operator>(const Vector2& b) const
    {
        return X > b.X && Y > b.Y;
    }

    bool operator>=(const Vector2& b) const
    {
        return X >= b.X && Y >= b.Y;
    }

    bool operator<(const Vector2& b) const
    {
        return X < b.X && Y < b.Y;
    }

    bool operator<=(const Vector2& b) const
    {
        return X <= b.X && Y <= b.Y;
    }

public:

    static bool NearEqual(const Vector2& a, const Vector2& b)
    {
        return Math::NearEqual(a.X, b.X) && Math::NearEqual(a.Y, b.Y);
    }

    static bool NearEqual(const Vector2& a, const Vector2& b, float epsilon)
    {
        return Math::NearEqual(a.X, b.X, epsilon) && Math::NearEqual(a.Y, b.Y, epsilon);
    }

public:

    static float Dot(const Vector2& a, const Vector2& b)
    {
        return a.X * b.X + a.Y * b.Y;
    }

    static float Cross(const Vector2& a, const Vector2& b)
    {
        return a.X * b.Y - a.Y * b.X;
    }

    static void Add(const Vector2& a, const Vector2& b, Vector2& result)
    {
        result.X = a.X + b.X;
        result.Y = a.Y + b.Y;
    }

    static Vector2 Add(const Vector2& a, const Vector2& b)
    {
        Vector2 result;
        Add(a, b, result);
        return result;
    }

    static void Subtract(const Vector2& a, const Vector2& b, Vector2& result)
    {
        result.X = a.X - b.X;
        result.Y = a.Y - b.Y;
    }

    static Vector2 Subtract(const Vector2& a, const Vector2& b)
    {
        Vector2 result;
        Subtract(a, b, result);
        return result;
    }

    static Vector2 Multiply(const Vector2& a, const Vector2& b)
    {
        return Vector2(a.X * b.X, a.Y * b.Y);
    }

    static Vector2 Multiply(const Vector2& a, float b)
    {
        return Vector2(a.X * b, a.Y * b);
    }

    static Vector2 Divide(const Vector2& a, const Vector2& b)
    {
        return Vector2(a.X / b.X, a.Y / b.Y);
    }

    static Vector2 Divide(const Vector2& a, float b)
    {
        return Vector2(a.X / b, a.Y / b);
    }

    // Calculates distance between two points in 2D
    // @param a 1st point
    // @param b 2nd point
    // @returns Distance
    static float Distance(const Vector2& a, const Vector2& b)
    {
        const float x = a.X - b.X;
        const float y = a.Y - b.Y;
        return Math::Sqrt(x * x + y * y);
    }

    // Calculates the squared distance between two points in 2D
    // @param a 1st point
    // @param b 2nd point
    // @returns Distance
    static float DistanceSquared(const Vector2& a, const Vector2& b)
    {
        const float x = a.X - b.X;
        const float y = a.Y - b.Y;
        return x * x + y * y;
    }

    // Clamp vector values within given range
    // @param v Vector to clamp
    // @param min Minimum value
    // @param max Maximum value
    // @returns Clamped vector
    static Vector2 Clamp(const Vector2& v, float min, float max)
    {
        return Vector2(Math::Clamp(v.X, min, max), Math::Clamp(v.Y, min, max));
    }

    // Clamp vector values within given range
    // @param v Vector to clamp
    // @param min Minimum value
    // @param max Maximum value
    // @returns Clamped vector
    static Vector2 Clamp(const Vector2& v, const Vector2& min, const Vector2& max)
    {
        return Vector2(Math::Clamp(v.X, min.X, max.X), Math::Clamp(v.Y, min.Y, max.Y));
    }

    // Performs vector normalization (scales vector up to unit length)
    void Normalize()
    {
        const float length = Length();
        if (!Math::IsZero(length))
        {
            const float invLength = 1.0f / length;
            X *= invLength;
            Y *= invLength;
        }
    }

public:

    // Gets a value indicting whether this instance is normalized
    bool IsNormalized() const
    {
        return Math::IsOne(X * X + Y * Y);
    }

    // Gets a value indicting whether this vector is zero
    bool IsZero() const
    {
        return Math::IsZero(X) && Math::IsZero(Y);
    }

    // Gets a value indicting whether any vector component is zero
    bool IsAnyZero() const
    {
        return Math::IsZero(X) || Math::IsZero(Y);
    }

    // Gets a value indicting whether this vector is zero
    bool IsOne() const
    {
        return Math::IsOne(X) && Math::IsOne(Y);
    }

    // Calculates length of the vector
    // @returns Length of the vector
    float Length() const
    {
        return Math::Sqrt(X * X + Y * Y);
    }

    // Calculates the squared length of the vector
    // @returns The squared length of the vector
    float LengthSquared() const
    {
        return X * X + Y * Y;
    }

    // Calculates inverted length of the vector (1 / Length())
    float InvLength() const
    {
        return 1.0f / Length();
    }

    // Calculates a vector with values being absolute values of that vector
    Vector2 GetAbsolute() const
    {
        return Vector2(Math::Abs(X), Math::Abs(Y));
    }

    // Calculates a vector with values being opposite to values of that vector
    Vector2 GetNegative() const
    {
        return Vector2(-X, -Y);
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
    /// <returns>Sum of X,Y and Z</returns>
    float SumValues() const
    {
        return X + Y;
    }

    /// <summary>
    /// Gets multiplication result of all vector components values
    /// </summary>
    /// <returns>X * Y</returns>
    float MulValues() const
    {
        return X * Y;
    }

    /// <summary>
    /// Returns minimum value of all the components
    /// </summary>
    /// <returns>Minimum value</returns>
    float MinValue() const
    {
        return Math::Min(X, Y);
    }

    /// <summary>
    /// Returns maximum value of all the components
    /// </summary>
    /// <returns>Maximum value</returns>
    float MaxValue() const
    {
        return Math::Max(X, Y);
    }

    /// <summary>
    /// Returns true if vector has one or more components is not a number (NaN)
    /// </summary>
    /// <returns>True if one or more components is not a number (NaN)</returns>
    bool IsNaN() const
    {
        return isnan(X) || isnan(Y);
    }

    /// <summary>
    /// Returns true if vector has one or more components equal to +/- infinity
    /// </summary>
    /// <returns>True if one or more components equal to +/- infinity</returns>
    bool IsInfinity() const
    {
        return isinf(X) || isinf(Y);
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

    // Performs a linear interpolation between two vectors
    // @param start Start vector
    // @param end End vector
    // @param amount Value between 0 and 1 indicating the weight of end
    // @param result When the method completes, contains the linear interpolation of the two vectors
    static void Lerp(const Vector2& start, const Vector2& end, float amount, Vector2& result)
    {
        result.X = Math::Lerp(start.X, end.X, amount);
        result.Y = Math::Lerp(start.Y, end.Y, amount);
    }

    // <summary>
    // Performs a linear interpolation between two vectors.
    // </summary>
    // @param start Start vector,
    // @param end End vector,
    // @param amount Value between 0 and 1 indicating the weight of @paramref end"/>,
    // @returns The linear interpolation of the two vectors
    static Vector2 Lerp(const Vector2& start, const Vector2& end, float amount)
    {
        Vector2 result;
        Lerp(start, end, amount, result);
        return result;
    }

    static Vector2 Abs(const Vector2& v)
    {
        return Vector2(Math::Abs(v.X), Math::Abs(v.Y));
    }

    // Creates vector from minimum components of two vectors
    static Vector2 Min(const Vector2& a, const Vector2& b)
    {
        return Vector2(a.X < b.X ? a.X : b.X, a.Y < b.Y ? a.Y : b.Y);
    }

    // Creates vector from minimum components of two vectors
    static void Min(const Vector2& a, const Vector2& b, Vector2& result)
    {
        result = Vector2(a.X < b.X ? a.X : b.X, a.Y < b.Y ? a.Y : b.Y);
    }

    // Creates vector from maximum components of two vectors
    static Vector2 Max(const Vector2& a, const Vector2& b)
    {
        return Vector2(a.X > b.X ? a.X : b.X, a.Y > b.Y ? a.Y : b.Y);
    }

    // Creates vector from maximum components of two vectors
    static void Max(const Vector2& a, const Vector2& b, Vector2& result)
    {
        result = Vector2(a.X > b.X ? a.X : b.X, a.Y > b.Y ? a.Y : b.Y);
    }

    // Returns normalized vector
    static Vector2 Normalize(const Vector2& v);

    static Vector2 Round(const Vector2& v)
    {
        return Vector2(Math::Round(v.X), Math::Round(v.Y));
    }

    static Vector2 Ceil(const Vector2& v)
    {
        return Vector2(Math::Ceil(v.X), Math::Ceil(v.Y));
    }

    static Vector2 Floor(const Vector2& v)
    {
        return Vector2(Math::Floor(v.X), Math::Floor(v.Y));
    }

    static Vector2 Frac(const Vector2& v)
    {
        return Vector2(v.X - (int32)v.X, v.Y - (int32)v.Y);
    }

    static Int2 CeilToInt(const Vector2& v);
    static Int2 FloorToInt(const Vector2& v);

    static Vector2 Mod(const Vector2& v)
    {
        return Vector2(
            (float)(v.X - (int32)v.X),
            (float)(v.Y - (int32)v.Y)
        );
    }

public:

    /// <summary>
    /// Calculates the area of the triangle.
    /// </summary>
    /// <param name="v0">The first triangle vertex.</param>
    /// <param name="v1">The second triangle vertex.</param>
    /// <param name="v2">The third triangle vertex.</param>
    /// <returns>The triangle area.</returns>
    static float TriangleArea(const Vector2& v0, const Vector2& v1, const Vector2& v2);
};

inline Vector2 operator+(float a, const Vector2& b)
{
    return b + a;
}

inline Vector2 operator-(float a, const Vector2& b)
{
    return Vector2(a) - b;
}

inline Vector2 operator*(float a, const Vector2& b)
{
    return b * a;
}

inline Vector2 operator/(float a, const Vector2& b)
{
    return Vector2(a) / b;
}

namespace Math
{
    FORCE_INLINE static bool NearEqual(const Vector2& a, const Vector2& b)
    {
        return Vector2::NearEqual(a, b);
    }
}

template<>
struct TIsPODType<Vector2>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(Vector2, "X:{0} Y:{1}", v.X, v.Y);
