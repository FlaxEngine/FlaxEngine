// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using FlaxEngine.Interop;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace FlaxEngine
{
    /// <summary>
    /// An animation spline represented by a set of keyframes, each representing an endpoint of an curve.
    /// </summary>
    /// <typeparam name="T">The animated value type.</typeparam>
    public abstract class CurveBase<T> where T : new()
    {
        /// <summary>
        /// The keyframes data access interface.
        /// </summary>
        /// <typeparam name="U">The type of the keyframe data.</typeparam>
        protected interface IKeyframeAccess<U> where U : new()
        {
            /// <summary>
            /// Gets the Bezier curve tangent.
            /// </summary>
            /// <param name="value">The value.</param>
            /// <param name="tangent">The tangent.</param>
            /// <param name="tangentScale">The tangent scale factor.</param>
            /// <param name="result">The result.</param>
            void GetTangent(ref U value, ref U tangent, float tangentScale, out U result);

            /// <summary>
            /// Calculates the linear interpolation at the specified alpha.
            /// </summary>
            /// <param name="a">The start value (alpha=0).</param>
            /// <param name="b">The end value (alpha=1).</param>
            /// <param name="alpha">The alpha.</param>
            /// <param name="result">The result.</param>
            void Linear(ref U a, ref U b, float alpha, out U result);

            /// <summary>
            /// Calculates the Bezier curve value at the specified alpha.
            /// </summary>
            /// <param name="p0">The p0.</param>
            /// <param name="p1">The p1.</param>
            /// <param name="p2">The p2.</param>
            /// <param name="p3">The p3.</param>
            /// <param name="alpha">The alpha.</param>
            /// <param name="result">The result.</param>
            void Bezier(ref U p0, ref U p1, ref U p2, ref U p3, float alpha, out U result);
        }

        private class KeyframeAccess :
        IKeyframeAccess<bool>,
        IKeyframeAccess<int>,
        IKeyframeAccess<double>,
        IKeyframeAccess<float>,
        IKeyframeAccess<Vector2>,
        IKeyframeAccess<Vector3>,
        IKeyframeAccess<Vector4>,
        IKeyframeAccess<Float2>,
        IKeyframeAccess<Float3>,
        IKeyframeAccess<Float4>,
        IKeyframeAccess<Double2>,
        IKeyframeAccess<Double3>,
        IKeyframeAccess<Double4>,
        IKeyframeAccess<Quaternion>,
        IKeyframeAccess<Color32>,
        IKeyframeAccess<Color>
        {
            public void GetTangent(ref bool value, ref bool tangent, float tangentScale, out bool result)
            {
                result = value;
            }

            public void Linear(ref bool a, ref bool b, float alpha, out bool result)
            {
                result = a;
            }

            public void Bezier(ref bool p0, ref bool p1, ref bool p2, ref bool p3, float alpha, out bool result)
            {
                result = p0;
            }

            public void GetTangent(ref int value, ref int tangent, float tangentScale, out int result)
            {
                result = value + (int)(tangent * tangentScale);
            }

            public void Linear(ref int a, ref int b, float alpha, out int result)
            {
                result = Mathf.Lerp(a, b, alpha);
            }

            public void Bezier(ref int p0, ref int p1, ref int p2, ref int p3, float alpha, out int result)
            {
                var p01 = Mathf.Lerp(p0, p1, alpha);
                var p12 = Mathf.Lerp(p1, p2, alpha);
                var p23 = Mathf.Lerp(p2, p3, alpha);
                var p012 = Mathf.Lerp(p01, p12, alpha);
                var p123 = Mathf.Lerp(p12, p23, alpha);
                result = Mathf.Lerp(p012, p123, alpha);
            }

            public void GetTangent(ref double value, ref double tangent, float tangentScale, out double result)
            {
                result = value + tangent * tangentScale;
            }

            public void Linear(ref double a, ref double b, float alpha, out double result)
            {
                result = Mathf.Lerp(a, b, alpha);
            }

            public void Bezier(ref double p0, ref double p1, ref double p2, ref double p3, float alpha, out double result)
            {
                var p01 = Mathf.Lerp(p0, p1, alpha);
                var p12 = Mathf.Lerp(p1, p2, alpha);
                var p23 = Mathf.Lerp(p2, p3, alpha);
                var p012 = Mathf.Lerp(p01, p12, alpha);
                var p123 = Mathf.Lerp(p12, p23, alpha);
                result = Mathf.Lerp(p012, p123, alpha);
            }

            public void GetTangent(ref float value, ref float tangent, float tangentScale, out float result)
            {
                result = value + tangent * tangentScale;
            }

            public void Linear(ref float a, ref float b, float alpha, out float result)
            {
                result = Mathf.Lerp(a, b, alpha);
            }

            public void Bezier(ref float p0, ref float p1, ref float p2, ref float p3, float alpha, out float result)
            {
                var p01 = Mathf.Lerp(p0, p1, alpha);
                var p12 = Mathf.Lerp(p1, p2, alpha);
                var p23 = Mathf.Lerp(p2, p3, alpha);
                var p012 = Mathf.Lerp(p01, p12, alpha);
                var p123 = Mathf.Lerp(p12, p23, alpha);
                result = Mathf.Lerp(p012, p123, alpha);
            }

            public void GetTangent(ref Vector2 value, ref Vector2 tangent, float tangentScale, out Vector2 result)
            {
                result = value + tangent * tangentScale;
            }

            public void Linear(ref Vector2 a, ref Vector2 b, float alpha, out Vector2 result)
            {
                Vector2.Lerp(ref a, ref b, alpha, out result);
            }

            public void Bezier(ref Vector2 p0, ref Vector2 p1, ref Vector2 p2, ref Vector2 p3, float alpha, out Vector2 result)
            {
                Vector2.Lerp(ref p0, ref p1, alpha, out var p01);
                Vector2.Lerp(ref p1, ref p2, alpha, out var p12);
                Vector2.Lerp(ref p2, ref p3, alpha, out var p23);
                Vector2.Lerp(ref p01, ref p12, alpha, out var p012);
                Vector2.Lerp(ref p12, ref p23, alpha, out var p123);
                Vector2.Lerp(ref p012, ref p123, alpha, out result);
            }

            public void GetTangent(ref Vector3 value, ref Vector3 tangent, float tangentScale, out Vector3 result)
            {
                result = value + tangent * tangentScale;
            }

            public void Linear(ref Vector3 a, ref Vector3 b, float alpha, out Vector3 result)
            {
                Vector3.Lerp(ref a, ref b, alpha, out result);
            }

            public void Bezier(ref Vector3 p0, ref Vector3 p1, ref Vector3 p2, ref Vector3 p3, float alpha, out Vector3 result)
            {
                Vector3.Lerp(ref p0, ref p1, alpha, out var p01);
                Vector3.Lerp(ref p1, ref p2, alpha, out var p12);
                Vector3.Lerp(ref p2, ref p3, alpha, out var p23);
                Vector3.Lerp(ref p01, ref p12, alpha, out var p012);
                Vector3.Lerp(ref p12, ref p23, alpha, out var p123);
                Vector3.Lerp(ref p012, ref p123, alpha, out result);
            }

            public void GetTangent(ref Vector4 value, ref Vector4 tangent, float tangentScale, out Vector4 result)
            {
                result = value + tangent * tangentScale;
            }

            public void Linear(ref Vector4 a, ref Vector4 b, float alpha, out Vector4 result)
            {
                Vector4.Lerp(ref a, ref b, alpha, out result);
            }

            public void Bezier(ref Vector4 p0, ref Vector4 p1, ref Vector4 p2, ref Vector4 p3, float alpha, out Vector4 result)
            {
                Vector4.Lerp(ref p0, ref p1, alpha, out var p01);
                Vector4.Lerp(ref p1, ref p2, alpha, out var p12);
                Vector4.Lerp(ref p2, ref p3, alpha, out var p23);
                Vector4.Lerp(ref p01, ref p12, alpha, out var p012);
                Vector4.Lerp(ref p12, ref p23, alpha, out var p123);
                Vector4.Lerp(ref p012, ref p123, alpha, out result);
            }

            public void GetTangent(ref Float2 value, ref Float2 tangent, float tangentScale, out Float2 result)
            {
                result = value + tangent * tangentScale;
            }

            public void Linear(ref Float2 a, ref Float2 b, float alpha, out Float2 result)
            {
                Float2.Lerp(ref a, ref b, alpha, out result);
            }

            public void Bezier(ref Float2 p0, ref Float2 p1, ref Float2 p2, ref Float2 p3, float alpha, out Float2 result)
            {
                Float2.Lerp(ref p0, ref p1, alpha, out var p01);
                Float2.Lerp(ref p1, ref p2, alpha, out var p12);
                Float2.Lerp(ref p2, ref p3, alpha, out var p23);
                Float2.Lerp(ref p01, ref p12, alpha, out var p012);
                Float2.Lerp(ref p12, ref p23, alpha, out var p123);
                Float2.Lerp(ref p012, ref p123, alpha, out result);
            }

            public void GetTangent(ref Float3 value, ref Float3 tangent, float tangentScale, out Float3 result)
            {
                result = value + tangent * tangentScale;
            }

            public void Linear(ref Float3 a, ref Float3 b, float alpha, out Float3 result)
            {
                Float3.Lerp(ref a, ref b, alpha, out result);
            }

            public void Bezier(ref Float3 p0, ref Float3 p1, ref Float3 p2, ref Float3 p3, float alpha, out Float3 result)
            {
                Float3.Lerp(ref p0, ref p1, alpha, out var p01);
                Float3.Lerp(ref p1, ref p2, alpha, out var p12);
                Float3.Lerp(ref p2, ref p3, alpha, out var p23);
                Float3.Lerp(ref p01, ref p12, alpha, out var p012);
                Float3.Lerp(ref p12, ref p23, alpha, out var p123);
                Float3.Lerp(ref p012, ref p123, alpha, out result);
            }

            public void GetTangent(ref Float4 value, ref Float4 tangent, float tangentScale, out Float4 result)
            {
                result = value + tangent * tangentScale;
            }

            public void Linear(ref Float4 a, ref Float4 b, float alpha, out Float4 result)
            {
                Float4.Lerp(ref a, ref b, alpha, out result);
            }

            public void Bezier(ref Float4 p0, ref Float4 p1, ref Float4 p2, ref Float4 p3, float alpha, out Float4 result)
            {
                Float4.Lerp(ref p0, ref p1, alpha, out var p01);
                Float4.Lerp(ref p1, ref p2, alpha, out var p12);
                Float4.Lerp(ref p2, ref p3, alpha, out var p23);
                Float4.Lerp(ref p01, ref p12, alpha, out var p012);
                Float4.Lerp(ref p12, ref p23, alpha, out var p123);
                Float4.Lerp(ref p012, ref p123, alpha, out result);
            }

            public void GetTangent(ref Double2 value, ref Double2 tangent, float tangentScale, out Double2 result)
            {
                result = value + tangent * tangentScale;
            }

            public void Linear(ref Double2 a, ref Double2 b, float alpha, out Double2 result)
            {
                Double2.Lerp(ref a, ref b, alpha, out result);
            }

            public void Bezier(ref Double2 p0, ref Double2 p1, ref Double2 p2, ref Double2 p3, float alpha, out Double2 result)
            {
                Double2.Lerp(ref p0, ref p1, alpha, out var p01);
                Double2.Lerp(ref p1, ref p2, alpha, out var p12);
                Double2.Lerp(ref p2, ref p3, alpha, out var p23);
                Double2.Lerp(ref p01, ref p12, alpha, out var p012);
                Double2.Lerp(ref p12, ref p23, alpha, out var p123);
                Double2.Lerp(ref p012, ref p123, alpha, out result);
            }

            public void GetTangent(ref Double3 value, ref Double3 tangent, float tangentScale, out Double3 result)
            {
                result = value + tangent * tangentScale;
            }

            public void Linear(ref Double3 a, ref Double3 b, float alpha, out Double3 result)
            {
                Double3.Lerp(ref a, ref b, alpha, out result);
            }

            public void Bezier(ref Double3 p0, ref Double3 p1, ref Double3 p2, ref Double3 p3, float alpha, out Double3 result)
            {
                Double3.Lerp(ref p0, ref p1, alpha, out var p01);
                Double3.Lerp(ref p1, ref p2, alpha, out var p12);
                Double3.Lerp(ref p2, ref p3, alpha, out var p23);
                Double3.Lerp(ref p01, ref p12, alpha, out var p012);
                Double3.Lerp(ref p12, ref p23, alpha, out var p123);
                Double3.Lerp(ref p012, ref p123, alpha, out result);
            }

            public void GetTangent(ref Double4 value, ref Double4 tangent, float tangentScale, out Double4 result)
            {
                result = value + tangent * tangentScale;
            }

            public void Linear(ref Double4 a, ref Double4 b, float alpha, out Double4 result)
            {
                Double4.Lerp(ref a, ref b, alpha, out result);
            }

            public void Bezier(ref Double4 p0, ref Double4 p1, ref Double4 p2, ref Double4 p3, float alpha, out Double4 result)
            {
                Double4.Lerp(ref p0, ref p1, alpha, out var p01);
                Double4.Lerp(ref p1, ref p2, alpha, out var p12);
                Double4.Lerp(ref p2, ref p3, alpha, out var p23);
                Double4.Lerp(ref p01, ref p12, alpha, out var p012);
                Double4.Lerp(ref p12, ref p23, alpha, out var p123);
                Double4.Lerp(ref p012, ref p123, alpha, out result);
            }

            public void GetTangent(ref Quaternion value, ref Quaternion tangent, float tangentScale, out Quaternion result)
            {
                Quaternion.Slerp(ref value, ref tangent, 1.0f / 3.0f, out result);
            }

            public void Linear(ref Quaternion a, ref Quaternion b, float alpha, out Quaternion result)
            {
                Quaternion.Slerp(ref a, ref b, alpha, out result);
            }

            public void Bezier(ref Quaternion p0, ref Quaternion p1, ref Quaternion p2, ref Quaternion p3, float alpha, out Quaternion result)
            {
                Quaternion.Slerp(ref p0, ref p1, alpha, out var p01);
                Quaternion.Slerp(ref p1, ref p2, alpha, out var p12);
                Quaternion.Slerp(ref p2, ref p3, alpha, out var p23);
                Quaternion.Slerp(ref p01, ref p12, alpha, out var p012);
                Quaternion.Slerp(ref p12, ref p23, alpha, out var p123);
                Quaternion.Slerp(ref p012, ref p123, alpha, out result);
            }

            public void GetTangent(ref Color32 value, ref Color32 tangent, float tangentScale, out Color32 result)
            {
                result = value + tangent * tangentScale;
            }

            public void Linear(ref Color32 a, ref Color32 b, float alpha, out Color32 result)
            {
                Color32.Lerp(ref a, ref b, alpha, out result);
            }

            public void Bezier(ref Color32 p0, ref Color32 p1, ref Color32 p2, ref Color32 p3, float alpha, out Color32 result)
            {
                Color32.Lerp(ref p0, ref p1, alpha, out var p01);
                Color32.Lerp(ref p1, ref p2, alpha, out var p12);
                Color32.Lerp(ref p2, ref p3, alpha, out var p23);
                Color32.Lerp(ref p01, ref p12, alpha, out var p012);
                Color32.Lerp(ref p12, ref p23, alpha, out var p123);
                Color32.Lerp(ref p012, ref p123, alpha, out result);
            }

            public void GetTangent(ref Color value, ref Color tangent, float tangentScale, out Color result)
            {
                result = value + tangent * tangentScale;
            }

            public void Linear(ref Color a, ref Color b, float alpha, out Color result)
            {
                Color.Lerp(ref a, ref b, alpha, out result);
            }

            public void Bezier(ref Color p0, ref Color p1, ref Color p2, ref Color p3, float alpha, out Color result)
            {
                Color.Lerp(ref p0, ref p1, alpha, out var p01);
                Color.Lerp(ref p1, ref p2, alpha, out var p12);
                Color.Lerp(ref p2, ref p3, alpha, out var p23);
                Color.Lerp(ref p01, ref p12, alpha, out var p012);
                Color.Lerp(ref p12, ref p23, alpha, out var p123);
                Color.Lerp(ref p012, ref p123, alpha, out result);
            }
        }

        /// <summary>
        /// The keyframes data accessor.
        /// </summary>
        [NoSerialize]
        protected readonly IKeyframeAccess<T> _accessor = new KeyframeAccess() as IKeyframeAccess<T>;

        /// <summary>
        /// Evaluates the animation curve value at the specified time.
        /// </summary>
        /// <param name="result">The interpolated value from the curve at provided time.</param>
        /// <param name="time">The time to evaluate the curve at.</param>
        /// <param name="loop">If true the curve will loop when it goes past the end or beginning. Otherwise the curve value will be clamped.</param>
        public abstract void Evaluate(out T result, float time, bool loop = true);

        /// <summary>
        /// Trims the curve keyframes to the specified time range.
        /// </summary>
        /// <param name="start">The time start.</param>
        /// <param name="end">The time end.</param>
        public abstract void Trim(float start, float end);

        /// <summary>
        /// Applies the linear transformation (scale and offset) to the keyframes time values.
        /// </summary>
        /// <param name="timeScale">The time scale.</param>
        /// <param name="timeOffset">The time offset.</param>
        public abstract void TransformTime(float timeScale, float timeOffset);

        /// <summary>
        /// Returns a pair of keys that can be used for interpolating to field the value at the provided time.
        /// </summary>
        /// <param name="time">The time for which to find the relevant keys from. It is expected to be clamped to a valid range within the curve.</param>
        /// <param name="leftKey">The index of the key to interpolate from.</param>
        /// <param name="rightKey">The index of the key to interpolate to.</param>
        public abstract void FindKeys(float time, out int leftKey, out int rightKey);

        /// <summary>
        /// Wraps the time for the curve sampling with looping mode.
        /// </summary>
        /// <param name="time">The time to wrap.</param>
        /// <param name="start">The start time.</param>
        /// <param name="end">The end time.</param>
        /// <param name="loop">If set to <c>true</c> loops the curve.</param>
        protected static void WrapTime(ref float time, float start, float end, bool loop)
        {
            float length = end - start;

            if (Mathf.NearEqual(length, 0.0f))
            {
                time = 0.0f;
                return;
            }

            // Clamp to start or loop
            if (time < start)
            {
                if (loop)
                    time += (Mathf.Floor(end - time) / length) * length;
                else
                    time = start;
            }

            // Clamp to end or loop
            if (time > end)
            {
                if (loop)
                    time -= Mathf.Floor((time - start) / length) * length;
                else
                    time = end;
            }
        }

        /// <summary>
        /// Raw memory copy (used by scripting bindings - see MarshalAs tag).
        /// </summary>
        /// <param name="keyframes">The keyframes array.</param>
        /// <returns>The raw keyframes data.</returns>
        protected static unsafe byte[] MarshalKeyframes<Keyframe>(Keyframe[] keyframes)
        {
            if (keyframes == null || keyframes.Length == 0)
                return null;
            var keyframeSize = Unsafe.SizeOf<Keyframe>();
            var result = new byte[keyframes.Length * keyframeSize];
            fixed (byte* resultPtr = result)
            {
                var keyframesHandle = ManagedHandle.Alloc(keyframes, GCHandleType.Pinned);
                var keyframesPtr = Marshal.UnsafeAddrOfPinnedArrayElement(keyframes, 0);
                Buffer.MemoryCopy((void*)keyframesPtr, resultPtr, (uint)result.Length, (uint)result.Length);
                keyframesHandle.Free();
            }
            return result;
        }

        /// <summary>
        /// Raw memory copy (used by scripting bindings - see MarshalAs tag).
        /// </summary>
        /// <param name="raw">The raw keyframes data.</param>
        /// <returns>The keyframes array.</returns>
        protected static unsafe Keyframe[] MarshalKeyframes<Keyframe>(byte[] raw) where Keyframe : struct
        {
            if (raw == null || raw.Length == 0)
                return null;
            fixed (byte* rawPtr = raw)
                return MemoryMarshal.Cast<byte, Keyframe>(new Span<byte>(rawPtr, raw.Length)).ToArray();
        }
    }

    /// <summary>
    /// An animation spline represented by a set of keyframes, each representing an endpoint of a linear curve.
    /// </summary>
    /// <typeparam name="T">The animated value type.</typeparam>
    public class LinearCurve<T> : CurveBase<T> where T : new()
    {
        /// <summary>
        /// A single keyframe that can be injected into linear curve.
        /// </summary>
        [StructLayout(LayoutKind.Sequential, Pack = 2)]
        public struct Keyframe : IComparable, IComparable<Keyframe>
        {
            /// <summary>
            /// The time of the keyframe.
            /// </summary>
            [EditorOrder(0), Limit(float.MinValue, float.MaxValue, 0.01f), Tooltip("The time of the keyframe.")]
            public float Time;

            /// <summary>
            /// The value of the curve at keyframe.
            /// </summary>
            [EditorOrder(1), Limit(float.MinValue, float.MaxValue, 0.01f), Tooltip("The value of the curve at keyframe.")]
            public T Value;

            /// <summary>
            /// Initializes a new instance of the <see cref="Keyframe"/> struct.
            /// </summary>
            /// <param name="time">The time.</param>
            /// <param name="value">The value.</param>
            public Keyframe(float time, T value)
            {
                Time = time;
                Value = value;
            }

            /// <inheritdoc />
            public int CompareTo(object obj)
            {
                if (obj is Keyframe other)
                    return Time > other.Time ? 1 : 0;
                return 1;
            }

            /// <inheritdoc />
            public int CompareTo(Keyframe other)
            {
                return Time > other.Time ? 1 : 0;
            }

            /// <inheritdoc />
            public override string ToString()
            {
                return Value?.ToString() ?? string.Empty;
            }
        }

        /// <summary>
        /// The keyframes collection. Can be directly modified but ensure to sort it after editing so keyframes are organized by ascending time value.
        /// </summary>
        [Serialize]
        public Keyframe[] Keyframes;

        /// <summary>
        /// Initializes a new instance of the <see cref="LinearCurve{T}"/> class.
        /// </summary>
        public LinearCurve()
        {
            Keyframes = Utils.GetEmptyArray<Keyframe>();
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="LinearCurve{T}"/> class.
        /// </summary>
        /// <param name="keyframes">The keyframes.</param>
        public LinearCurve(params Keyframe[] keyframes)
        {
            Keyframes = keyframes;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="LinearCurve{T}"/> class.
        /// </summary>
        /// <param name="keyframes">The keyframes.</param>
        public LinearCurve(IEnumerable<Keyframe> keyframes)
        {
            Keyframes = keyframes.ToArray();
        }

        /// <summary>
        /// Evaluates the animation curve key at the specified time.
        /// </summary>
        /// <param name="result">The interpolated key from the curve at provided time.</param>
        /// <param name="time">The time to evaluate the curve at.</param>
        /// <param name="loop">If true the curve will loop when it goes past the end or beginning. Otherwise the curve value will be clamped.</param>
        public void EvaluateKey(out Keyframe result, float time, bool loop = true)
        {
            if (Keyframes.Length == 0)
            {
                result = new Keyframe(time, default);
                return;
            }

            float start = Mathf.Min(Keyframes.First().Time, 0.0f);
            float end = Keyframes.Last().Time;
            WrapTime(ref time, start, end, loop);

            FindKeys(time, out var leftKeyIdx, out var rightKeyIdx);
            Keyframe leftKey = Keyframes[leftKeyIdx];
            Keyframe rightKey = Keyframes[rightKeyIdx];
            if (leftKeyIdx == rightKeyIdx)
            {
                result = leftKey;
                return;
            }

            // Scale from arbitrary range to [0, 1]
            float length = rightKey.Time - leftKey.Time;
            float t = Mathf.NearEqual(length, 0.0f) ? 0.0f : (time - leftKey.Time) / length;

            // Evaluate the key at the curve
            result.Time = leftKey.Time + length * t;
            _accessor.Linear(ref leftKey.Value, ref rightKey.Value, t, out result.Value);
        }

        /// <inheritdoc />
        public override void Evaluate(out T result, float time, bool loop = true)
        {
            if (Keyframes.Length == 0)
            {
                result = default;
                return;
            }

            float start = Mathf.Min(Keyframes.First().Time, 0.0f);
            float end = Keyframes.Last().Time;
            WrapTime(ref time, start, end, loop);

            FindKeys(time, out var leftKeyIdx, out var rightKeyIdx);
            Keyframe leftKey = Keyframes[leftKeyIdx];
            Keyframe rightKey = Keyframes[rightKeyIdx];
            if (leftKeyIdx == rightKeyIdx)
            {
                result = leftKey.Value;
                return;
            }

            // Scale from arbitrary range to [0, 1]
            float length = rightKey.Time - leftKey.Time;
            float t = Mathf.NearEqual(length, 0.0f) ? 0.0f : (time - leftKey.Time) / length;

            // Evaluate the value at the curve
            _accessor.Linear(ref leftKey.Value, ref rightKey.Value, t, out result);
        }

        /// <inheritdoc />
        public override void Trim(float start, float end)
        {
            // Early out
            if (Keyframes.Length == 0 || (Keyframes.First().Time >= start && Keyframes.Last().Time <= end))
                return;
            if (end - start <= Mathf.Epsilon)
            {
                // Erase the curve
                Keyframes = Utils.GetEmptyArray<Keyframe>();
                return;
            }

            var result = new List<Keyframe>(Keyframes);

            EvaluateKey(out var startValue, start, false);
            EvaluateKey(out var endValue, end, false);

            // Begin
            for (int i = 0; i < result.Count && result.Count > 0; i++)
            {
                if (result[i].Time < start)
                {
                    result.RemoveAt(i);
                    i--;
                }
                else
                {
                    break;
                }
            }
            if (result.Count == 0 || !Mathf.NearEqual(result.First().Time, start))
            {
                Keyframe key = startValue;
                key.Time = start;
                result.Insert(0, key);
            }

            // End
            for (int i = result.Count - 1; i >= 0 && result.Count > 0; i--)
            {
                if (result[i].Time > end)
                {
                    result.RemoveAt(i);
                }
                else
                {
                    break;
                }
            }
            if (result.Count == 0 || !Mathf.NearEqual(result.Last().Time, end))
            {
                Keyframe key = endValue;
                key.Time = end;
                result.Add(key);
            }

            Keyframes = result.ToArray();

            // Rebase the keyframes time
            if (!Mathf.IsZero(start))
            {
                for (int i = 0; i < Keyframes.Length; i++)
                    Keyframes[i].Time -= start;
            }
        }

        /// <inheritdoc />
        public override void TransformTime(float timeScale, float timeOffset)
        {
            for (int i = 0; i < Keyframes.Length; i++)
                Keyframes[i].Time = Keyframes[i].Time * timeScale + timeOffset;
        }

        /// <inheritdoc />
        public override void FindKeys(float time, out int leftKey, out int rightKey)
        {
            int start = 0;
            int searchLength = Keyframes.Length;

            while (searchLength > 0)
            {
                int half = searchLength >> 1;
                int mid = start + half;

                if (time < Keyframes[mid].Time)
                {
                    searchLength = half;
                }
                else
                {
                    start = mid + 1;
                    searchLength -= (half + 1);
                }
            }

            leftKey = Mathf.Max(0, start - 1);
            rightKey = Mathf.Min(start, Keyframes.Length - 1);
        }

        /// <summary>
        /// Raw memory copy (used by scripting bindings - see MarshalAs tag).
        /// </summary>
        /// <param name="curve">The curve to copy.</param>
        /// <returns>The raw keyframes data.</returns>
        public static unsafe implicit operator byte[](LinearCurve<T> curve)
        {
            if (curve == null)
                return null;
            return MarshalKeyframes<Keyframe>(curve.Keyframes);
        }

        /// <summary>
        /// Raw memory copy (used by scripting bindings - see MarshalAs tag).
        /// </summary>
        /// <param name="raw">The raw keyframes data.</param>
        /// <returns>The curve.</returns>
        public static unsafe implicit operator LinearCurve<T>(byte[] raw)
        {
            if (raw == null || raw.Length == 0)
                return null;
            return new LinearCurve<T>(MarshalKeyframes<Keyframe>(raw));
        }
    }

    /// <summary>
    /// An animation spline represented by a set of keyframes, each representing an endpoint of a Bezier curve.
    /// </summary>
    /// <typeparam name="T">The animated value type.</typeparam>
    public class BezierCurve<T> : CurveBase<T> where T : new()
    {
        /// <summary>
        /// A single keyframe that can be injected into Bezier curve.
        /// </summary>
        [StructLayout(LayoutKind.Sequential, Pack = 2)]
        public struct Keyframe : IComparable, IComparable<Keyframe>
        {
            /// <summary>
            /// The time of the keyframe.
            /// </summary>
            [EditorOrder(0), Limit(float.MinValue, float.MaxValue, 0.01f), Tooltip("The time of the keyframe.")]
            public float Time;

            /// <summary>
            /// The value of the curve at keyframe.
            /// </summary>
            [EditorOrder(1), Limit(float.MinValue, float.MaxValue, 0.01f), Tooltip("The value of the curve at keyframe.")]
            public T Value;

            /// <summary>
            /// The input tangent (going from the previous key to this one) of the key.
            /// </summary>
            [EditorOrder(2), Limit(float.MinValue, float.MaxValue, 0.01f), Tooltip("The input tangent (going from the previous key to this one) of the key."), EditorDisplay(null, "Tangent In")]
            public T TangentIn;

            /// <summary>
            /// The output tangent (going from this key to next one) of the key.
            /// </summary>
            [EditorOrder(3), Limit(float.MinValue, float.MaxValue, 0.01f), Tooltip("The output tangent (going from this key to next one) of the key.")]
            public T TangentOut;

            /// <summary>
            /// Initializes a new instance of the <see cref="Keyframe"/> struct.
            /// </summary>
            /// <param name="time">The time.</param>
            /// <param name="value">The value.</param>
            public Keyframe(float time, T value)
            {
                Time = time;
                Value = value;
                TangentIn = TangentOut = default;
            }

            /// <summary>
            /// Initializes a new instance of the <see cref="Keyframe"/> struct.
            /// </summary>
            /// <param name="time">The time.</param>
            /// <param name="value">The value.</param>
            /// <param name="tangentIn">The start tangent.</param>
            /// <param name="tangentOut">The end tangent.</param>
            public Keyframe(float time, T value, T tangentIn, T tangentOut)
            {
                Time = time;
                Value = value;
                TangentIn = tangentIn;
                TangentOut = tangentOut;
            }

            /// <inheritdoc />
            public int CompareTo(object obj)
            {
                if (obj is Keyframe other)
                    return Time > other.Time ? 1 : 0;
                return 1;
            }

            /// <inheritdoc />
            public int CompareTo(Keyframe other)
            {
                return Time > other.Time ? 1 : 0;
            }

            /// <inheritdoc />
            public override string ToString()
            {
                return Value?.ToString() ?? string.Empty;
            }
        }

        /// <summary>
        /// The keyframes collection. Can be directly modified but ensure to sort it after editing so keyframes are organized by ascending time value.
        /// </summary>
        [Serialize]
        public Keyframe[] Keyframes;

        /// <summary>
        /// Initializes a new instance of the <see cref="BezierCurve{T}"/> class.
        /// </summary>
        public BezierCurve()
        {
            Keyframes = Utils.GetEmptyArray<Keyframe>();
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="BezierCurve{T}"/> class.
        /// </summary>
        /// <param name="keyframes">The keyframes.</param>
        public BezierCurve(params Keyframe[] keyframes)
        {
            Keyframes = keyframes;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="BezierCurve{T}"/> class.
        /// </summary>
        /// <param name="keyframes">The keyframes.</param>
        public BezierCurve(IEnumerable<Keyframe> keyframes)
        {
            Keyframes = keyframes.ToArray();
        }

        /// <summary>
        /// Evaluates the animation curve key at the specified time.
        /// </summary>
        /// <param name="result">The interpolated key from the curve at provided time.</param>
        /// <param name="time">The time to evaluate the curve at.</param>
        /// <param name="loop">If true the curve will loop when it goes past the end or beginning. Otherwise the curve value will be clamped.</param>
        public void EvaluateKey(out Keyframe result, float time, bool loop = true)
        {
            if (Keyframes.Length == 0)
            {
                result = new Keyframe(time, default);
                return;
            }

            float start = Mathf.Min(Keyframes.First().Time, 0.0f);
            float end = Keyframes.Last().Time;
            WrapTime(ref time, start, end, loop);

            FindKeys(time, out var leftKeyIdx, out var rightKeyIdx);
            Keyframe leftKey = Keyframes[leftKeyIdx];
            Keyframe rightKey = Keyframes[rightKeyIdx];
            if (leftKeyIdx == rightKeyIdx)
            {
                result = leftKey;
                return;
            }

            // Scale from arbitrary range to [0, 1]
            float length = rightKey.Time - leftKey.Time;
            float t = Mathf.NearEqual(length, 0.0f) ? 0.0f : (time - leftKey.Time) / length;

            // Evaluate the key at the curve
            result.Time = leftKey.Time + length * t;
            float tangentScale = length / 3.0f;
            _accessor.GetTangent(ref leftKey.Value, ref leftKey.TangentOut, tangentScale, out var leftTangent);
            _accessor.GetTangent(ref rightKey.Value, ref rightKey.TangentIn, tangentScale, out var rightTangent);
            _accessor.Bezier(ref leftKey.Value, ref leftTangent, ref rightTangent, ref rightKey.Value, t, out result.Value);
            result.TangentIn = leftKey.TangentOut;
            result.TangentOut = rightKey.TangentIn;
        }

        /// <inheritdoc />
        public override void Evaluate(out T result, float time, bool loop = true)
        {
            if (Keyframes.Length == 0)
            {
                result = default;
                return;
            }

            float start = Mathf.Min(Keyframes.First().Time, 0.0f);
            float end = Keyframes.Last().Time;
            WrapTime(ref time, start, end, loop);

            FindKeys(time, out var leftKeyIdx, out var rightKeyIdx);
            Keyframe leftKey = Keyframes[leftKeyIdx];
            Keyframe rightKey = Keyframes[rightKeyIdx];
            if (leftKeyIdx == rightKeyIdx)
            {
                result = leftKey.Value;
                return;
            }

            // Scale from arbitrary range to [0, 1]
            float length = rightKey.Time - leftKey.Time;
            float t = Mathf.NearEqual(length, 0.0f) ? 0.0f : (time - leftKey.Time) / length;

            // Evaluate the value at the curve
            float tangentScale = length / 3.0f;
            _accessor.GetTangent(ref leftKey.Value, ref leftKey.TangentOut, tangentScale, out var leftTangent);
            _accessor.GetTangent(ref rightKey.Value, ref rightKey.TangentIn, tangentScale, out var rightTangent);
            _accessor.Bezier(ref leftKey.Value, ref leftTangent, ref rightTangent, ref rightKey.Value, t, out result);
        }

        /// <inheritdoc />
        public override void Trim(float start, float end)
        {
            // Early out
            if (Keyframes.Length == 0 || (Keyframes.First().Time >= start && Keyframes.Last().Time <= end))
                return;
            if (end - start <= Mathf.Epsilon)
            {
                // Erase the curve
                Keyframes = Utils.GetEmptyArray<Keyframe>();
                return;
            }

            var result = new List<Keyframe>(Keyframes);

            EvaluateKey(out var startValue, start, false);
            EvaluateKey(out var endValue, end, false);

            // Begin
            for (int i = 0; i < result.Count && result.Count > 0; i++)
            {
                if (result[i].Time < start)
                {
                    result.RemoveAt(i);
                    i--;
                }
                else
                {
                    break;
                }
            }
            if (result.Count == 0 || !Mathf.NearEqual(result.First().Time, start))
            {
                Keyframe key = startValue;
                key.Time = start;
                result.Insert(0, key);
            }

            // End
            for (int i = result.Count - 1; i >= 0 && result.Count > 0; i--)
            {
                if (result[i].Time > end)
                {
                    result.RemoveAt(i);
                }
                else
                {
                    break;
                }
            }
            if (result.Count == 0 || !Mathf.NearEqual(result.Last().Time, end))
            {
                Keyframe key = endValue;
                key.Time = end;
                result.Add(key);
            }

            Keyframes = result.ToArray();

            // Rebase the keyframes time
            if (!Mathf.IsZero(start))
            {
                for (int i = 0; i < Keyframes.Length; i++)
                    Keyframes[i].Time -= start;
            }
        }

        /// <inheritdoc />
        public override void TransformTime(float timeScale, float timeOffset)
        {
            for (int i = 0; i < Keyframes.Length; i++)
                Keyframes[i].Time = Keyframes[i].Time * timeScale + timeOffset;
        }

        /// <inheritdoc />
        public override void FindKeys(float time, out int leftKey, out int rightKey)
        {
            int start = 0;
            int searchLength = Keyframes.Length;

            while (searchLength > 0)
            {
                int half = searchLength >> 1;
                int mid = start + half;

                if (time < Keyframes[mid].Time)
                {
                    searchLength = half;
                }
                else
                {
                    start = mid + 1;
                    searchLength -= (half + 1);
                }
            }

            leftKey = Mathf.Max(0, start - 1);
            rightKey = Mathf.Min(start, Keyframes.Length - 1);
        }

        /// <summary>
        /// Raw memory copy (used by scripting bindings - see MarshalAs tag).
        /// </summary>
        /// <param name="curve">The curve to copy.</param>
        /// <returns>The raw keyframes data.</returns>
        public static unsafe implicit operator byte[](BezierCurve<T> curve)
        {
            if (curve == null)
                return null;
            return MarshalKeyframes<Keyframe>(curve.Keyframes);
        }

        /// <summary>
        /// Raw memory copy (used by scripting bindings - see MarshalAs tag).
        /// </summary>
        /// <param name="raw">The raw keyframes data.</param>
        /// <returns>The curve.</returns>
        public static unsafe implicit operator BezierCurve<T>(byte[] raw)
        {
            if (raw == null || raw.Length == 0)
                return null;
            return new BezierCurve<T>(MarshalKeyframes<Keyframe>(raw));
        }
    }
}
