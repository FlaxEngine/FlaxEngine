// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Math.h"
#include "Mathd.h"
#include "Engine/Core/Formatting.h"
#include "Engine/Core/Templates.h"

struct Double2;
struct Double3;
struct Vector2;
struct Vector3;
struct Vector4;
struct Color;
struct Matrix;
struct Rectangle;
class String;
struct Int2;
struct Int3;
struct Int4;

/// <summary>
/// Represents a four dimensional mathematical vector with 64-bit precision (per-component).
/// </summary>
API_STRUCT() struct FLAXENGINE_API Double4
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(Double4);
public:
    union
    {
        struct
        {
            /// <summary>
            /// The X component.
            /// </summary>
            API_FIELD() double X;

            /// <summary>
            /// The Y component.
            /// </summary>
            API_FIELD() double Y;

            /// <summary>
            /// The Z component.
            /// </summary>
            API_FIELD() double Z;

            /// <summary>
            /// The W component.
            /// </summary>
            API_FIELD() double W;
        };

        /// <summary>
        /// The raw vector values (in xyzw order).
        /// </summary>
        double Raw[4];
    };

public:
    // Vector with all components equal 0
    static const Double4 Zero;

    // Vector with all components equal 1
    static const Double4 One;

    // Vector X=1, Y=0, Z=0, W=0
    static const Double4 UnitX;

    // Vector X=0, Y=1, Z=0, W=0
    static const Double4 UnitY;

    // Vector X=0, Y=0, Z=1, W=0
    static const Double4 UnitZ;

    // Vector X=0, Y=0, Z=0, W=1
    static const Double4 UnitW;

    // A minimum Double4
    static const Double4 Minimum;

    // A maximum Double4
    static const Double4 Maximum;

public:
    /// <summary>
    /// Empty constructor.
    /// </summary>
    Double4()
    {
    }

    // Init
    // @param xyzw Value to assign to the all components
    Double4(double xyzw)
        : X(xyzw)
        , Y(xyzw)
        , Z(xyzw)
        , W(xyzw)
    {
    }

    // Init
    // @param xyzw Values to assign
    explicit Double4(double xyzw[4]);

    // Init
    // @param x X component value
    // @param y Y component value
    // @param z Z component value
    // @param w W component value
    Double4(double x, double y, double z, double w)
        : X(x)
        , Y(y)
        , Z(z)
        , W(w)
    {
    }

    explicit Double4(const Vector2& xy, double z, double w);
    explicit Double4(const Vector2& xy, const Vector2& zw);
    explicit Double4(const Vector3& xyz, double w);
    Double4(const Vector4& xyzw);
    explicit Double4(const Int2& xy, double z, double w);
    explicit Double4(const Int3& xyz, double w);
    explicit Double4(const Int4& xyzw);
    explicit Double4(const Double2& xy, double z, double w);
    explicit Double4(const Double3& xyz, double w);
    explicit Double4(const Color& color);
    explicit Double4(const Rectangle& rect);

public:
    String ToString() const;

public:
    // Gets a value indicting whether this vector is zero.
    bool IsZero() const
    {
        return Math::IsZero(X) && Math::IsZero(Y) && Math::IsZero(Z) && Math::IsZero(W);
    }

    // Gets a value indicting whether any vector component is zero.
    bool IsAnyZero() const
    {
        return Math::IsZero(X) || Math::IsZero(Y) || Math::IsZero(Z) || Math::IsZero(W);
    }

    // Gets a value indicting whether this vector is one.
    bool IsOne() const
    {
        return Math::IsOne(X) && Math::IsOne(Y) && Math::IsOne(Z) && Math::IsOne(W);
    }

    /// <summary>
    /// Calculates a vector with values being absolute values of that vector.
    /// </summary>
    Double4 GetAbsolute() const
    {
        return Double4(Math::Abs(X), Math::Abs(Y), Math::Abs(Z), Math::Abs(W));
    }

    /// <summary>
    /// Calculates a vector with values being opposite to values of that vector.
    /// </summary>
    Double4 GetNegative() const
    {
        return Double4(-X, -Y, -Z, -W);
    }

    /// <summary>
    /// Returns the average arithmetic of all the components.
    /// </summary>
    double AverageArithmetic() const
    {
        return (X + Y + Z + W) * 0.25;
    }

    /// <summary>
    /// Gets the sum of all vector components values.
    /// </summary>
    double SumValues() const
    {
        return X + Y + Z + W;
    }

    /// <summary>
    /// Returns the minimum value of all the components.
    /// </summary>
    double MinValue() const
    {
        return Math::Min(X, Y, Z, W);
    }

    /// <summary>
    /// Returns the maximum value of all the components.
    /// </summary>
    double MaxValue() const
    {
        return Math::Max(X, Y, Z, W);
    }

    /// <summary>
    /// Returns true if vector has one or more components is not a number (NaN).
    /// </summary>
    bool IsNaN() const
    {
        return isnan(X) || isnan(Y) || isnan(Z) || isnan(W);
    }

    /// <summary>
    /// Returns true if vector has one or more components equal to +/- infinity.
    /// </summary>
    bool IsInfinity() const
    {
        return isinf(X) || isinf(Y) || isinf(Z) || isinf(W);
    }

    /// <summary>
    /// Returns true if vector has one or more components equal to +/- infinity or NaN.
    /// </summary>
    bool IsNanOrInfinity() const
    {
        return IsInfinity() || IsNaN();
    }

public:
    // Arithmetic operators with Double4
    inline Double4 operator+(const Double4& b) const
    {
        return Add(*this, b);
    }

    inline Double4 operator-(const Double4& b) const
    {
        return Subtract(*this, b);
    }

    inline Double4 operator*(const Double4& b) const
    {
        return Multiply(*this, b);
    }

    inline Double4 operator/(const Double4& b) const
    {
        return Divide(*this, b);
    }

    // op= operators with Double4
    inline Double4& operator+=(const Double4& b)
    {
        *this = Add(*this, b);
        return *this;
    }

    inline Double4& operator-=(const Double4& b)
    {
        *this = Subtract(*this, b);
        return *this;
    }

    inline Double4& operator*=(const Double4& b)
    {
        *this = Multiply(*this, b);
        return *this;
    }

    inline Double4& operator/=(const Double4& b)
    {
        *this = Divide(*this, b);
        return *this;
    }

    // Arithmetic operators with double
    inline Double4 operator+(double b) const
    {
        return Add(*this, b);
    }

    inline Double4 operator-(double b) const
    {
        return Subtract(*this, b);
    }

    inline Double4 operator*(double b) const
    {
        return Multiply(*this, b);
    }

    inline Double4 operator/(double b) const
    {
        return Divide(*this, b);
    }

    // op= operators with double
    inline Double4& operator+=(double b)
    {
        *this = Add(*this, b);
        return *this;
    }

    inline Double4& operator-=(double b)
    {
        *this = Subtract(*this, b);
        return *this;
    }

    inline Double4& operator*=(double b)
    {
        *this = Multiply(*this, b);
        return *this;
    }

    inline Double4& operator/=(double b)
    {
        *this = Divide(*this, b);
        return *this;
    }

    // Comparison
    inline bool operator==(const Double4& b) const
    {
        return X == b.X && Y == b.Y && Z == b.Z && W == b.W;
    }

    inline bool operator!=(const Double4& b) const
    {
        return X != b.X || Y != b.Y || Z != b.Z || W != b.W;
    }

    inline bool operator>(const Double4& b) const
    {
        return X > b.X && Y > b.Y && Z > b.Z && W > b.W;
    }

    inline bool operator>=(const Double4& b) const
    {
        return X >= b.X && Y >= b.Y && Z >= b.Z && W >= b.W;
    }

    inline bool operator<(const Double4& b) const
    {
        return X < b.X && Y < b.Y && Z < b.Z && W < b.W;
    }

    inline bool operator<=(const Double4& b) const
    {
        return X <= b.X && Y <= b.Y && Z <= b.Z && W <= b.W;
    }

public:
    static bool NearEqual(const Double4& a, const Double4& b)
    {
        return Math::NearEqual(a.X, b.X) && Math::NearEqual(a.Y, b.Y) && Math::NearEqual(a.Z, b.Z) && Math::NearEqual(a.W, b.W);
    }

    static bool NearEqual(const Double4& a, const Double4& b, double epsilon)
    {
        return Math::NearEqual(a.X, b.X, epsilon) && Math::NearEqual(a.Y, b.Y, epsilon) && Math::NearEqual(a.Z, b.Z, epsilon) && Math::NearEqual(a.W, b.W, epsilon);
    }

public:
    static void Add(const Double4& a, const Double4& b, Double4& result)
    {
        result.X = a.X + b.X;
        result.Y = a.Y + b.Y;
        result.Z = a.Z + b.Z;
        result.W = a.W + b.W;
    }

    static Double4 Add(const Double4& a, const Double4& b)
    {
        Double4 result;
        Add(a, b, result);
        return result;
    }

    static void Subtract(const Double4& a, const Double4& b, Double4& result)
    {
        result.X = a.X - b.X;
        result.Y = a.Y - b.Y;
        result.Z = a.Z - b.Z;
        result.W = a.W - b.W;
    }

    static Double4 Subtract(const Double4& a, const Double4& b)
    {
        Double4 result;
        Subtract(a, b, result);
        return result;
    }

    static Double4 Multiply(const Double4& a, const Double4& b)
    {
        return Double4(a.X * b.X, a.Y * b.Y, a.Z * b.Z, a.W * b.W);
    }

    static Double4 Multiply(const Double4& a, double b)
    {
        return Double4(a.X * b, a.Y * b, a.Z * b, a.W * b);
    }

    static Double4 Divide(const Double4& a, const Double4& b)
    {
        return Double4(a.X / b.X, a.Y / b.Y, a.Z / b.Z, a.W / b.W);
    }

    static Double4 Divide(const Double4& a, double b)
    {
        return Double4(a.X / b, a.Y / b, a.Z / b, a.W / b);
    }

    static Double4 Floor(const Double4& v);
    static Double4 Frac(const Double4& v);
    static Double4 Round(const Double4& v);
    static Double4 Ceil(const Double4& v);

public:
    // Restricts a value to be within a specified range
    // @param value The value to clamp
    // @param min The minimum value,
    // @param max The maximum value
    // @returns Clamped value
    static Double4 Clamp(const Double4& value, const Double4& min, const Double4& max);

    // Restricts a value to be within a specified range
    // @param value The value to clamp
    // @param min The minimum value,
    // @param max The maximum value
    // @param result When the method completes, contains the clamped value
    static void Clamp(const Double4& value, const Double4& min, const Double4& max, Double4& result);

    // Performs a linear interpolation between two vectors
    // @param start Start vector
    // @param end End vector
    // @param amount Value between 0 and 1 indicating the weight of end
    // @param result When the method completes, contains the linear interpolation of the two vectors
    static void Lerp(const Double4& start, const Double4& end, double amount, Double4& result)
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
    static Double4 Lerp(const Double4& start, const Double4& end, double amount)
    {
        Double4 result;
        Lerp(start, end, amount, result);
        return result;
    }

    static Double4 Transform(const Double4& v, const Matrix& m);
};

inline Double4 operator+(double a, const Double4& b)
{
    return b + a;
}

inline Double4 operator-(double a, const Double4& b)
{
    return Double4(a) - b;
}

inline Double4 operator*(double a, const Double4& b)
{
    return b * a;
}

inline Double4 operator/(double a, const Double4& b)
{
    return Double4(a) / b;
}

namespace Math
{
    FORCE_INLINE static bool NearEqual(const Double4& a, const Double4& b)
    {
        return Double4::NearEqual(a, b);
    }
}

template<>
struct TIsPODType<Double4>
{
    enum { Value = true };
};

DEFINE_DEFAULT_FORMATTING(Double4, "X:{0} Y:{1} Z:{2} W:{3}", v.X, v.Y, v.Z, v.W);
