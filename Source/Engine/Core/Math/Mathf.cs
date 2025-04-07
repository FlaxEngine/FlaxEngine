// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.ComponentModel;

namespace FlaxEngine
{
    /// <summary>
    /// A collection of common math functions on single floating-points.
    /// </summary>
    [HideInEditor]
    public static class Mathf
    {
        /// <summary>
        /// The value for which all absolute numbers smaller than are considered equal to zero.
        /// </summary>
        public const float Epsilon = 1e-6f;

        /// <summary>
        /// A value specifying the approximation of π which is 180 degrees.
        /// </summary>
        public const float Pi = (float)Math.PI;

        /// <summary>
        /// A value specifying the approximation of 2π which is 360 degrees.
        /// </summary>
        public const float TwoPi = (float)(2 * Math.PI);

        /// <summary>
        /// A value specifying the approximation of π/2 which is 90 degrees.
        /// </summary>
        public const float PiOverTwo = (float)(Math.PI / 2);

        /// <summary>
        /// A value specifying the approximation of π/4 which is 45 degrees.
        /// </summary>
        public const float PiOverFour = (float)(Math.PI / 4);

        /// <summary>
        /// A value specifying the golden mean
        /// </summary>
        public const float GoldenRatio = 1.6180339887f;

        /// <summary>
        /// Returns the absolute value of f.
        /// </summary>
        /// <param name="f"></param>
        public static float Abs(float f)
        {
            return Math.Abs(f);
        }

        /// <summary>
        /// Returns the absolute value of f.
        /// </summary>
        /// <param name="f"></param>
        public static double Abs(double f)
        {
            return Math.Abs(f);
        }

        /// <summary>
        /// Returns the absolute value of value.
        /// </summary>
        /// <param name="value"></param>
        public static int Abs(int value)
        {
            return Math.Abs(value);
        }

        /// <summary>
        /// Returns the arc-cosine of f - the angle in radians whose cosine is f.
        /// </summary>
        /// <param name="f"></param>
        public static float Acos(float f)
        {
            return (float)Math.Acos(f);
        }

        /// <summary>
        /// Compares two floating point values if they are similar.
        /// </summary>
        /// <param name="a"></param>
        /// <param name="b"></param>
        public static bool Approximately(float a, float b)
        {
            return Abs(b - a) < Max(1E-06f * Max(Abs(a), Abs(b)), Epsilon * 8f);
        }

        /// <summary>
        /// Returns the arc-sine of f - the angle in radians whose sine is f.
        /// </summary>
        /// <param name="f"></param>
        public static float Asin(float f)
        {
            return (float)Math.Asin(f);
        }

        /// <summary>
        /// Returns the arc-tangent of f - the angle in radians whose tangent is f.
        /// </summary>
        /// <param name="f"></param>
        public static float Atan(float f)
        {
            return (float)Math.Atan(f);
        }

        /// <summary>
        /// Returns the angle in radians whose Tan is y/x.
        /// </summary>
        /// <param name="y"></param>
        /// <param name="x"></param>
        public static float Atan2(float y, float x)
        {
            return (float)Math.Atan2(y, x);
        }

        /// <summary>
        /// Returns the smallest integer greater to or equal to f.
        /// </summary>
        /// <param name="f"></param>
        public static float Ceil(float f)
        {
            return (float)Math.Ceiling(f);
        }

        /// <summary>
        /// Returns the smallest integer greater to or equal to f.
        /// </summary>
        /// <param name="f"></param>
        public static int CeilToInt(float f)
        {
            return (int)Math.Ceiling(f);
        }

        /// <summary>
        /// Clamps value between 0 and 1 and returns value.
        /// </summary>
        /// <param name="value">Value to clamp</param>
        /// <returns>Result value</returns>
        public static float Saturate(float value)
        {
            if (value < 0f)
                return 0f;
            return value > 1f ? 1f : value;
        }

        /// <summary>
        /// Clamps value between 0 and 1 and returns value.
        /// </summary>
        /// <param name="value">Value to clamp</param>
        /// <returns>Result value</returns>
        public static double Saturate(double value)
        {
            if (value < 0f)
                return 0f;
            return value > 1f ? 1f : value;
        }

        /// <summary>
        /// Returns the cosine of angle f in radians.
        /// </summary>
        /// <param name="f"></param>
        public static float Cos(float f)
        {
            return (float)Math.Cos(f);
        }

        /// <summary>
        /// Calculates the shortest difference between two given angles given in degrees.
        /// </summary>
        /// <param name="current"></param>
        /// <param name="target"></param>
        public static float DeltaAngle(float current, float target)
        {
            float t = Repeat(target - current, 360f);
            if (t > 180f)
                t -= 360f;
            return t;
        }

        /// <summary>
        /// Returns e raised to the specified power.
        /// </summary>
        /// <param name="power"></param>
        public static float Exp(float power)
        {
            return (float)Math.Exp(power);
        }

        /// <summary>
        /// Returns the largest integer smaller to or equal to f.
        /// </summary>
        /// <param name="f"></param>
        public static float Floor(float f)
        {
            return (float)Math.Floor(f);
        }

        /// <summary>
        /// Returns the largest integer smaller to or equal to f.
        /// </summary>
        /// <param name="f"></param>
        public static int FloorToInt(float f)
        {
            return (int)Math.Floor(f);
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
        public static float Remap(float value, float fromMin, float fromMax, float toMin, float toMax)
        {
            return (value - fromMin) / (fromMax - fromMin) * (toMax - toMin) + toMin;
        }

        /// <summary>
        /// Calculates the linear parameter t that produces the interpolation value within the range [a, b].
        /// </summary>
        /// <param name="a"></param>
        /// <param name="b"></param>
        /// <param name="value"></param>
        public static float InverseLerp(float a, float b, float value)
        {
            if (a == b)
                return 0f;
            return Saturate((value - a) / (b - a));
        }

        /// <summary>
        /// Same as Lerp but makes sure the values interpolate correctly when they wrap around 360 degrees.
        /// </summary>
        /// <param name="a"></param>
        /// <param name="b"></param>
        /// <param name="t"></param>
        public static float LerpAngle(float a, float b, float t)
        {
            float c = Repeat(b - a, 360f);
            if (c > 180f)
                c -= 360f;
            return a + c * Saturate(t);
        }

        /// <summary>
        /// Returns the logarithm of a specified number in a specified base.
        /// </summary>
        /// <param name="f"></param>
        /// <param name="p"></param>
        public static float Log(float f, float p)
        {
            return (float)Math.Log(f, p);
        }

        /// <summary>
        /// Returns the natural (base e) logarithm of a specified number.
        /// </summary>
        /// <param name="f"></param>
        public static float Log(float f)
        {
            return (float)Math.Log(f);
        }

        /// <summary>
        /// Returns the base 10 logarithm of a specified number.
        /// </summary>
        /// <param name="f"></param>
        public static float Log10(float f)
        {
            return (float)Math.Log10(f);
        }

        /// <summary>
        /// Returns largest of two or more values.
        /// </summary>
        /// <param name="a"></param>
        /// <param name="b"></param>
        public static float Max(float a, float b)
        {
            return a <= b ? b : a;
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
        /// <param name="a"></param>
        /// <param name="b"></param>
        public static long Max(long a, long b)
        {
            return a <= b ? b : a;
        }

        /// <summary>
        /// Returns largest of two or more values.
        /// </summary>
        /// <param name="a"></param>
        /// <param name="b"></param>
        public static ulong Max(ulong a, ulong b)
        {
            return a <= b ? b : a;
        }

        /// <summary>
        /// Returns largest of two or more values.
        /// </summary>
        /// <param name="values"></param>
        public static float Max(params float[] values)
        {
            int length = values.Length;
            if (length == 0)
                return 0f;

            float t = values[0];
            for (var i = 1; i < length; i++)
                if (values[i] > t)
                    t = values[i];
            return t;
        }

        /// <summary>
        /// Returns the largest of two or more values.
        /// </summary>
        /// <param name="a"></param>
        /// <param name="b"></param>
        public static int Max(int a, int b)
        {
            return a <= b ? b : a;
        }

        /// <summary>
        /// Returns the largest of two or more values.
        /// </summary>
        /// <param name="a"></param>
        /// <param name="b"></param>
        public static uint Max(uint a, uint b)
        {
            return a <= b ? b : a;
        }

        /// <summary>
        /// Returns the largest of two or more values.
        /// </summary>
        /// <param name="values"></param>
        public static int Max(params int[] values)
        {
            int length = values.Length;
            if (length == 0)
                return 0;

            int t = values[0];
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
        public static float Min(float a, float b)
        {
            return a >= b ? b : a;
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
        /// <param name="a"></param>
        /// <param name="b"></param>
        public static long Min(long a, long b)
        {
            return a >= b ? b : a;
        }

        /// <summary>
        /// Returns the smallest of two or more values.
        /// </summary>
        /// <param name="a"></param>
        /// <param name="b"></param>
        public static ulong Min(ulong a, ulong b)
        {
            return a >= b ? b : a;
        }

        /// <summary>
        /// Returns the smallest of two or more values.
        /// </summary>
        /// <param name="values"></param>
        public static float Min(params float[] values)
        {
            int length = values.Length;
            if (length == 0)
                return 0f;

            float t = values[0];
            for (var i = 1; i < length; i++)
                if (values[i] < t)
                    t = values[i];

            return t;
        }

        /// <summary>
        /// Returns the smallest of two or more values.
        /// </summary>
        /// <param name="a"></param>
        /// <param name="b"></param>
        public static int Min(int a, int b)
        {
            return a >= b ? b : a;
        }

        /// <summary>
        /// Returns the smallest of two or more values.
        /// </summary>
        /// <param name="a"></param>
        /// <param name="b"></param>
        public static uint Min(uint a, uint b)
        {
            return a >= b ? b : a;
        }

        /// <summary>
        /// Returns the smallest of two or more values.
        /// </summary>
        /// <param name="values"></param>
        public static int Min(params int[] values)
        {
            int length = values.Length;
            if (length == 0)
                return 0;

            int num = values[0];
            for (var i = 1; i < length; i++)
                if (values[i] < num)
                    num = values[i];

            return num;
        }

        /// <summary>
        /// Moves a value current towards target.
        /// </summary>
        /// <param name="current">The current value.</param>
        /// <param name="target">The value to move towards.</param>
        /// <param name="maxDelta">The maximum change that should be applied to the value.</param>
        public static float MoveTowards(float current, float target, float maxDelta)
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
        public static float MoveTowardsAngle(float current, float target, float maxDelta)
        {
            float delta = DeltaAngle(current, target);
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
        public static float PingPong(float t, float length)
        {
            t = Repeat(t, length * 2f);
            return length - Abs(t - length);
        }

        /// <summary>
        /// Returns f raised to power p.
        /// </summary>
        /// <param name="f"></param>
        /// <param name="p"></param>
        public static float Pow(float f, float p)
        {
            return (float)Math.Pow(f, p);
        }

        /// <summary>
        /// Loops the value t, so that it is never larger than length and never smaller than 0.
        /// </summary>
        /// <param name="t"></param>
        /// <param name="length"></param>
        public static float Repeat(float t, float length)
        {
            return t - Floor(t / length) * length;
        }

        /// <summary>
        /// Returns f rounded to the nearest integer.
        /// </summary>
        /// <param name="f"></param>
        public static float Round(float f)
        {
            return (float)Math.Round(f);
        }

        /// <summary>
        /// Returns f rounded to the nearest integer.
        /// </summary>
        /// <param name="f"></param>
        public static int RoundToInt(float f)
        {
            return (int)Math.Round(f);
        }

        /// <summary>
        /// Returns the sign of f.
        /// </summary>
        /// <param name="f"></param>
        public static float Sign(float f)
        {
            return f > 0.0f ? 1.0f : f < 0.0f ? -1.0f : 0.0f;
        }

        /// <summary>
        /// Returns the sine of angle f in radians.
        /// </summary>
        /// <param name="f"></param>
        public static float Sin(float f)
        {
            return (float)Math.Sin(f);
        }

        /// <summary>
        /// Returns signed fractional part of a float.
        /// </summary>
        /// <param name="value">Floating point value to convert.</param>
        /// <returns>A float between [0 ; 1) for nonnegative input. A float between [-1; 0) for negative input.</returns>
        public static float Frac(float value)
        {
            return value - (int)value;
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
        public static float SmoothDamp(float current, float target, ref float currentVelocity, float smoothTime, float maxSpeed)
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
        public static float SmoothDamp(float current, float target, ref float currentVelocity, float smoothTime)
        {
            return SmoothDamp(current, target, ref currentVelocity, smoothTime, float.PositiveInfinity, Time.DeltaTime);
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
        public static float SmoothDamp(float current, float target, ref float currentVelocity, float smoothTime, [DefaultValue("float.PositiveInfinity")] float maxSpeed, [DefaultValue("Time.DeltaTime")] float deltaTime)
        {
            smoothTime = Max(0.0001f, smoothTime);
            float a = 2f / smoothTime;
            float b = a * deltaTime;
            float c = 1f / (1f + b + 0.48f * b * b + 0.235f * b * b * b);
            float d = current - target;
            float e = target;
            float f = maxSpeed * smoothTime;
            d = Clamp(d, -f, f);
            target = current - d;
            float g = (currentVelocity + a * d) * deltaTime;
            currentVelocity = (currentVelocity - a * g) * c;
            float h = target + (d + g) * c;
            if (e - current > 0f == h > e)
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
        public static float SmoothDampAngle(float current, float target, ref float currentVelocity, float smoothTime, float maxSpeed)
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
        public static float SmoothDampAngle(float current, float target, ref float currentVelocity, float smoothTime)
        {
            return SmoothDampAngle(current, target, ref currentVelocity, smoothTime, float.PositiveInfinity, Time.DeltaTime);
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
        public static float SmoothDampAngle(float current, float target, ref float currentVelocity, float smoothTime, [DefaultValue("float.PositiveInfinity")] float maxSpeed, [DefaultValue("Time.DeltaTime")] float deltaTime)
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
        public static float SmoothStep(float from, float to, float t)
        {
            t = Saturate(t);
            t = -2f * t * t * t + 3f * t * t;
            return to * t + from * (1f - t);
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
        public static float CubicInterp(float p0, float t0, float p1, float t1, float alpha)
        {
            float alpha2 = alpha * alpha;
            float alpha3 = alpha2 * alpha;
            return (((2 * alpha3) - (3 * alpha2) + 1) * p0) + ((alpha3 - (2 * alpha2) + alpha) * t0) + ((alpha3 - alpha2) * t1) + (((-2 * alpha3) + (3 * alpha2)) * p1);
        }

        /// <summary>
        /// Interpolate between A and B, applying an ease in function. Exponent controls the degree of the curve.
        /// </summary>
        public static float InterpEaseIn(float a, float b, float alpha, float exponent)
        {
            float modifiedAlpha = Pow(alpha, exponent);
            return Lerp(a, b, modifiedAlpha);
        }

        /// <summary>
        /// Interpolate between A and B, applying an ease out function. Exponent controls the degree of the curve.
        /// </summary>
        public static float InterpEaseOut(float a, float b, float alpha, float exponent)
        {
            float modifiedAlpha = 1.0f - Pow(1.0f - alpha, exponent);
            return Lerp(a, b, modifiedAlpha);
        }

        /// <summary>
        /// Interpolate between A and B, applying an ease in/out function. Exponent controls the degree of the curve.
        /// </summary>
        public static float InterpEaseInOut(float a, float b, float alpha, float exponent)
        {
            return Lerp(a, b, (alpha < 0.5f) ? InterpEaseIn(0.0f, 1.0f, alpha * 2.0f, exponent) * 0.5f : InterpEaseOut(0.0f, 1.0f, alpha * 2.0f - 1.0f, exponent) * 0.5f + 0.5f);
        }

        /// <summary>
        /// Interpolation between A and B, applying a sinusoidal in function.
        /// </summary>
        public static float InterpSinIn(float a, float b, float alpha)
        {
            float modifiedAlpha = -1.0f * Cos(alpha * PiOverTwo) + 1.0f;
            return Lerp(a, b, modifiedAlpha);
        }

        /// <summary>
        /// Interpolation between A and B, applying a sinusoidal out function.
        /// </summary>
        public static float InterpSinOut(float a, float b, float alpha)
        {
            float modifiedAlpha = Sin(alpha * PiOverTwo);
            return Lerp(a, b, modifiedAlpha);
        }

        /// <summary>
        /// Interpolation between A and B, applying a sinusoidal in/out function.
        /// </summary>
        public static float InterpSinInOut(float a, float b, float alpha)
        {
            return Lerp(a, b, (alpha < 0.5f) ? InterpSinIn(0.0f, 1.0f, alpha * 2.0f) * 0.5f : InterpSinOut(0.0f, 1.0f, alpha * 2.0f - 1.0f) * 0.5f + 0.5f);
        }

        /// <summary>
        /// Interpolation between A and B, applying an exponential in function.
        /// </summary>
        public static float InterpExpoIn(float a, float b, float alpha)
        {
            float modifiedAlpha = (alpha == 0.0f) ? 0.0f : Pow(2.0f, 10.0f * (alpha - 1.0f));
            return Lerp(a, b, modifiedAlpha);
        }

        /// <summary>
        /// Interpolation between A and B, applying an exponential out function.
        /// </summary>
        public static float InterpExpoOut(float a, float b, float alpha)
        {
            float modifiedAlpha = (alpha == 1.0f) ? 1.0f : -Pow(2.0f, -10.0f * alpha) + 1.0f;
            return Lerp(a, b, modifiedAlpha);
        }

        /// <summary>
        /// Interpolation between A and B, applying an exponential in/out function.
        /// </summary>
        public static float InterpExpoInOut(float a, float b, float alpha)
        {
            return Lerp(a, b, (alpha < 0.5f) ? InterpExpoIn(0.0f, 1.0f, alpha * 2.0f) * 0.5f : InterpExpoOut(0.0f, 1.0f, alpha * 2.0f - 1.0f) * 0.5f + 0.5f);
        }

        /// <summary>
        /// Interpolation between A and B, applying a circular in function.
        /// </summary>
        public static float InterpCircularIn(float a, float b, float alpha)
        {
            float modifiedAlpha = -1.0f * (Sqrt(1.0f - alpha * alpha) - 1.0f);
            return Lerp(a, b, modifiedAlpha);
        }

        /// <summary>
        /// Interpolation between A and B, applying a circular out function.
        /// </summary>
        public static float InterpCircularOut(float a, float b, float alpha)
        {
            alpha -= 1.0f;
            float modifiedAlpha = Sqrt(1.0f - alpha * alpha);
            return Lerp(a, b, modifiedAlpha);
        }

        /// <summary>
        /// Interpolation between A and B, applying a circular in/out function.
        /// </summary>
        public static float InterpCircularInOut(float a, float b, float alpha)
        {
            return Lerp(a, b, (alpha < 0.5f) ? InterpCircularIn(0.0f, 1.0f, alpha * 2.0f) * 0.5f : InterpCircularOut(0.0f, 1.0f, alpha * 2.0f - 1.0f) * 0.5f + 0.5f);
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
        public static float Map(float value, float fromMin, float fromMax, float toMin, float toMax)
        {
            float t = (value - fromMin) / (fromMax - fromMin);
            return toMin + t * (toMax - toMin);
        }

        /// <summary>
        /// Determines whether the specified x is pow of 2.
        /// </summary>
        /// <param name="x">The x.</param>
        /// <returns><c>true</c> if the specified x is pow2; otherwise, <c>false</c>.</returns>
        public static bool IsPowerOfTwo(int x)
        {
            return ((x != 0) && (x & (x - 1)) == 0);
        }

        /// <summary>
        /// Get the next power of two for a size.
        /// </summary>
        /// <param name="size">The size.</param>
        /// <returns>System.Int32.</returns>
        public static int NextPowerOfTwo(int size)
        {
            return 1 << (int)Math.Ceiling(Math.Log(size, 2));
        }

        /// <summary>
        /// Get the next power of two for a size.
        /// </summary>
        /// <param name="size">The size.</param>
        /// <returns>System.Int32.</returns>
        public static float NextPowerOfTwo(float size)
        {
            return (float)Math.Pow(2, Math.Ceiling(Math.Log(size, 2)));
        }

        /// <summary>
        /// Converts a float value from sRGB to linear.
        /// </summary>
        /// <param name="sRgbValue">The sRGB value.</param>
        /// <returns>A linear value.</returns>
        public static float SRgbToLinear(float sRgbValue)
        {
            if (sRgbValue < 0.04045f)
                return sRgbValue / 12.92f;
            return (float)Math.Pow((sRgbValue + 0.055) / 1.055, 2.4);
        }

        /// <summary>
        /// Converts a float value from linear to sRGB.
        /// </summary>
        /// <param name="linearValue">The linear value.</param>
        /// <returns>The encoded sRGB value.</returns>
        public static float LinearToSRgb(float linearValue)
        {
            if (linearValue < 0.0031308f)
                return linearValue * 12.92f;
            return (float)(1.055 * Math.Pow(linearValue, 1 / 2.4) - 0.055);
        }

        /// <summary>
        /// Returns square root of f.
        /// </summary>
        /// <param name="f"></param>
        public static float Sqrt(float f)
        {
            return (float)Math.Sqrt(f);
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
        public static int Square(int f)
        {
            return f * f;
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
        /// Returns square of the given value.
        /// </summary>
        /// <param name="f">The value.</param>
        /// <returns>The value * value.</returns>
        public static float Square(float f)
        {
            return f * f;
        }

        /// <summary>
        /// Returns the tangent of angle f in radians.
        /// </summary>
        /// <param name="f"></param>
        public static float Tan(float f)
        {
            return (float)Math.Tan(f);
        }

        /// <summary>
        /// Checks if a and b are almost equals, taking into account the magnitude of floating point numbers (unlike <see cref="WithinEpsilon(float,float,float)" /> method).
        /// </summary>
        /// <param name="a">The left value to compare.</param>
        /// <param name="b">The right value to compare.</param>
        /// <returns><c>true</c> if a almost equal to b, <c>false</c> otherwise</returns>
        /// <remarks>The code is using the technique described by Bruce Dawson in <a href="http://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/">Comparing Floating point numbers 2012 edition</a>.</remarks>
        public static unsafe bool NearEqual(float a, float b)
        {
            // Check if the numbers are really close -- needed when comparing numbers near zero.
            if (Math.Abs(a - b) < Epsilon)
                return true;

            // Original from Bruce Dawson: http://randomascii.wordpress.com/2012/02/25/comparing-floating-point-numbers-2012-edition/
            int aInt = *(int*)&a;
            int bInt = *(int*)&b;

            // Different signs means they do not match.
            if (aInt < 0 != bInt < 0)
                return false;

            // Find the difference in ULPs.
            int ulp = Math.Abs(aInt - bInt);

            // Choose of maxUlp = 4
            // according to http://code.google.com/p/googletest/source/browse/trunk/include/gtest/internal/gtest-internal.h
            const int maxUlp = 4;
            return ulp <= maxUlp;
        }

        /// <summary>
        /// Checks if a and b are almost equals, taking into account the magnitude of floating point numbers .
        /// See remarks.
        /// </summary>
        /// <param name="a">The left value to compare.</param>
        /// <param name="b">The right value to compare.</param>
        /// <returns><c>true</c> if a almost equal to b, <c>false</c> otherwise</returns>
        public static bool NearEqual(double a, double b)
        {
            return Math.Abs(a - b) < Mathd.Epsilon;
        }

        /// <summary>
        /// Determines whether the specified value is close to zero (0.0f).
        /// </summary>
        /// <param name="a">The floating value.</param>
        /// <returns><c>true</c> if the specified value is close to zero (0.0f); otherwise, <c>false</c>.</returns>
        public static bool IsZero(float a)
        {
            return Math.Abs(a) < Epsilon;
        }

        /// <summary>
        /// Determines whether the specified value is close to one (1.0f).
        /// </summary>
        /// <param name="a">The floating value.</param>
        /// <returns><c>true</c> if the specified value is close to one (1.0f); otherwise, <c>false</c>.</returns>
        public static bool IsOne(float a)
        {
            return Math.Abs(a - 1.0f) < Epsilon;
        }

        /// <summary>
        /// Determines whether the specified value is close to zero (0.0f).
        /// </summary>
        /// <param name="a">The floating value.</param>
        /// <returns><c>true</c> if the specified value is close to zero (0.0f); otherwise, <c>false</c>.</returns>
        public static bool IsZero(double a)
        {
            return Math.Abs(a) < Mathd.Epsilon;
        }

        /// <summary>
        /// Determines whether the specified value is close to one (1.0f).
        /// </summary>
        /// <param name="a">The floating value.</param>
        /// <returns><c>true</c> if the specified value is close to one (1.0f); otherwise, <c>false</c>.</returns>
        public static bool IsOne(double a)
        {
            return Math.Abs(a - 1.0f) < Mathd.Epsilon;
        }

        /// <summary>
        /// Checks if a - b are almost equals within a float epsilon.
        /// </summary>
        /// <param name="a">The left value to compare.</param>
        /// <param name="b">The right value to compare.</param>
        /// <param name="epsilon">Epsilon value</param>
        /// <returns><c>true</c> if a almost equal to b within a float epsilon, <c>false</c> otherwise</returns>
        public static bool WithinEpsilon(float a, float b, float epsilon)
        {
            float num = a - b;
            return (-epsilon <= num) && (num <= epsilon);
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
        /// <returns><c>true</c> if the specified value is in a given range; otherwise, <c>false</c>.</returns>
        public static bool IsInRange(float value, float min, float max)
        {
            return value >= min && value <= max;
        }

        /// <summary>
        /// Determines whether the specified value is NOT in a given range [min; max].
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="min">The minimum.</param>
        /// <param name="max">The maximum.</param>
        /// <returns><c>true</c> if the specified value is NOT in a given range; otherwise, <c>false</c>.</returns>
        public static bool IsNotInRange(float value, float min, float max)
        {
            return value < min || value > max;
        }

        /// <summary>
        /// Determines whether the specified value is in a given range [min; max].
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="min">The minimum.</param>
        /// <param name="max">The maximum.</param>
        /// <returns><c>true</c> if the specified value is in a given range; otherwise, <c>false</c>.</returns>
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
        /// <returns><c>true</c> if the specified value is NOT in a given range; otherwise, <c>false</c>.</returns>
        public static bool IsNotInRange(double value, double min, double max)
        {
            return value < min || value > max;
        }

        /// <summary>
        /// Determines whether the specified value is in a given range [min; max].
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="min">The minimum.</param>
        /// <param name="max">The maximum.</param>
        /// <returns><c>true</c> if the specified value is in a given range; otherwise, <c>false</c>.</returns>
        public static bool IsInRange(int value, int min, int max)
        {
            return value >= min && value <= max;
        }

        /// <summary>
        /// Determines whether the specified value is NOT in a given range [min; max].
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="min">The minimum.</param>
        /// <param name="max">The maximum.</param>
        /// <returns><c>true</c> if the specified value is NOT in a given range; otherwise, <c>false</c>.</returns>
        public static bool IsNotInRange(int value, int min, int max)
        {
            return value < min || value > max;
        }

        #region Angle units conversions

        /// <summary>
        /// Converts revolutions to degrees.
        /// </summary>
        public static float RevolutionsToDegrees = 360.0f;

        /// <summary>
        /// Converts revolutions to radians.
        /// </summary>
        public static float RevolutionsToRadians = TwoPi;

        /// <summary>
        /// Converts revolutions to gradians.
        /// </summary>
        public static float RevolutionsToGradians = 400.0f;

        /// <summary>
        /// Converts degrees to revolutions.
        /// </summary>
        public static float DegreesToRevolutions = (1.0f / 360.0f);

        /// <summary>
        /// Converts degrees to radians.
        /// </summary>
        public static float DegreesToRadians = (Pi / 180.0f);

        /// <summary>
        /// Converts radians to revolutions.
        /// </summary>
        public static float RadiansToRevolutions = (1.0f / TwoPi);

        /// <summary>
        /// Converts radians to gradians.
        /// </summary>
        public static float RadiansToGradians = (200.0f / Pi);

        /// <summary>
        /// Converts gradians to revolutions.
        /// </summary>
        public static float GradiansToRevolutions = (1.0f / 400.0f);

        /// <summary>
        /// Converts gradians to degrees.
        /// </summary>
        public static float GradiansToDegrees = (9.0f / 10.0f);

        /// <summary>
        /// Converts gradians to radians.
        /// </summary>
        public static float GradiansToRadians = (Pi / 200.0f);

        /// <summary>
        /// Converts radians to degrees.
        /// </summary>
        public static float RadiansToDegrees = (180.0f / Pi);

        #endregion

        /// <summary>
        /// Given a heading which may be outside the +/- PI range, 'unwind' it back into that range.
        /// </summary>
        /// <remarks>Optimized version of <see cref="UnwindRadiansAccurate"/> that is it faster and has fixed cost but with large angle values (100 for example) starts to lose accuracy floating point problem.</remarks>
        /// <param name="angle">Angle in radians to unwind.</param>
        /// <returns>Valid angle in radians.</returns>
        public static float UnwindRadians(float angle)
        {
            var a = angle - (float)Math.Floor(angle / TwoPi) * TwoPi; // Loop function between 0 and TwoPi
            return a > Pi ? a - TwoPi : a; // Change range so it become Pi and -Pi
        }

        /// <summary>
        /// The same as <see cref="UnwindRadians"/> but is more computation intensive with large <see href="angle"/> and has better accuracy with large <see href="angle"/>.
        /// <br>cost of this function is <see href="angle"/> % <see cref="Pi"/></br>
        /// </summary>
        /// <param name="angle">Angle in radians to unwind.</param>
        /// <returns>Valid angle in radians.</returns>
        public static float UnwindRadiansAccurate(float angle)
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
        public static float UnwindDegrees(float angle)
        {
            var a = angle - (float)Math.Floor(angle / 360.0f) * 360.0f; // Loop function between 0 and 360
            return a > 180 ? a - 360.0f : a; // Change range so it become 180 and -180
        }

        /// <summary>
        /// The same as <see cref="UnwindDegrees"/> but is more computation intensive with large <see href="angle"/> and has better accuracy with large <see href="angle"/>.
        /// <br>cost of this function is <see href="angle"/> % 180.0f</br>
        /// </summary>
        /// <param name="angle">Angle in radians to unwind.</param>
        /// <returns>Valid angle in radians.</returns>
        public static float UnwindDegreesAccurate(float angle)
        {
            while (angle > 180.0f)
                angle -= 360.0f;
            while (angle < -180.0f)
                angle += 360.0f;
            return angle;
        }

        /// <summary>
        /// Clamps the specified value.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="min">The min.</param>
        /// <param name="max">The max.</param>
        /// <returns>The result of clamping a value between min and max</returns>
        public static long Clamp(long value, long min, long max)
        {
            return value < min ? min : value > max ? max : value;
        }

        /// <summary>
        /// Clamps the specified value.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="min">The min.</param>
        /// <param name="max">The max.</param>
        /// <returns>The result of clamping a value between min and max</returns>
        public static ulong Clamp(ulong value, ulong min, ulong max)
        {
            return value < min ? min : value > max ? max : value;
        }

        /// <summary>
        /// Clamps the specified value.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="min">The min.</param>
        /// <param name="max">The max.</param>
        /// <returns>The result of clamping a value between min and max</returns>
        public static float Clamp(float value, float min, float max)
        {
            return value < min ? min : value > max ? max : value;
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
        /// Clamps the specified value.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="min">The min.</param>
        /// <param name="max">The max.</param>
        /// <returns>The result of clamping a value between min and max</returns>
        public static int Clamp(int value, int min, int max)
        {
            return value < min ? min : value > max ? max : value;
        }

        /// <summary>
        /// Clamps the specified value.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="min">The min.</param>
        /// <param name="max">The max.</param>
        /// <returns>The result of clamping a value between min and max</returns>
        public static uint Clamp(uint value, uint min, uint max)
        {
            return value < min ? min : value > max ? max : value;
        }

        /// <summary>
        /// Interpolates between two values using a linear function by a given amount.
        /// </summary>
        /// <remarks>See http://www.encyclopediaofmath.org/index.php/Linear_interpolation and http://fgiesen.wordpress.com/2012/08/15/linear-interpolation-past-present-and-future/</remarks>
        /// <param name="from">Value to interpolate from.</param>
        /// <param name="to">Value to interpolate to.</param>
        /// <param name="amount">Interpolation amount.</param>
        /// <returns>The result of linear interpolation of values based on the amount.</returns>
        public static double Lerp(double from, double to, double amount)
        {
            return from + (to - from) * amount;
        }

        /// <summary>
        /// Interpolates between two values using a linear function by a given amount.
        /// </summary>
        /// <remarks>
        /// See:
        /// <br><seealso href="http://www.encyclopediaofmath.org/index.php/Linear_interpolation"/></br>
        /// <br><seealso href="http://fgiesen.wordpress.com/2012/08/15/linear-interpolation-past-present-and-future/"/></br>
        /// </remarks>
        /// /// <param name="from">Value to interpolate from.</param>
        /// <param name="to">Value to interpolate to.</param>
        /// <param name="amount">Interpolation amount.</param>
        /// <returns>The result of linear interpolation of values based on the amount.</returns>
        public static float Lerp(float from, float to, float amount)
        {
            return from + (to - from) * amount;
        }

        /// <summary>
        /// Interpolates between two values using a linear function by a given amount.
        /// </summary>
        /// <remarks>
        /// See:
        /// <br><seealso href="http://www.encyclopediaofmath.org/index.php/Linear_interpolation"/></br>
        /// <br><seealso href="http://fgiesen.wordpress.com/2012/08/15/linear-interpolation-past-present-and-future/"/></br>
        /// </remarks>       
        /// /// <param name="from">Value to interpolate from.</param>
        /// <param name="to">Value to interpolate to.</param>
        /// <param name="amount">Interpolation amount.</param>
        /// <returns>The result of linear interpolation of values based on the amount.</returns>
        public static int Lerp(int from, int to, float amount)
        {
            return (int)(from + (to - from) * amount);
        }

        /// <summary>
        /// Interpolates between two values using a linear function by a given amount.
        /// </summary>
        /// <remarks>
        /// See:
        /// <br><seealso href="http://www.encyclopediaofmath.org/index.php/Linear_interpolation"/></br>
        /// <br><seealso href="http://fgiesen.wordpress.com/2012/08/15/linear-interpolation-past-present-and-future/"/></br>
        /// </remarks>
        /// /// <param name="from">Value to interpolate from.</param>
        /// <param name="to">Value to interpolate to.</param>
        /// <param name="amount">Interpolation amount.</param>
        /// <returns>The result of linear interpolation of values based on the amount.</returns>
        public static byte Lerp(byte from, byte to, float amount)
        {
            return (byte)(from + (to - from) * amount);
        }

        /// <summary>
        /// Performs smooth (cubic Hermite) interpolation between 0 and 1.
        /// </summary>
        /// <remarks>
        /// See: 
        /// <br><seealso href="https://en.wikipedia.org/wiki/Smoothstep"/></br>
        /// </remarks>
        /// <param name="amount">Value between 0 and 1 indicating interpolation amount.</param>
        public static float SmoothStep(float amount)
        {
            return amount <= 0 ? 0 : amount >= 1 ? 1 : amount * amount * (3 - 2 * amount);
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
            return amount <= 0 ? 0 : amount >= 1 ? 1 : amount * amount * (3 - 2 * amount);
        }

        /// <summary>
        /// Performs a smooth(er) interpolation between 0 and 1 with 1st and 2nd order derivatives of zero at endpoints.
        /// </summary>
        /// <remarks>
        /// See: 
        /// <br><seealso href="https://en.wikipedia.org/wiki/Smoothstep"/></br>
        /// </remarks>
        /// <param name="amount">Value between 0 and 1 indicating interpolation amount.</param>
        public static float SmootherStep(float amount)
        {
            return amount <= 0 ? 0 : amount >= 1 ? 1 : amount * amount * amount * (amount * (amount * 6 - 15) + 10);
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
            return amount <= 0 ? 0 : amount >= 1 ? 1 : amount * amount * amount * (amount * (amount * 6 - 15) + 10);
        }

        /// <summary>
        /// Calculates the modulo of the specified value.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="modulo">The modulo.</param>
        /// <returns>The result of the modulo applied to value</returns>
        public static float Mod(float value, float modulo)
        {
            if (modulo == 0.0f)
                return value;
            return value % modulo;
        }

        /// <summary>
        /// Calculates the modulo 2*PI of the specified value.
        /// </summary>
        /// <param name="value">The value.</param>
        /// <returns>The result of the modulo applied to value</returns>
        public static float Mod2PI(float value)
        {
            return Mod(value, TwoPi);
        }

        /// <summary>
        /// Wraps the specified value into a range [min, max]
        /// </summary>
        /// <param name="value">The value to wrap.</param>
        /// <param name="min">The min.</param>
        /// <param name="max">The max.</param>
        /// <returns>Result of the wrapping.</returns>
        /// <exception cref="ArgumentException">Is thrown when <paramref name="min" /> is greater than <paramref name="max" />.</exception>
        public static int Wrap(int value, int min, int max)
        {
            if (min > max)
                throw new ArgumentException(string.Format("min {0} should be less than or equal to max {1}", min, max), nameof(min));

            // Code from http://stackoverflow.com/a/707426/1356325
            int rangeSize = max - min + 1;

            if (value < min)
                value += rangeSize * ((min - value) / rangeSize + 1);

            return min + (value - min) % rangeSize;
        }

        /// <summary>
        /// Wraps the specified value into a range [min, max]
        /// </summary>
        /// <param name="value">The value.</param>
        /// <param name="min">The min.</param>
        /// <param name="max">The max.</param>
        /// <returns>Result of the wrapping.</returns>
        /// <exception cref="ArgumentException">Is thrown when <paramref name="min" /> is greater than <paramref name="max" />.</exception>
        public static float Wrap(float value, float min, float max)
        {
            if (NearEqual(min, max))
                return min;

            double mind = min;
            double maxd = max;
            double valued = value;

            if (mind > maxd)
                throw new ArgumentException(string.Format("min {0} should be less than or equal to max {1}", min, max), nameof(min));

            double rangeSize = maxd - mind;
            return (float)(mind + (valued - mind) - rangeSize * Math.Floor((valued - mind) / rangeSize));
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
        public static float Gauss(float amplitude, float x, float y, float centerX, float centerY, float sigmaX, float sigmaY)
        {
            return (float)Gauss((double)amplitude, x, y, centerX, centerY, sigmaX, sigmaY);
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
        public static float InterpolateAlphaBlend(float alpha, AlphaBlendMode mode)
        {
            switch (mode)
            {
            case AlphaBlendMode.Sinusoidal:
                alpha = (Sin(alpha * Pi - PiOverTwo) + 1.0f) / 2.0f;
                break;
            case AlphaBlendMode.Cubic:
                alpha = CubicInterp(0.0f, 0.0f, 1.0f, 0.0f, alpha);
                break;
            case AlphaBlendMode.QuadraticInOut:
                alpha = InterpEaseInOut(0.0f, 1.0f, alpha, 2);
                break;
            case AlphaBlendMode.CubicInOut:
                alpha = InterpEaseInOut(0.0f, 1.0f, alpha, 3);
                break;
            case AlphaBlendMode.HermiteCubic:
                alpha = SmoothStep(0.0f, 1.0f, alpha);
                break;
            case AlphaBlendMode.QuarticInOut:
                alpha = InterpEaseInOut(0.0f, 1.0f, alpha, 4);
                break;
            case AlphaBlendMode.QuinticInOut:
                alpha = InterpEaseInOut(0.0f, 1.0f, alpha, 5);
                break;
            case AlphaBlendMode.CircularIn:
                alpha = InterpCircularIn(0.0f, 1.0f, alpha);
                break;
            case AlphaBlendMode.CircularOut:
                alpha = InterpCircularOut(0.0f, 1.0f, alpha);
                break;
            case AlphaBlendMode.CircularInOut:
                alpha = InterpCircularInOut(0.0f, 1.0f, alpha);
                break;
            case AlphaBlendMode.ExpIn:
                alpha = InterpExpoIn(0.0f, 1.0f, alpha);
                break;
            case AlphaBlendMode.ExpOut:
                alpha = InterpExpoOut(0.0f, 1.0f, alpha);
                break;
            case AlphaBlendMode.ExpInOut:
                alpha = InterpExpoInOut(0.0f, 1.0f, alpha);
                break;
            }

            return Saturate(alpha);
        }
    }
}
