// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using FlaxEditor.CustomEditors;
using FlaxEditor.GUI.ContextMenu;
using FlaxEngine;
using FlaxEngine.GUI;

// ReSharper disable RedundantAssignment

namespace FlaxEditor.GUI
{
    /// <summary>
    /// The base class for <see cref="CurveBase{T}"/> editors. Allows to use generic curve editor without type information at compile-time.
    /// </summary>
    [HideInEditor]
    public abstract class CurveEditorBase : ContainerControl
    {
        /// <summary>
        /// The UI use mode flags.
        /// </summary>
        [Flags]
        public enum UseMode
        {
            /// <summary>
            /// Disable usage.
            /// </summary>
            Off = 0,

            /// <summary>
            /// Allow only vertical usage.
            /// </summary>
            Vertical = 1,

            /// <summary>
            /// Allow only horizontal usage.
            /// </summary>
            Horizontal = 2,

            /// <summary>
            /// Allow both vertical and horizontal usage.
            /// </summary>
            On = Vertical | Horizontal,
        }

        /// <summary>
        /// Occurs when curve gets edited.
        /// </summary>
        public event Action Edited;

        /// <summary>
        /// Occurs when curve data editing starts (via UI).
        /// </summary>
        public event Action EditingStart;

        /// <summary>
        /// Occurs when curve data editing ends (via UI).
        /// </summary>
        public event Action EditingEnd;

        /// <summary>
        /// The maximum amount of keyframes to use in a single curve.
        /// </summary>
        public int MaxKeyframes = ushort.MaxValue;

        /// <summary>
        /// True if enable view zooming. Otherwise user won't be able to zoom in or out.
        /// </summary>
        public UseMode EnableZoom = UseMode.On;

        /// <summary>
        /// True if enable view panning. Otherwise user won't be able to move the view area.
        /// </summary>
        public UseMode EnablePanning = UseMode.On;

        /// <summary>
        /// Gets or sets the scroll bars usage.
        /// </summary>
        public abstract ScrollBars ScrollBars { get; set; }

        /// <summary>
        /// Enables drawing start/end values continuous lines.
        /// </summary>
        public bool ShowStartEndLines;

        /// <summary>
        /// Enables drawing background.
        /// </summary>
        public bool ShowBackground = true;

        /// <summary>
        /// Enables drawing time and values axes (lines and labels).
        /// </summary>
        public bool ShowAxes = true;

        /// <summary>
        /// Gets the type of the curves keyframes value.
        /// </summary>
        public abstract Type ValueType { get; }

        /// <summary>
        /// The amount of frames per second of the curve animation (optional). Can be sued to restrict the keyframes time values to the given time quantization rate.
        /// </summary>
        public abstract float? FPS { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether show curve collapsed as a list of keyframe points rather than a full curve.
        /// </summary>
        public abstract bool ShowCollapsed { get; set; }

        /// <summary>
        /// Gets or sets the view offset (via scroll bars).
        /// </summary>
        public abstract Vector2 ViewOffset { get; set; }

        /// <summary>
        /// Gets or sets the view scale.
        /// </summary>
        public abstract Vector2 ViewScale { get; set; }

        /// <summary>
        /// Gets the amount of keyframes added to the curve.
        /// </summary>
        public abstract int KeyframesCount { get; }

        /// <summary>
        /// Called when curve gets edited.
        /// </summary>
        public void OnEdited()
        {
            Edited?.Invoke();
        }

        /// <summary>
        /// Called when curve data editing starts (via UI).
        /// </summary>
        public void OnEditingStart()
        {
            EditingStart?.Invoke();
        }

        /// <summary>
        /// Called when curve data editing ends (via UI).
        /// </summary>
        public void OnEditingEnd()
        {
            EditingEnd?.Invoke();
        }

        /// <summary>
        /// Updates the keyframes positioning.
        /// </summary>
        public abstract void UpdateKeyframes();

        /// <summary>
        /// Updates the tangents positioning.
        /// </summary>
        public abstract void UpdateTangents();

        /// <summary>
        /// Evaluates the animation curve value at the specified time.
        /// </summary>
        /// <param name="result">The interpolated value from the curve at provided time.</param>
        /// <param name="time">The time to evaluate the curve at.</param>
        /// <param name="loop">If true the curve will loop when it goes past the end or beginning. Otherwise the curve value will be clamped.</param>
        public abstract void Evaluate(out object result, float time, bool loop = false);

        /// <summary>
        /// Gets the keyframes collection (as boxed objects).
        /// </summary>
        /// <returns>The array of boxed keyframe values of type <see cref="BezierCurve{T}.Keyframe"/> or <see cref="LinearCurve{T}.Keyframe"/>.</returns>
        public abstract object[] GetKeyframes();

        /// <summary>
        /// Sets the keyframes collection (as boxed objects).
        /// </summary>
        /// <param name="keyframes">The array of boxed keyframe values of type <see cref="BezierCurve{T}.Keyframe"/> or <see cref="LinearCurve{T}.Keyframe"/>.</param>
        public abstract void SetKeyframes(object[] keyframes);

        /// <summary>
        /// Adds the new keyframe (as boxed object).
        /// </summary>
        /// <param name="time">The keyframe time.</param>
        /// <param name="value">The keyframe value.</param>
        public abstract void AddKeyframe(float time, object value);

        /// <summary>
        /// Gets the keyframe data (as boxed objects).
        /// </summary>
        /// <param name="index">The keyframe index.</param>
        /// <param name="time">The keyframe time.</param>
        /// <param name="value">The keyframe value (boxed).</param>
        /// <param name="tangentIn">The keyframe 'In' tangent value (boxed).</param>
        /// <param name="tangentOut">The keyframe 'Out' tangent value (boxed).</param>
        public abstract void GetKeyframe(int index, out float time, out object value, out object tangentIn, out object tangentOut);

        /// <summary>
        /// Gets the existing keyframe value (as boxed object).
        /// </summary>
        /// <param name="index">The keyframe index.</param>
        /// <returns>The keyframe value.</returns>
        public abstract object GetKeyframe(int index);

        /// <summary>
        /// Sets the existing keyframe value (as boxed object).
        /// </summary>
        /// <param name="index">The keyframe index.</param>
        /// <param name="value">The keyframe value.</param>
        public abstract void SetKeyframeValue(int index, object value);

        /// <summary>
        /// Converts the <see cref="UseMode"/> into the <see cref="Vector2"/> mask.
        /// </summary>
        /// <param name="mode">The mode.</param>
        /// <returns>The mask.</returns>
        protected static Vector2 GetUseModeMask(UseMode mode)
        {
            return new Vector2((mode & UseMode.Horizontal) == UseMode.Horizontal ? 1.0f : 0.0f, (mode & UseMode.Vertical) == UseMode.Vertical ? 1.0f : 0.0f);
        }

        /// <summary>
        /// Filters the given value using the <see cref="UseMode"/>.
        /// </summary>
        /// <param name="mode">The mode.</param>
        /// <param name="value">The value to process.</param>
        /// <param name="defaultValue">The default value.</param>
        /// <returns>The combined value.</returns>
        protected static Vector2 ApplyUseModeMask(UseMode mode, Vector2 value, Vector2 defaultValue)
        {
            return new Vector2(
                               (mode & UseMode.Horizontal) == UseMode.Horizontal ? value.X : defaultValue.X,
                               (mode & UseMode.Vertical) == UseMode.Vertical ? value.Y : defaultValue.Y
                              );
        }
    }

    /// <summary>
    /// The generic curve editor control.
    /// </summary>
    /// <typeparam name="T">The keyframe value type.</typeparam>
    /// <seealso cref="CurveEditorBase" />
    public abstract class CurveEditor<T> : CurveEditorBase where T : new()
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
                value = curve;
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
                value = curve;
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
                return value[component];
            }

            void IKeyframeAccess<Vector2>.SetCurveValue(float curve, ref Vector2 value, int component)
            {
                value[component] = curve;
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
                return value[component];
            }

            void IKeyframeAccess<Vector3>.SetCurveValue(float curve, ref Vector3 value, int component)
            {
                value[component] = curve;
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
                return value[component];
            }

            void IKeyframeAccess<Vector4>.SetCurveValue(float curve, ref Vector4 value, int component)
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
                euler[component] = curve;
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
                value[component] = curve;
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

        private class Popup : ContextMenuBase
        {
            private CustomEditorPresenter _presenter;
            private CurveEditor<T> _editor;
            private List<int> _keyframeIndices;
            private bool _isDirty;

            public Popup(CurveEditor<T> editor, object[] selection, List<int> keyframeIndices = null, float height = 140.0f)
            : this(editor, height)
            {
                _presenter.Select(selection);
                _presenter.OpenAllGroups();
                _keyframeIndices = keyframeIndices;
                if (keyframeIndices != null && selection.Length != keyframeIndices.Count)
                    throw new Exception();
            }

            private Popup(CurveEditor<T> editor, float height)
            {
                _editor = editor;
                const float width = 340.0f;
                Size = new Vector2(width, height);
                var panel1 = new Panel(ScrollBars.Vertical)
                {
                    Bounds = new Rectangle(0, 0.0f, width, height),
                    Parent = this
                };
                _presenter = new CustomEditorPresenter(null);
                _presenter.Panel.AnchorPreset = AnchorPresets.HorizontalStretchTop;
                _presenter.Panel.IsScrollable = true;
                _presenter.Panel.Parent = panel1;
                _presenter.Modified += OnModified;
            }

            private void OnModified()
            {
                if (!_isDirty)
                {
                    _editor.OnEditingStart();
                }
                _isDirty = true;

                if (_keyframeIndices != null)
                {
                    for (int i = 0; i < _presenter.SelectionCount; i++)
                    {
                        _editor.SetKeyframeInternal(_keyframeIndices[i], _presenter.Selection[i]);
                    }
                }
                else if (_presenter.Selection[0] is IAllKeyframesProxy proxy)
                {
                    proxy.Apply();
                }

                _editor.UpdateFPS();
                _editor.UpdateKeyframes();
                _editor.UpdateTooltips();
            }

            /// <inheritdoc />
            protected override void OnShow()
            {
                Focus();

                base.OnShow();
            }

            /// <inheritdoc />
            public override void Hide()
            {
                if (!Visible)
                    return;

                Focus(null);

                if (_isDirty)
                {
                    _editor.OnEdited();
                    _editor.OnEditingEnd();
                }

                if (_editor._popup == this)
                    _editor._popup = null;
                _presenter = null;
                _editor = null;
                _keyframeIndices = null;

                base.Hide();
            }

            /// <inheritdoc />
            public override bool OnKeyDown(KeyboardKeys key)
            {
                if (base.OnKeyDown(key))
                    return true;

                if (key == KeyboardKeys.Escape)
                {
                    Hide();
                    return true;
                }

                return false;
            }
        }

        /// <summary>
        /// The curve contents container control.
        /// </summary>
        /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
        protected class ContentsBase : ContainerControl
        {
            private readonly CurveEditor<T> _editor;
            internal bool _leftMouseDown;
            private bool _rightMouseDown;
            internal Vector2 _leftMouseDownPos = Vector2.Minimum;
            private Vector2 _rightMouseDownPos = Vector2.Minimum;
            internal Vector2 _mousePos = Vector2.Minimum;
            private float _mouseMoveAmount;
            internal bool _isMovingSelection;
            internal bool _isMovingTangent;
            private TangentPoint _movingTangent;
            private Vector2 _movingSelectionViewPos;
            private Vector2 _cmShowPos;

            /// <summary>
            /// Initializes a new instance of the <see cref="ContentsBase"/> class.
            /// </summary>
            /// <param name="editor">The curve editor.</param>
            public ContentsBase(CurveEditor<T> editor)
            {
                _editor = editor;
            }

            private void UpdateSelectionRectangle()
            {
                var selectionRect = Rectangle.FromPoints(_leftMouseDownPos, _mousePos);

                // Find controls to select
                for (int i = 0; i < Children.Count; i++)
                {
                    if (Children[i] is KeyframePoint p)
                    {
                        p.IsSelected = p.Bounds.Intersects(ref selectionRect);
                    }
                }

                _editor.UpdateTangents();
            }

            /// <inheritdoc />
            public override bool IntersectsContent(ref Vector2 locationParent, out Vector2 location)
            {
                // Pass all events
                location = PointFromParent(ref locationParent);
                return true;
            }

            /// <inheritdoc />
            public override void OnMouseEnter(Vector2 location)
            {
                _mousePos = location;

                base.OnMouseEnter(location);
            }

            /// <inheritdoc />
            public override void OnMouseMove(Vector2 location)
            {
                _mousePos = location;

                // Moving view
                if (_rightMouseDown)
                {
                    // Calculate delta
                    Vector2 delta = location - _rightMouseDownPos;
                    delta *= GetUseModeMask(_editor.EnablePanning);
                    if (delta.LengthSquared > 0.01f)
                    {
                        // Move view
                        _mouseMoveAmount += delta.Length;
                        _editor.ViewOffset += delta * _editor.ViewScale;
                        _rightMouseDownPos = location;
                        Cursor = CursorType.SizeAll;
                    }

                    return;
                }
                // Moving selection
                else if (_isMovingSelection)
                {
                    // Calculate delta (apply view offset)
                    Vector2 viewDelta = _editor.ViewOffset - _movingSelectionViewPos;
                    _movingSelectionViewPos = _editor.ViewOffset;
                    var viewRect = _editor._mainPanel.GetClientArea();
                    var delta = location - _leftMouseDownPos - viewDelta;
                    _mouseMoveAmount += delta.Length;
                    if (delta.LengthSquared > 0.01f)
                    {
                        // Move selected keyframes
                        var keyframeDelta = PointToKeyframes(location, ref viewRect) - PointToKeyframes(_leftMouseDownPos - viewDelta, ref viewRect);
                        var accessor = _editor.Accessor;
                        var components = accessor.GetCurveComponents();
                        for (var i = 0; i < _editor._points.Count; i++)
                        {
                            var p = _editor._points[i];
                            if (p.IsSelected)
                            {
                                var k = _editor.GetKeyframe(p.Index);
                                float time = _editor.GetKeyframeTime(k);
                                float value = _editor.GetKeyframeValue(k, p.Component);

                                float minTime = p.Index != 0 ? _editor.GetKeyframeTime(_editor.GetKeyframe(p.Index - 1)) + Mathf.Epsilon : float.MinValue;
                                float maxTime = p.Index != _editor.KeyframesCount - 1 ? _editor.GetKeyframeTime(_editor.GetKeyframe(p.Index + 1)) - Mathf.Epsilon : float.MaxValue;

                                if (!_editor.ShowCollapsed)
                                {
                                    // Move on value axis
                                    value += keyframeDelta.Y;
                                }

                                // Let the first selected point of this keyframe to edit time
                                bool isFirstSelected = false;
                                for (var j = 0; j < components; j++)
                                {
                                    var idx = p.Index * components + j;
                                    if (idx == i)
                                    {
                                        isFirstSelected = true;
                                        break;
                                    }
                                    if (_editor._points[idx].IsSelected)
                                        break;
                                }
                                if (isFirstSelected)
                                {
                                    time += keyframeDelta.X;

                                    if (_editor.FPS.HasValue)
                                    {
                                        float fps = _editor.FPS.Value;
                                        time = Mathf.Floor(time * fps) / fps;
                                    }

                                    time = Mathf.Clamp(time, minTime, maxTime);
                                }

                                // TODO: snapping keyframes to grid when moving

                                _editor.SetKeyframeInternal(p.Index, time, value, p.Component);
                            }
                        }
                        _editor.UpdateKeyframes();
                        _editor.UpdateTooltips();
                        if (_editor.EnablePanning == UseMode.On)
                        {
                            _editor._mainPanel.ScrollViewTo(PointToParent(location));
                        }
                        _leftMouseDownPos = location;
                        Cursor = CursorType.SizeAll;
                    }

                    return;
                }
                // Moving tangent
                else if (_isMovingTangent)
                {
                    // Calculate delta (apply view offset)
                    Vector2 viewDelta = _editor.ViewOffset - _movingSelectionViewPos;
                    _movingSelectionViewPos = _editor.ViewOffset;
                    var viewRect = _editor._mainPanel.GetClientArea();
                    var delta = location - _leftMouseDownPos - viewDelta;
                    _mouseMoveAmount += delta.Length;
                    if (delta.LengthSquared > 0.01f)
                    {
                        // Move selected tangent
                        var keyframeDelta = PointToKeyframes(location, ref viewRect) - PointToKeyframes(_leftMouseDownPos - viewDelta, ref viewRect);
                        var direction = _movingTangent.IsIn ? -1.0f : 1.0f;
                        _movingTangent.TangentValue += direction * keyframeDelta.Y;
                        _editor.UpdateTangents();
                        _leftMouseDownPos = location;
                        Cursor = CursorType.SizeNS;
                    }

                    return;
                }
                // Selecting
                else if (_leftMouseDown)
                {
                    UpdateSelectionRectangle();
                    return;
                }

                base.OnMouseMove(location);
            }

            /// <inheritdoc />
            public override void OnLostFocus()
            {
                // Clear flags and state
                if (_leftMouseDown)
                {
                    _leftMouseDown = false;
                }
                if (_rightMouseDown)
                {
                    _rightMouseDown = false;
                    Cursor = CursorType.Default;
                }
                _isMovingSelection = false;
                _isMovingTangent = false;

                base.OnLostFocus();
            }

            /// <inheritdoc />
            public override bool OnMouseDown(Vector2 location, MouseButton button)
            {
                if (base.OnMouseDown(location, button))
                {
                    // Clear flags
                    _isMovingSelection = false;
                    _isMovingTangent = false;
                    _rightMouseDown = false;
                    _leftMouseDown = false;
                    return true;
                }

                // Cache data
                _isMovingSelection = false;
                _isMovingTangent = false;
                _mousePos = location;
                if (button == MouseButton.Left)
                {
                    _leftMouseDown = true;
                    _leftMouseDownPos = location;
                }
                if (button == MouseButton.Right)
                {
                    _rightMouseDown = true;
                    _rightMouseDownPos = location;
                }

                // Check if any node is under the mouse
                var underMouse = GetChildAt(location);
                if (underMouse is KeyframePoint keyframe)
                {
                    if (_leftMouseDown)
                    {
                        // Check if user is pressing control
                        if (Root.GetKey(KeyboardKeys.Control))
                        {
                            // Add to selection
                            keyframe.Select();
                            _editor.UpdateTangents();
                        }
                        // Check if node isn't selected
                        else if (!keyframe.IsSelected)
                        {
                            // Select node
                            _editor.ClearSelection();
                            keyframe.Select();
                            _editor.UpdateTangents();
                        }

                        // Start moving selected nodes
                        StartMouseCapture();
                        _mouseMoveAmount = 0;
                        _isMovingSelection = true;
                        _movingSelectionViewPos = _editor.ViewOffset;
                        _editor.OnEditingStart();
                        Focus();
                        return true;
                    }
                }
                else if (underMouse is TangentPoint tangent && tangent.Visible)
                {
                    if (_leftMouseDown)
                    {
                        // Start moving tangent
                        StartMouseCapture();
                        _mouseMoveAmount = 0;
                        _isMovingTangent = true;
                        _movingTangent = tangent;
                        _movingSelectionViewPos = _editor.ViewOffset;
                        _editor.OnEditingStart();
                        Focus();
                        return true;
                    }
                }
                else
                {
                    if (_leftMouseDown)
                    {
                        // Start selecting
                        StartMouseCapture();
                        _editor.ClearSelection();
                        _editor.UpdateTangents();
                        Focus();
                        return true;
                    }
                    if (_rightMouseDown)
                    {
                        // Start navigating
                        StartMouseCapture();
                        Focus();
                        return true;
                    }
                }

                Focus();
                return true;
            }

            /// <inheritdoc />
            public override bool OnMouseUp(Vector2 location, MouseButton button)
            {
                _mousePos = location;

                if (_leftMouseDown && button == MouseButton.Left)
                {
                    _leftMouseDown = false;
                    EndMouseCapture();
                    Cursor = CursorType.Default;

                    // Editing tangent
                    if (_isMovingTangent)
                    {
                        if (_mouseMoveAmount > 3.0f)
                        {
                            _editor.OnEdited();
                            _editor.OnEditingEnd();
                        }
                    }
                    // Moving keyframes
                    else if (_isMovingSelection)
                    {
                        if (_mouseMoveAmount > 3.0f)
                        {
                            _editor.OnEdited();
                            _editor.OnEditingEnd();
                        }
                    }
                    // Selecting
                    else
                    {
                        UpdateSelectionRectangle();
                    }

                    _isMovingSelection = false;
                    _isMovingTangent = false;
                }
                if (_rightMouseDown && button == MouseButton.Right)
                {
                    _rightMouseDown = false;
                    EndMouseCapture();
                    Cursor = CursorType.Default;

                    // Check if no move has been made at all
                    if (_mouseMoveAmount < 3.0f)
                    {
                        var selectionCount = _editor.SelectionCount;
                        var underMouse = GetChildAt(location);
                        if (selectionCount == 0 && underMouse is KeyframePoint point)
                        {
                            // Select node
                            selectionCount = 1;
                            point.Select();
                            _editor.UpdateTangents();
                        }

                        var viewRect = _editor._mainPanel.GetClientArea();
                        _cmShowPos = PointToKeyframes(location, ref viewRect);

                        var cm = new ContextMenu.ContextMenu();
                        cm.AddButton("Add keyframe", () => _editor.AddKeyframe(_cmShowPos)).Enabled = _editor.KeyframesCount < _editor.MaxKeyframes;
                        if (selectionCount == 0)
                        {
                        }
                        else if (selectionCount == 1)
                        {
                            cm.AddButton("Edit keyframe", () => _editor.EditKeyframes(this, location));
                            cm.AddButton("Remove keyframe", _editor.RemoveKeyframes);
                        }
                        else
                        {
                            cm.AddButton("Edit keyframes", () => _editor.EditKeyframes(this, location));
                            cm.AddButton("Remove keyframes", _editor.RemoveKeyframes);
                        }
                        cm.AddButton("Edit all keyframes", () => _editor.EditAllKeyframes(this, location));
                        if (_editor.EnableZoom != UseMode.Off || _editor.EnablePanning != UseMode.Off)
                        {
                            cm.AddSeparator();
                            cm.AddButton("Show whole curve", _editor.ShowWholeCurve);
                            cm.AddButton("Reset view", _editor.ResetView);
                        }
                        _editor.OnShowContextMenu(cm, selectionCount);
                        cm.Show(this, location);
                    }
                    _mouseMoveAmount = 0;
                }

                if (base.OnMouseUp(location, button))
                {
                    // Clear flags
                    _rightMouseDown = false;
                    _leftMouseDown = false;
                    return true;
                }

                return true;
            }

            /// <inheritdoc />
            public override bool OnMouseWheel(Vector2 location, float delta)
            {
                if (base.OnMouseWheel(location, delta))
                    return true;

                // Zoom in/out
                if (_editor.EnableZoom != UseMode.Off && IsMouseOver && !_leftMouseDown && RootWindow.GetKey(KeyboardKeys.Control))
                {
                    // TODO: preserve the view center point for easier zooming
                    _editor.ViewScale += GetUseModeMask(_editor.EnableZoom) * (delta * 0.1f);
                    return true;
                }

                return false;
            }

            /// <inheritdoc />
            protected override void SetScaleInternal(ref Vector2 scale)
            {
                base.SetScaleInternal(ref scale);

                _editor.UpdateKeyframes();
            }

            /// <summary>
            /// Converts the input point from curve editor contents control space into the keyframes time/value coordinates.
            /// </summary>
            /// <param name="point">The point.</param>
            /// <param name="curveContentAreaBounds">The curve contents area bounds.</param>
            /// <returns>The result.</returns>
            private Vector2 PointToKeyframes(Vector2 point, ref Rectangle curveContentAreaBounds)
            {
                // Contents -> Keyframes
                return new Vector2(
                                   (point.X + Location.X) / UnitsPerSecond,
                                   (point.Y + Location.Y - curveContentAreaBounds.Height) / -UnitsPerSecond
                                  );
            }
        }

        /// <summary>
        /// The single keyframe control.
        /// </summary>
        protected class KeyframePoint : Control
        {
            /// <summary>
            /// The parent curve editor.
            /// </summary>
            public CurveEditor<T> Editor;

            /// <summary>
            /// The keyframe index.
            /// </summary>
            public int Index;

            /// <summary>
            /// The component index.
            /// </summary>
            public int Component;

            /// <summary>
            /// Flag for selected keyframes.
            /// </summary>
            public bool IsSelected;

            /// <inheritdoc />
            public override void Draw()
            {
                var rect = new Rectangle(Vector2.Zero, Size);
                var color = Editor.ShowCollapsed ? Color.Gray : Editor.Colors[Component];
                if (IsSelected)
                    color = Editor.ContainsFocus ? Color.YellowGreen : Color.Lerp(Color.Gray, Color.YellowGreen, 0.4f);
                if (IsMouseOver)
                    color *= 1.1f;
                Render2D.FillRectangle(rect, color);
            }

            /// <inheritdoc />
            public override bool OnMouseDoubleClick(Vector2 location, MouseButton button)
            {
                if (base.OnMouseDoubleClick(location, button))
                    return true;

                if (button == MouseButton.Left)
                {
                    Editor.EditKeyframes(this, location, new List<int> { Index });
                    return true;
                }

                return false;
            }

            /// <summary>
            /// Adds this keyframe to the selection.
            /// </summary>
            public void Select()
            {
                IsSelected = true;
            }

            /// <summary>
            /// Removes this keyframe from the selection.
            /// </summary>
            public void Deselect()
            {
                IsSelected = false;
            }

            /// <summary>
            /// Updates the tooltip.
            /// </summary>
            public void UpdateTooltip()
            {
                var k = Editor.GetKeyframe(Index);
                var time = Editor.GetKeyframeTime(k);
                var value = Editor.GetKeyframeValue(k);
                if (Editor.ShowCollapsed)
                    TooltipText = string.Format("Time: {0}, Value: {1}", time, value);
                else
                    TooltipText = string.Format("Time: {0}, Value: {1}", time, Editor.Accessor.GetCurveValue(ref value, Component));
            }
        }

        /// <summary>
        /// The single keyframe tangent control.
        /// </summary>
        protected class TangentPoint : Control
        {
            /// <summary>
            /// The parent curve editor.
            /// </summary>
            public CurveEditor<T> Editor;

            /// <summary>
            /// The keyframe index.
            /// </summary>
            public int Index;

            /// <summary>
            /// The component index.
            /// </summary>
            public int Component;

            /// <summary>
            /// True if tangent is `In`, otherwise it's `Out`.
            /// </summary>
            public bool IsIn;

            /// <summary>
            /// The keyframe.
            /// </summary>
            public KeyframePoint Point;

            /// <summary>
            /// Gets the tangent value on curve.
            /// </summary>
            public float TangentValue
            {
                get => Editor.GetKeyframeTangentInternal(Index, IsIn, Component);
                set => Editor.SetKeyframeTangentInternal(Index, IsIn, Component, value);
            }

            /// <inheritdoc />
            public override void Draw()
            {
                var pointPos = PointFromParent(Point.Center);
                Render2D.DrawLine(Size * 0.5f, pointPos, Color.Gray);

                var rect = new Rectangle(Vector2.Zero, Size);
                var color = Color.MediumVioletRed;
                if (IsMouseOver)
                    color *= 1.1f;
                Render2D.FillRectangle(rect, color);
            }

            /// <summary>
            /// Updates the tooltip.
            /// </summary>
            public void UpdateTooltip()
            {
                TooltipText = string.Format("Tangent {0}: {1}", IsIn ? "in" : "out", TangentValue);
            }
        }

        /// <summary>
        /// The timeline intervals metric area size (in pixels).
        /// </summary>
        protected static readonly float LabelsSize = 10.0f;

        /// <summary>
        /// The timeline units per second (on time axis).
        /// </summary>
        public static readonly float UnitsPerSecond = 100.0f;

        /// <summary>
        /// The keyframes size.
        /// </summary>
        protected static readonly Vector2 KeyframesSize = new Vector2(5.0f);

        /// <summary>
        /// The colors for the keyframe points.
        /// </summary>
        protected Color[] Colors = Utilities.Utils.CurveKeyframesColors;

        /// <summary>
        /// The curve time/value axes tick steps.
        /// </summary>
        protected float[] TickSteps = Utilities.Utils.CurveTickSteps;

        /// <summary>
        /// The curve contents area.
        /// </summary>
        protected ContentsBase _contents;

        /// <summary>
        /// The main UI panel with scroll bars.
        /// </summary>
        protected Panel _mainPanel;

        /// <summary>
        /// True if refresh keyframes positioning before drawing.
        /// </summary>
        protected bool _refreshAfterEdit;

        /// <summary>
        /// True if curve is collapsed.
        /// </summary>
        protected bool _showCollapsed;

        private float[] _tickStrengths;
        private Popup _popup;
        private float? _fps;

        private Color _contentsColor;
        private Color _linesColor;
        private Color _labelsColor;
        private Font _labelsFont;

        /// <summary>
        /// The keyframe UI points.
        /// </summary>
        protected readonly List<KeyframePoint> _points = new List<KeyframePoint>();

        /// <summary>
        /// The tangents UI points.
        /// </summary>
        protected readonly TangentPoint[] _tangents = new TangentPoint[2];

        /// <inheritdoc />
        public override Vector2 ViewOffset
        {
            get => _mainPanel.ViewOffset;
            set => _mainPanel.ViewOffset = value;
        }

        /// <inheritdoc />
        public override Vector2 ViewScale
        {
            get => _contents.Scale;
            set => _contents.Scale = Vector2.Clamp(value, new Vector2(0.0001f), new Vector2(1000.0f));
        }

        /// <summary>
        /// The keyframes data accessor.
        /// </summary>
        public readonly IKeyframeAccess<T> Accessor = new KeyframeAccess() as IKeyframeAccess<T>;

        /// <summary>
        /// Gets a value indicating whether user is editing the curve.
        /// </summary>
        public bool IsUserEditing => _popup != null || _contents._leftMouseDown;

        /// <summary>
        /// The default value.
        /// </summary>
        public T DefaultValue;

        /// <inheritdoc />
        public override ScrollBars ScrollBars
        {
            get => _mainPanel.ScrollBars;
            set => _mainPanel.ScrollBars = value;
        }

        /// <inheritdoc />
        public override Type ValueType => typeof(T);

        /// <inheritdoc />
        public override float? FPS
        {
            get => _fps;
            set
            {
                if (_fps.HasValue == value.HasValue && (!value.HasValue || Mathf.NearEqual(_fps.Value, value.Value)))
                    return;

                _fps = value;
                UpdateFPS();
            }
        }

        /// <inheritdoc />
        public override bool ShowCollapsed
        {
            get => _showCollapsed;
            set
            {
                if (_showCollapsed == value)
                    return;

                _showCollapsed = value;
                UpdateKeyframes();
                UpdateTangents();
                if (value)
                    ShowWholeCurve();
            }
        }

        /// <summary>
        /// Occurs when keyframes collection gets changed (keyframe added or removed).
        /// </summary>
        public event Action KeyframesChanged;

        /// <summary>
        /// Initializes a new instance of the <see cref="CurveEditor{T}"/> class.
        /// </summary>
        protected CurveEditor()
        {
            _tickStrengths = new float[TickSteps.Length];
            Accessor.GetDefaultValue(out DefaultValue);

            var style = Style.Current;
            _contentsColor = style.Background.RGBMultiplied(0.7f);
            _linesColor = style.ForegroundDisabled.RGBMultiplied(0.7f);
            _labelsColor = style.ForegroundDisabled;
            _labelsFont = style.FontSmall;

            _mainPanel = new Panel(ScrollBars.Both)
            {
                ScrollMargin = new Margin(150.0f),
                AlwaysShowScrollbars = true,
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                Parent = this
            };
            _contents = new ContentsBase(this)
            {
                ClipChildren = false,
                CullChildren = false,
                AutoFocus = false,
                Parent = _mainPanel,
                Bounds = Rectangle.Empty,
            };
        }

        /// <summary>
        /// Updates the keyframes to match the FPS.
        /// </summary>
        protected abstract void UpdateFPS();

        /// <summary>
        /// Updates the keyframes tooltips.
        /// </summary>
        protected void UpdateTooltips()
        {
            for (var i = 0; i < _points.Count; i++)
                _points[i].UpdateTooltip();
        }

        /// <summary>
        /// Called when keyframes collection gets changed (keyframe added or removed).
        /// </summary>
        protected virtual void OnKeyframesChanged()
        {
            KeyframesChanged?.Invoke();
        }

        /// <summary>
        /// Evaluates the animation curve value at the specified time.
        /// </summary>
        /// <param name="result">The interpolated value from the curve at provided time.</param>
        /// <param name="time">The time to evaluate the curve at.</param>
        /// <param name="loop">If true the curve will loop when it goes past the end or beginning. Otherwise the curve value will be clamped.</param>
        public abstract void Evaluate(out T result, float time, bool loop = false);

        /// <summary>
        /// Gets the time of the keyframe
        /// </summary>
        /// <param name="keyframe">The keyframe object.</param>
        /// <returns>The keyframe time.</returns>
        protected abstract float GetKeyframeTime(object keyframe);

        /// <summary>
        /// Gets the value of the keyframe.
        /// </summary>
        /// <param name="keyframe">The keyframe object.</param>
        /// <returns>The keyframe value.</returns>
        protected abstract T GetKeyframeValue(object keyframe);

        /// <summary>
        /// Gets the value of the keyframe (single component).
        /// </summary>
        /// <param name="keyframe">The keyframe object.</param>
        /// <param name="component">The keyframe value component index.</param>
        /// <returns>The keyframe component value.</returns>
        protected abstract float GetKeyframeValue(object keyframe, int component);

        /// <summary>
        /// Adds a new keyframe at the given location (in keyframes space).
        /// </summary>
        /// <param name="keyframesPos">The new keyframe position (in keyframes space).</param>
        protected abstract void AddKeyframe(Vector2 keyframesPos);

        /// <summary>
        /// Sets the keyframe data (internally).
        /// </summary>
        /// <param name="index">The keyframe index.</param>
        /// <param name="keyframe">The keyframe to set.</param>
        protected abstract void SetKeyframeInternal(int index, object keyframe);

        /// <summary>
        /// Sets the keyframe data (internally).
        /// </summary>
        /// <param name="index">The keyframe index.</param>
        /// <param name="time">The time to set.</param>
        /// <param name="value">The value to set.</param>
        /// <param name="component">The value component.</param>
        protected abstract void SetKeyframeInternal(int index, float time, float value, int component);

        /// <summary>
        /// Gets the keyframe tangent value (internally).
        /// </summary>
        /// <param name="index">The keyframe index.</param>
        /// <param name="isIn">True if tangent is `In`, otherwise it's `Out`.</param>
        /// <param name="component">The value component index.</param>
        /// <returns>The tangent component value.</returns>
        protected abstract float GetKeyframeTangentInternal(int index, bool isIn, int component);

        /// <summary>
        /// Sets the keyframe tangent value (internally).
        /// </summary>
        /// <param name="index">The keyframe index.</param>
        /// <param name="isIn">True if tangent is `In`, otherwise it's `Out`.</param>
        /// <param name="component">The value component index.</param>
        /// <param name="value">The tangent component value.</param>
        protected abstract void SetKeyframeTangentInternal(int index, bool isIn, int component, float value);

        /// <summary>
        /// Removes the keyframes data (internally).
        /// </summary>
        /// <param name="indicesToRemove">The list of indices of the keyframes to remove.</param>
        protected abstract void RemoveKeyframesInternal(HashSet<int> indicesToRemove);

        /// <summary>
        /// Called when showing a context menu. Can be used to add custom buttons with actions.
        /// </summary>
        /// <param name="cm">The menu.</param>
        /// <param name="selectionCount">The amount of selected keyframes.</param>
        protected virtual void OnShowContextMenu(ContextMenu.ContextMenu cm, int selectionCount)
        {
        }

        /// <summary>
        /// Gets proxy object for all keyframes list.
        /// </summary>
        /// <returns>The proxy object.</returns>
        protected abstract IAllKeyframesProxy GetAllKeyframesEditingProxy();

        /// <summary>
        /// Interface for keyframes editing proxy objects.
        /// </summary>
        protected interface IAllKeyframesProxy
        {
            /// <summary>
            /// Applies the proxy data to the editor.
            /// </summary>
            void Apply();
        }

        private void EditAllKeyframes(Control control, Vector2 pos)
        {
            _popup = new Popup(this, new object[] { GetAllKeyframesEditingProxy() }, null, 400.0f);
            _popup.Show(control, pos);
        }

        private void EditKeyframes(Control control, Vector2 pos)
        {
            var keyframeIndices = new List<int>();
            for (int i = 0; i < _points.Count; i++)
            {
                var p = _points[i];
                if (!p.IsSelected || keyframeIndices.Contains(p.Index))
                    continue;
                keyframeIndices.Add(p.Index);
            }
            EditKeyframes(control, pos, keyframeIndices);
        }

        private void EditKeyframes(Control control, Vector2 pos, List<int> keyframeIndices)
        {
            var selection = new object[keyframeIndices.Count];
            var keyframes = GetKeyframes();
            for (int i = 0; i < keyframeIndices.Count; i++)
                selection[i] = keyframes[keyframeIndices[i]];
            _popup = new Popup(this, selection, keyframeIndices);
            _popup.Show(control, pos);
        }

        private void RemoveKeyframes()
        {
            var indicesToRemove = new HashSet<int>();
            for (int i = 0; i < _points.Count; i++)
            {
                var p = _points[i];
                if (p.IsSelected)
                {
                    p.Deselect();
                    indicesToRemove.Add(p.Index);
                }
            }
            if (indicesToRemove.Count == 0)
                return;

            OnEditingStart();
            RemoveKeyframesInternal(indicesToRemove);
            OnKeyframesChanged();
            OnEdited();
            OnEditingEnd();
        }

        /// <summary>
        /// Shows the whole curve.
        /// </summary>
        public void ShowWholeCurve()
        {
            ViewScale = ApplyUseModeMask(EnableZoom, _mainPanel.Size / _contents.Size, ViewScale);
            ViewOffset = ApplyUseModeMask(EnablePanning, -_mainPanel.ControlsBounds.Location, ViewOffset);
            UpdateKeyframes();
        }

        /// <summary>
        /// Resets the view.
        /// </summary>
        public void ResetView()
        {
            ViewScale = ApplyUseModeMask(EnableZoom, Vector2.One, ViewScale);
            ViewOffset = ApplyUseModeMask(EnablePanning, Vector2.Zero, ViewOffset);
            UpdateKeyframes();
        }

        /// <inheritdoc />
        public override void Evaluate(out object result, float time, bool loop = false)
        {
            Evaluate(out var value, time, loop);
            result = value;
        }

        private int SelectionCount
        {
            get
            {
                int result = 0;
                for (int i = 0; i < _points.Count; i++)
                    if (_points[i].IsSelected)
                        result++;
                return result;
            }
        }

        private void ClearSelection()
        {
            for (int i = 0; i < _points.Count; i++)
            {
                _points[i].Deselect();
            }
        }

        private void SelectAll()
        {
            for (int i = 0; i < _points.Count; i++)
            {
                _points[i].Select();
            }
        }

        /// <summary>
        /// Converts the input point from curve editor control space into the keyframes time/value coordinates.
        /// </summary>
        /// <param name="point">The point.</param>
        /// <param name="curveContentAreaBounds">The curve contents area bounds.</param>
        /// <returns>The result.</returns>
        protected Vector2 PointToKeyframes(Vector2 point, ref Rectangle curveContentAreaBounds)
        {
            // Curve Editor -> Main Panel
            point = _mainPanel.PointFromParent(point);

            // Main Panel -> Contents
            point = _contents.PointFromParent(point);

            // Contents -> Keyframes
            return new Vector2(
                               (point.X + _contents.Location.X) / UnitsPerSecond,
                               (point.Y + _contents.Location.Y - curveContentAreaBounds.Height) / -UnitsPerSecond
                              );
        }

        /// <summary>
        /// Converts the input point from the keyframes time/value coordinates into the curve editor control space.
        /// </summary>
        /// <param name="point">The point.</param>
        /// <param name="curveContentAreaBounds">The curve contents area bounds.</param>
        /// <returns>The result.</returns>
        protected Vector2 PointFromKeyframes(Vector2 point, ref Rectangle curveContentAreaBounds)
        {
            // Keyframes -> Contents
            point = new Vector2(
                                point.X * UnitsPerSecond - _contents.Location.X,
                                point.Y * -UnitsPerSecond + curveContentAreaBounds.Height - _contents.Location.Y
                               );

            // Contents -> Main Panel
            point = _contents.PointToParent(point);

            // Main Panel -> Curve Editor
            return _mainPanel.PointToParent(point);
        }

        private void DrawAxis(Vector2 axis, ref Rectangle viewRect, float min, float max, float pixelRange)
        {
            int minDistanceBetweenTicks = 20;
            int maxDistanceBetweenTicks = 60;
            var range = max - min;

            // Find the strength for each modulo number tick marker
            int smallestTick = 0;
            int biggestTick = TickSteps.Length - 1;
            for (int i = TickSteps.Length - 1; i >= 0; i--)
            {
                // Calculate how far apart these modulo tick steps are spaced
                float tickSpacing = TickSteps[i] * pixelRange / range;

                // Calculate the strength of the tick markers based on the spacing
                _tickStrengths[i] = Mathf.Saturate((tickSpacing - minDistanceBetweenTicks) / (maxDistanceBetweenTicks - minDistanceBetweenTicks));

                // Beyond threshold the ticks don't get any bigger or fatter
                if (_tickStrengths[i] >= 1)
                    biggestTick = i;

                // Do not show small tick markers
                if (tickSpacing <= minDistanceBetweenTicks)
                {
                    smallestTick = i;
                    break;
                }
            }

            // Draw all tick levels
            int tickLevels = biggestTick - smallestTick + 1;
            for (int level = 0; level < tickLevels; level++)
            {
                float strength = _tickStrengths[smallestTick + level];
                if (strength <= Mathf.Epsilon)
                    continue;

                // Draw all ticks
                int l = Mathf.Clamp(smallestTick + level, 0, TickSteps.Length - 1);
                int startTick = Mathf.FloorToInt(min / TickSteps[l]);
                int endTick = Mathf.CeilToInt(max / TickSteps[l]);
                for (int i = startTick; i <= endTick; i++)
                {
                    if (l < biggestTick && (i % Mathf.RoundToInt(TickSteps[l + 1] / TickSteps[l]) == 0))
                        continue;

                    var tick = i * TickSteps[l];
                    var p = PointFromKeyframes(axis * tick, ref viewRect);

                    // Draw line
                    var lineRect = new Rectangle
                    (
                     viewRect.Location + (p - 0.5f) * axis,
                     Vector2.Lerp(viewRect.Size, Vector2.One, axis)
                    );
                    Render2D.FillRectangle(lineRect, _linesColor.AlphaMultiplied(strength));

                    // Draw label
                    string label = tick.ToString(CultureInfo.InvariantCulture);
                    var labelRect = new Rectangle
                    (
                     viewRect.X + 4.0f + (p.X * axis.X),
                     viewRect.Y - LabelsSize + (p.Y * axis.Y) + (viewRect.Size.Y * axis.X),
                     50,
                     LabelsSize
                    );
                    Render2D.DrawText(_labelsFont, label, labelRect, _labelsColor.AlphaMultiplied(strength), TextAlignment.Near, TextAlignment.Center, TextWrapping.NoWrap, 1.0f, 0.7f);
                }
            }
        }

        /// <summary>
        /// Draws the curve.
        /// </summary>
        /// <param name="viewRect">The main panel client area used as a view bounds.</param>
        protected abstract void DrawCurve(ref Rectangle viewRect);

        /// <inheritdoc />
        public override void Draw()
        {
            // Hack to refresh UI after keyframes edit
            if (_refreshAfterEdit)
            {
                _refreshAfterEdit = false;
                UpdateKeyframes();
            }

            var style = Style.Current;
            var rect = new Rectangle(Vector2.Zero, Size);
            var viewRect = _mainPanel.GetClientArea();

            // Draw background
            if (ShowBackground)
            {
                Render2D.FillRectangle(rect, _contentsColor);
            }

            // Draw time and values axes
            if (ShowAxes)
            {
                var upperLeft = PointToKeyframes(viewRect.Location, ref viewRect);
                var bottomRight = PointToKeyframes(viewRect.Size, ref viewRect);

                var min = Vector2.Min(upperLeft, bottomRight);
                var max = Vector2.Max(upperLeft, bottomRight);
                var pixelRange = (max - min) * ViewScale * UnitsPerSecond;

                Render2D.PushClip(ref viewRect);

                DrawAxis(Vector2.UnitX, ref viewRect, min.X, max.X, pixelRange.X);
                DrawAxis(Vector2.UnitY, ref viewRect, min.Y, max.Y, pixelRange.Y);

                Render2D.PopClip();
            }

            // Draw curve
            if (!_showCollapsed)
            {
                Render2D.PushClip(ref rect);
                DrawCurve(ref viewRect);
                Render2D.PopClip();
            }

            // Draw selection rectangle
            if (_contents._leftMouseDown && !_contents._isMovingSelection && !_contents._isMovingTangent)
            {
                var selectionRect = Rectangle.FromPoints
                (
                 _mainPanel.PointToParent(_contents.PointToParent(_contents._leftMouseDownPos)),
                 _mainPanel.PointToParent(_contents.PointToParent(_contents._mousePos))
                );
                Render2D.FillRectangle(selectionRect, Color.Orange * 0.4f);
                Render2D.DrawRectangle(selectionRect, Color.Orange);
            }

            base.Draw();

            // Draw border
            if (ContainsFocus)
            {
                Render2D.DrawRectangle(rect, style.BackgroundSelected);
            }
        }

        /// <inheritdoc />
        protected override void OnSizeChanged()
        {
            base.OnSizeChanged();

            UpdateKeyframes();
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (base.OnKeyDown(key))
                return true;

            if (key == KeyboardKeys.Delete)
            {
                RemoveKeyframes();
                return true;
            }

            if (Root.GetKey(KeyboardKeys.Control))
            {
                switch (key)
                {
                case KeyboardKeys.A:
                    SelectAll();
                    UpdateTangents();
                    return true;
                }
            }

            return false;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _mainPanel = null;
            _contents = null;
            _popup = null;
            _points.Clear();
            _labelsFont = null;
            Colors = null;
            DefaultValue = default;
            TickSteps = null;
            _tickStrengths = null;
            KeyframesChanged = null;

            base.OnDestroy();
        }
    }

    /// <summary>
    /// The linear curve editor control.
    /// </summary>
    /// <typeparam name="T">The keyframe value type.</typeparam>
    /// <seealso cref="LinearCurve{T}"/>
    /// <seealso cref="CurveEditor{T}"/>
    /// <seealso cref="CurveEditorBase" />
    public class LinearCurveEditor<T> : CurveEditor<T> where T : new()
    {
        /// <summary>
        /// The keyframes collection.
        /// </summary>
        protected List<LinearCurve<T>.Keyframe> _keyframes = new List<LinearCurve<T>.Keyframe>();

        /// <summary>
        /// Gets the keyframes collection (read-only).
        /// </summary>
        public IReadOnlyList<LinearCurve<T>.Keyframe> Keyframes => _keyframes;

        /// <summary>
        /// Initializes a new instance of the <see cref="LinearCurveEditor{T}"/> class.
        /// </summary>
        public LinearCurveEditor()
        {
            for (int i = 0; i < _tangents.Length; i++)
            {
                _tangents[i] = new TangentPoint
                {
                    AutoFocus = false,
                    Size = KeyframesSize,
                    Editor = this,
                    Component = i / 2,
                    Parent = _contents,
                    Visible = false,
                    IsIn = false,
                };
            }
            for (int i = 0; i < _tangents.Length; i += 2)
            {
                _tangents[i].IsIn = true;
            }
        }

        /// <summary>
        /// Adds the new keyframe.
        /// </summary>
        /// <param name="k">The keyframe to add.</param>
        public void AddKeyframe(LinearCurve<T>.Keyframe k)
        {
            if (FPS.HasValue)
            {
                float fps = FPS.Value;
                k.Time = Mathf.Floor(k.Time * fps) / fps;
            }
            int pos = 0;
            while (pos < _keyframes.Count && _keyframes[pos].Time < k.Time)
                pos++;
            _keyframes.Insert(pos, k);

            OnKeyframesChanged();
            OnEdited();
        }

        /// <summary>
        /// Sets the keyframes collection.
        /// </summary>
        /// <param name="keyframes">The keyframes.</param>
        public void SetKeyframes(IEnumerable<LinearCurve<T>.Keyframe> keyframes)
        {
            if (keyframes == null)
                throw new ArgumentNullException(nameof(keyframes));
            var keyframesArray = keyframes as LinearCurve<T>.Keyframe[] ?? keyframes.ToArray();
            if (_keyframes.SequenceEqual(keyframesArray))
                return;
            if (keyframesArray.Length > MaxKeyframes)
            {
                var tmp = keyframesArray;
                keyframesArray = new LinearCurve<T>.Keyframe[MaxKeyframes];
                Array.Copy(tmp, keyframesArray, MaxKeyframes);
            }

            _keyframes.Clear();
            _keyframes.AddRange(keyframesArray);
            _keyframes.Sort((a, b) => a.Time.CompareTo(b.Time));

            UpdateFPS();
            OnKeyframesChanged();
        }

        /// <inheritdoc />
        protected override void UpdateFPS()
        {
            if (FPS.HasValue)
            {
                float fps = FPS.Value;
                for (int i = 0; i < _keyframes.Count; i++)
                {
                    var k = _keyframes[i];
                    k.Time = Mathf.Floor(k.Time * fps) / fps;
                    _keyframes[i] = k;
                }
            }
        }

        /// <summary>
        /// Gets the keyframe point (in keyframes space).
        /// </summary>
        /// <param name="k">The keyframe.</param>
        /// <param name="component">The keyframe value component index.</param>
        /// <returns>The point in time/value space.</returns>
        private Vector2 GetKeyframePoint(ref LinearCurve<T>.Keyframe k, int component)
        {
            return new Vector2(k.Time, Accessor.GetCurveValue(ref k.Value, component));
        }

        private void DrawLine(LinearCurve<T>.Keyframe startK, LinearCurve<T>.Keyframe endK, int component, ref Rectangle viewRect)
        {
            var start = GetKeyframePoint(ref startK, component);
            var end = GetKeyframePoint(ref endK, component);

            var p1 = PointFromKeyframes(start, ref viewRect);
            var p2 = PointFromKeyframes(end, ref viewRect);

            var color = Colors[component].RGBMultiplied(0.6f);
            Render2D.DrawLine(p1, p2, color, 1.6f);
        }

        /// <inheritdoc />
        protected override void OnKeyframesChanged()
        {
            var components = Accessor.GetCurveComponents();
            while (_points.Count > _keyframes.Count * components)
            {
                var last = _points.Count - 1;
                _points[last].Dispose();
                _points.RemoveAt(last);
            }

            while (_points.Count < _keyframes.Count * components)
            {
                _points.Add(new KeyframePoint
                {
                    AutoFocus = false,
                    Size = KeyframesSize,
                    Editor = this,
                    Index = _points.Count / components,
                    Component = _points.Count % components,
                    Parent = _contents,
                });

                _refreshAfterEdit = true;
            }

            UpdateKeyframes();
            UpdateTooltips();

            base.OnKeyframesChanged();
        }

        /// <inheritdoc />
        public override void Evaluate(out T result, float time, bool loop = false)
        {
            var curve = new LinearCurve<T>
            {
                Keyframes = _keyframes.ToArray()
            };
            curve.Evaluate(out result, time, loop);
        }

        /// <inheritdoc />
        protected override float GetKeyframeTime(object keyframe)
        {
            return ((LinearCurve<T>.Keyframe)keyframe).Time;
        }

        /// <inheritdoc />
        protected override T GetKeyframeValue(object keyframe)
        {
            return ((LinearCurve<T>.Keyframe)keyframe).Value;
        }

        /// <inheritdoc />
        protected override float GetKeyframeValue(object keyframe, int component)
        {
            var value = ((LinearCurve<T>.Keyframe)keyframe).Value;
            return Accessor.GetCurveValue(ref value, component);
        }

        /// <inheritdoc />
        protected override void AddKeyframe(Vector2 keyframesPos)
        {
            var k = new LinearCurve<T>.Keyframe
            {
                Time = keyframesPos.X,
            };
            var components = Accessor.GetCurveComponents();
            for (int component = 0; component < components; component++)
            {
                Accessor.SetCurveValue(keyframesPos.Y, ref k.Value, component);
            }
            OnEditingStart();
            AddKeyframe(k);
            OnEditingEnd();
        }

        /// <inheritdoc />
        protected override void SetKeyframeInternal(int index, object keyframe)
        {
            _keyframes[index] = (LinearCurve<T>.Keyframe)keyframe;
        }

        /// <inheritdoc />
        protected override void SetKeyframeInternal(int index, float time, float value, int component)
        {
            var k = _keyframes[index];
            k.Time = time;
            Accessor.SetCurveValue(value, ref k.Value, component);
            _keyframes[index] = k;
        }

        /// <inheritdoc />
        protected override float GetKeyframeTangentInternal(int index, bool isIn, int component)
        {
            return 0.0f;
        }

        /// <inheritdoc />
        protected override void SetKeyframeTangentInternal(int index, bool isIn, int component, float value)
        {
        }

        /// <inheritdoc />
        protected override void RemoveKeyframesInternal(HashSet<int> indicesToRemove)
        {
            var keyframes = _keyframes;
            _keyframes = new List<LinearCurve<T>.Keyframe>();
            for (int i = 0; i < keyframes.Count; i++)
            {
                if (!indicesToRemove.Contains(i))
                    _keyframes.Add(keyframes[i]);
            }
        }

        sealed class AllKeyframesProxy : IAllKeyframesProxy
        {
            [HideInEditor, NoSerialize]
            public LinearCurveEditor<T> Editor;

            [Collection(CanReorderItems = false, Spacing = 10)]
            public LinearCurve<T>.Keyframe[] Keyframes;

            public void Apply()
            {
                Editor.SetKeyframes(Keyframes);
            }
        }

        /// <inheritdoc />
        protected override IAllKeyframesProxy GetAllKeyframesEditingProxy()
        {
            return new AllKeyframesProxy
            {
                Editor = this,
                Keyframes = _keyframes.ToArray(),
            };
        }

        /// <inheritdoc />
        public override object[] GetKeyframes()
        {
            var keyframes = new object[_keyframes.Count];
            for (int i = 0; i < keyframes.Length; i++)
                keyframes[i] = _keyframes[i];
            return keyframes;
        }

        /// <inheritdoc />
        public override void SetKeyframes(object[] keyframes)
        {
            var data = new LinearCurve<T>.Keyframe[keyframes.Length];
            for (int i = 0; i < keyframes.Length; i++)
            {
                if (keyframes[i] is LinearCurve<T>.Keyframe asT)
                    data[i] = asT;
                else if (keyframes[i] is LinearCurve<object>.Keyframe asObj)
                    data[i] = new LinearCurve<T>.Keyframe(asObj.Time, (T)asObj.Value);
            }
            SetKeyframes(data);
        }

        /// <inheritdoc />
        public override void AddKeyframe(float time, object value)
        {
            AddKeyframe(new LinearCurve<T>.Keyframe(time, (T)value));
        }

        /// <inheritdoc />
        public override void GetKeyframe(int index, out float time, out object value, out object tangentIn, out object tangentOut)
        {
            var k = _keyframes[index];
            time = k.Time;
            value = k.Value;
            tangentIn = tangentOut = 0.0f;
        }

        /// <inheritdoc />
        public override object GetKeyframe(int index)
        {
            return _keyframes[index];
        }

        /// <inheritdoc />
        public override void SetKeyframeValue(int index, object value)
        {
            var k = _keyframes[index];
            k.Value = (T)value;
            _keyframes[index] = k;

            UpdateKeyframes();
            UpdateTooltips();
            OnEdited();
        }

        /// <inheritdoc />
        public override int KeyframesCount => _keyframes.Count;

        /// <inheritdoc />
        public override void UpdateKeyframes()
        {
            if (_points.Count == 0)
            {
                // No keyframes
                _contents.Bounds = Rectangle.Empty;
                return;
            }

            var wasLocked = _mainPanel.IsLayoutLocked;
            _mainPanel.IsLayoutLocked = true;

            // Place keyframes
            Rectangle curveContentAreaBounds = _mainPanel.GetClientArea();
            var viewScale = ViewScale;
            for (int i = 0; i < _points.Count; i++)
            {
                var p = _points[i];
                var k = _keyframes[p.Index];

                var location = GetKeyframePoint(ref k, p.Component);
                var point = new Vector2
                (
                 location.X * UnitsPerSecond - p.Width * 0.5f,
                 location.Y * -UnitsPerSecond - p.Height * 0.5f + curveContentAreaBounds.Height
                );

                if (_showCollapsed)
                {
                    point.Y = 1.0f;
                    p.Size = new Vector2(4.0f / viewScale.X, Height - 2.0f);
                    p.Visible = p.Component == 0;
                }
                else
                {
                    p.Size = KeyframesSize / viewScale;
                    p.Visible = true;
                }
                p.Location = point;
            }

            // Calculate bounds
            var bounds = _points[0].Bounds;
            for (var i = 1; i < _points.Count; i++)
            {
                bounds = Rectangle.Union(bounds, _points[i].Bounds);
            }

            // Adjust contents bounds to fill the curve area
            if (EnablePanning != UseMode.Off)
            {
                bounds.Width = Mathf.Max(bounds.Width, 1.0f);
                bounds.Height = Mathf.Max(bounds.Height, 1.0f);
                bounds.Location = ApplyUseModeMask(EnablePanning, bounds.Location, _contents.Location);
                bounds.Size = ApplyUseModeMask(EnablePanning, bounds.Size, _contents.Size);
                _contents.Bounds = bounds;
            }
            else if (_contents.Bounds == Rectangle.Empty)
            {
                _contents.Bounds = Rectangle.Union(bounds, new Rectangle(Vector2.Zero, Vector2.One));
            }

            // Offset the keyframes (parent container changed its location)
            var posOffset = _contents.Location;
            for (var i = 0; i < _points.Count; i++)
            {
                _points[i].Location -= posOffset;
            }

            UpdateTangents();

            if (!wasLocked)
                _mainPanel.UnlockChildrenRecursive();
            _mainPanel.PerformLayout();
        }

        /// <inheritdoc />
        public override void UpdateTangents()
        {
            for (int i = 0; i < _tangents.Length; i++)
                _tangents[i].Visible = false;
        }

        /// <inheritdoc />
        protected override void DrawCurve(ref Rectangle viewRect)
        {
            var components = Accessor.GetCurveComponents();
            for (int component = 0; component < components; component++)
            {
                if (ShowStartEndLines)
                {
                    var start = new LinearCurve<T>.Keyframe
                    {
                        Value = DefaultValue,
                        Time = -10000000.0f,
                    };
                    var end = new LinearCurve<T>.Keyframe
                    {
                        Value = DefaultValue,
                        Time = 10000000.0f,
                    };

                    if (_keyframes.Count == 0)
                    {
                        DrawLine(start, end, component, ref viewRect);
                    }
                    else
                    {
                        DrawLine(start, _keyframes[0], component, ref viewRect);
                        DrawLine(_keyframes[_keyframes.Count - 1], end, component, ref viewRect);
                    }
                }

                var color = Colors[component];
                for (int i = 1; i < _keyframes.Count; i++)
                {
                    var startK = _keyframes[i - 1];
                    var endK = _keyframes[i];

                    var start = GetKeyframePoint(ref startK, component);
                    var end = GetKeyframePoint(ref endK, component);

                    var p1 = PointFromKeyframes(start, ref viewRect);
                    var p2 = PointFromKeyframes(end, ref viewRect);

                    Render2D.DrawLine(p1, p2, color);
                }
            }
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _keyframes.Clear();

            base.OnDestroy();
        }
    }

    /// <summary>
    /// The Bezier curve editor control.
    /// </summary>
    /// <typeparam name="T">The keyframe value type.</typeparam>
    /// <seealso cref="BezierCurve{T}"/>
    /// <seealso cref="CurveEditor{T}"/>
    /// <seealso cref="CurveEditorBase" />
    public class BezierCurveEditor<T> : CurveEditor<T> where T : new()
    {
        /// <summary>
        /// The keyframes collection.
        /// </summary>
        protected List<BezierCurve<T>.Keyframe> _keyframes = new List<BezierCurve<T>.Keyframe>();

        /// <summary>
        /// Gets the keyframes collection (read-only).
        /// </summary>
        public IReadOnlyList<BezierCurve<T>.Keyframe> Keyframes => _keyframes;

        /// <summary>
        /// Initializes a new instance of the <see cref="BezierCurveEditor{T}"/> class.
        /// </summary>
        public BezierCurveEditor()
        {
            for (int i = 0; i < _tangents.Length; i++)
            {
                _tangents[i] = new TangentPoint
                {
                    AutoFocus = false,
                    Size = KeyframesSize,
                    Editor = this,
                    Component = i / 2,
                    Parent = _contents,
                    Visible = false,
                    IsIn = false,
                };
            }
            for (int i = 0; i < _tangents.Length; i += 2)
            {
                _tangents[i].IsIn = true;
            }
        }

        /// <summary>
        /// Adds the new keyframe.
        /// </summary>
        /// <param name="k">The keyframe to add.</param>
        public void AddKeyframe(BezierCurve<T>.Keyframe k)
        {
            if (FPS.HasValue)
            {
                float fps = FPS.Value;
                k.Time = Mathf.Floor(k.Time * fps) / fps;
            }
            int pos = 0;
            while (pos < _keyframes.Count && _keyframes[pos].Time < k.Time)
                pos++;
            _keyframes.Insert(pos, k);

            OnKeyframesChanged();
            OnEdited();
        }

        /// <summary>
        /// Sets the keyframes collection.
        /// </summary>
        /// <param name="keyframes">The keyframes.</param>
        public void SetKeyframes(IEnumerable<BezierCurve<T>.Keyframe> keyframes)
        {
            if (keyframes == null)
                throw new ArgumentNullException(nameof(keyframes));
            var keyframesArray = keyframes as BezierCurve<T>.Keyframe[] ?? keyframes.ToArray();
            if (_keyframes.SequenceEqual(keyframesArray))
                return;
            if (keyframesArray.Length > MaxKeyframes)
            {
                var tmp = keyframesArray;
                keyframesArray = new BezierCurve<T>.Keyframe[MaxKeyframes];
                Array.Copy(tmp, keyframesArray, MaxKeyframes);
            }

            _keyframes.Clear();
            _keyframes.AddRange(keyframesArray);
            _keyframes.Sort((a, b) => a.Time.CompareTo(b.Time));

            UpdateFPS();

            OnKeyframesChanged();
        }

        private void ResetTangents()
        {
            bool edited = false;
            for (int i = 0; i < _points.Count; i++)
            {
                var p = _points[i];
                if (!p.IsSelected)
                    continue;
                if (!edited)
                    OnEditingStart();
                edited = true;

                var k = _keyframes[p.Index];

                if (p.Index > 0)
                {
                    Accessor.SetCurveValue(0.0f, ref k.TangentIn, p.Component);
                }

                if (p.Index < _keyframes.Count - 1)
                {
                    Accessor.SetCurveValue(0.0f, ref k.TangentOut, p.Component);
                }

                _keyframes[p.Index] = k;
            }
            if (!edited)
                return;

            UpdateTangents();
            OnEdited();
            OnEditingEnd();
        }

        private void SetTangentsLinear()
        {
            bool edited = false;
            for (int i = 0; i < _points.Count; i++)
            {
                var p = _points[i];
                if (!p.IsSelected)
                    continue;
                if (!edited)
                    OnEditingStart();
                edited = true;

                var k = _keyframes[p.Index];
                var value = Accessor.GetCurveValue(ref k.Value, p.Component);

                if (p.Index > 0)
                {
                    var o = _keyframes[p.Index - 1];
                    var oValue = Accessor.GetCurveValue(ref o.Value, p.Component);
                    var slope = (value - oValue) / (k.Time - o.Time);
                    Accessor.SetCurveValue(slope, ref k.TangentIn, p.Component);
                }

                if (p.Index < _keyframes.Count - 1)
                {
                    var o = _keyframes[p.Index + 1];
                    var oValue = Accessor.GetCurveValue(ref o.Value, p.Component);
                    var slope = (oValue - value) / (o.Time - k.Time);
                    Accessor.SetCurveValue(slope, ref k.TangentOut, p.Component);
                }

                _keyframes[p.Index] = k;
            }
            if (!edited)
                return;

            UpdateTangents();
            OnEdited();
            OnEditingEnd();
        }

        // Computes control points given knots k, this is the brain of the operation
        private static void ComputeControlPoints(float[] k, out float[] p1, out float[] p2)
        {
            // Reference: https://www.particleincell.com/2012/bezier-splines/

            var n = k.Length - 1;
            p1 = new float[n];
            p2 = new float[n];

            // rhs vector
            var a = new float[n];
            var b = new float[n];
            var c = new float[n];
            var r = new float[n];

            // left most segment
            a[0] = 0;
            b[0] = 2;
            c[0] = 1;
            r[0] = k[0] + 2 * k[1];

            // internal segments
            for (var i = 1; i < n - 1; i++)
            {
                a[i] = 1;
                b[i] = 4;
                c[i] = 1;
                r[i] = 4 * k[i] + 2 * k[i + 1];
            }

            // right segment
            a[n - 1] = 2;
            b[n - 1] = 7;
            c[n - 1] = 0;
            r[n - 1] = 8 * k[n - 1] + k[n];

            // solves Ax=b with the Thomas algorithm (from Wikipedia)
            for (var i = 1; i < n; i++)
            {
                var m = a[i] / b[i - 1];
                b[i] = b[i] - m * c[i - 1];
                r[i] = r[i] - m * r[i - 1];
            }

            p1[n - 1] = r[n - 1] / b[n - 1];
            for (var i = n - 2; i >= 0; --i)
                p1[i] = (r[i] - c[i] * p1[i + 1]) / b[i];

            // we have p1, now compute p2
            for (var i = 0; i < n - 1; i++)
                p2[i] = 2 * k[i + 1] - p1[i + 1];

            p2[n - 1] = 0.5f * (k[n] + p1[n - 1]);
        }

        private void SetTangentsSmooth()
        {
            var k1 = new T[_keyframes.Count];
            var k2 = new T[_keyframes.Count];
            var kk = new float[_keyframes.Count];
            var components = Accessor.GetCurveComponents();
            for (int component = 0; component < components; component++)
            {
                for (int i = 0; i < _keyframes.Count; i++)
                {
                    var v = _keyframes[i].Value;
                    kk[i] = Accessor.GetCurveValue(ref v, component);
                }
                ComputeControlPoints(kk, out var p1, out var p2);
                for (int i = 0; i < p1.Length; i++)
                {
                    Accessor.SetCurveValue(p1[i], ref k1[i], component);
                    Accessor.SetCurveValue(p2[i], ref k2[i], component);
                }
            }

            bool edited = false;
            for (int i = 0; i < _points.Count; i++)
            {
                var p = _points[i];
                if (!p.IsSelected)
                    continue;
                if (!edited)
                    OnEditingStart();
                edited = true;

                var k = _keyframes[p.Index];
                var value = Accessor.GetCurveValue(ref k.Value, p.Component);

                if (p.Index > 0)
                {
                    var o = _keyframes[p.Index - 1];
                    var slope = (Accessor.GetCurveValue(ref k2[p.Index], p.Component) - value) / (o.Time - k.Time);
                    Accessor.SetCurveValue(slope, ref k.TangentIn, p.Component);
                }

                if (p.Index < _keyframes.Count - 1)
                {
                    var o = _keyframes[p.Index + 1];
                    var slope = (Accessor.GetCurveValue(ref k1[p.Index], p.Component) - value) / (o.Time - k.Time);
                    Accessor.SetCurveValue(slope, ref k.TangentOut, p.Component);
                }

                _keyframes[p.Index] = k;
            }
            if (!edited)
                return;

            UpdateTangents();
            OnEdited();
            OnEditingEnd();
        }

        /// <inheritdoc />
        protected override void UpdateFPS()
        {
            if (FPS.HasValue)
            {
                float fps = FPS.Value;
                for (int i = 0; i < _keyframes.Count; i++)
                {
                    var k = _keyframes[i];
                    k.Time = Mathf.Floor(k.Time * fps) / fps;
                    _keyframes[i] = k;
                }
            }
        }

        /// <summary>
        /// Gets the keyframe point (in keyframes space).
        /// </summary>
        /// <param name="k">The keyframe.</param>
        /// <param name="component">The keyframe value component index.</param>
        /// <returns>The point in time/value space.</returns>
        private Vector2 GetKeyframePoint(ref BezierCurve<T>.Keyframe k, int component)
        {
            return new Vector2(k.Time, Accessor.GetCurveValue(ref k.Value, component));
        }

        private void DrawLine(BezierCurve<T>.Keyframe startK, BezierCurve<T>.Keyframe endK, int component, ref Rectangle viewRect)
        {
            var start = GetKeyframePoint(ref startK, component);
            var end = GetKeyframePoint(ref endK, component);

            var p1 = PointFromKeyframes(start, ref viewRect);
            var p2 = PointFromKeyframes(end, ref viewRect);

            var color = Colors[component].RGBMultiplied(0.6f);
            Render2D.DrawLine(p1, p2, color, 1.6f);
        }

        /// <inheritdoc />
        protected override void OnKeyframesChanged()
        {
            var components = Accessor.GetCurveComponents();
            while (_points.Count > _keyframes.Count * components)
            {
                var last = _points.Count - 1;
                _points[last].Dispose();
                _points.RemoveAt(last);
            }

            while (_points.Count < _keyframes.Count * components)
            {
                _points.Add(new KeyframePoint
                {
                    AutoFocus = false,
                    Size = KeyframesSize,
                    Editor = this,
                    Index = _points.Count / components,
                    Component = _points.Count % components,
                    Parent = _contents,
                });

                _refreshAfterEdit = true;
            }

            UpdateKeyframes();
            UpdateTooltips();

            base.OnKeyframesChanged();
        }

        /// <inheritdoc />
        public override void Evaluate(out T result, float time, bool loop = false)
        {
            var curve = new BezierCurve<T>
            {
                Keyframes = _keyframes.ToArray()
            };
            curve.Evaluate(out result, time, loop);
        }

        /// <inheritdoc />
        protected override float GetKeyframeTime(object keyframe)
        {
            return ((BezierCurve<T>.Keyframe)keyframe).Time;
        }

        /// <inheritdoc />
        protected override T GetKeyframeValue(object keyframe)
        {
            return ((BezierCurve<T>.Keyframe)keyframe).Value;
        }

        /// <inheritdoc />
        protected override float GetKeyframeValue(object keyframe, int component)
        {
            var value = ((BezierCurve<T>.Keyframe)keyframe).Value;
            return Accessor.GetCurveValue(ref value, component);
        }

        /// <inheritdoc />
        protected override void AddKeyframe(Vector2 keyframesPos)
        {
            var k = new BezierCurve<T>.Keyframe
            {
                Time = keyframesPos.X,
            };
            var components = Accessor.GetCurveComponents();
            for (int component = 0; component < components; component++)
            {
                Accessor.SetCurveValue(keyframesPos.Y, ref k.Value, component);
                Accessor.SetCurveValue(0.0f, ref k.TangentIn, component);
                Accessor.SetCurveValue(0.0f, ref k.TangentOut, component);
            }
            OnEditingStart();
            AddKeyframe(k);
            OnEditingEnd();
        }

        /// <inheritdoc />
        protected override void SetKeyframeInternal(int index, object keyframe)
        {
            _keyframes[index] = (BezierCurve<T>.Keyframe)keyframe;
        }

        /// <inheritdoc />
        protected override void SetKeyframeInternal(int index, float time, float value, int component)
        {
            var k = _keyframes[index];
            k.Time = time;
            Accessor.SetCurveValue(value, ref k.Value, component);
            _keyframes[index] = k;
        }

        /// <inheritdoc />
        protected override float GetKeyframeTangentInternal(int index, bool isIn, int component)
        {
            var k = _keyframes[index];
            var value = isIn ? k.TangentIn : k.TangentOut;
            return Accessor.GetCurveValue(ref value, component);
        }

        /// <inheritdoc />
        protected override void SetKeyframeTangentInternal(int index, bool isIn, int component, float value)
        {
            var k = _keyframes[index];
            if (isIn)
                Accessor.SetCurveValue(value, ref k.TangentIn, component);
            else
                Accessor.SetCurveValue(value, ref k.TangentOut, component);
            _keyframes[index] = k;
        }

        /// <inheritdoc />
        protected override void RemoveKeyframesInternal(HashSet<int> indicesToRemove)
        {
            var keyframes = _keyframes;
            _keyframes = new List<BezierCurve<T>.Keyframe>();
            for (int i = 0; i < keyframes.Count; i++)
            {
                if (!indicesToRemove.Contains(i))
                    _keyframes.Add(keyframes[i]);
            }
        }

        sealed class AllKeyframesProxy : IAllKeyframesProxy
        {
            [HideInEditor, NoSerialize]
            public BezierCurveEditor<T> Editor;

            [Collection(CanReorderItems = false, Spacing = 10)]
            public BezierCurve<T>.Keyframe[] Keyframes;

            public void Apply()
            {
                Editor.SetKeyframes(Keyframes);
            }
        }

        /// <inheritdoc />
        protected override IAllKeyframesProxy GetAllKeyframesEditingProxy()
        {
            return new AllKeyframesProxy
            {
                Editor = this,
                Keyframes = _keyframes.ToArray(),
            };
        }

        /// <inheritdoc />
        public override object[] GetKeyframes()
        {
            var keyframes = new object[_keyframes.Count];
            for (int i = 0; i < keyframes.Length; i++)
                keyframes[i] = _keyframes[i];
            return keyframes;
        }

        /// <inheritdoc />
        public override void SetKeyframes(object[] keyframes)
        {
            var data = new BezierCurve<T>.Keyframe[keyframes.Length];
            for (int i = 0; i < keyframes.Length; i++)
            {
                if (keyframes[i] is BezierCurve<T>.Keyframe asT)
                    data[i] = asT;
                else if (keyframes[i] is BezierCurve<object>.Keyframe asObj)
                    data[i] = new BezierCurve<T>.Keyframe(asObj.Time, (T)asObj.Value, (T)asObj.TangentIn, (T)asObj.TangentOut);
            }
            SetKeyframes(data);
        }

        /// <inheritdoc />
        public override void AddKeyframe(float time, object value)
        {
            AddKeyframe(new BezierCurve<T>.Keyframe(time, (T)value));
        }

        /// <inheritdoc />
        public override void GetKeyframe(int index, out float time, out object value, out object tangentIn, out object tangentOut)
        {
            var k = _keyframes[index];
            time = k.Time;
            value = k.Value;
            tangentIn = k.TangentIn;
            tangentOut = k.TangentOut;
        }

        /// <inheritdoc />
        public override object GetKeyframe(int index)
        {
            return _keyframes[index];
        }

        /// <inheritdoc />
        public override void SetKeyframeValue(int index, object value)
        {
            var k = _keyframes[index];
            k.Value = (T)value;
            _keyframes[index] = k;

            UpdateKeyframes();
            UpdateTooltips();
            OnEdited();
        }

        /// <inheritdoc />
        public override int KeyframesCount => _keyframes.Count;

        /// <inheritdoc />
        public override void UpdateKeyframes()
        {
            if (_points.Count == 0)
            {
                // No keyframes
                _contents.Bounds = Rectangle.Empty;
                return;
            }

            _mainPanel.LockChildrenRecursive();

            // Place keyframes
            Rectangle curveContentAreaBounds = _mainPanel.GetClientArea();
            var viewScale = ViewScale;
            for (int i = 0; i < _points.Count; i++)
            {
                var p = _points[i];
                var k = _keyframes[p.Index];

                var location = GetKeyframePoint(ref k, p.Component);
                var point = new Vector2
                (
                 location.X * UnitsPerSecond - p.Width * 0.5f,
                 location.Y * -UnitsPerSecond - p.Height * 0.5f + curveContentAreaBounds.Height
                );

                if (_showCollapsed)
                {
                    point.Y = 1.0f;
                    p.Size = new Vector2(4.0f / viewScale.X, Height - 2.0f);
                    p.Visible = p.Component == 0;
                }
                else
                {
                    p.Size = KeyframesSize / viewScale;
                    p.Visible = true;
                }
                p.Location = point;
            }

            // Calculate bounds
            var bounds = _points[0].Bounds;
            for (var i = 1; i < _points.Count; i++)
            {
                bounds = Rectangle.Union(bounds, _points[i].Bounds);
            }

            // Adjust contents bounds to fill the curve area
            if (EnablePanning != UseMode.Off)
            {
                bounds.Width = Mathf.Max(bounds.Width, 1.0f);
                bounds.Height = Mathf.Max(bounds.Height, 1.0f);
                bounds.Location = ApplyUseModeMask(EnablePanning, bounds.Location, _contents.Location);
                bounds.Size = ApplyUseModeMask(EnablePanning, bounds.Size, _contents.Size);
                _contents.Bounds = bounds;
            }
            else if (_contents.Bounds == Rectangle.Empty)
            {
                _contents.Bounds = Rectangle.Union(bounds, new Rectangle(Vector2.Zero, Vector2.One));
            }

            // Offset the keyframes (parent container changed its location)
            var posOffset = _contents.Location;
            for (var i = 0; i < _points.Count; i++)
            {
                _points[i].Location -= posOffset;
            }

            UpdateTangents();

            _mainPanel.UnlockChildrenRecursive();
            _mainPanel.PerformLayout();
        }

        /// <inheritdoc />
        public override void UpdateTangents()
        {
            // Find selected keyframe
            Rectangle curveContentAreaBounds = _mainPanel.GetClientArea();
            var selectedCount = 0;
            var selectedIndex = -1;
            KeyframePoint selectedKeyframe = null;
            var selectedComponent = -1;
            for (int i = 0; i < _points.Count; i++)
            {
                var p = _points[i];
                if (p.IsSelected)
                {
                    selectedIndex = p.Index;
                    selectedKeyframe = p;
                    selectedComponent = p.Component;
                    selectedCount++;
                }
            }

            // Place tangents (only for a single selected keyframe)
            if (selectedCount == 1 && !_showCollapsed)
            {
                var posOffset = _contents.Location;
                var k = _keyframes[selectedIndex];
                for (int i = 0; i < _tangents.Length; i++)
                {
                    var t = _tangents[i];

                    t.Index = selectedIndex;
                    t.Point = selectedKeyframe;
                    t.Component = selectedComponent;

                    var tangent = t.TangentValue;
                    var direction = t.IsIn ? -1.0f : 1.0f;
                    var offset = 30.0f * direction;
                    var location = GetKeyframePoint(ref k, selectedComponent);
                    t.Size = KeyframesSize / ViewScale;
                    t.Location = new Vector2
                    (
                     location.X * UnitsPerSecond - t.Width * 0.5f + offset,
                     location.Y * -UnitsPerSecond - t.Height * 0.5f + curveContentAreaBounds.Height - offset * tangent
                    );

                    var isFirst = selectedIndex == 0 && t.IsIn;
                    var isLast = selectedIndex == _keyframes.Count - 1 && !t.IsIn;
                    t.Visible = !isFirst && !isLast;
                    t.UpdateTooltip();

                    if (t.Visible)
                        _tangents[i].Location -= posOffset;
                }
            }
            else
            {
                for (int i = 0; i < _tangents.Length; i++)
                {
                    _tangents[i].Visible = false;
                }
            }
        }

        /// <inheritdoc />
        protected override void OnShowContextMenu(ContextMenu.ContextMenu cm, int selectionCount)
        {
            if (selectionCount != 0)
            {
                cm.AddSeparator();
                cm.AddButton("Reset tangents", ResetTangents);
                cm.AddButton("Linear tangents", SetTangentsLinear);
                cm.AddButton("Smooth tangents", SetTangentsSmooth);
            }
        }

        /// <inheritdoc />
        protected override void DrawCurve(ref Rectangle viewRect)
        {
            var components = Accessor.GetCurveComponents();
            for (int component = 0; component < components; component++)
            {
                if (ShowStartEndLines)
                {
                    var start = new BezierCurve<T>.Keyframe
                    {
                        Value = DefaultValue,
                        Time = -10000000.0f,
                    };
                    var end = new BezierCurve<T>.Keyframe
                    {
                        Value = DefaultValue,
                        Time = 10000000.0f,
                    };

                    if (_keyframes.Count == 0)
                    {
                        DrawLine(start, end, component, ref viewRect);
                    }
                    else
                    {
                        DrawLine(start, _keyframes[0], component, ref viewRect);
                        DrawLine(_keyframes[_keyframes.Count - 1], end, component, ref viewRect);
                    }
                }

                var color = Colors[component];
                for (int i = 1; i < _keyframes.Count; i++)
                {
                    var startK = _keyframes[i - 1];
                    var endK = _keyframes[i];

                    var start = GetKeyframePoint(ref startK, component);
                    var end = GetKeyframePoint(ref endK, component);

                    var startTangent = Accessor.GetCurveValue(ref startK.TangentOut, component);
                    var endTangent = Accessor.GetCurveValue(ref endK.TangentIn, component);

                    var offset = (end.X - start.X) * 0.5f;

                    var p1 = PointFromKeyframes(start, ref viewRect);
                    var p2 = PointFromKeyframes(start + new Vector2(offset, startTangent * offset), ref viewRect);
                    var p3 = PointFromKeyframes(end - new Vector2(offset, endTangent * offset), ref viewRect);
                    var p4 = PointFromKeyframes(end, ref viewRect);

                    Render2D.DrawBezier(p1, p2, p3, p4, color);
                }
            }
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            _keyframes.Clear();

            base.OnDestroy();
        }
    }
}
