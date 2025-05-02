// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_LARGE_WORLDS
using Real = System.Double;
#else
using Real = System.Single;
#endif

using FlaxEngine;

// ReSharper disable RedundantAssignment

namespace FlaxEditor.GUI
{
    partial class CurveEditor<T>
    {
        /// <summary>
        /// The generic keyframe value accessor object for curve editor.
        /// </summary>
        /// <typeparam name="U">The keyframe value type.</typeparam>
        public interface IKeyframeAccess<U> where U : new()
        {
            /// <summary>
            /// Gets the default value.
            /// </summary>
            /// <param name="value">The value.</param>
            void GetDefaultValue(out U value);

            /// <summary>
            /// Gets the curve components count. Vector types should return amount of component to use for value editing.
            /// </summary>
            /// <returns>The components count.</returns>
            int GetCurveComponents();

            /// <summary>
            /// Gets the value of the component for the curve.
            /// </summary>
            /// <param name="value">The keyframe value.</param>
            /// <param name="component">The component index.</param>
            /// <returns>The curve value.</returns>
            float GetCurveValue(ref U value, int component);

            /// <summary>
            /// Sets the curve value of the component.
            /// </summary>
            /// <param name="curve">The curve value to assign.</param>
            /// <param name="value">The keyframe value.</param>
            /// <param name="component">The component index.</param>
            void SetCurveValue(float curve, ref U value, int component);
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
            void IKeyframeAccess<bool>.GetDefaultValue(out bool value)
            {
                value = false;
            }

            int IKeyframeAccess<bool>.GetCurveComponents()
            {
                return 1;
            }

            float IKeyframeAccess<bool>.GetCurveValue(ref bool value, int component)
            {
                return value ? 1 : 0;
            }

            void IKeyframeAccess<bool>.SetCurveValue(float curve, ref bool value, int component)
            {
                value = curve >= 0.5f;
            }

            void IKeyframeAccess<int>.GetDefaultValue(out int value)
            {
                value = 0;
            }

            int IKeyframeAccess<int>.GetCurveComponents()
            {
                return 1;
            }

            float IKeyframeAccess<int>.GetCurveValue(ref int value, int component)
            {
                return value;
            }

            void IKeyframeAccess<int>.SetCurveValue(float curve, ref int value, int component)
            {
                value = (int)curve;
            }

            void IKeyframeAccess<double>.GetDefaultValue(out double value)
            {
                value = 0.0;
            }

            int IKeyframeAccess<double>.GetCurveComponents()
            {
                return 1;
            }

            float IKeyframeAccess<double>.GetCurveValue(ref double value, int component)
            {
                return (float)value;
            }

            void IKeyframeAccess<double>.SetCurveValue(float curve, ref double value, int component)
            {
                value = (double)curve;
            }

            void IKeyframeAccess<float>.GetDefaultValue(out float value)
            {
                value = 0.0f;
            }

            int IKeyframeAccess<float>.GetCurveComponents()
            {
                return 1;
            }

            float IKeyframeAccess<float>.GetCurveValue(ref float value, int component)
            {
                return value;
            }

            void IKeyframeAccess<float>.SetCurveValue(float curve, ref float value, int component)
            {
                value = (float)curve;
            }

            void IKeyframeAccess<Vector2>.GetDefaultValue(out Vector2 value)
            {
                value = Vector2.Zero;
            }

            int IKeyframeAccess<Vector2>.GetCurveComponents()
            {
                return 2;
            }

            float IKeyframeAccess<Vector2>.GetCurveValue(ref Vector2 value, int component)
            {
                return (float)value[component];
            }

            void IKeyframeAccess<Vector2>.SetCurveValue(float curve, ref Vector2 value, int component)
            {
                value[component] = (Real)curve;
            }

            void IKeyframeAccess<Vector3>.GetDefaultValue(out Vector3 value)
            {
                value = Vector3.Zero;
            }

            int IKeyframeAccess<Vector3>.GetCurveComponents()
            {
                return 3;
            }

            float IKeyframeAccess<Vector3>.GetCurveValue(ref Vector3 value, int component)
            {
                return (float)value[component];
            }

            void IKeyframeAccess<Vector3>.SetCurveValue(float curve, ref Vector3 value, int component)
            {
                value[component] = (Real)curve;
            }

            void IKeyframeAccess<Vector4>.GetDefaultValue(out Vector4 value)
            {
                value = Vector4.Zero;
            }

            int IKeyframeAccess<Vector4>.GetCurveComponents()
            {
                return 4;
            }

            float IKeyframeAccess<Vector4>.GetCurveValue(ref Vector4 value, int component)
            {
                return (float)value[component];
            }

            void IKeyframeAccess<Vector4>.SetCurveValue(float curve, ref Vector4 value, int component)
            {
                value[component] = (Real)curve;
            }

            void IKeyframeAccess<Float2>.GetDefaultValue(out Float2 value)
            {
                value = Float2.Zero;
            }

            int IKeyframeAccess<Float2>.GetCurveComponents()
            {
                return 2;
            }

            float IKeyframeAccess<Float2>.GetCurveValue(ref Float2 value, int component)
            {
                return value[component];
            }

            void IKeyframeAccess<Float2>.SetCurveValue(float curve, ref Float2 value, int component)
            {
                value[component] = curve;
            }

            void IKeyframeAccess<Float3>.GetDefaultValue(out Float3 value)
            {
                value = Float3.Zero;
            }

            int IKeyframeAccess<Float3>.GetCurveComponents()
            {
                return 3;
            }

            float IKeyframeAccess<Float3>.GetCurveValue(ref Float3 value, int component)
            {
                return value[component];
            }

            void IKeyframeAccess<Float3>.SetCurveValue(float curve, ref Float3 value, int component)
            {
                value[component] = curve;
            }

            void IKeyframeAccess<Float4>.GetDefaultValue(out Float4 value)
            {
                value = Float4.Zero;
            }

            int IKeyframeAccess<Float4>.GetCurveComponents()
            {
                return 4;
            }

            float IKeyframeAccess<Float4>.GetCurveValue(ref Float4 value, int component)
            {
                return value[component];
            }

            void IKeyframeAccess<Float4>.SetCurveValue(float curve, ref Float4 value, int component)
            {
                value[component] = curve;
            }

            void IKeyframeAccess<Double2>.GetDefaultValue(out Double2 value)
            {
                value = Double2.Zero;
            }

            int IKeyframeAccess<Double2>.GetCurveComponents()
            {
                return 2;
            }

            float IKeyframeAccess<Double2>.GetCurveValue(ref Double2 value, int component)
            {
                return (float)value[component];
            }

            void IKeyframeAccess<Double2>.SetCurveValue(float curve, ref Double2 value, int component)
            {
                value[component] = curve;
            }

            void IKeyframeAccess<Double3>.GetDefaultValue(out Double3 value)
            {
                value = Double3.Zero;
            }

            int IKeyframeAccess<Double3>.GetCurveComponents()
            {
                return 3;
            }

            float IKeyframeAccess<Double3>.GetCurveValue(ref Double3 value, int component)
            {
                return (float)value[component];
            }

            void IKeyframeAccess<Double3>.SetCurveValue(float curve, ref Double3 value, int component)
            {
                value[component] = curve;
            }

            void IKeyframeAccess<Double4>.GetDefaultValue(out Double4 value)
            {
                value = Double4.Zero;
            }

            int IKeyframeAccess<Double4>.GetCurveComponents()
            {
                return 4;
            }

            float IKeyframeAccess<Double4>.GetCurveValue(ref Double4 value, int component)
            {
                return (float)value[component];
            }

            void IKeyframeAccess<Double4>.SetCurveValue(float curve, ref Double4 value, int component)
            {
                value[component] = curve;
            }

            public void GetDefaultValue(out Quaternion value)
            {
                value = Quaternion.Identity;
            }

            int IKeyframeAccess<Quaternion>.GetCurveComponents()
            {
                return 3;
            }

            float IKeyframeAccess<Quaternion>.GetCurveValue(ref Quaternion value, int component)
            {
                return value.EulerAngles[component];
            }

            void IKeyframeAccess<Quaternion>.SetCurveValue(float curve, ref Quaternion value, int component)
            {
                var euler = value.EulerAngles;
                euler[component] = (float)curve;
                Quaternion.Euler(euler.X, euler.Y, euler.Z, out value);
            }

            void IKeyframeAccess<Color>.GetDefaultValue(out Color value)
            {
                value = Color.Black;
            }

            int IKeyframeAccess<Color>.GetCurveComponents()
            {
                return 4;
            }

            float IKeyframeAccess<Color>.GetCurveValue(ref Color value, int component)
            {
                return value[component];
            }

            void IKeyframeAccess<Color>.SetCurveValue(float curve, ref Color value, int component)
            {
                value[component] = (float)curve;
            }

            void IKeyframeAccess<Color32>.GetDefaultValue(out Color32 value)
            {
                value = Color32.Black;
            }

            int IKeyframeAccess<Color32>.GetCurveComponents()
            {
                return 4;
            }

            float IKeyframeAccess<Color32>.GetCurveValue(ref Color32 value, int component)
            {
                return value[component];
            }

            void IKeyframeAccess<Color32>.SetCurveValue(float curve, ref Color32 value, int component)
            {
                value[component] = (byte)Mathf.Clamp(curve, 0, 255);
            }
        }
    }
}
