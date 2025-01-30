// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Math/Color.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Math/Vector3.h"
#include "Engine/Core/Math/Quaternion.h"
#include "Engine/Core/Math/Transform.h"

namespace AnimationUtils
{
    template<class T>
    FORCE_INLINE static T GetZero()
    {
        return T();
    }

    template<>
    FORCE_INLINE int32 GetZero<int32>()
    {
        return 0;
    }

    template<>
    FORCE_INLINE float GetZero<float>()
    {
        return 0.0f;
    }

    template<>
    FORCE_INLINE double GetZero<double>()
    {
        return 0.0;
    }

    template<>
    FORCE_INLINE Float3 GetZero<Float3>()
    {
        return Float3::Zero;
    }

    template<>
    FORCE_INLINE Quaternion GetZero<Quaternion>()
    {
        return Quaternion::Identity;
    }

    template<>
    FORCE_INLINE Transform GetZero<Transform>()
    {
        return Transform::Identity;
    }

    template<>
    FORCE_INLINE Color GetZero<Color>()
    {
        return Color::Black;
    }

    template<>
    FORCE_INLINE Color32 GetZero<Color32>()
    {
        return Color32::Black;
    }

    template<class T>
    FORCE_INLINE static void GetTangent(const T& value, const T& tangent, float tangentScale, T& result)
    {
        result = value + tangent * tangentScale;
    }

    template<>
    FORCE_INLINE void GetTangent<Quaternion>(const Quaternion& value, const Quaternion& tangent, float tangentScale, Quaternion& result)
    {
        Quaternion::Slerp(value, tangent, 1.0f / 3.0f, result);
    }

    template<>
    FORCE_INLINE void GetTangent<Transform>(const Transform& value, const Transform& tangent, float tangentScale, Transform& result)
    {
        GetTangent(value.Translation, tangent.Translation, tangentScale, result.Translation);
        GetTangent(value.Orientation, tangent.Orientation, tangentScale, result.Orientation);
        GetTangent(value.Scale, tangent.Scale, tangentScale, result.Scale);
    }

    template<class T>
    FORCE_INLINE static void Interpolate(const T& a, const T& b, float t, T& result)
    {
        result = (T)(a + t * (b - a));
    }

    template<>
    FORCE_INLINE void Interpolate<Float2>(const Float2& a, const Float2& b, float t, Float2& result)
    {
        result.X = Math::Lerp(a.X, b.X, t);
        result.Y = Math::Lerp(a.Y, b.Y, t);
    }

    template<>
    FORCE_INLINE void Interpolate<Float3>(const Float3& a, const Float3& b, float t, Float3& result)
    {
        result.X = Math::Lerp(a.X, b.X, t);
        result.Y = Math::Lerp(a.Y, b.Y, t);
        result.Z = Math::Lerp(a.Z, b.Z, t);
    }

    template<>
    FORCE_INLINE void Interpolate<Quaternion>(const Quaternion& a, const Quaternion& b, float t, Quaternion& result)
    {
        Quaternion::Slerp(a, b, t, result);
    }

    template<>
    FORCE_INLINE void Interpolate<Transform>(const Transform& a, const Transform& b, float t, Transform& result)
    {
        Vector3::Lerp(a.Translation, b.Translation, t, result.Translation);
        Quaternion::Slerp(a.Orientation, b.Orientation, t, result.Orientation);
        Float3::Lerp(a.Scale, b.Scale, t, result.Scale);
    }

    static void WrapTime(float& time, float start, float end, bool loop)
    {
        const float length = end - start;

        if (Math::NearEqual(length, 0.0f))
        {
            time = 0.0f;
            return;
        }

        // Clamp to start or loop
        if (time < start)
        {
            if (loop)
                time = time + (Math::Floor(end - time) / length) * length;
            else
                time = start;
        }

        // Clamp to end or loop
        if (time > end)
        {
            if (loop)
                time = time - Math::Floor((time - start) / length) * length;
            else
                time = end;
        }
    }

    /// <summary>
    /// Evaluates a cubic Hermite curve at a specific point.
    /// </summary>
    /// <param name="t">The time parameter that at which to evaluate the curve, in range [0, 1].</param>
    /// <param name="p0">The starting point (at t=0).</param>
    /// <param name="p1">The ending point (at t=1).</param>
    /// <param name="t0">The starting tangent (at t=0).</param>
    /// <param name="t1">The ending tangent (at t = 1).</param>
    /// <param name="result">The evaluated value.</param>
    template<class T>
    static void CubicHermite(const T& p0, const T& p1, const T& t0, const T& t1, float t, T& result)
    {
        const float tt = t * t;
        const float ttt = tt * t;
        result = (2 * ttt - 3 * tt + 1) * p0 + (ttt - 2 * tt + t) * t0 + (-2 * ttt + 3 * tt) * p1 + (ttt - tt) * t1;
    }

    /// <summary>
    /// Evaluates the first derivative of a cubic Hermite curve at a specific point.
    /// </summary>
    /// <param name="t">The time parameter that at which to evaluate the curve, in range [0, 1].</param>
    /// <param name="p0">The starting point (at t=0).</param>
    /// <param name="p1">The ending point (at t=1).</param>
    /// <param name="t0">The starting tangent (at t=0).</param>
    /// <param name="t1">The ending tangent (at t=1).</param>
    /// <param name="result">The evaluated value.</param>
    template<class T>
    static void CubicHermiteFirstDerivative(const T& p0, const T& p1, const T& t0, const T& t1, float t, T& result)
    {
        const float tt = t * t;
        result = (6 * tt - 6 * t) * p0 + (3 * tt - 4 * t + 1) * t0 + (-6 * tt + 6 * t) * p1 + (3 * tt - 2 * t) * t1;
    }

    template<class T>
    static void Bezier(const T& p0, const T& p1, const T& p2, const T& p3, float t, T& result)
    {
        T p01, p12, p23, p012, p123;
        Interpolate(p0, p1, t, p01);
        Interpolate(p1, p2, t, p12);
        Interpolate(p2, p3, t, p23);
        Interpolate(p01, p12, t, p012);
        Interpolate(p12, p23, t, p123);
        Interpolate(p012, p123, t, result);
    }

    template<>
    void Bezier<Float2>(const Float2& p0, const Float2& p1, const Float2& p2, const Float2& p3, float t, Float2& result)
    {
        const float u = 1.0f - t;
        const float tt = t * t;
        const float uu = u * u;
        const float uuu = uu * u;
        const float ttt = tt * t;
        result = uuu * p0 + (3 * uu * t) * p1 + (3 * u * tt) * p2 + ttt * p3;
    }

    template<>
    void Bezier<Float3>(const Float3& p0, const Float3& p1, const Float3& p2, const Float3& p3, float t, Float3& result)
    {
        const float u = 1.0f - t;
        const float tt = t * t;
        const float uu = u * u;
        const float uuu = uu * u;
        const float ttt = tt * t;
        result = uuu * p0 + (3 * uu * t) * p1 + (3 * u * tt) * p2 + ttt * p3;
    }

    template<>
    void Bezier<Double3>(const Double3& p0, const Double3& p1, const Double3& p2, const Double3& p3, float t, Double3& result)
    {
        const double u = 1.0f - t;
        const double tt = t * t;
        const double uu = u * u;
        const double uuu = uu * u;
        const double ttt = tt * t;
        result = uuu * p0 + (3 * uu * t) * p1 + (3 * u * tt) * p2 + ttt * p3;
    }

    template<>
    void Bezier<Quaternion>(const Quaternion& p0, const Quaternion& p1, const Quaternion& p2, const Quaternion& p3, float t, Quaternion& result)
    {
        Quaternion p01, p12, p23, p012, p123;
        Quaternion::Slerp(p0, p1, t, p01);
        Quaternion::Slerp(p1, p2, t, p12);
        Quaternion::Slerp(p2, p3, t, p23);
        Quaternion::Slerp(p01, p12, t, p012);
        Quaternion::Slerp(p12, p23, t, p123);
        Quaternion::Slerp(p012, p123, t, result);
    }

    template<>
    void Bezier<Transform>(const Transform& p0, const Transform& p1, const Transform& p2, const Transform& p3, float t, Transform& result)
    {
        Bezier<Vector3>(p0.Translation, p1.Translation, p2.Translation, p3.Translation, t, result.Translation);
        Bezier<Quaternion>(p0.Orientation, p1.Orientation, p2.Orientation, p3.Orientation, t, result.Orientation);
        Bezier<Float3>(p0.Scale, p1.Scale, p2.Scale, p3.Scale, t, result.Scale);
    }

    template<class T, class TReal = float>
    static void BezierFirstDerivative(const T& p0, const T& p1, const T& p2, const T& p3, TReal t, T& result)
    {
        const TReal u = 1.0f - t;
        const TReal tt = t * t;
        const TReal uu = u * u;
        result = (TReal)(3.0f * uu) * (p1 - p0) + (TReal)(6.0f * u * t) * (p2 - p1) + (TReal)(3.0f * tt) * (p3 - p2);
    }

    template<>
    void BezierFirstDerivative<Quaternion>(const Quaternion& p0, const Quaternion& p1, const Quaternion& p2, const Quaternion& p3, float t, Quaternion& result)
    {
        Float3 euler;
        BezierFirstDerivative<Float3>(p0.GetEuler(), p1.GetEuler(), p2.GetEuler(), p3.GetEuler(), t, euler);
        result = Quaternion::Euler(euler);
    }

    template<>
    void BezierFirstDerivative<Transform>(const Transform& p0, const Transform& p1, const Transform& p2, const Transform& p3, float t, Transform& result)
    {
        BezierFirstDerivative<Vector3, Real>(p0.Translation, p1.Translation, p2.Translation, p3.Translation, t, result.Translation);
        BezierFirstDerivative<Quaternion>(p0.Orientation, p1.Orientation, p2.Orientation, p3.Orientation, t, result.Orientation);
        BezierFirstDerivative<Float3>(p0.Scale, p1.Scale, p2.Scale, p3.Scale, t, result.Scale);
    }
}
