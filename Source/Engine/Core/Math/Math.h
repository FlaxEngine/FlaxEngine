// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Core.h"
#include "Engine/Core/Types/BaseTypes.h"
#include <math.h>

#undef PI
#define PI 3.1415926535897932f
#define TWO_PI 6.28318530718f
#define PI_INV 0.31830988618f
#define PI_OVER_2 1.57079632679f
#define PI_OVER_4 0.78539816339f
#define PI_HALF PI_OVER_2
#define GOLDEN_RATIO 1.6180339887f

// The value for which all absolute numbers smaller than are considered equal to zero.
#define ZeroTolerance 1e-6f
#define ZeroToleranceDouble 1e-16

// Converts radians to degrees.
#define RadiansToDegrees (180.0f / PI)

// Converts degrees to radians.
#define DegreesToRadians (PI / 180.0f)

namespace Math
{
    /// <summary>
    /// Computes the sine and cosine of a scalar float.
    /// </summary>
    /// <param name="angle">The input angle (in radians).</param>
    /// <param name="sine">The output sine value.</param>
    /// <param name="cosine">The output cosine value.</param>
    FLAXENGINE_API void SinCos(float angle, float& sine, float& cosine);

    /// <summary>
    /// Computes the base 2 logarithm for an integer value that is greater than 0. The result is rounded down to the nearest integer.
    /// </summary>
    /// <param name="value">The value to compute the log of.</param>
    /// <returns>The Log2 of value. 0 if value is 0.</returns>
    FLAXENGINE_API uint32 FloorLog2(uint32 value);

    static FORCE_INLINE float Trunc(float value)
    {
        return truncf(value);
    }

    static FORCE_INLINE float Round(float value)
    {
        return roundf(value);
    }

    static FORCE_INLINE float Floor(float value)
    {
        return floorf(value);
    }

    static FORCE_INLINE float Ceil(float value)
    {
        return ceilf(value);
    }

    static FORCE_INLINE float Sin(float value)
    {
        return sinf(value);
    }

    static FORCE_INLINE float Asin(float value)
    {
        return asinf(value < -1.f ? -1.f : value < 1.f ? value : 1.f);
    }

    static FORCE_INLINE float Sinh(float value)
    {
        return sinhf(value);
    }

    static FORCE_INLINE float Cos(float value)
    {
        return cosf(value);
    }

    static FORCE_INLINE float Acos(float value)
    {
        return acosf(value < -1.f ? -1.f : value < 1.f ? value : 1.f);
    }

    static FORCE_INLINE float Tan(float value)
    {
        return tanf(value);
    }

    static FORCE_INLINE float Atan(float value)
    {
        return atanf(value);
    }

    static FORCE_INLINE float Atan2(float y, float x)
    {
        return atan2f(y, x);
    }

    static FORCE_INLINE float InvSqrt(float value)
    {
        return 1.0f / sqrtf(value);
    }

    static FORCE_INLINE float Log(const float value)
    {
        return logf(value);
    }

    static FORCE_INLINE float Log2(const float value)
    {
        return log2f(value);
    }

    static FORCE_INLINE float Log10(const float value)
    {
        return log10f(value);
    }

    static FORCE_INLINE float Pow(const float base, const float exponent)
    {
        return powf(base, exponent);
    }

    static FORCE_INLINE float Sqrt(const float value)
    {
        return sqrtf(value);
    }

    static FORCE_INLINE float Exp(const float value)
    {
        return expf(value);
    }

    static FORCE_INLINE float Exp2(const float value)
    {
        return exp2f(value);
    }

    static FORCE_INLINE float Abs(const float value)
    {
        return fabsf(value);
    }

    static FORCE_INLINE int32 Abs(const int32 value)
    {
        return value < 0 ? -value : value;
    }

    static FORCE_INLINE int64 Abs(const int64 value)
    {
        return value < 0 ? -value : value;
    }

    static FORCE_INLINE int32 Mod(const int32 a, const int32 b)
    {
        return (int32)fmodf((float)a, (float)b);
    }

    static FORCE_INLINE float Mod(const float a, const float b)
    {
        return fmodf(a, b);
    }

    static FORCE_INLINE float ModF(const float a, float* b)
    {
        return modff(a, b);
    }

    static FORCE_INLINE float Frac(float value)
    {
        return value - Floor(value);
    }

    /// <summary>
    /// Returns signed fractional part of a float.
    /// </summary>
    /// <param name="value">Floating point value to convert.</param>
    /// <returns>A float between [0 ; 1) for nonnegative input. A float between [-1; 0) for negative input.</returns>
    static FORCE_INLINE float Fractional(float value)
    {
        return value - Trunc(value);
    }

    static int32 TruncToInt(float value)
    {
        return (int32)value;
    }

    static int32 FloorToInt(float value)
    {
        return TruncToInt(floorf(value));
    }

    static FORCE_INLINE int32 RoundToInt(float value)
    {
        return FloorToInt(value + 0.5f);
    }

    static FORCE_INLINE int32 CeilToInt(float value)
    {
        return TruncToInt(ceilf(value));
    }

    /// <summary>
    /// Rounds up the value to the next power of 2.
    /// </summary>
    /// <param name="value">The value.</param>
    /// <returns>The power of 2.</returns>
    static int32 RoundUpToPowerOf2(int32 value)
    {
        // Source: http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
        value--;
        value |= value >> 1;
        value |= value >> 2;
        value |= value >> 4;
        value |= value >> 8;
        value |= value >> 16;
        return value + 1;
    }

    /// <summary>
    /// Rounds up the value to the next power of 2.
    /// </summary>
    /// <param name="value">The value.</param>
    /// <returns>The power of 2.</returns>
    static uint32 RoundUpToPowerOf2(uint32 value)
    {
        // Source: http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
        value--;
        value |= value >> 1;
        value |= value >> 2;
        value |= value >> 4;
        value |= value >> 8;
        value |= value >> 16;
        return value + 1;
    }

    /// <summary>
    /// Rounds up the value to the next power of 2.
    /// </summary>
    /// <param name="value">The value.</param>
    /// <returns>The power of 2.</returns>
    static int64 RoundUpToPowerOf2(int64 value)
    {
        // Source: http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
        value--;
        value |= value >> 1;
        value |= value >> 2;
        value |= value >> 4;
        value |= value >> 8;
        value |= value >> 16;
        value |= value >> 32;
        return value + 1;
    }

    /// <summary>
    /// Rounds up the value to the next power of 2.
    /// </summary>
    /// <param name="value">The value.</param>
    /// <returns>The power of 2.</returns>
    static uint64 RoundUpToPowerOf2(uint64 value)
    {
        // Source: http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2
        value--;
        value |= value >> 1;
        value |= value >> 2;
        value |= value >> 4;
        value |= value >> 8;
        value |= value >> 16;
        value |= value >> 32;
        return value + 1;
    }

    // Divides two integers and rounds up
    template<class T>
    static FORCE_INLINE T DivideAndRoundUp(T dividend, T divisor)
    {
        return (dividend + divisor - 1) / divisor;
    }

    template<class T>
    static FORCE_INLINE T DivideAndRoundDown(T dividend, T divisor)
    {
        return dividend / divisor;
    }

    // Checks if value is inside the given range.
    template<class T>
    static bool IsInRange(const T value, const T min, const T max)
    {
        return value >= min && value <= max;
    }

    // Checks if value isn't inside the given range.
    template<class T>
    static bool IsNotInRange(const T value, const T min, const T max)
    {
        return value < min || value > max;
    }

    // Checks whether a number is a power of two.
    static bool IsPowerOfTwo(uint32 value)
    {
        return (value & value - 1) == 0;
    }

    // Clamps value to be between minimum and maximum values, inclusive
    template<class T>
    static T Clamp(const T value, const T min, const T max)
    {
        return value < min ? min : value < max ? value : max;
    }

    // Clamps value to be between 0 and 1 range, inclusive
    template<class T>
    static T Saturate(const T value)
    {
        return value < 0 ? 0 : value < 1 ? value : 1;
    }

    /// <summary>
    /// Returns average arithmetic of two values.
    /// </summary>
    /// <param name="a">The first value.</param>
    /// <param name="b">The second value.</param>
    /// <returns>Average arithmetic of the two values.</returns>
    template<class T>
    static float AverageArithmetic(const T a, const T b)
    {
        return (a + b) * 0.5f;
    }

    // Returns highest of 2 values
    template<class T>
    static T Max(const T a, const T b)
    {
        return a > b ? a : b;
    }

    // Returns lowest of 2 values
    template<class T>
    static T Min(const T a, const T b)
    {
        return a < b ? a : b;
    }

    // Returns highest of 3 values
    template<class T>
    static T Max(const T a, const T b, const T c)
    {
        return Max(Max(a, b), c);
    }

    // Returns highest of 4 values
    template<class T>
    static T Max(const T a, const T b, const T c, const T d)
    {
        return Max(Max(Max(a, b), c), d);
    }

    // Returns lowest of 3 values
    template<class T>
    static T Min(const T a, const T b, const T c)
    {
        return Min(Min(a, b), c);
    }

    // Returns lowest of 4 values
    template<class T>
    static T Min(const T a, const T b, const T c, const T d)
    {
        return Min(Min(Min(a, b), c), d);
    }

    /// <summary>
    /// Moves a value current towards target.
    /// </summary>
    /// <param name="current">The current value.</param>
    /// <param name="target">The value to move towards.</param>
    /// <param name="maxDelta">The maximum change that should be applied to the value.</param>
    template<class T>
    static T MoveTowards(const T current, const T target, const T maxDelta)
    {
        if (Abs(target - current) <= maxDelta)
            return target;
        return current + Sign(target - current) * maxDelta;
    }

    /// <summary>
    /// Same as MoveTowards but makes sure the values interpolate correctly when they wrap around 360 degrees.
    /// </summary>
    /// <param name="current"></param>
    /// <param name="target"></param>
    /// <param name="maxDelta"></param>
    template<class T>
    static T MoveTowardsAngle(const T current, const T target, const T maxDelta)
    {
        T delta = DeltaAngle(current, target);
        if ((-maxDelta < delta) && (delta < maxDelta))
            return target;
        T deltaTarget = current + delta;
        return MoveTowards(current, deltaTarget, maxDelta);
    }

    /// <summary>
    /// Calculates the shortest difference between two given angles given in degrees.
    /// </summary>
    /// <param name="current"></param>
    /// <param name="target"></param>
    template<class T>
    static T DeltaAngle(const T current, const T target)
    {
        T t = Repeat(target - current, (T)360);
        if (t > (T)180)
            t -= (T)360;
        return t;
    }

    /// <summary>
    /// Loops the value t, so that it is never larger than length and never smaller than 0.
    /// </summary>
    /// <param name="t"></param>
    /// <param name="length"></param>
    template<class T>
    static T Repeat(const T t, const T length)
    {
        return t - Floor(t / length) * length;
    }

    // Multiply value by itself
    template<class T>
    static T Square(const T a)
    {
        return a * a;
    }

    // Performs a linear interpolation between two values, alpha ranges from 0-1
    template<class T, class U>
    static T Lerp(const T& a, const T& b, const U& alpha)
    {
        return (T)(a + alpha * (b - a));
    }

    // Performs a linear interpolation between two values, alpha ranges from 0-1. Handles full numeric range of T
    template<class T>
    static T LerpStable(const T& a, const T& b, double alpha)
    {
        return (T)(a * (1.0 - alpha) + b * alpha);
    }

    // Performs a linear interpolation between two values, alpha ranges from 0-1. Handles full numeric range of T
    template<class T>
    static T LerpStable(const T& a, const T& b, float alpha)
    {
        return (T)(a * (1.0f - alpha) + b * alpha);
    }

    // Calculates the linear parameter t that produces the interpolation value within the range [a, b].
    template<class T, class U>
    static T InverseLerp(const T& a, const T& b, const U& value)
    {
        if (a == b)
            return (T)0;
        return Saturate((value - a) / (b - a));
    }

    // Performs smooth (cubic Hermite) interpolation between 0 and 1.
    static float SmoothStep(float amount)
    {
        return amount <= 0 ? 0 : amount >= 1 ? 1 : amount * amount * (3 - 2 * amount);
    }

    // Performs a smooth(er) interpolation between 0 and 1 with 1st and 2nd order derivatives of zero at endpoints.
    static float SmootherStep(float amount)
    {
        return amount <= 0 ? 0 : amount >= 1 ? 1 : amount * amount * amount * (amount * (amount * 6 - 15) + 10);
    }

    // Determines whether the specified value is close to zero (0.0).
    inline int32 IsZero(int32 a)
    {
        return a == 0;
    }

    // Determines whether the specified value is close to zero (0.0f).
    inline bool IsZero(float a)
    {
        return Abs(a) < ZeroTolerance;
    }

    // Determines whether the specified value is close to one (1.0).
    inline bool IsOne(int32 a)
    {
        return a == 1;
    }

    // Determines whether the specified value is close to one (1.0f).
    inline bool IsOne(float a)
    {
        return IsZero(a - 1.0f);
    }

    // Returns a value indicating the sign of a number.
    inline float Sign(float v)
    {
        return v > 0.0f ? 1.0f : v < 0.0f ? -1.0f : 0.0f;
    }

    /// <summary>
    /// Compares the sign of two floating-point values.
    /// </summary>
    /// <param name="a">The first value.</param>
    /// <param name="b">The second value.</param>
    /// <returns>True if given values have the same sign (both positive or negative); otherwise false.</returns>
    inline bool SameSign(const float a, const float b)
    {
        return a * b >= 0.0f;
    }

    /// <summary>
    /// Compares the sign of two floating-point values.
    /// </summary>
    /// <param name="a">The first value.</param>
    /// <param name="b">The second value.</param>
    /// <returns>True if given values don't have the same sign (first is positive and second is negative or vice versa); otherwise false.</returns>
    inline bool NotSameSign(const float a, const float b)
    {
        return a * b < 0.0f;
    }

    /// <summary>
    /// Checks if a and b are almost equals, taking into account the magnitude of floating point numbers.
    /// </summary>
    /// <remarks>The code is using the technique described by Bruce Dawson in <a href="http://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/">Comparing Floating point numbers 2012 edition</a>.</remarks>
    /// <param name="a">The left value to compare</param>
    /// <param name="b">The right value to compare</param>
    /// <returns>True if a almost equal to b, otherwise false</returns>
    static bool NearEqual(float a, float b)
    {
        // Check if the numbers are really close - needed when comparing numbers near zero
        if (Abs(a - b) < ZeroTolerance)
            return true;

        // Original from Bruce Dawson: http://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
        const int32 aInt = *(int32*)&a;
        const int32 bInt = *(int32*)&b;

        // Different signs means they do not match
        if (aInt < 0 != bInt < 0)
            return false;

        // Find the difference in ULPs
        const int ulp = Abs(aInt - bInt);

        // Choose of maxUlp = 4
        // according to http://code.google.com/p/googletest/source/browse/trunk/include/gtest/internal/gtest-internal.h
        const int maxUlp = 4;
        return ulp <= maxUlp;
    }

    /// <summary>
    /// Checks if a and b are not even almost equal, taking into account the magnitude of floating point numbers
    /// </summary>
    /// <param name="a">The left value to compare</param>
    /// <param name="b">The right value to compare</param>
    /// <returns>False if a almost equal to b, otherwise true</returns>
    static FORCE_INLINE bool NotNearEqual(float a, float b)
    {
        return !NearEqual(a, b);
    }

    /// <summary>
    /// Checks if a and b are almost equals within the given epsilon value.
    /// </summary>
    /// <param name="a">The left value to compare.</param>
    /// <param name="b">The right value to compare.</param>
    /// <param name="eps">The comparision epsilon value. Should be 1e-4 or less.</param>
    /// <returns>True if a almost equal to b, otherwise false</returns>
    static bool NearEqual(float a, float b, float eps)
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
    static float Remap(float value, float fromMin, float fromMax, float toMin, float toMax)
    {
        return (value - fromMin) / (fromMax - fromMin) * (toMax - toMin) + toMin;
    }

    template<typename T>
    static T AlignUpWithMask(T value, T mask)
    {
        return (T)((value + mask) & ~mask);
    }

    template<typename T>
    static T AlignDownWithMask(T value, T mask)
    {
        return (T)(value & ~mask);
    }

    /// <summary>
    /// Aligns value up to match desire alignment.
    /// </summary>
    /// <param name="value">The value.</param>
    /// <param name="alignment">The alignment.</param>
    /// <returns>Aligned value (multiple of alignment).</returns>
    template<typename T>
    static T AlignUp(T value, T alignment)
    {
        T mask = alignment - 1;
        return (T)((value + mask) & ~mask);
    }

    /// <summary>
    /// Aligns value down to match desire alignment.
    /// </summary>
    /// <param name="value">The value.</param>
    /// <param name="alignment">The alignment.</param>
    /// <returns>Aligned value (multiple of alignment).</returns>
    template<typename T>
    static T AlignDown(T value, T alignment)
    {
        T mask = alignment - 1;
        return (T)(value & ~mask);
    }

    /// <summary>
    /// Determines whether the specified value is aligned.
    /// </summary>
    /// <param name="value">The value.</param>
    /// <param name="alignment">The alignment.</param>
    /// <returns><c>true</c> if the specified value is aligned; otherwise, <c>false</c>.</returns>
    template<typename T>
    static bool IsAligned(T value, T alignment)
    {
        return 0 == (value & (alignment - 1));
    }

    template<typename T>
    static T DivideByMultiple(T value, T alignment)
    {
        return (T)((value + alignment - 1) / alignment);
    }

    static float ClampAxis(float angle)
    {
        angle = Mod(angle, 360.f);
        if (angle < 0.0f)
            angle += 360.0f;
        return angle;
    }

    static float NormalizeAxis(float angle)
    {
        angle = ClampAxis(angle);
        if (angle > 180.f)
            angle -= 360.f;
        return angle;
    }

    // Find the smallest angle between two headings (in radians).
    static float FindDeltaAngle(float a1, float a2)
    {
        float delta = a2 - a1;
        if (delta > PI)
            delta = delta - TWO_PI;
        else if (delta < -PI)
            delta = delta + TWO_PI;
        return delta;
    }

    // Given a heading which may be outside the +/- PI range, 'unwind' it back into that range
    template<typename T>
    static T UnwindRadians(T a)
    {
        while (a > PI)
            a -= TWO_PI;
        while (a < -PI)
            a += TWO_PI;
        return a;
    }

    // Utility to ensure angle is between +/- 180 degrees by unwinding
    template<typename T>
    static T UnwindDegrees(T a)
    {
        while (a > 180)
            a -= 360;
        while (a < -180)
            a += 360;
        return a;
    }

    /// <summary>
    ///  Returns value based on comparand. The main purpose of this function is to avoid branching based on floating point comparison which can be avoided via compiler intrinsics.
    /// </summary>
    /// <remarks>Please note that this doesn't define what happens in the case of NaNs as there might be platform specific differences.</remarks>
    /// <param name="comparand">Comparand the results are based on.</param>
    /// <param name="valueGEZero">The result value if comparand >= 0.</param>
    /// <param name="valueLTZero">The result value if comparand < 0.</param>
    /// <returns>the valueGEZero if comparand >= 0, valueLTZero otherwise</returns>
    static float FloatSelect(float comparand, float valueGEZero, float valueLTZero)
    {
        return comparand >= 0.f ? valueGEZero : valueLTZero;
    }

    /// <summary>
    /// Returns a smooth Hermite interpolation between 0 and 1 for the value X (where X ranges between A and B). Clamped to 0 for X <= A and 1 for X >= B.
    /// </summary>
    /// <param name="a">The minimum value of x.</param>
    /// <param name="b">The maximum value of x.</param>
    /// <param name="x">The x.</param>
    /// <returns>The smoothed value between 0 and 1.</returns>
    static float SmoothStep(float a, float b, float x)
    {
        if (x < a)
            return 0.0f;
        if (x >= b)
            return 1.0f;
        const float fraction = (x - a) / (b - a);
        return fraction * fraction * (3.0f - 2.0f * fraction);
    }

    /// <summary>
    /// Performs a cubic interpolation.
    /// </summary>
    /// <param name="p0">The first point.</param>
    /// <param name="t1">The tangent direction at first point.</param>
    /// <param name="p1">The second point.</param>
    /// <param name="t1">The tangent direction at second point.</paramm>
    /// <param name="alpha">The distance along the spline.</param>
    /// <returns>The interpolated value.</returns>
    template<class T, class U>
    static FORCE_INLINE T CubicInterp(const T& p0, const T& t0, const T& p1, const T& t1, const U& alpha)
    {
        const float alpha2 = alpha * alpha;
        const float alpha3 = alpha2 * alpha;
        return (T)((2 * alpha3 - 3 * alpha2 + 1) * p0) + (alpha3 - 2 * alpha2 + alpha) * t0 + (alpha3 - alpha2) * t1 + (-2 * alpha3 + 3 * alpha2) * p1;
    }

    /// <summary>
    /// Interpolate between A and B, applying an ease in function. Exponent controls the degree of the curve.
    /// </summary>
    template<class T>
    static FORCE_INLINE T InterpEaseIn(const T& a, const T& b, float alpha, float exponent)
    {
        const float blend = Pow(alpha, exponent);
        return Lerp<T>(a, b, blend);
    }

    /// <summary>
    /// Interpolate between A and B, applying an ease out function. Exponent controls the degree of the curve.
    /// </summary>
    template<class T>
    static FORCE_INLINE T InterpEaseOut(const T& a, const T& b, float alpha, float exponent)
    {
        const float blend = 1.f - Pow(1.f - alpha, exponent);
        return Lerp<T>(a, b, blend);
    }

    /// <summary>
    /// Interpolate between A and B, applying an ease in/out function. Exponent controls the degree of the curve.
    /// </summary>
    template<class T>
    static FORCE_INLINE T InterpEaseInOut(const T& a, const T& b, float alpha, float exponent)
    {
        return Lerp<T>(a, b, alpha < 0.5f ? InterpEaseIn(0.f, 1.f, alpha * 2.f, exponent) * 0.5f : InterpEaseOut(0.f, 1.f, alpha * 2.f - 1.f, exponent) * 0.5f + 0.5f);
    }

    /// <summary>
    /// Interpolation between A and B, applying a sinusoidal in function.
    /// </summary>
    template<class T>
    static FORCE_INLINE T InterpSinIn(const T& a, const T& b, float alpha)
    {
        const float blend = -1.f * Cos(alpha * PI_HALF) + 1.f;
        return Lerp<T>(a, b, blend);
    }

    /// <summary>
    /// Interpolation between A and B, applying a sinusoidal out function.
    /// </summary>
    template<class T>
    static FORCE_INLINE T InterpSinOut(const T& a, const T& b, float alpha)
    {
        const float blend = Sin(alpha * PI_HALF);
        return Lerp<T>(a, b, blend);
    }

    /// <summary>
    /// Interpolation between A and B, applying a sinusoidal in/out function.
    /// </summary>
    template<class T>
    static FORCE_INLINE T InterpSinInOut(const T& a, const T& b, float alpha)
    {
        return Lerp<T>(a, b, alpha < 0.5f ? InterpSinIn(0.f, 1.f, alpha * 2.f) * 0.5f : InterpSinOut(0.f, 1.f, alpha * 2.f - 1.f) * 0.5f + 0.5f);
    }

    /// <summary>
    /// Interpolation between A and B, applying an exponential in function.
    /// </summary>
    template<class T>
    static FORCE_INLINE T InterpExpoIn(const T& a, const T& b, float alpha)
    {
        const float blend = alpha == 0.f ? 0.f : Pow(2.f, 10.f * (alpha - 1.f));
        return Lerp<T>(a, b, blend);
    }

    /// <summary>
    /// Interpolation between A and B, applying an exponential out function.
    /// </summary>
    template<class T>
    static FORCE_INLINE T InterpExpoOut(const T& a, const T& b, float alpha)
    {
        const float blend = alpha == 1.f ? 1.f : -Pow(2.f, -10.f * alpha) + 1.f;
        return Lerp<T>(a, b, blend);
    }

    /// <summary>
    /// Interpolation between A and B, applying an exponential in/out function.
    /// </summary>
    template<class T>
    static FORCE_INLINE T InterpExpoInOut(const T& a, const T& b, float alpha)
    {
        return Lerp<T>(a, b, alpha < 0.5f ? InterpExpoIn(0.f, 1.f, alpha * 2.f) * 0.5f : InterpExpoOut(0.f, 1.f, alpha * 2.f - 1.f) * 0.5f + 0.5f);
    }

    /// <summary>
    /// Interpolation between A and B, applying a circular in function.
    /// </summary>
    template<class T>
    static FORCE_INLINE T InterpCircularIn(const T& a, const T& b, float alpha)
    {
        const float blend = -1.f * (Sqrt(1.f - alpha * alpha) - 1.f);
        return Lerp<T>(a, b, blend);
    }

    /// <summary>
    /// Interpolation between A and B, applying a circular out function.
    /// </summary>
    template<class T>
    static FORCE_INLINE T InterpCircularOut(const T& a, const T& b, float alpha)
    {
        alpha -= 1.f;
        const float blend = Sqrt(1.f - alpha * alpha);
        return Lerp<T>(a, b, blend);
    }

    /// <summary>
    /// Interpolation between A and B, applying a circular in/out function.
    /// </summary>
    template<class T>
    static FORCE_INLINE T InterpCircularInOut(const T& a, const T& b, float alpha)
    {
        return Lerp<T>(a, b, alpha < 0.5f ? InterpCircularIn(0.f, 1.f, alpha * 2.f) * 0.5f : InterpCircularOut(0.f, 1.f, alpha * 2.f - 1.f) * 0.5f + 0.5f);
    }

    /// <summary>
    /// Ping pongs the value <paramref name="t"/>, so that it is never larger than <paramref name="length"/> and never smaller than 0.
    /// </summary>
    /// <param name="t"></param>
    /// <param name="length"></param>
    /// <returns></returns>
    template<class T>
    static FORCE_INLINE T PingPong(const T& t, T length)
    {
        return length - Abs(Repeat(t, length * 2.0f) - length);
    }

    // Rotates position about the input axis by the given angle (in radians), and returns the delta to position
    Vector3 FLAXENGINE_API RotateAboutAxis(const Vector3& normalizedRotationAxis, float angle, const Vector3& positionOnAxis, const Vector3& position);

    Vector3 FLAXENGINE_API ExtractLargestComponent(const Vector3& v);
}
