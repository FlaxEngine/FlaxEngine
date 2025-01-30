// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using FlaxEditor.GUI;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.CustomEditors.Dedicated
{
    /// <summary>
    /// Custom editor for <see cref="BezierCurve{T}"/>.
    /// </summary>
    class BezierCurveObjectEditor<T> : CustomEditor where T : struct
    {
        private bool _isSetting;
        private int _firstTimeShow;
        private BezierCurveEditor<T> _curve;
        private Splitter _splitter;
        private string _heightCachedPath;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            var item = layout.CustomContainer<BezierCurveEditor<T>>();
            _curve = item.CustomControl;
            var height = 120.0f;
            var presenter = Presenter;
            if (presenter != null && (presenter.Features & FeatureFlags.CacheExpandedGroups) != 0)
            {
                // Try to restore curve height
                _heightCachedPath = layout.GetLayoutCachePath("Height");
                if (Editor.Instance.ProjectCache.TryGetCustomData(_heightCachedPath, out float cachedHeight) && cachedHeight > 10.0f)
                    height = cachedHeight;
            }
            _curve.Height = height;
            _curve.Edited += OnCurveEdited;
            _firstTimeShow = 4; // For some weird reason it needs several frames of warmup (probably due to sliders smoothing)
            _splitter = new Splitter
            {
                Moved = OnSplitterMoved,
                Parent = _curve,
                AnchorPreset = AnchorPresets.HorizontalStretchBottom,
                Bounds = new Rectangle(0, _curve.Height - Splitter.DefaultHeight, _curve.Width, Splitter.DefaultHeight),
            };
        }

        private void OnCurveEdited()
        {
            if (_isSetting)
                return;

            _isSetting = true;
            SetValue(new BezierCurve<T>(_curve.Keyframes));
            _isSetting = false;
        }

        private void OnSplitterMoved(Float2 location)
        {
            _curve.Height = Mathf.Clamp(_splitter.PointToParent(location).Y, 50.0f, 1000.0f);

            // Cache curve height
            if (_heightCachedPath != null)
                Editor.Instance.ProjectCache.SetCustomData(_heightCachedPath, _curve.Height);
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
            if (_firstTimeShow-- > 0)
                _curve.ShowWholeCurve();
        }

        /// <inheritdoc />
        protected override void Deinitialize()
        {
            _curve = null;
            _splitter = null;

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
        private int _firstTimeShow;
        private LinearCurveEditor<T> _curve;
        private Splitter _splitter;
        private string _heightCachedPath;

        /// <inheritdoc />
        public override void Initialize(LayoutElementsContainer layout)
        {
            var item = layout.CustomContainer<LinearCurveEditor<T>>();
            _curve = item.CustomControl;
            var height = 120.0f;
            var presenter = Presenter;
            if (presenter != null && (presenter.Features & FeatureFlags.CacheExpandedGroups) != 0)
            {
                // Try to restore curve height
                _heightCachedPath = layout.GetLayoutCachePath("Height");
                if (Editor.Instance.ProjectCache.TryGetCustomData(_heightCachedPath, out float cachedHeight) && cachedHeight > 10.0f)
                    height = cachedHeight;
            }
            _curve.Height = height;
            _curve.Edited += OnCurveEdited;
            _firstTimeShow = 4; // For some weird reason it needs several frames of warmup (probably due to sliders smoothing)
            _splitter = new Splitter
            {
                Moved = OnSplitterMoved,
                Parent = _curve,
                AnchorPreset = AnchorPresets.HorizontalStretchBottom,
                Bounds = new Rectangle(0, _curve.Height - Splitter.DefaultHeight, _curve.Width, Splitter.DefaultHeight),
            };
        }

        private void OnCurveEdited()
        {
            if (_isSetting)
                return;

            _isSetting = true;
            SetValue(new LinearCurve<T>(_curve.Keyframes));
            _isSetting = false;
        }

        private void OnSplitterMoved(Float2 location)
        {
            _curve.Height  = Mathf.Clamp(_splitter.PointToParent(location).Y, 50.0f, 1000.0f);

            // Cache curve height
            if (_heightCachedPath != null)
                Editor.Instance.ProjectCache.SetCustomData(_heightCachedPath, _curve.Height);
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
            if (_firstTimeShow-- > 0)
                _curve.ShowWholeCurve();
        }

        /// <inheritdoc />
        protected override void Deinitialize()
        {
            _curve = null;
            _splitter = null;

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
