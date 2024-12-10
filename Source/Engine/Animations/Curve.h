// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "AnimationUtils.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Types/Span.h"

// @formatter:off

/// <summary>
/// A single keyframe that can be injected into an animation curve (step).
/// </summary>
PACK_BEGIN()
template<typename T>
struct StepCurveKeyframe
{
public:

    /// <summary>
    /// The time of the keyframe.
    /// </summary>
    float Time;

    /// <summary>
    /// The value of the curve at keyframe.
    /// </summary>
    T Value;

public:

    StepCurveKeyframe()
    {
    }

    StepCurveKeyframe(float time, const T& value)
    {
        Time = time;
        Value = value;
    }

public:

    FORCE_INLINE static void Interpolate(const StepCurveKeyframe& a, const StepCurveKeyframe& b, float alpha, float length, T& result)
    {
        result = a.Value;
    }

    FORCE_INLINE static void InterpolateFirstDerivative(const StepCurveKeyframe& a, const StepCurveKeyframe& b, float alpha, float length, T& result)
    {
        result = AnimationUtils::GetZero<T>();
    }

    FORCE_INLINE static void InterpolateKey(const StepCurveKeyframe& a, const StepCurveKeyframe& b, float alpha, float length, StepCurveKeyframe& result)
    {
        result = a;
    }
    
    bool operator==(const StepCurveKeyframe& other) const
    {
        return Math::NearEqual(Time, other.Time) && Math::NearEqual(Value, other.Value);
    }
} PACK_END();

/// <summary>
/// A single keyframe that can be injected into an animation curve (linear).
/// </summary>
PACK_BEGIN()
template<typename T>
struct LinearCurveKeyframe
{
public:

    /// <summary>
    /// The time of the keyframe.
    /// </summary>
    float Time;

    /// <summary>
    /// The value of the curve at keyframe.
    /// </summary>
    T Value;

public:

    LinearCurveKeyframe()
    {
    }

    LinearCurveKeyframe(float time, const T& value)
    {
        Time = time;
        Value = value;
    }

public:

    FORCE_INLINE static void Interpolate(const LinearCurveKeyframe& a, const LinearCurveKeyframe& b, float alpha, float length, T& result)
    {
        AnimationUtils::Interpolate(a.Value, b.Value, alpha, result);
    }

    FORCE_INLINE static void InterpolateFirstDerivative(const LinearCurveKeyframe& a, const LinearCurveKeyframe& b, float alpha, float length, T& result)
    {
        result = b.Value - a.Value;
    }

    FORCE_INLINE static void InterpolateKey(const LinearCurveKeyframe& a, const LinearCurveKeyframe& b, float alpha, float length, LinearCurveKeyframe& result)
    {
        result.Time = a.Time + (b.Time - a.Time) * alpha;
        AnimationUtils::Interpolate(a.Value, b.Value, alpha, result.Value);
    }

    bool operator==(const LinearCurveKeyframe& other) const
    {
        return Math::NearEqual(Time, other.Time) && Math::NearEqual(Value, other.Value);
    }
} PACK_END();

/// <summary>
/// A single keyframe that can be injected into cubic hermite curve.
/// </summary>
PACK_BEGIN()
template<class T>
struct HermiteCurveKeyframe
{
public:

    /// <summary>
    /// The time of the keyframe.
    /// </summary>
    float Time;

    /// <summary>
    /// The value of the curve at keyframe.
    /// </summary>
    T Value;

    /// <summary>
    /// The input tangent (going from the previous key to this one) of the key.
    /// </summary>
    T TangentIn;

    /// <summary>
    /// The output tangent (going from this key to next one) of the key.
    /// </summary>
    T TangentOut;

public:

    HermiteCurveKeyframe()
    {
    }

    HermiteCurveKeyframe(float time, const T& value)
    {
        Time = time;
        Value = value;
        TangentIn = TangentOut = AnimationUtils::GetZero<T>();
    }

public:

    static void Interpolate(const HermiteCurveKeyframe& a, const HermiteCurveKeyframe& b, float alpha, float length, T& result)
    {
        T leftTangent = a.Value + a.TangentOut * length;
        T rightTangent = b.Value + b.TangentIn * length;
        AnimationUtils::CubicHermite( a.Value, b.Value, leftTangent, rightTangent, alpha, result);
    }

    static void InterpolateFirstDerivative(const HermiteCurveKeyframe& a, const HermiteCurveKeyframe& b, float alpha, float length, T& result)
    {
        T leftTangent = a.Value + a.TangentOut * length;
        T rightTangent = b.Value + b.TangentIn * length;
        AnimationUtils::CubicHermiteFirstDerivative( a.Value, b.Value, leftTangent, rightTangent, alpha, result);
    }

    static void InterpolateKey(const HermiteCurveKeyframe& a, const HermiteCurveKeyframe& b, float alpha, float length, HermiteCurveKeyframe& result)
    {
        result.Time = a.Time + length * alpha;
        T leftTangent = a.Value + a.TangentOut * length;
        T rightTangent = b.Value + b.TangentIn * length;
        AnimationUtils::CubicHermite(a.Value, b.Value, leftTangent, rightTangent, alpha, result.Value);
        AnimationUtils::CubicHermiteFirstDerivative(a.Value, b.Value, leftTangent, rightTangent, alpha, result.TangentIn);
        result.TangentIn /= length;
        result.TangentOut = result.TangentIn;
    }
    
    bool operator==(const HermiteCurveKeyframe& other) const
    {
        return Math::NearEqual(Time, other.Time) && Math::NearEqual(Value, other.Value) && Math::NearEqual(TangentIn, other.TangentIn) && Math::NearEqual(TangentOut, other.TangentOut);
    }
} PACK_END();

/// <summary>
/// A single keyframe that can be injected into Bezier curve.
/// </summary>
PACK_BEGIN()
template<class T>
struct BezierCurveKeyframe
{
public:

    /// <summary>
    /// The time of the keyframe.
    /// </summary>
    float Time;

    /// <summary>
    /// The value of the curve at keyframe.
    /// </summary>
    T Value;

    /// <summary>
    /// The input tangent (going from the previous key to this one) of the key.
    /// </summary>
    T TangentIn;

    /// <summary>
    /// The output tangent (going from this key to next one) of the key.
    /// </summary>
    T TangentOut;

public:

    BezierCurveKeyframe()
    {
    }

    BezierCurveKeyframe(float time, const T& value)
    {
        Time = time;
        Value = value;
        TangentIn = TangentOut = AnimationUtils::GetZero<T>();
    }

    BezierCurveKeyframe(float time, const T& value, const T& tangentIn, const T& tangentOut)
    {
        Time = time;
        Value = value;
        TangentIn = tangentIn;
        TangentOut = tangentOut;
    }

public:

    static void Interpolate(const BezierCurveKeyframe& a, const BezierCurveKeyframe& b, float alpha, float length, T& result)
    {
        T leftTangent, rightTangent;
        const float tangentScale = length / 3.0f;
        AnimationUtils::GetTangent(a.Value, a.TangentOut, tangentScale, leftTangent);
        AnimationUtils::GetTangent(b.Value, b.TangentIn, tangentScale, rightTangent);
        AnimationUtils::Bezier(a.Value, leftTangent, rightTangent, b.Value, alpha, result);
    }

    static void InterpolateFirstDerivative(const BezierCurveKeyframe& a, const BezierCurveKeyframe& b, float alpha, float length, T& result)
    {
        T leftTangent, rightTangent;
        const float tangentScale = length / 3.0f;
        AnimationUtils::GetTangent(a.Value, a.TangentOut, tangentScale, leftTangent);
        AnimationUtils::GetTangent(b.Value, b.TangentIn, tangentScale, rightTangent);
        AnimationUtils::BezierFirstDerivative(a.Value, leftTangent, rightTangent, b.Value, alpha, result);
    }

    static void InterpolateKey(const BezierCurveKeyframe& a, const BezierCurveKeyframe& b, float alpha, float length, BezierCurveKeyframe& result)
    {
        result.Time = a.Time + length * alpha;
        T leftTangent, rightTangent;
        const float tangentScale = length / 3.0f;
        AnimationUtils::GetTangent(a.Value, a.TangentOut, tangentScale, leftTangent);
        AnimationUtils::GetTangent(b.Value, b.TangentIn, tangentScale, rightTangent);
        AnimationUtils::Bezier(a.Value, leftTangent, rightTangent, b.Value, alpha, result.Value);
        result.TangentIn = a.TangentOut;
        result.TangentOut = b.TangentIn;
    }

    bool operator==(const BezierCurveKeyframe& other) const
    {
        return Math::NearEqual(Time, other.Time) && Math::NearEqual(Value, other.Value) && Math::NearEqual(TangentIn, other.TangentIn) && Math::NearEqual(TangentOut, other.TangentOut);
    }
} PACK_END();

// @formatter:on

/// <summary>
/// An animation spline represented by a set of read-only keyframes, each representing an endpoint of a curve.
/// </summary>
template<class T, typename KeyFrame = LinearCurveKeyframe<T>>
class CurveBase
{
public:
    typedef Span<KeyFrame> KeyFrameData;

protected:
    T _default;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="Curve"/> class.
    /// </summary>
    CurveBase()
        : _default(AnimationUtils::GetZero<T>())
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Curve"/> class.
    /// </summary>
    /// <param name="defaultValue">The default keyframe value.</param>
    CurveBase(const T& defaultValue)
        : _default(defaultValue)
    {
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="Curve"/> class.
    /// </summary>
    ~CurveBase()
    {
    }

public:
    /// <summary>
    /// Gets the default value for the keyframes.
    /// </summary>
    FORCE_INLINE const T& GetDefaultValue() const
    {
        return _default;
    }

    /// <summary>
    /// Gets the default value for the keyframes.
    /// </summary>
    FORCE_INLINE T GetDefaultValue()
    {
        return _default;
    }

public:
    /// <summary>
    /// Evaluates the animation curve value at the specified time.
    /// </summary>
    /// <param name="data">The keyframes data container.</param>
    /// <param name="result">The interpolated value from the curve at provided time.</param>
    /// <param name="time">The time to evaluate the curve at.</param>
    /// <param name="loop">If true the curve will loop when it goes past the end or beginning. Otherwise the curve value will be clamped.</param>
    void Evaluate(const KeyFrameData& data, T& result, float time, bool loop = true) const
    {
        const int32 count = data.Length();
        if (count == 0)
        {
            result = _default;
            return;
        }

        const float start = Math::Min(data[0].Time, 0.0f);
        const float end = data[count - 1].Time;
        AnimationUtils::WrapTime(time, start, end, loop);

        int32 leftKeyIdx;
        int32 rightKeyIdx;
        FindKeys(data, time, leftKeyIdx, rightKeyIdx);

        const KeyFrame& leftKey = data[leftKeyIdx];
        const KeyFrame& rightKey = data[rightKeyIdx];

        if (leftKeyIdx == rightKeyIdx)
        {
            result = leftKey.Value;
            return;
        }

        const float length = rightKey.Time - leftKey.Time;

        // Scale from arbitrary range to [0, 1]
        float t = Math::NearEqual(length, 0.0f) ? 0.0f : (time - leftKey.Time) / length;

        // Evaluate the value at the curve
        KeyFrame::Interpolate(leftKey, rightKey, t, length, result);
    }

    /// <summary>
    /// Evaluates the first derivative of the animation curve at the specified time (aka velocity).
    /// </summary>
    /// <param name="data">The keyframes data container.</param>
    /// <param name="result">The calculated first derivative from the curve at provided time.</param>
    /// <param name="time">The time to evaluate the curve at.</param>
    /// <param name="loop">If true the curve will loop when it goes past the end or beginning. Otherwise the curve value will be clamped.</param>
    void EvaluateFirstDerivative(const KeyFrameData& data, T& result, float time, bool loop = true) const
    {
        const int32 count = data.Length();
        if (count == 0)
        {
            result = _default;
            return;
        }

        const float start = Math::Min(data[0].Time, 0.0f);
        const float end = data[count - 1].Time;
        AnimationUtils::WrapTime(time, start, end, loop);

        int32 leftKeyIdx;
        int32 rightKeyIdx;
        FindKeys(data, time, leftKeyIdx, rightKeyIdx);

        const KeyFrame& leftKey = data[leftKeyIdx];
        const KeyFrame& rightKey = data[rightKeyIdx];

        if (leftKeyIdx == rightKeyIdx)
        {
            result = leftKey.Value;
            return;
        }

        const float length = rightKey.Time - leftKey.Time;

        // Scale from arbitrary range to [0, 1]
        float t = Math::NearEqual(length, 0.0f) ? 0.0f : (time - leftKey.Time) / length;

        // Evaluate the derivative at the curve
        KeyFrame::InterpolateFirstDerivative(leftKey, rightKey, t, length, result);
    }

    /// <summary>
    /// Evaluates the animation curve key at the specified time.
    /// </summary>
    /// <param name="data">The keyframes data container.</param>
    /// <param name="result">The interpolated key from the curve at provided time.</param>
    /// <param name="time">The time to evaluate the curve at.</param>
    /// <param name="loop">If true the curve will loop when it goes past the end or beginning. Otherwise the curve value will be clamped.</param>
    void EvaluateKey(const KeyFrameData& data, KeyFrame& result, float time, bool loop = true) const
    {
        const int32 count = data.Length();
        if (count == 0)
        {
            result = KeyFrame(time, _default);
            return;
        }

        const float start = Math::Min(data[0].Time, 0.0f);
        const float end = data[count - 1].Time;
        AnimationUtils::WrapTime(time, start, end, loop);

        int32 leftKeyIdx;
        int32 rightKeyIdx;
        FindKeys(data, time, leftKeyIdx, rightKeyIdx);

        const KeyFrame& leftKey = data[leftKeyIdx];
        const KeyFrame& rightKey = data[rightKeyIdx];

        if (leftKeyIdx == rightKeyIdx)
        {
            result = leftKey;
            return;
        }

        const float length = rightKey.Time - leftKey.Time;

        // Scale from arbitrary range to [0, 1]
        float t = Math::NearEqual(length, 0.0f) ? 0.0f : (time - leftKey.Time) / length;

        // Evaluate the key at the curve
        KeyFrame::InterpolateKey(leftKey, rightKey, t, length, result);
    }

protected:
    /// <summary>
    /// Returns a pair of keys that can be used for interpolating to field the value at the provided time.
    /// </summary>
    /// <param name="data">The keyframes data container.</param>
    /// <param name="time">The time for which to find the relevant keys from. It is expected to be clamped to a valid range within the curve.</param>
    /// <param name="leftKey">The index of the key to interpolate from.</param>
    /// <param name="rightKey">The index of the key to interpolate to.</param>
    void FindKeys(const KeyFrameData& data, float time, int32& leftKey, int32& rightKey) const
    {
        int32 start = 0;
        int32 searchLength = data.Length();

        while (searchLength > 0)
        {
            const int32 half = searchLength >> 1;
            int32 mid = start + half;

            if (time < data[mid].Time)
            {
                searchLength = half;
            }
            else
            {
                start = mid + 1;
                searchLength -= half + 1;
            }
        }

        leftKey = Math::Max(0, start - 1);
        rightKey = Math::Min(start, data.Length() - 1);
    }
};

/// <summary>
/// An animation spline represented by a set of keyframes, each representing an endpoint of a curve.
/// </summary>
template<class T, typename KeyFrame = LinearCurveKeyframe<T>>
API_CLASS(InBuild, Template, MarshalAs=Span<byte>) class Curve : public CurveBase<T, KeyFrame>
{
public:
    typedef CurveBase<T, KeyFrame> Base;
    using KeyFrameCollection = Array<KeyFrame>;

private:
    KeyFrameCollection _keyframes;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="Curve"/> class.
    /// </summary>
    Curve()
        : Base()
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Curve"/> class.
    /// </summary>
    /// <param name="defaultValue">The default keyframe value.</param>
    Curve(const T& defaultValue)
        : Base(defaultValue)
    {
    }

    /// <summary>
    /// Initializes a new instance of the <see cref="Curve"/> class.
    /// </summary>
    /// <param name="keyframes">The keyframes to initialize the curve with.</param>
    Curve(const KeyFrameCollection& keyframes)
        : Base()
    {
        SetKeyframes(keyframes);
    }

    /// <summary>
    /// Finalizes an instance of the <see cref="Curve"/> class.
    /// </summary>
    ~Curve()
    {
    }

public:
    /// <summary>
    /// Gets the length of the animation curve, from time zero to last keyframe.
    /// </summary>
    float GetLength() const
    {
        return _keyframes.HasItems() ? _keyframes.Last().Time : 0.0f;
    }

    /// <summary>
    /// Gets the keyframes collection (for read-only).
    /// </summary>
    FORCE_INLINE const KeyFrameCollection& GetKeyframes() const
    {
        return _keyframes;
    }

    /// <summary>
    /// Gets the keyframes collection.
    /// </summary>
    FORCE_INLINE KeyFrameCollection& GetKeyframes()
    {
        return _keyframes;
    }

    /// <summary>
    /// Determines whether this curve is empty (has no keyframes).
    /// </summary>
    FORCE_INLINE bool IsEmpty() const
    {
        return _keyframes.IsEmpty();
    }

    /// <summary>
    /// Clears this keyframes collection.
    /// </summary>
    FORCE_INLINE void Clear()
    {
        _keyframes.Resize(0);
    }

    /// <summary>
    /// Resizes the keyframes collection specified amount. Drops the data.
    /// </summary>
    /// <param name="count">The count.</param>
    /// <returns>The keyframes memory pointer.</returns>
    KeyFrame* Resize(int32 count)
    {
        _keyframes.Resize(count, false);
        return _keyframes.Get();
    }

    /// <summary>
    /// Sets the keyframes collection.
    /// </summary>
    /// <param name="keyframes">The keyframes collection to assign.</param>
    void SetKeyframes(const KeyFrameCollection& keyframes)
    {
        _keyframes = keyframes;

#if BUILD_DEBUG
        // Ensure that keyframes are sorted
        if (keyframes.HasItems())
        {
            float time = keyframes[0].Time;
            for (int32 i = 1; i < keyframes.Count(); i++)
            {
                ASSERT(keyframes[i].Time >= time);
                time = keyframes[i].Time;
            }
        }
#endif
    }

public:
    /// <summary>
    /// Evaluates the animation curve value at the specified time.
    /// </summary>
    /// <param name="result">The interpolated value from the curve at provided time.</param>
    /// <param name="time">The time to evaluate the curve at.</param>
    /// <param name="loop">If true the curve will loop when it goes past the end or beginning. Otherwise the curve value will be clamped.</param>
    void Evaluate(T& result, float time, bool loop = true) const
    {
        typename Base::KeyFrameData data(_keyframes.Get(), _keyframes.Count());
        Base::Evaluate(data, result, time, loop);
    }

    /// <summary>
    /// Evaluates the first derivative of the animation curve at the specified time (aka velocity).
    /// </summary>
    /// <param name="result">The calculated first derivative from the curve at provided time.</param>
    /// <param name="time">The time to evaluate the curve at.</param>
    /// <param name="loop">If true the curve will loop when it goes past the end or beginning. Otherwise the curve value will be clamped.</param>
    void EvaluateFirstDerivative(T& result, float time, bool loop = true) const
    {
        typename Base::KeyFrameData data(_keyframes.Get(), _keyframes.Count());
        Base::EvaluateFirstDerivative(data, result, time, loop);
    }

    /// <summary>
    /// Evaluates the animation curve key at the specified time.
    /// </summary>
    /// <param name="result">The interpolated key from the curve at provided time.</param>
    /// <param name="time">The time to evaluate the curve at.</param>
    /// <param name="loop">If true the curve will loop when it goes past the end or beginning. Otherwise the curve value will be clamped.</param>
    void EvaluateKey(KeyFrame& result, float time, bool loop = true) const
    {
        typename Base::KeyFrameData data(_keyframes.Get(), _keyframes.Count());
        Base::EvaluateKey(data, result, time, loop);
    }

    /// <summary>
    /// Trims the curve keyframes to the specified time range.
    /// </summary>
    /// <param name="start">The time start.</param>
    /// <param name="end">The time end.</param>
    void Trim(float start, float end)
    {
        // Early out
        if (_keyframes.IsEmpty() || (_keyframes.First().Time >= start && _keyframes.Last().Time <= end))
            return;
        if (end - start <= ZeroTolerance)
        {
            // Erase the curve
            _keyframes.Clear();
            return;
        }

        typename Base::KeyFrameData data(_keyframes.Get(), _keyframes.Count());
        KeyFrame startValue, endValue;
        Base::EvaluateKey(data, startValue, start, false);
        Base::EvaluateKey(data, endValue, end, false);

        // Begin
        for (int32 i = 0; i < _keyframes.Count() && _keyframes.HasItems(); i++)
        {
            if (_keyframes[i].Time < start)
            {
                _keyframes.RemoveAtKeepOrder(i);
                i--;
            }
            else
            {
                break;
            }
        }
        if (_keyframes.IsEmpty() || Math::NotNearEqual(_keyframes.First().Time, start))
        {
            KeyFrame key = startValue;
            key.Time = start;
            _keyframes.Insert(0, key);
        }

        // End
        for (int32 i = _keyframes.Count() - 1; i >= 0 && _keyframes.HasItems(); i--)
        {
            if (_keyframes[i].Time > end)
            {
                _keyframes.RemoveAtKeepOrder(i);
            }
            else
            {
                break;
            }
        }
        if (_keyframes.IsEmpty() || Math::NotNearEqual(_keyframes.Last().Time, end))
        {
            KeyFrame key = endValue;
            key.Time = end;
            _keyframes.Add(key);
        }

        // Rebase the keyframes time
        if (!Math::IsZero(start))
        {
            for (int32 i = 0; i < _keyframes.Count(); i++)
                _keyframes[i].Time -= start;
        }
    }

    /// <summary>
    /// Applies the linear transformation (scale and offset) to the keyframes time values.
    /// </summary>
    /// <param name="timeScale">The time scale.</param>
    /// <param name="timeOffset">The time offset.</param>
    void TransformTime(float timeScale, float timeOffset)
    {
        for (int32 i = 0; i < _keyframes.Count(); i++)
            _keyframes[i].Time = _keyframes[i].Time * timeScale + timeOffset;
    }

    uint64 GetMemoryUsage() const
    {
        return _keyframes.Capacity() * sizeof(KeyFrame);
    }

public:
    FORCE_INLINE KeyFrame& operator[](int32 index)
    {
        return _keyframes[index];
    }

    FORCE_INLINE const KeyFrame& operator[](int32 index) const
    {
        return _keyframes[index];
    }

    bool operator==(const Curve& other) const
    {
        if (_keyframes.Count() != other._keyframes.Count())
            return false;
        for (int32 i = 0; i < _keyframes.Count(); i++)
        {
            if (!(_keyframes[i] == other._keyframes[i]))
                return false;
        }
        return true;
    }

    // Raw memory copy (used by scripting bindings - see MarshalAs tag).
    Curve& operator=(const Span<byte>& raw)
    {
        ASSERT((raw.Length() % sizeof(KeyFrame)) == 0);
        const int32 count = raw.Length() / sizeof(KeyFrame);
        _keyframes.Resize(count, false);
        Platform::MemoryCopy(_keyframes.Get(), raw.Get(), sizeof(KeyFrame) * count);
        return *this;
    }
    operator Span<byte>()
    {
        return Span<byte>((const byte*)_keyframes.Get(), _keyframes.Count() * sizeof(KeyFrame));
    }
};

/// <summary>
/// An animation spline represented by a set of keyframes, each representing a value point. 
/// </summary>
template<typename T>
API_TYPEDEF() using StepCurve = Curve<T, StepCurveKeyframe<T>>;

/// <summary>
/// An animation spline represented by a set of keyframes, each representing an endpoint of a linear curve. 
/// </summary>
template<typename T>
API_TYPEDEF() using LinearCurve = Curve<T, LinearCurveKeyframe<T>>;

/// <summary>
/// An animation spline represented by a set of keyframes, each representing an endpoint of a cubic hermite curve.
/// </summary>
template<typename T>
API_TYPEDEF() using HermiteCurve = Curve<T, HermiteCurveKeyframe<T>>;

/// <summary>
/// An animation spline represented by a set of keyframes, each representing an endpoint of Bezier curve.
/// </summary>
template<typename T>
API_TYPEDEF() using BezierCurve = Curve<T, BezierCurveKeyframe<T>>;
