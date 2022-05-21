// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Math.h"
#include "Mathd.h"
#include "Engine/Core/Formatting.h"
#include "Engine/Core/Templates.h"

struct Double3;
struct Double4;
struct Vector2;
struct Vector3;
struct Vector4;
struct Int2;
struct Int3;
struct Int4;
struct Color;
struct Matrix;

/// <summary>
/// Represents a two dimensional mathematical vector with 64-bit precision (per-component).
/// </summary>
API_STRUCT() struct FLAXENGINE_API Double2
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(Double2);
public:
    union
    {
        struct
        {
            /// <summary>
            /// The X component of the vector.
            /// </summary>
            API_FIELD() double X;

            /// <summary>
            /// The Y component of the vector.
            /// </summary>
            API_FIELD() double Y;
        };

        // Raw values
        double Raw[2];
    };

public:
    // Vector with all components equal 0
    static const Double2 Zero;

    // Vector with all components equal 1
    static const Double2 One;

    // Vector X=1, Y=0
    static const Double2 UnitX;

    // Vector X=0, Y=1
    static const Double2 UnitY;

    // A minimum Double2
    static const Double2 Minimum;

    // A maximum Double2
    static const Double2 Maximum;

public:
    /// <summary>
    /// Empty constructor.
    /// </summary>
    Double2()
    {
    }

    // Init
    // @param xy Value to assign to the all components
    Double2(double xy)
        : X(xy)
        , Y(xy)
    {
    }

    // Init
    // @param x X component value
    // @param y Y component value
    Double2(double x, double y)
        : X(x)
        , Y(y)
    {
    }

    /// <summary>
    /// Init
    /// </summary>
    /// <param name="v">X and Z components in an array</param>
    explicit Double2(double xy[2])
        : X(xy[0])
        , Y(xy[1])
    {
    }

    explicit Double2(const Int2& xy);
    explicit Double2(const Int3& xyz);
    explicit Double2(const Int4& xyzw);
    Double2(const Vector2& xy);
    explicit Double2(const Vector3& xyz);
    explicit Double2(const Vector4& xyzw);
    explicit Double2(const Double3& xyz);
    explicit Double2(const Double4& xyzw);
    explicit Double2(const Color& color);

public:
    String ToString() const;

public:
    // Arithmetic operators with Double2
    Double2 operator+(const Double2& b) const
    {
        return Add(*this, b);
    }

    Double2 operator-(const Double2& b) const
    {
        return Subtract(*this, b);
    }

    Double2 operator*(const Double2& b) const
    {
        return Multiply(*this, b);
    }

    Double2 operator/(const Double2& b) const
    {
        return Divide(*this, b);
    }

    Double2 operator-() const
    {
        return Double2(-X, -Y);
    }

    // op= operators with Double2
    Double2& operator+=(const Double2& b)
    {
        *this = Add(*this, b);
        return *this;
    }

    Double2& operator-=(const Double2& b)
    {
        *this = Subtract(*this, b);
        return *this;
    }

    Double2& operator*=(const Double2& b)
    {
        *this = Multiply(*this, b);
        return *this;
    }

    Double2& operator/=(const Double2& b)
    {
        *this = Divide(*this, b);
        return *this;
    }

    // Arithmetic operators with double
    Double2 operator+(double b) const
    {
        return Add(*this, b);
    }

    Double2 operator-(double b) const
    {
        return Subtract(*this, b);
    }

    Double2 operator*(double b) const
    {
        return Multiply(*this, b);
    }

    Double2 operator/(double b) const
    {
        return Divide(*this, b);
    }

    // op= operators with double
    Double2& operator+=(double b)
    {
        *this = Add(*this, b);
        return *this;
    }

    Double2& operator-=(double b)
    {
        *this = Subtract(*this, b);
        return *this;
    }

    Double2& operator*=(double b)
    {
        *this = Multiply(*this, b);
        return *this;
    }

    Double2& operator/=(double b)
    {
        *this = Divide(*this, b);
        return *this;
    }

    // Comparison operators
    bool operator==(const Double2& b) const
    {
        return X == b.X && Y == b.Y;
    }

    bool operator!=(const Double2& b) const
    {
        return X != b.X || Y != b.Y;
    }

    bool operator>(const Double2& b) const
    {
        return X > b.X && Y > b.Y;
    }

    bool operator>=(const Double2& b) const
    {
        return X >= b.X && Y >= b.Y;
    }

    bool operator<(const Double2& b) const
    {
        return X < b.X && Y < b.Y;
    }

    bool operator<=(const Double2& b) const
    {
        return X <= b.X && Y <= b.Y;
    }

public:
    static bool NearEqual(const Double2& a, const Double2& b)
    {
        return Math::NearEqual(a.X, b.X) && Math::NearEqual(a.Y, b.Y);
    }

    static bool NearEqual(const Double2& a, const Double2& b, double epsilon)
    {
        return Math::NearEqual(a.X, b.X, epsilon) && Math::NearEqual(a.Y, b.Y, epsilon);
    }

public:
    static double Dot(const Double2& a, const Double2& b)
    {
        return a.X * b.X + a.Y * b.Y;
    }

    static double Cross(const Double2& a, const Double2& b)
    {
        return a.X * b.Y - a.Y * b.X;
    }

    static void Add(const Double2& a, const Double2& b, Double2& result)
    {
        result.X = a.X + b.X;
        result.Y = a.Y + b.Y;
    }

    static Double2 Add(const Double2& a, const Double2& b)
    {
        Double2 result;
        Add(a, b, result);
        return result;
    }

    static void Subtract(const Double2& a, const Double2& b, Double2& result)
    {
        result.X = a.X - b.X;
        result.Y = a.Y - b.Y;
    }

    static Double2 Subtract(const Double2& a, const Double2& b)
    {
        Double2 result;
        Subtract(a, b, result);
        return result;
    }

    static Double2 Multiply(const Double2& a, const Double2& b)
    {
        return Double2(a.X * b.X, a.Y * b.Y);
    }

    static Double2 Multiply(const Double2& a, double b)
    {
        return Double2(a.X * b, a.Y * b);
    }

    static Double2 Divide(const Double2& a, const Double2& b)
    {
        return Double2(a.X / b.X, a.Y / b.Y);
    }

    static Double2 Divide(const Double2& a, double b)
    {
        return Double2(a.X / b, a.Y / b);
    }

    // Calculates distance between two points in 2D
    // @param a 1st point
    // @param b 2nd point
    // @returns Distance
    static double Distance(const Double2& a, const Double2& b)
    {
        const double x = a.X - b.X;
        const double y = a.Y - b.Y;
        return Math::Sqrt(x * x + y * y);
    }

    // Calculates the squared distance between two points in 2D
    // @param a 1st point
    // @param b 2nd point
    // @returns Distance
    static double DistanceSquared(const Double2& a, const Double2& b)
    {
        const double x = a.X - b.X;
        const double y = a.Y - b.Y;
        return x * x + y * y;
    }

    // Clamp vector values within given range
    // @param v Vector to clamp
    // @param min Minimum value
    // @param max Maximum value
    // @returns Clamped vector
    static Double2 Clamp(const Double2& v, double min, double max)
    {
        return Double2(Math::Clamp(v.X, min, max), Math::Clamp(v.Y, min, max));
    }

    // Clamp vector values within given range
    // @param v Vector to clamp
    // @param min Minimum value
    // @param max Maximum value
    // @returns Clamped vector
    static Double2 Clamp(const Double2& v, const Double2& min, const Double2& max)
    {
        return Double2(Math::Clamp(v.X, min.X, max.X), Math::Clamp(v.Y, min.Y, max.Y));
    }

    // Performs vector normalization (scales vector up to unit length)
    void Normalize()
    {
        const double length = Length();
        if (!Math::IsZero(length))
        {
            const double invLength = 1.0 / length;
            X *= invLength;
            Y *= invLength;
        }
    }

public:
    // Gets a value indicting whether this instance is normalized.
    bool IsNormalized() const
    {
        return Math::IsOne(X * X + Y * Y);
    }

    // Gets a value indicting whether this vector is zero.
    bool IsZero() const
    {
        return Math::IsZero(X) && Math::IsZero(Y);
    }

    // Gets a value indicting whether any vector component is zero.
    bool IsAnyZero() const
    {
        return Math::IsZero(X) || Math::IsZero(Y);
    }

    // Gets a value indicting whether this vector is zero.
    bool IsOne() const
    {
        return Math::IsOne(X) && Math::IsOne(Y);
    }

    // Calculates length of the vector.
    double Length() const
    {
        return Math::Sqrt(X * X + Y * Y);
    }

    // Calculates the squared length of the vector.
    double LengthSquared() const
    {
        return X * X + Y * Y;
    }

    // Calculates inverted length of the vector (1 / length).
    double InvLength() const
    {
        return 1. / Length();
    }

    // Calculates a vector with values being absolute values of that vector.
    Double2 GetAbsolute() const
    {
        return Double2(Math::Abs(X), Math::Abs(Y));
    }

    // Calculates a vector with values being opposite to values of that vector.
    Double2 GetNegative() const
    {
        return Double2(-X, -Y);
    }

    /// <summary>
    /// Returns the average arithmetic of all the components.
    /// </summary>
    double AverageArithmetic() const
    {
        return (X + Y) * 0.5;
    }

    /// <summary>
    /// Gets the sum of all vector components values.
    /// </summary>
    double SumValues() const
    {
        return X + Y;
    }

    /// <summary>
    /// Gets the multiplication result of all vector components values.
    /// </summary>
    double MulValues() const
    {
        return X * Y;
    }

    /// <summary>
    /// Returns the minimum value of all the components.
    /// </summary>
    double MinValue() const
    {
        return Math::Min(X, Y);
    }

    /// <summary>
    /// Returns the maximum value of all the components.
    /// </summary>
    double MaxValue() const
    {
        return Math::Max(X, Y);
    }

    /// <summary>
    /// Returns true if vector has one or more components is not a number (NaN).
    /// </summary>
    bool IsNaN() const
    {
        return isnan(X) || isnan(Y);
    }

    /// <summary>
    /// Returns true if vector has one or more components equal to +/- infinity.
    /// </summary>
    bool IsInfinity() const
    {
        return isinf(X) || isinf(Y);
    }

    /// <summary>
    /// Returns true if vector has one or more components equal to +/- infinity or NaN.
    /// </summary>
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
    static void Lerp(const Double2& start, const Double2& end, double amount, Double2& result)
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
    static Double2 Lerp(const Double2& start, const Double2& end, double amount)
    {
        Double2 result;
        Lerp(start, end, amount, result);
        return result;
    }

    static Double2 Abs(const Double2& v)
    {
        return Double2(Math::Abs(v.X), Math::Abs(v.Y));
    }

    // Creates vector from minimum components of two vectors
    static Double2 Min(const Double2& a, const Double2& b)
    {
        return Double2(a.X < b.X ? a.X : b.X, a.Y < b.Y ? a.Y : b.Y);
    }

    // Creates vector from minimum components of two vectors
    static void Min(const Double2& a, const Double2& b, Double2& result)
    {
        result = Double2(a.X < b.X ? a.X : b.X, a.Y < b.Y ? a.Y : b.Y);
    }

    // Creates vector from maximum components of two vectors
    static Double2 Max(const Double2& a, const Double2& b)
    {
        return Double2(a.X > b.X ? a.X : b.X, a.Y > b.Y ? a.Y : b.Y);
    }

    // Creates vector from maximum components of two vectors
    static void Max(const Double2& a, const Double2& b, Double2& result)
    {
        result = Double2(a.X > b.X ? a.X : b.X, a.Y > b.Y ? a.Y : b.Y);
    }

    // Returns normalized vector
    static Double2 Normalize(const Double2& v);

    static Double2 Round(const Double2& v)
    {
        return Double2(Math::Round(v.X), Math::Round(v.Y));
    }

    static Double2 Ceil(const Double2& v)
    {
        return Double2(Math::Ceil(v.X), Math::Ceil(v.Y));
    }

    static Double2 Floor(const Double2& v)
    {
        return Double2(Math::Floor(v.X), Math::Floor(v.Y));
    }

    static Double2 Frac(const Double2& v)
    {
        return Double2(v.X - (int64)v.X, v.Y - (int64)v.Y);
    }

    static Int2 CeilToInt(const Double2& v);
    static Int2 FloorToInt(const Double2& v);

    static Double2 Mod(const Double2& v)
    {
        return Double2(
            v.X - (int64)v.X,
            v.Y - (int64)v.Y
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
    static double TriangleArea(const Double2& v0, const Double2& v1, const Double2& v2);

    /// <summary>
    /// Calculates the angle (in radians) between from and to. This is always the smallest value.
    /// </summary>
    /// <param name="from">The first vector.</param>
    /// <param name="to">The second vector.</param>
    /// <returns>The angle (in radians).</returns>
    static double Angle(const Double2& from, const Double2& to);
};

inline Double2 operator+(double a, const Double2& b)
{
    return b + a;
}

inline Double2 operator-(double a, const Double2& b)
{
    return Double2(a) - b;
}

inline Double2 operator*(double a, const Double2& b)
{
    return b * a;
}

inline Double2 operator/(double a, const Double2& b)
{
    return Double2(a) / b;
}

namespace Math
{
    FORCE_INLINE static bool NearEqual(const Double2& a, const Double2& b)
    {
        return Double2::NearEqual(a, b);
    }
}

template<>
struct TIsPODType<Double2>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(Double2, "X:{0} Y:{1}", v.X, v.Y);
