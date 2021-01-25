// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
        return 0.0f;
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
    FORCE_INLINE Vector3 GetZero<Vector3>()
    {
        return Vector3::Zero;
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
    FORCE_INLINE static void GetTangent(const T& a, const T& b, float length, T& result)
    {
        const float oneThird = 1.0f / 3.0f;
        result = a + b * (length * oneThird);
    }

    template<>
    FORCE_INLINE void GetTangent<Quaternion>(const Quaternion& a, const Quaternion& b, float length, Quaternion& result)
    {
        const float oneThird = 1.0f / 3.0f;
        Quaternion::Slerp(a, b, oneThird, result);
    }

    template<>
    FORCE_INLINE void GetTangent<Transform>(const Transform& a, const Transform& b, float length, Transform& result)
    {
        const float oneThird = 1.0f / 3.0f;
        const float oneThirdLength = length * oneThird;
        result.Translation = a.Translation + b.Translation * oneThirdLength;
        Quaternion::Slerp(a.Orientation, b.Orientation, oneThird, result.Orientation);
        result.Scale = a.Scale + b.Scale * oneThirdLength;
    }

    template<class T>
    FORCE_INLINE static void Interpolate(const T& a, const T& b, float alpha, T& result)
    {
        result = (T)(a + alpha * (b - a));
    }

    template<>
    FORCE_INLINE void Interpolate<Vector3>(const Vector3& a, const Vector3& b, float alpha, Vector3& result)
    {
        Vector3::Lerp(a, b, alpha, result);
    }

    template<>
    FORCE_INLINE void Interpolate<Quaternion>(const Quaternion& a, const Quaternion& b, float alpha, Quaternion& result)
    {
        Quaternion::Slerp(a, b, alpha, result);
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
    /// <param name="pointA">The starting point (at t=0).</param>
    /// <param name="pointB">The ending point (at t=1).</param>
    /// <param name="tangentA">The starting tangent (at t=0).</param>
    /// <param name="tangentB">The ending tangent (at t = 1).</param>
    /// <param name="result">The evaluated value.</param>
    template<class T>
    static void CubicHermite(const float t, const T& pointA, const T& pointB, const T& tangentA, const T& tangentB, T* result)
    {
        const float t2 = t * t;
        const float t3 = t2 * t;

        float a = 2 * t3 - 3 * t2 + 1;
        float b = t3 - 2 * t2 + t;
        float c = -2 * t3 + 3 * t2;
        float d = t3 - t2;

        *result = a * pointA + b * tangentA + c * pointB + d * tangentB;
    }

    /// <summary>
    /// Evaluates the first derivative of a cubic Hermite curve at a specific point.
    /// </summary>
    /// <param name="t">The time parameter that at which to evaluate the curve, in range [0, 1].</param>
    /// <param name="pointA">The starting point (at t=0).</param>
    /// <param name="pointB">The ending point (at t=1).</param>
    /// <param name="tangentA">The starting tangent (at t=0).</param>
    /// <param name="tangentB">The ending tangent (at t = 1).</param>
    /// <param name="result">The evaluated value.</param>
    template<class T>
    static void CubicHermiteD1(const float t, const T& pointA, const T& pointB, const T& tangentA, const T& tangentB, T* result)
    {
        const float t2 = t * t;

        float a = 6 * t2 - 6 * t;
        float b = 3 * t2 - 4 * t + 1;
        float c = -6 * t2 + 6 * t;
        float d = 3 * t2 - 2 * t;

        *result = a * pointA + b * tangentA + c * pointB + d * tangentB;
    }

    template<class T>
    static void Bezier(const T& p0, const T& p1, const T& p2, const T& p3, float alpha, T& result)
    {
        T p01, p12, p23, p012, p123;
        Interpolate(p0, p1, alpha, p01);
        Interpolate(p1, p2, alpha, p12);
        Interpolate(p2, p3, alpha, p23);
        Interpolate(p01, p12, alpha, p012);
        Interpolate(p12, p23, alpha, p123);
        Interpolate(p012, p123, alpha, result);
    }

    template<>
    void Bezier<Vector2>(const Vector2& p0, const Vector2& p1, const Vector2& p2, const Vector2& p3, float alpha, Vector2& result)
    {
        const float u = 1.0f - alpha;
        const float tt = alpha * alpha;
        const float uu = u * u;
        const float uuu = uu * u;
        const float ttt = tt * alpha;
        result = uuu * p0 + 3 * uu * alpha * p1 + 3 * u * tt * p2 + ttt * p3;
    }

    template<>
    void Bezier<Vector3>(const Vector3& p0, const Vector3& p1, const Vector3& p2, const Vector3& p3, float alpha, Vector3& result)
    {
        const float u = 1.0f - alpha;
        const float tt = alpha * alpha;
        const float uu = u * u;
        const float uuu = uu * u;
        const float ttt = tt * alpha;
        result = uuu * p0 + 3 * uu * alpha * p1 + 3 * u * tt * p2 + ttt * p3;
    }

    template<>
    void Bezier<Quaternion>(const Quaternion& p0, const Quaternion& p1, const Quaternion& p2, const Quaternion& p3, float alpha, Quaternion& result)
    {
        Quaternion p01, p12, p23, p012, p123;
        Quaternion::Slerp(p0, p1, alpha, p01);
        Quaternion::Slerp(p1, p2, alpha, p12);
        Quaternion::Slerp(p2, p3, alpha, p23);
        Quaternion::Slerp(p01, p12, alpha, p012);
        Quaternion::Slerp(p12, p23, alpha, p123);
        Quaternion::Slerp(p012, p123, alpha, result);
    }

    template<>
    void Bezier<Transform>(const Transform& p0, const Transform& p1, const Transform& p2, const Transform& p3, float alpha, Transform& result)
    {
        Bezier<Vector3>(p0.Translation, p1.Translation, p2.Translation, p3.Translation, alpha, result.Translation);
        Bezier<Quaternion>(p0.Orientation, p1.Orientation, p2.Orientation, p3.Orientation, alpha, result.Orientation);
        Bezier<Vector3>(p0.Scale, p1.Scale, p2.Scale, p3.Scale, alpha, result.Scale);
    }
}
