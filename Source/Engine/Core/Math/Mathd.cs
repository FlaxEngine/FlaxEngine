// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.ComponentModel;

namespace FlaxEngine
{
    /// <summary>
    /// A collection of common math functions on double floating-points.
    /// </summary>
    [HideInEditor]
    public static class Mathd
    {
        /// <summary>
        /// The value for which all absolute numbers smaller than are considered equal to zero.
        /// </summary>
        public const double Epsilon = 1e-16;

        /// <summary>
        /// A value specifying the approximation of π which is 180 degrees.
        /// </summary>
        public const double Pi = Math.PI;

        /// <summary>
        /// A value specifying the approximation of 2π which is 360 degrees.
        /// </summary>
        public const double TwoPi = 2.0 * Math.PI;

        /// <summary>
        /// A value specifying the approximation of π/2 which is 90 degrees.
        /// </summary>
        public const double PiOverTwo = Math.PI / 2.0;

        /// <summary>
        /// A value specifying the approximation of π/4 which is 45 degrees.
        /// </summary>
        public const double PiOverFour = Math.PI / 4.0;

        /// <summary>
        /// A value specifying the golden mean
        /// </summary>
        public const double GoldenRatio = 1.6180339887;

        /// <summary>
        /// Returns the absolute value of f.
        /// </summary>
        /// <param name="f"></param>
        public static double Abs(double f)
        {
            return Math.Abs(f);
        }

        /// <summary>
        /// Returns the arc-cosine of f - the angle in radians whose cosine is f.
        /// </summary>
        /// <param name="f"></param>
        public static double Acos(double f)
        {
            return Math.Acos(f);
        }

        /// <summary>
        /// Compares two floating point values if they are similar.
        /// </summary>
        /// <param name="a"></param>
        /// <param name="b"></param>
        public static bool Approximately(double a, double b)
        {
            return Abs(b - a) < Max(Epsilon * Max(Abs(a), Abs(b)), Epsilon * 8f);
        }

        /// <summary>
        /// Returns the arc-sine of f - the angle in radians whose sine is f.
        /// </summary>
        /// <param name="f"></param>
        public static double Asin(double f)
        {
            return Math.Asin(f);
        }

        /// <summary>
        /// Returns the arc-tangent of f - the angle in radians whose tangent is f.
        /// </summary>
        /// <param name="f"></param>
        public static double Atan(double f)
        {
            return Math.Atan(f);
        }

        /// <summary>
        /// Returns the angle in radians whose Tan is y/x.
        /// </summary>
        /// <param name="y"></param>
        /// <param name="x"></param>
        public static double Atan2(double y, double x)
        {
            return Math.Atan2(y, x);
        }

        /// <summary>
        /// Returns the smallest integer greater to or equal to f.
        /// </summary>
        /// <param name="f"></param>
        public static double Ceil(double f)
        {
            return Math.Ceiling(f);
        }

        /// <summary>
        /// Returns the smallest integer greater to or equal to f.
        /// </summary>
        /// <param name="f"></param>
        public static long CeilToInt(double f)
        {
            return (long)Math.Ceiling(f);
        }

        /// <summary>
        /// Clamps value between 0 and 1 and returns value.
        /// </summary>
        /// <param name="value">Value to clamp</param>
        /// <returns>Result value</returns>
        public static double Saturate(double value)
        {
            if (value < 0d)
                return 0d;
            return value > 1d ? 1d : value;
        }

        /// <summary>
        /// Returns the cosine of angle f in radians.
        /// </summary>
        /// <param name="f"></param>
        public static double Cos(double f)
        {
            return Math.Cos(f);
        }

        /// <summary>
        /// Calculates the shortest difference between two given angles given in degrees.
        /// </summary>
        /// <param name="current"></param>
        /// <param name="target"></param>
        public static double DeltaAngle(double current, double target)
        {
            double t = Repeat(target - current, 360f);
            if (t > 180d)
                t -= 360d;
            return t;
        }

        /// <summary>
        /// Returns e raised to the specified power.
        /// </summary>
        /// <param name="power"></param>
        public static double Exp(double power)
        {
            return Math.Exp(power);
        }

        /// <summary>
        /// Returns the largest integer smaller to or equal to f.
        /// </summary>
        /// <param name="f"></param>
        public static double Floor(double f)
        {
            return Math.Floor(f);
        }

        /// <summary>
        /// Returns the largest integer smaller to or equal to f.
        /// </summary>
        /// <param name="f"></param>
        public static long FloorToInt(double f)
        {
            return (long)Math.Floor(f);
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
        public static double Remap(double value, double fromMin, double fromMax, double toMin, double toMax)
        {
            return (value - fromMin) / (fromMax - fromMin) * (toMax - toMin) + toMin;
        }

        /// <summary>
        /// Calculates the linear parameter t that produces the interpolation value within the range [a, b].
        /// </summary>
        /// <param name="a"></param>
        /// <param name="b"></param>
        /// <param name="value"></param>
        public static double InverseLerp(double a, double b, double value)
        {
            if (a == b)
                return 0d;
            return Saturate((value - a) / (b - a));
        }

        /// <summary>
        /// Same as Lerp but makes sure the values interpolate correctly when they wrap around 360 degrees.
        /// </summary>
        /// <param name="a"></param>
        /// <param name="b"></param>
        /// <param name="t"></param>
        public static double LerpAngle(double a, double b, double t)
        {
            double c = Repeat(b - a, 360d);
            if (c > 180d)
                c -= 360d;
            return a + c * Saturate(t);
        }

        /// <summary>
        /// Returns the logarithm of a specified number in a specified base.
        /// </summary>
        /// <param name="f"></param>
        /// <param name="p"></param>
        public static double Log(double f, double p)
        {
            return Math.Log(f, p);
        }

        /// <summary>
        /// Returns the natural (base e) logarithm of a specified number.
        /// </summary>
        /// <param name="f"></param>
        public static double Log(double f)
        {
            return Math.Log(f);
        }

        /// <summary>
        /// Returns the base 10 logarithm of a specified number.
        /// </summary>
        /// <param name="f"></param>
        public static double Log10(double f)
        {
            return Math.Log10(f);
        }

        /// <summary>
        /// Returns largest of two or more values.
        /// </summary>
        /// <param name="a"></param>
        /// <param name="b"></param>
        public static double Max(double a, double b)
        {
            return a <= b ? b : a;
        }


        /// <summary>
        /// Returns largest of two or more values.
        /// </summary>
        /// <param name="values"></param>
        public static double Max(params double[] values)
        {
            int length = values.Length;
            if (length == 0)
                return 0d;

            double t = values[0];
            for (var i = 1; i < length; i++)
                if (values[i] > t)
                    t = values[i];
            return t;
        }


        /// <summary>
        /// Returns the smallest of two or more values.
        /// </summary>
        /// <param name="a"></param>
        /// <param name="b"></param>
        public static double Min(double a, double b)
        {
            return a >= b ? b : a;
        }


        /// <summary>
        /// Returns the smallest of two or more values.
        /// </summary>
        /// <param name="values"></param>
        public static double Min(params double[] values)
        {
            int length = values.Length;
            if (length == 0)
                return 0d;

            double t = values[0];
            for (var i = 1; i < length; i++)
                if (values[i] < t)
                    t = values[i];

            return t;
        }

        /// <summary>
        /// Moves a value current towards target.
        /// </summary>
        /// <param name="current">The current value.</param>
        /// <param name="target">The value to move towards.</param>
        /// <param name="maxDelta">The maximum change that should be applied to the value.</param>
        public static double MoveTowards(double current, double target, double maxDelta)
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
        public static double MoveTowardsAngle(double current, double target, double maxDelta)
        {
            double delta = DeltaAngle(current, target);
            if ((-maxDelta < delta) && (delta < maxDelta))
                return target;
            target = current + delta;
            return MoveTowards(current, target, maxDelta);
        }

        /// <summary>
        /// PingPongs the value t, so that it is never larger than length and never smaller than 0.
        /// </summary>
        /// <param name="t"></param>
        /// <param name="length"></param>
        public static double PingPong(double t, double length)
        {
            t = Repeat(t, length * 2f);
            return length - Abs(t - length);
        }

        /// <summary>
        /// Returns f raised to power p.
        /// </summary>
        /// <param name="f"></param>
        /// <param name="p"></param>
        public static double Pow(double f, double p)
        {
            return Math.Pow(f, p);
        }

        /// <summary>
        /// Loops the value t, so that it is never larger than length and never smaller than 0.
        /// </summary>
        /// <param name="t"></param>
        /// <param name="length"></param>
        public static double Repeat(double t, double length)
        {
            return t - Floor(t / length) * length;
        }

        /// <summary>
        /// Returns f rounded to the nearest integer.
        /// </summary>
        /// <param name="f"></param>
        public static double Round(double f)
        {
            return Math.Round(f);
        }

        /// <summary>
        /// Returns f rounded to the nearest integer.
        /// </summary>
        /// <param name="f"></param>
        public static int RoundToInt(double f)
        {
            return (int)Math.Round(f);
        }

        /// <summary>
        /// Returns f rounded to the nearest integer.
        /// </summary>
        /// <param name="f"></param>
        public static long RoundToLong(double f)
        {
            return (long)Math.Round(f);
        }

        /// <summary>
        /// Returns the sign of f.
        /// </summary>
        /// <param name="f"></param>
        public static double Sign(double f)
        {
            return f > 0.0d ? 1.0d : f < 0.0d ? -1.0d : 0.0d;
        }

        /// <summary>
        /// Returns the sine of angle f in radians.
        /// </summary>
        /// <param name="f"></param>
        public static double Sin(double f)
        {
            return Math.Sin(f);
        }

        /// <summary>
        /// Gradually changes a value towards a desired goal over time with smoothing.
        /// </summary>
        /// <param name="current">The current value.</param>
        /// <param name="target">The target value.</param>
        /// <param name="currentVelocity">The current velocity.</param>
        /// <param name="smoothTime">The smoothing time. Smaller values increase blending time.</param>
        /// <param name="maxSpeed">The maximum speed.</param>
        /// <returns>The smoothed value.</returns>
        public static double SmoothDamp(double current, double target, ref double currentVelocity, double smoothTime, double maxSpeed)
        {
            return SmoothDamp(current, target, ref currentVelocity, smoothTime, maxSpeed, Time.DeltaTime);
        }

        /// <summary>
        /// Gradually changes a value towards a desired goal over time with smoothing.
        /// </summary>
        /// <param name="current">The current value.</param>
        /// <param name="target">The target value.</param>
        /// <param name="currentVelocity">The current velocity.</param>
        /// <param name="smoothTime">The smoothing time. Smaller values increase blending time.</param>
        /// <returns>The smoothed value.</returns>
        public static double SmoothDamp(double current, double target, ref double currentVelocity, double smoothTime)
        {
            return SmoothDamp(current, target, ref currentVelocity, smoothTime, double.PositiveInfinity, Time.DeltaTime);
        }

        /// <summary>
        /// Gradually changes a value towards a desired goal over time with smoothing.
        /// </summary>
        /// <param name="current">The current value.</param>
        /// <param name="target">The target value.</param>
        /// <param name="currentVelocity">The current velocity.</param>
        /// <param name="smoothTime">The smoothing time. Smaller values increase blending time.</param>
        /// <param name="maxSpeed">The maximum speed.</param>
        /// <param name="deltaTime">The delta time (in seconds) since last update.</param>
        /// <returns>The smoothed value.</returns>
        public static double SmoothDamp(double current, double target, ref double currentVelocity, double smoothTime, [DefaultValue("double.PositiveInfinity")] double maxSpeed, [DefaultValue("Time.DeltaTime")] double deltaTime)
        {
            smoothTime = Max(0.0001d, smoothTime);
            double a = 2d / smoothTime;
            double b = a * deltaTime;
            double c = 1d / (1d + b + 0.48d * b * b + 0.235d * b * b * b);
            double d = current - target;
            double e = target;
            double f = maxSpeed * smoothTime;
            d = Clamp(d, -f, f);
            target = current - d;
            double g = (currentVelocity + a * d) * deltaTime;
            currentVelocity = (currentVelocity - a * g) * c;
            double h = target + (d + g) * c;
            if (e - current > 0d == h > e)
            {
                h = e;
                currentVelocity = (h - e) / deltaTime;
            }
            return h;
        }

        /// <summary>
        /// Gradually changes an angle towards a desired goal over time with smoothing.
        /// </summary>
        /// <param name="current">The current angle.</param>
        /// <param name="target">The target angle.</param>
        /// <param name="currentVelocity">The current velocity.</param>
        /// <param name="smoothTime">The smoothing time. Smaller values increase blending time.</param>
        /// <param name="maxSpeed">The maximum speed.</param>
        /// <returns>The smoothed value.</returns>
        public static double SmoothDampAngle(double current, double target, ref double currentVelocity, double smoothTime, double maxSpeed)
        {
            return SmoothDampAngle(current, target, ref currentVelocity, smoothTime, maxSpeed, Time.DeltaTime);
        }

        /// <summary>
        /// Gradually changes an angle towards a desired goal over time with smoothing.
        /// </summary>
        /// <param name="current">The current angle.</param>
        /// <param name="target">The target angle.</param>
        /// <param name="currentVelocity">The current velocity.</param>
        /// <param name="smoothTime">The smoothing time. Smaller values increase blending time.</param>
        /// <returns>The smoothed value.</returns>
        public static double SmoothDampAngle(double current, double target, ref double currentVelocity, double smoothTime)
        {
            return SmoothDampAngle(current, target, ref currentVelocity, smoothTime, double.PositiveInfinity, Time.DeltaTime);
        }

        /// <summary>
        /// Gradually changes an angle towards a desired goal over time with smoothing.
        /// </summary>
        /// <param name="current">The current angle.</param>
        /// <param name="target">The target angle.</param>
        /// <param name="currentVelocity">The current velocity.</param>
        /// <param name="smoothTime">The smoothing time. Smaller values increase blending time.</param>
        /// <param name="maxSpeed">The maximum speed.</param>
        /// <param name="deltaTime">The delta time (in seconds) since last update.</param>
        /// <returns>The smoothed value.</returns>
        public static double SmoothDampAngle(double current, double target, ref double currentVelocity, double smoothTime, [DefaultValue("double.PositiveInfinity")] double maxSpeed, [DefaultValue("Time.DeltaTime")] double deltaTime)
        {
            target = current + DeltaAngle(current, target);
            return SmoothDamp(current, target, ref currentVelocity, smoothTime, maxSpeed, deltaTime);
        }

        /// <summary>
        /// Interpolates between min and max with smoothing at the limits.
        /// </summary>
        /// <param name="from"></param>
        /// <param name="to"></param>
        /// <param name="t"></param>
        public static double SmoothStep(double from, double to, double t)
        {
            t = Saturate(t);
            t = -2d * t * t * t + 3d * t * t;
            return to * t + from * (1d - t);
        }

        /// <summary>
        /// Performs a cubic interpolation.
        /// </summary>
        /// <param name="p0">The first point.</param>
        /// <param name="t0">The tangent direction at first point.</param>
        /// <param name="p1">The second point.</param>
        /// <param name="t1">The tangent direction at second point.</param>
        /// <param name="alpha">The distance along the spline.</param>
        /// <returns>The interpolated value.</returns>
        public static double CubicInterp(double p0, double t0, double p1, double t1, double alpha)
        {
            double alpha2 = alpha * alpha;
            double alpha3 = alpha2 * alpha;
            return (((2d * alpha3) - (3d * alpha2) + 1d) * p0) + ((alpha3 - (2d * alpha2) + alpha) * t0) + ((alpha3 - alpha2) * t1) + (((-2d * alpha3) + (3d * alpha2)) * p1);
        }

        /// <summary>
        /// Interpolate between A and B, applying an ease in function. Exponent controls the degree of the curve.
        /// </summary>
        public static double InterpEaseIn(double a, double b, double alpha, double exponent)
        {
            double modifiedAlpha = Pow(alpha, exponent);
            return Lerp(a, b, modifiedAlpha);
        }

        /// <summary>
        /// Interpolate between A and B, applying an ease out function. Exponent controls the degree of the curve.
        /// </summary>
        public static double InterpEaseOut(double a, double b, double alpha, double exponent)
        {
            double modifiedAlpha = 1d - Pow(1d - alpha, exponent);
            return Lerp(a, b, modifiedAlpha);
        }

        /// <summary>
        /// Interpolate between A and B, applying an ease in/out function. Exponent controls the degree of the curve.
        /// </summary>
        public static double InterpEaseInOut(double a, double b, double alpha, double exponent)
        {
            return Lerp(a, b, (alpha < 0.5d) ? InterpEaseIn(0d, 1d, alpha * 2d, exponent) * 0.5d : InterpEaseOut(0d, 1d, alpha * 2d - 1d, exponent) * 0.5d + 0.5d);
        }

        /// <summary>
        /// Interpolation between A and B, applying a sinusoidal in function.
        /// </summary>
        public static double InterpSinIn(double a, double b, double alpha)
        {
            double modifiedAlpha = -1d * Cos(alpha * PiOverTwo) + 1d;
            return Lerp(a, b, modifiedAlpha);
        }

        /// <summary>
        /// Interpolation between A and B, applying a sinusoidal out function.
        /// </summary>
        public static double InterpSinOut(double a, double b, double alpha)
        {
            double modifiedAlpha = Sin(alpha * PiOverTwo);
            return Lerp(a, b, modifiedAlpha);
        }

        /// <summary>
        /// Interpolation between A and B, applying a sinusoidal in/out function.
        /// </summary>
        public static double InterpSinInOut(double a, double b, double alpha)
        {
            return Lerp(a, b, (alpha < 0.5d) ? InterpSinIn(0d, 1d, alpha * 2d) * 0.5d : InterpSinOut(0d, 1d, alpha * 2d - 1d) * 0.5d + 0.5d);
        }

        /// <summary>
        /// Interpolation between A and B, applying an exponential in function.
        /// </summary>
        public static double InterpExpoIn(double a, double b, double alpha)
        {
            double modifiedAlpha = (alpha == 0d) ? 0d : Pow(2d, 10d * (alpha - 1d));
            return Lerp(a, b, modifiedAlpha);
        }

        /// <summary>
        /// Interpolation between A and B, applying an exponential out function.
        /// </summary>
        public static double InterpExpoOut(double a, double b, double alpha)
        {
            double modifiedAlpha = (alpha == 1d) ? 1d : -Pow(2d, -10d * alpha) + 1d;
            return Lerp(a, b, modifiedAlpha);
        }

        /// <summary>
        /// Interpolation between A and B, applying an exponential in/out function.
        /// </summary>
        public static double InterpExpoInOut(double a, double b, double alpha)
        {
            return Lerp(a, b, (alpha < 0.5d) ? InterpExpoIn(0d, 1d, alpha * 2d) * 0.5d : InterpExpoOut(0d, 1d, alpha * 2d - 1d) * 0.5d + 0.5d);
        }

        /// <summary>
        /// Interpolation between A and B, applying a circular in function.
        /// </summary>
        public static double InterpCircularIn(double a, double b, double alpha)
        {
            double modifiedAlpha = -1d * (Sqrt(1d - alpha * alpha) - 1d);
            return Lerp(a, b, modifiedAlpha);
        }

        /// <summary>
        /// Interpolation between A and B, applying a circular out function.
        /// </summary>
        public static double InterpCircularOut(double a, double b, double alpha)
        {
            alpha -= 1d;
            double modifiedAlpha = Sqrt(1d - alpha * alpha);
            return Lerp(a, b, modifiedAlpha);
        }

        /// <summary>
        /// Interpolation between A and B, applying a circular in/out function.
        /// </summary>
        public static double InterpCircularInOut(double a, double b, double alpha)
        {
            return Lerp(a, b, (alpha < 0.5d) ? InterpCircularIn(0d, 1d, alpha * 2d) * 0.5d : InterpCircularOut(0d, 1d, alpha * 2d - 1d) * 0.5d + 0.5d);
        }

        /// <summary>
        /// Maps the specified value from the given range into another.
        /// [Deprecated on 17.04.2023, expires on 17.04.2024]
        /// </summary>
        /// <param name="value">The value to map from range [fromMin; fromMax].</param>
        /// <param name="fromMin">The source range minimum value.</param>
        /// <param name="fromMax">The source range maximum value.</param>
        /// <param name="toMin">The destination range minimum value.</param>
        /// <param name="toMax">The destination range maximum value.</param>
        /// <returns>The mapped value in range [toMin; toMax].</returns>
        [Obsolete("Use Remap instead")]
        public static double Map(double value, double fromMin, double fromMax, double toMin, double toMax)
        {
            double t = (value - fromMin) / (fromMax - fromMin);
            return toMin + t * (toMax - toMin);
        }

        /// <summary>
        /// Get the next power of two for a size.
        /// </summary>
        /// <param name="size">The size.</param>
        /// <returns>System.Int32.</returns>
        public static double NextPowerOfTwo(double size)
        {
            return Math.Pow(2d, Math.Ceiling(Math.Log(size, 2d)));
        }

        /// <summary>
        /// Converts a float value from sRGB to linear.
        /// </summary>
        /// <param name="sRgbValue">The sRGB value.</param>
        /// <returns>A linear value.</returns>
        public static double SRgbToLinear(double sRgbValue)
        {
            if (sRgbValue < 0.04045d)
                return sRgbValue / 12.92d;
            return Math.Pow((sRgbValue + 0.055d) / 1.055d, 2.4d);
        }

        /// <summary>
        /// Converts a float value from linear to sRGB.
        /// </summary>
        /// <param name="linearValue">The linear value.</param>
        /// <returns>The encoded sRGB value.</returns>
        public static double LinearToSRgb(double linearValue)
        {
            if (linearValue < 0.0031308d)
                return linearValue * 12.92d;
            return 1.055d * Math.Pow(linearValue, 1d / 2.4d) - 0.055d;
        }

        /// <summary>
        /// Returns square root of f.
        /// </summary>
        /// <param name="f"></param>
        public static double Sqrt(double f)
        {
            return Math.Sqrt(f);
        }

        /// <summary>
        /// Returns square of the given value.
        /// </summary>
        /// <param name="f">The value.</param>
        /// <returns>The value * value.</returns>
        public static double Square(double f)
        {
            return f * f;
        }

        /// <summary>
        /// Returns the tangent of angle f in radians.
        /// </summary>
        /// <param name="f"></param>
        public static double Tan(double f)
        {
            return Math.Tan(f);
        }

        /// <summary>
        /// Checks if a and b are almost equals, taking into account the magnitude of floating point numbers (unlike <see cref="WithinEpsilon" /> method). See Remarks. See remarks.
        /// </summary>
        /// <param name="a">The left value to compare.</param>
        /// <param name="b">The right value to compare.</param>
        /// <returns><c>true</c> if a almost equal to b, <c>false</c> otherwise</returns>
        /// <remarks>The code is using the technique described by Bruce Dawson in <a href="http://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/">Comparing Floating point numbers 2012 edition</a>.</remarks>
        public static unsafe bool NearEqual(double a, double b)
        {
            // Check if the numbers are really close -- needed
            // when comparing numbers near zero.
            if (IsZero(a - b))
                return true;

            // Original from Bruce Dawson: http://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
            long aInt = *(long*)&a;
            long bInt = *(long*)&b;

            // Different signs means they do not match.
            if (aInt < 0 != bInt < 0)
                return false;

            // Find the difference in ULPs.
            long ulp = Math.Abs(aInt - bInt);

            // Choose of maxUlp = 4
            // according to http://code.google.com/p/googletest/source/browse/trunk/include/gtest/internal/gtest-internal.h
            const long maxUlp = 4;
            return ulp <= maxUlp;
        }

        /// <summary>
        /// Determines whether the specified value is close to zero (0.0f).
        /// </summary>
        /// <param name="a">The floating value.</param>
        /// <returns><c>true</c> if the specified value is close to zero (0.0f); otherwise, <c>false</c>.</returns>
        public static bool IsZero(double a)
        {
            return Math.Abs(a) < Epsilon;
        }

        /// <summary>
        /// Determines whether the specified value is close to one (1.0f).
        /// </summary>
        /// <param name="a">The floating value.</param>
        /// <returns><c>true</c> if the specified value is close to one (1.0f); otherwise, <c>false</c>.</returns>
        public static bool IsOne(double a)
        {
            return IsZero(a - 1d);
        }

        /// <summary>
        /// Checks if a - b are almost equals within a float epsilon.
        /// </summary>
        /// <param name="a">The left value to compare.</param>
        /// <param name="b">The right value to compare.</param>
        /// <param name="epsilon">Epsilon value</param>
        /// <returns><c>true</c> if a almost equal to b within a float epsilon, <c>false</c> otherwise</returns>
        public static bool WithinEpsilon(double a, double b, double epsilon)
        {
            double num = a - b;
            return (-epsilon <= num) && (num <= epsilon);
        }

        /// <summary>
        /// Determines whether the specified value is in a given range [min; max].
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="min">The minimum.</param>
        /// <param name="max">The maximum.</param>
        /// <returns>
        ///   <c>true</c> if the specified value is in a given range; otherwise, <c>false</c>.
        /// </returns>
        public static bool IsInRange(double value, double min, double max)
        {
            return value >= min && value <= max;
        }

        /// <summary>
        /// Determines whether the specified value is NOT in a given range [min; max].
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="min">The minimum.</param>
        /// <param name="max">The maximum.</param>
        /// <returns>
        ///   <c>true</c> if the specified value is NOT in a given range; otherwise, <c>false</c>.
        /// </returns>
        public static bool IsNotInRange(double value, double min, double max)
        {
            return value < min || value > max;
        }

        #region Angle units conversions

        /// <summary>
        /// Converts revolutions to degrees.
        /// </summary>
        public static double RevolutionsToDegrees = 360d;

        /// <summary>
        /// Converts revolutions to radians.
        /// </summary>
        public static double RevolutionsToRadians = TwoPi;

        /// <summary>
        /// Converts revolutions to gradians.
        /// </summary>
        public static double RevolutionsToGradians = 400d;

        /// <summary>
        /// Converts degrees to revolutions.
        /// </summary>
        public static double DegreesToRevolutions = (1d / 360d);

        /// <summary>
        /// Converts degrees to radians.
        /// </summary>
        public static double DegreesToRadians = (Pi / 180d);

        /// <summary>
        /// Converts radians to revolutions.
        /// </summary>
        public static double RadiansToRevolutions = (1d / TwoPi);

        /// <summary>
        /// Converts radians to gradians.
        /// </summary>
        public static double RadiansToGradians = (200d / Pi);

        /// <summary>
        /// Converts gradians to revolutions.
        /// </summary>
        public static double GradiansToRevolutions = (1d / 400d);

        /// <summary>
        /// Converts gradians to degrees.
        /// </summary>
        public static double GradiansToDegrees = (9.0f / 10d);

        /// <summary>
        /// Converts gradians to radians.
        /// </summary>
        public static double GradiansToRadians = (Pi / 200d);

        /// <summary>
        /// Converts radians to degrees.
        /// </summary>
        public static double RadiansToDegrees = (180d / Pi);

        #endregion

        /// <summary>
        /// Given a heading which may be outside the +/- PI range, 'unwind' it back into that range.
        /// </summary>
        /// <remarks>Optimized version of <see cref="UnwindRadiansAccurate"/> that is it faster and has fixed cost but with large angle values (100 for example) starts to lose accuracy floating point problem.</remarks>
        /// <param name="angle">Angle in radians to unwind.</param>
        /// <returns>Valid angle in radians.</returns>
        public static double UnwindRadians(double angle)
        {
            var a = angle - Math.Floor(angle / TwoPi) * TwoPi; // Loop function between 0 and TwoPi
            return a > Pi ? a - TwoPi : a; // Change range so it become Pi and -Pi
        }

        /// <summary>
        /// The same as <see cref="UnwindRadians"/> but is more computation intensive with large <see href="angle"/> and has better accuracy with large <see href="angle"/>.
        /// <br>cost of this function is <see href="angle"/> % <see cref="Pi"/></br>
        /// </summary>
        /// <param name="angle">Angle in radians to unwind.</param>
        /// <returns>Valid angle in radians.</returns>
        public static double UnwindRadiansAccurate(double angle)
        {
            while (angle > Pi)
                angle -= TwoPi;
            while (angle < -Pi)
                angle += TwoPi;
            return angle;
        }

        /// <summary>
        /// Utility to ensure angle is between +/- 180 degrees by unwinding.
        /// </summary>
        /// <remarks>Optimized version of <see cref="UnwindDegreesAccurate"/> that is it faster and has fixed cost but with large angle values (100 for example) starts to lose accuracy floating point problem.</remarks>
        /// <param name="angle">Angle in degrees to unwind.</param>
        /// <returns>Valid angle in degrees.</returns>
        public static double UnwindDegrees(double angle)
        {
            var a = angle - Math.Floor(angle / 360.0) * 360.0; // Loop function between 0 and 360
            return a > 180 ? a - 360.0 : a; // Change range so it become 180 and -180
        }

        /// <summary>
        /// The same as <see cref="UnwindDegrees"/> but is more computation intensive with large <see href="angle"/> and has better accuracy with large <see href="angle"/>.
        /// <br>cost of this function is <see href="angle"/> % 180.0f</br>
        /// </summary>
        /// <param name="angle">Angle in radians to unwind.</param>
        /// <returns>Valid angle in radians.</returns>
        public static double UnwindDegreesAccurate(double angle)
        {
            while (angle > 180.0)
                angle -= 360.0;
            while (angle < -180.0)
                angle += 360.0;
            return angle;
        }

        /// <summary>
        /// Clamps the specified value.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="min">The min.</param>
        /// <param name="max">The max.</param>
        /// <returns>The result of clamping a value between min and max</returns>
        public static double Clamp(double value, double min, double max)
        {
            return value < min ? min : value > max ? max : value;
        }

        /// <summary>
        /// Interpolates between two values using a linear function by a given amount.
        /// </summary>
        /// <remarks>
        /// See:
        /// <br><seealso href="http://www.encyclopediaofmath.org/index.php/Linear_interpolation"/></br>
        /// <br><seealso href="http://fgiesen.wordpress.com/2012/08/15/linear-interpolation-past-present-and-future/"/></br>
        /// </remarks>
        /// <param name="from">Value to interpolate from.</param>
        /// <param name="to">Value to interpolate to.</param>
        /// <param name="amount">Interpolation amount.</param>
        /// <returns>The result of linear interpolation of values based on the amount.</returns>
        public static double Lerp(double from, double to, double amount)
        {
            return from + (to - from) * amount;
        }


        /// <summary>
        /// Performs smooth (cubic Hermite) interpolation between 0 and 1.
        /// </summary>
        /// <remarks>
        /// See: 
        /// <br><seealso href="https://en.wikipedia.org/wiki/Smoothstep"/></br>
        /// </remarks>
        /// <param name="amount">Value between 0 and 1 indicating interpolation amount.</param>
        public static double SmoothStep(double amount)
        {
            return amount <= 0d ? 0d : amount >= 1d ? 1d : amount * amount * (3d - 2d * amount);
        }

        /// <summary>
        /// Performs a smooth(er) interpolation between 0 and 1 with 1st and 2nd order derivatives of zero at endpoints.
        /// </summary>
        /// <remarks>
        /// See: 
        /// <br><seealso href="https://en.wikipedia.org/wiki/Smoothstep"/></br>
        /// </remarks>
        /// <param name="amount">Value between 0 and 1 indicating interpolation amount.</param>
        public static double SmootherStep(double amount)
        {
            return amount <= 0d ? 0d : amount >= 1d ? 1d : amount * amount * amount * (amount * (amount * 6d - 15d) + 10d);
        }

        /// <summary>
        /// Calculates the modulo of the specified value.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="modulo">The modulo.</param>
        /// <returns>The result of the modulo applied to value</returns>
        public static double Mod(double value, double modulo)
        {
            if (modulo == 0d)
                return value;
            return value % modulo;
        }

        /// <summary>
        /// Calculates the modulo 2*PI of the specified value.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the modulo applied to value</returns>
        public static double Mod2PI(double value)
        {
            return Mod(value, TwoPi);
        }

        /// <summary>
        /// Wraps the specified value into a range [min, max]
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="min">The min.</param>
        /// <param name="max">The max.</param>
        /// <returns>Result of the wrapping.</returns>
        /// <exception cref="ArgumentException">Is thrown when <paramref name="min" /> is greater than <paramref name="max" />.</exception>
        public static double Wrap(double value, double min, double max)
        {
            if (NearEqual(min, max))
                return min;

            double mind = min;
            double maxd = max;
            double valued = value;

            if (mind > maxd)
                throw new ArgumentException(string.Format("min {0} should be less than or equal to max {1}", min, max), nameof(min));

            double rangeSize = maxd - mind;
            return mind + (valued - mind) - rangeSize * Math.Floor((valued - mind) / rangeSize);
        }

        /// <summary>
        /// Gauss function.
        /// <br><seealso href="http://en.wikipedia.org/wiki/Gaussian_function#Two-dimensional_Gaussian_function"/></br>
        /// </summary>
        /// <param name="amplitude">Curve amplitude.</param>
        /// <param name="x">Position X.</param>
        /// <param name="y">Position Y</param>
        /// <param name="centerX">Center X.</param>
        /// <param name="centerY">Center Y.</param>
        /// <param name="sigmaX">Curve sigma X.</param>
        /// <param name="sigmaY">Curve sigma Y.</param>
        /// <returns>The result of Gaussian function.</returns>
        public static double Gauss(double amplitude, double x, double y, double centerX, double centerY, double sigmaX, double sigmaY)
        {
            double cx = x - centerX;
            double cy = y - centerY;

            double componentX = cx * cx / (2 * sigmaX * sigmaX);
            double componentY = cy * cy / (2 * sigmaY * sigmaY);

            return amplitude * Math.Exp(-(componentX + componentY));
        }

        /// <summary>
        /// Converts the input alpha value from a linear 0-1 value into the output alpha described by blend mode.
        /// </summary>
        /// <param name="alpha">The alpha (normalized to 0-1).</param>
        /// <param name="mode">The mode.</param>
        /// <returns>The output alpha (normalized to 0-1).</returns>
        public static double InterpolateAlphaBlend(double alpha, AlphaBlendMode mode)
        {
            switch (mode)
            {
            case AlphaBlendMode.Sinusoidal:
                alpha = (Sin(alpha * Pi - PiOverTwo) + 1d) / 2d;
                break;
            case AlphaBlendMode.Cubic:
                alpha = CubicInterp(0d, 0d, 1d, 0d, alpha);
                break;
            case AlphaBlendMode.QuadraticInOut:
                alpha = InterpEaseInOut(0d, 1d, alpha, 2d);
                break;
            case AlphaBlendMode.CubicInOut:
                alpha = InterpEaseInOut(0d, 1d, alpha, 3d);
                break;
            case AlphaBlendMode.HermiteCubic:
                alpha = SmoothStep(0d, 1d, alpha);
                break;
            case AlphaBlendMode.QuarticInOut:
                alpha = InterpEaseInOut(0d, 1d, alpha, 4d);
                break;
            case AlphaBlendMode.QuinticInOut:
                alpha = InterpEaseInOut(0d, 1d, alpha, 5d);
                break;
            case AlphaBlendMode.CircularIn:
                alpha = InterpCircularIn(0d, 1d, alpha);
                break;
            case AlphaBlendMode.CircularOut:
                alpha = InterpCircularOut(0d, 1d, alpha);
                break;
            case AlphaBlendMode.CircularInOut:
                alpha = InterpCircularInOut(0d, 1d, alpha);
                break;
            case AlphaBlendMode.ExpIn:
                alpha = InterpExpoIn(0d, 1d, alpha);
                break;
            case AlphaBlendMode.ExpOut:
                alpha = InterpExpoOut(0d, 1d, alpha);
                break;
            case AlphaBlendMode.ExpInOut:
                alpha = InterpExpoInOut(0d, 1d, alpha);
                break;
            }

            return Saturate(alpha);
        }
    }
}
