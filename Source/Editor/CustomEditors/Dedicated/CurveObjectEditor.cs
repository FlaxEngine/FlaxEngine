// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using FlaxEditor.GUI;
using FlaxEngine;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Custom editor for <see cref="BezierCurve{T}"/>.
    /// </summary>
    class BezierCurveObjectEditor<T> : CustomEditor where T : struct
    {
        private bool _isSetting;
        private BezierCurveEditor<T> _curve;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            var item = layout.CustomContainer<BezierCurveEditor<T>>();
            _curve = item.CustomControl;
            _curve.Height = 120.0f;
            _curve.Edited += OnCurveEdited;
        }

        private void OnCurveEdited()
        {
            if (_isSetting)
                return;

            _isSetting = true;
            SetValue(new BezierCurve<T>(_curve.Keyframes));
            _isSetting = false;
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            var value = (BezierCurve<T>)Values[0];
            if (value != null && !_curve.IsUserEditing && !Utils.ArraysEqual(value.Keyframes, _curve.Keyframes))
            {
                _isSetting = true;
                _curve.SetKeyframes(value.Keyframes);
                _isSetting = false;
            }
        }

        /// <inheritdoc />
        protected override void Deinitialize()
        {
            _curve = null;

            base.Deinitialize();
        }
    }

    [CustomEditor(typeof(BezierCurve<int>)), DefaultEditor]
    sealed class IntBezierCurveObjectEditor : BezierCurveObjectEditor<int>
    {
    }

    [CustomEditor(typeof(BezierCurve<float>)), DefaultEditor]
    sealed class FloatBezierCurveObjectEditor : BezierCurveObjectEditor<float>
    {
    }

    [CustomEditor(typeof(BezierCurve<Vector2>)), DefaultEditor]
    sealed class Vector2BezierCurveObjectEditor : BezierCurveObjectEditor<Vector2>
    {
    }

    [CustomEditor(typeof(BezierCurve<Vector3>)), DefaultEditor]
    sealed class Vector3BezierCurveObjectEditor : BezierCurveObjectEditor<Vector3>
    {
    }

    [CustomEditor(typeof(BezierCurve<Vector4>)), DefaultEditor]
    sealed class Vector4BezierCurveObjectEditor : BezierCurveObjectEditor<Vector4>
    {
    }

    [CustomEditor(typeof(BezierCurve<Float2>)), DefaultEditor]
    sealed class Float2BezierCurveObjectEditor : BezierCurveObjectEditor<Float2>
    {
    }

    [CustomEditor(typeof(BezierCurve<Float3>)), DefaultEditor]
    sealed class Float3BezierCurveObjectEditor : BezierCurveObjectEditor<Float3>
    {
    }

    [CustomEditor(typeof(BezierCurve<Float4>)), DefaultEditor]
    sealed class Float4BezierCurveObjectEditor : BezierCurveObjectEditor<Float4>
    {
    }

    [CustomEditor(typeof(BezierCurve<Quaternion>)), DefaultEditor]
    sealed class QuaternionBezierCurveObjectEditor : BezierCurveObjectEditor<Quaternion>
    {
    }

    [CustomEditor(typeof(BezierCurve<Color>)), DefaultEditor]
    sealed class ColorBezierCurveObjectEditor : BezierCurveObjectEditor<Color>
    {
    }

    /// <summary>
    /// Custom editor for <see cref="LinearCurve{T}"/>.
    /// </summary>
    class LinearCurveObjectEditor<T> : CustomEditor where T : struct
    {
        private bool _isSetting;
        private LinearCurveEditor<T> _curve;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            var item = layout.CustomContainer<LinearCurveEditor<T>>();
            _curve = item.CustomControl;
            _curve.Height = 120.0f;
            _curve.Edited += OnCurveEdited;
        }

        private void OnCurveEdited()
        {
            if (_isSetting)
                return;

            _isSetting = true;
            SetValue(new LinearCurve<T>(_curve.Keyframes));
            _isSetting = false;
        }

        /// <inheritdoc />
        public override void Refresh()
        {
            base.Refresh();

            var value = (LinearCurve<T>)Values[0];
            if (value != null && !_curve.IsUserEditing && !Utils.ArraysEqual(value.Keyframes, _curve.Keyframes))
            {
                _isSetting = true;
                _curve.SetKeyframes(value.Keyframes);
                _isSetting = false;
            }
        }

        /// <inheritdoc />
        protected override void Deinitialize()
        {
            _curve = null;

            base.Deinitialize();
        }
    }

    [CustomEditor(typeof(LinearCurve<int>)), DefaultEditor]
    sealed class IntLinearCurveObjectEditor : LinearCurveObjectEditor<int>
    {
    }

    [CustomEditor(typeof(LinearCurve<float>)), DefaultEditor]
    sealed class FloatLinearCurveObjectEditor : LinearCurveObjectEditor<float>
    {
    }

    [CustomEditor(typeof(LinearCurve<Vector2>)), DefaultEditor]
    sealed class Vector2LinearCurveObjectEditor : LinearCurveObjectEditor<Vector2>
    {
    }

    [CustomEditor(typeof(LinearCurve<Vector3>)), DefaultEditor]
    sealed class Vector3LinearCurveObjectEditor : LinearCurveObjectEditor<Vector3>
    {
    }

    [CustomEditor(typeof(LinearCurve<Vector4>)), DefaultEditor]
    sealed class Vector4LinearCurveObjectEditor : LinearCurveObjectEditor<Vector4>
    {
    }

    [CustomEditor(typeof(LinearCurve<Float2>)), DefaultEditor]
    sealed class Float2LinearCurveObjectEditor : LinearCurveObjectEditor<Float2>
    {
    }

    [CustomEditor(typeof(LinearCurve<Float3>)), DefaultEditor]
    sealed class Float3LinearCurveObjectEditor : LinearCurveObjectEditor<Float3>
    {
    }

    [CustomEditor(typeof(LinearCurve<Float4>)), DefaultEditor]
    sealed class Float4LinearCurveObjectEditor : LinearCurveObjectEditor<Float4>
    {
    }

    [CustomEditor(typeof(LinearCurve<Quaternion>)), DefaultEditor]
    sealed class QuaternionLinearCurveObjectEditor : LinearCurveObjectEditor<Quaternion>
    {
    }

    [CustomEditor(typeof(LinearCurve<Color>)), DefaultEditor]
    sealed class ColorLinearCurveObjectEditor : LinearCurveObjectEditor<Color>
    {
    }
}
