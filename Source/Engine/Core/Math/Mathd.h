// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include <math.h>

namespace Math
{
    /// <summary>
    /// Computes the sine and cosine of a scalar double.
    /// </summary>
    /// <param name="angle">The input angle (in radians).</param>
    /// <param name="sine">The output sine value.</param>
    /// <param name="cosine">The output cosine value.</param>
    FLAXENGINE_API void SinCos(double angle, double& sine, double& cosine);

    static FORCE_INLINE double Trunc(double value)
    {
        return trunc(value);
    }

    static FORCE_INLINE double Round(double value)
    {
        return round(value);
    }

    static FORCE_INLINE double Floor(double value)
    {
        return floor(value);
    }

    static FORCE_INLINE double Ceil(double value)
    {
        return ceil(value);
    }

    static FORCE_INLINE double Sin(double value)
    {
        return sin(value);
    }

    static FORCE_INLINE double Asin(double value)
    {
        return asin(value < -1. ? -1. : value < 1. ? value : 1.);
    }

    static FORCE_INLINE double Sinh(double value)
    {
        return sinh(value);
    }

    static FORCE_INLINE double Cos(double value)
    {
        return cos(value);
    }

    static FORCE_INLINE double Acos(double value)
    {
        return acos(value < -1. ? -1. : value < 1. ? value : 1.);
    }

    static FORCE_INLINE double Tan(double value)
    {
        return tan(value);
    }

    static FORCE_INLINE double Atan(double value)
    {
        return atan(value);
    }

    static FORCE_INLINE double Atan2(double y, double x)
    {
        return atan2(y, x);
    }

    static FORCE_INLINE double InvSqrt(double value)
    {
        return 1.0 / sqrt(value);
    }

    static FORCE_INLINE double Log(const double value)
    {
        return log(value);
    }

    static FORCE_INLINE double Log2(const double value)
    {
        return log2(value);
    }

    static FORCE_INLINE double Log10(const double value)
    {
        return log10(value);
    }

    static FORCE_INLINE double Pow(const double base, const double exponent)
    {
        return pow(base, exponent);
    }

    static FORCE_INLINE double Sqrt(const double value)
    {
        return sqrt(value);
    }

    static FORCE_INLINE double Exp(const double value)
    {
        return exp(value);
    }

    static FORCE_INLINE double Exp2(const double value)
    {
        return exp2(value);
    }

    static FORCE_INLINE double Abs(const double value)
    {
        return fabs(value);
    }

    static FORCE_INLINE double Mod(const double a, const double b)
    {
        return fmod(a, b);
    }

    static FORCE_INLINE double ModF(double a, double* b)
    {
        return modf(a, b);
    }

    static FORCE_INLINE double Frac(double value)
    {
        return value - Floor(value);
    }

    /// <summary>
    /// Returns signed fractional part of a double.
    /// </summary>
    /// <param name="value">Double point value to convert.</param>
    /// <returns>A double between [0 ; 1) for nonnegative input. A double between [-1; 0) for negative input.</returns>
    static FORCE_INLINE double Fractional(double value)
    {
        return value - Trunc(value);
    }

    static int64 TruncToInt(double value)
    {
        return (int64)value;
    }

    static int64 FloorToInt(double value)
    {
        return TruncToInt(floor(value));
    }

    static FORCE_INLINE int64 RoundToInt(double value)
    {
        return FloorToInt(value + 0.5);
    }

    static FORCE_INLINE int64 CeilToInt(double value)
    {
        return TruncToInt(ceil(value));
    }

    // Performs smooth (cubic Hermite) interpolation between 0 and 1.
    static double SmoothStep(double amount)
    {
        return amount <= 0. ? 0. : amount >= 1. ? 1. : amount * amount * (3. - 2. * amount);
    }

    // Performs a smooth(er) interpolation between 0 and 1 with 1st and 2nd order derivatives of zero at endpoints.
    static double SmootherStep(double amount)
    {
        return amount <= 0. ? 0. : amount >= 1. ? 1. : amount * amount * amount * (amount * (amount * 6. - 15.) + 10.);
    }

    // Determines whether the specified value is close to zero (0.0).
    inline bool IsZero(double a)
    {
        return Abs(a) < ZeroTolerance;
    }

    // Determines whether the specified value is close to one (1.0f).
    inline bool IsOne(double a)
    {
        return IsZero(a - 1.);
    }

    // Returns a value indicating the sign of a number.
    inline double Sign(double v)
    {
        return v > 0. ? 1. : v < 0. ? -1. : 0.;
    }

    /// <summary>
    /// Compares the sign of two double values.
    /// </summary>
    /// <param name="a">The first value.</param>
    /// <param name="b">The second value.</param>
    /// <returns>True if given values have the same sign (both positive or negative); otherwise false.</returns>
    inline bool SameSign(const double a, const double b)
    {
        return a * b >= 0.;
    }

    /// <summary>
    /// Compares the sign of two double values.
    /// </summary>
    /// <param name="a">The first value.</param>
    /// <param name="b">The second value.</param>
    /// <returns>True if given values don't have the same sign (first is positive and second is negative or vice versa); otherwise false.</returns>
    inline bool NotSameSign(const double a, const double b)
    {
        return a * b < 0.;
    }

    /// <summary>
    /// Checks if a and b are not even almost equal, taking into account the magnitude of double numbers
    /// </summary>
    /// <param name="a">The left value to compare</param>
    /// <param name="b">The right value to compare</param>
    /// <returns>False if a almost equal to b, otherwise true</returns>
    static bool NotNearEqual(double a, double b)
    {
        return Abs(a - b) >= ZeroToleranceDouble;
    }

    /// <summary>
    /// Checks if a and b are almost equals, taking into account the magnitude of double precision floating point numbers
    /// </summary>
    /// <param name="a">The left value to compare</param>
    /// <param name="b">The right value to compare</param>
    /// <returns>True if a almost equal to b, otherwise false</returns>
    static bool NearEqual(double a, double b)
    {
        return Abs(a - b) < ZeroToleranceDouble;
    }

    /// <summary>
    /// Checks if a and b are almost equals within the given epsilon value.
    /// </summary>
    /// <param name="a">The left value to compare.</param>
    /// <param name="b">The right value to compare.</param>
    /// <param name="eps">The comparision epsilon value. Should be 1e-4 or less.</param>
    /// <returns>True if a almost equal to b, otherwise false</returns>
    static bool NearEqual(double a, double b, double eps)
    {
        return Abs(a - b) < eps;
    }

    /// <summary>
    /// Remaps the specified value from the specified range to another.
    /// </summary>
    /// <param name="value">The value to remap.</param>
    /// <param name="fromMin">The source range minimum.</param>
    /// <param name="fromMax">The source range maximum.</param>
    /// <param name="toMin">The destination range minimum.</param>
    /// <param name="toMax">The destination range maximum.</param>
    /// <returns>The remapped value.</returns>
    static double Remap(double value, double fromMin, double fromMax, double toMin, double toMax)
    {
        return (value - fromMin) / (fromMax - fromMin) * (toMax - toMin) + toMin;
    }

    static double ClampAxis(double angle)
    {
        angle = Mod(angle, 360.);
        if (angle < 0.)
            angle += 360.;
        return angle;
    }

    static double NormalizeAxis(double angle)
    {
        angle = ClampAxis(angle);
        if (angle > 180.)
            angle -= 360.;
        return angle;
    }

    // Find the smallest angle between two headings (in radians).
    static double FindDeltaAngle(double a1, double a2)
    {
        double delta = a2 - a1;
        if (delta > PI)
            delta = delta - TWO_PI;
        else if (delta < -PI)
            delta = delta + TWO_PI;
        return delta;
    }

    /// <summary>
    ///  Returns value based on comparand. The main purpose of this function is to avoid branching based on floating point comparison which can be avoided via compiler intrinsics.
    /// </summary>
    /// <remarks>
    /// Please note that this doesn't define what happens in the case of NaNs as there might be platform specific differences.
    /// </remarks>
    /// <param name="comparand">Comparand the results are based on.</param>
    /// <param name="valueGEZero">The result value if comparand >= 0.</param>
    /// <param name="valueLTZero">The result value if comparand < 0.</param>
    /// <returns>the valueGEZero if comparand >= 0, valueLTZero otherwise</returns>
    static double DoubleSelect(double comparand, double valueGEZero, double valueLTZero)
    {
        return comparand >= 0. ? valueGEZero : valueLTZero;
    }

    /// <summary>
    /// Returns a smooth Hermite interpolation between 0 and 1 for the value X (where X ranges between A and B). Clamped to 0 for X <= A and 1 for X >= B.
    /// </summary>
    /// <param name="a">The minimum value of x.</param>
    /// <param name="b">The maximum value of x.</param>
    /// <param name="x">The x.</param>
    /// <returns>The smoothed value between 0 and 1.</returns>
    static double SmoothStep(double a, double b, double x)
    {
        if (x < a)
            return 0.;
        if (x >= b)
            return 1.;
        const double fraction = (x - a) / (b - a);
        return fraction * fraction * (3. - 2. * fraction);
    }
}
