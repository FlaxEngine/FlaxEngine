// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Text;
using FlaxEditor.CustomEditors;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Options;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Json;
using FlaxEngine.Utilities;

namespace FlaxEditor.GUI
{
    /// <summary>
    /// The generic keyframes animation editor control.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    [HideInEditor]
    public class KeyframesEditor : ContainerControl, IKeyframesEditor
    {
        /// <summary>
        /// A single keyframe.
        /// </summary>
        public struct Keyframe : IComparable, IComparable<Keyframe>
        {
            /// <summary>
            /// The time of the keyframe.
            /// </summary>
            [EditorOrder(0), Limit(float.MinValue, float.MaxValue, 0.01f), Tooltip("The time of the keyframe.")]
            public float Time;

            /// <summary>
            /// The value of the keyframe.
            /// </summary>
            [EditorOrder(1), Limit(float.MinValue, float.MaxValue, 0.01f), Tooltip("The value of the keyframe.")]
            public object Value;

            /// <summary>
            /// Initializes a new instance of the <see cref="Keyframe"/> struct.
            /// </summary>
            /// <param name="time">The time.</param>
            /// <param name="value">The value.</param>
            public Keyframe(float time, object value)
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
        /// The keyframes contents container control.
        /// </summary>
        /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
        private class Contents : ContainerControl
        {
            private readonly KeyframesEditor _editor;
            internal bool _leftMouseDown;
            private bool _rightMouseDown;
            internal Float2 _leftMouseDownPos = Float2.Minimum;
            private Float2 _rightMouseDownPos = Float2.Minimum;
            internal Float2 _mousePos = Float2.Minimum;
            private Float2 _movingViewLastPos;
            internal bool _isMovingSelection;
            private bool _movedKeyframes;
            private float _movingSelectionStart;
            private float[] _movingSelectionOffsets;
            private Float2 _cmShowPos;

            /// <summary>
            /// Initializes a new instance of the <see cref="Contents"/> class.
            /// </summary>
            /// <param name="editor">The editor.</param>
            public Contents(KeyframesEditor editor)
            {
                _editor = editor;
            }

            private void UpdateSelectionRectangle()
            {
                var selectionRect = Rectangle.FromPoints(_leftMouseDownPos, _mousePos);
                if (_editor.KeyframesEditorContext != null)
                    _editor.KeyframesEditorContext.OnKeyframesSelection(_editor, this, selectionRect);
                else
                    UpdateSelection(ref selectionRect);
            }

            internal void UpdateSelection(ref Rectangle selectionRect)
            {
                // Find controls to select
                for (int i = 0; i < Children.Count; i++)
                {
                    if (Children[i] is KeyframePoint p)
                    {
                        p.IsSelected = p.Bounds.Intersects(ref selectionRect);
                    }
                }
            }

            internal void OnMoveStart(Float2 location)
            {
                // Start moving selected nodes
                _isMovingSelection = true;
                _movedKeyframes = false;
                var viewRect = _editor._mainPanel.GetClientArea();
                _movingSelectionStart = PointToKeyframes(location, ref viewRect).X;
                if (_movingSelectionOffsets == null || _movingSelectionOffsets.Length != _editor._keyframes.Count)
                    _movingSelectionOffsets = new float[_editor._keyframes.Count];
                for (int i = 0; i < _movingSelectionOffsets.Length; i++)
                    _movingSelectionOffsets[i] = _editor._keyframes[i].Time - _movingSelectionStart;
                _editor.OnEditingStart();
            }

            internal void OnMove(Float2 location)
            {
                var viewRect = _editor._mainPanel.GetClientArea();
                var locationKeyframes = PointToKeyframes(location, ref viewRect);
                for (var i = 0; i < _editor._points.Count; i++)
                {
                    var p = _editor._points[i];
                    if (p.IsSelected)
                    {
                        var k = _editor._keyframes[p.Index];

                        float minTime = p.Index != 0 ? _editor._keyframes[p.Index - 1].Time : float.MinValue;
                        float maxTime = p.Index != _editor._keyframes.Count - 1 ? _editor._keyframes[p.Index + 1].Time : float.MaxValue;

                        var offset = _movingSelectionOffsets[p.Index];
                        k.Time = locationKeyframes.X + offset;
                        if (_editor.FPS.HasValue)
                        {
                            float fps = _editor.FPS.Value;
                            k.Time = Mathf.Floor(k.Time * fps) / fps;
                        }
                        k.Time = Mathf.Clamp(k.Time, minTime, maxTime);

                        // TODO: snapping keyframes to grid when moving

                        _editor._keyframes[p.Index] = k;
                    }
                    _editor.UpdateKeyframes();
                    if (_editor.EnablePanning)
                    {
                        //_editor._mainPanel.ScrollViewTo(PointToParent(_editor._mainPanel, location));
                    }
                    Cursor = CursorType.SizeAll;
                    _movedKeyframes = true;
                }
            }

            internal void OnMoveEnd(Float2 location)
            {
                if (_movedKeyframes)
                {
                    _editor.OnEdited();
                    _editor.OnEditingEnd();
                    _editor.UpdateKeyframes();
                    _movedKeyframes = false;
                }
                _isMovingSelection = false;
            }

            /// <inheritdoc />
            public override bool IntersectsContent(ref Float2 locationParent, out Float2 location)
            {
                // Pass all events
                location = PointFromParent(ref locationParent);
                return true;
            }

            /// <inheritdoc />
            public override void OnMouseEnter(Float2 location)
            {
                _mousePos = location;

                base.OnMouseEnter(location);
            }

            /// <inheritdoc />
            public override void OnMouseMove(Float2 location)
            {
                _mousePos = location;

                // Start moving selection if movement started from the keyframe
                if (_leftMouseDown && !_isMovingSelection && GetChildAt(_leftMouseDownPos) is KeyframePoint)
                {
                    if (_editor.KeyframesEditorContext != null)
                        _editor.KeyframesEditorContext.OnKeyframesMove(_editor, this, location, true, false);
                    else
                        OnMoveStart(location);
                }

                // Moving view
                if (_rightMouseDown)
                {
                    // Calculate delta
                    Float2 delta = location - _movingViewLastPos;
                    if (delta.LengthSquared > 0.01f)
                    {
                        if (_editor.CustomViewPanning != null)
                            delta = _editor.CustomViewPanning(delta);
                        if (_editor.EnablePanning)
                        {
                            // Move view
                            _editor.ViewOffset += delta * _editor.ViewScale;
                            _movingViewLastPos = location;
                            Cursor = CursorType.SizeAll;
                        }
                        else if (_editor.CustomViewPanning != null)
                            Cursor = CursorType.SizeAll;
                    }

                    return;
                }
                // Moving selection
                else if (_isMovingSelection)
                {
                    if (_editor.KeyframesEditorContext != null)
                        _editor.KeyframesEditorContext.OnKeyframesMove(_editor, this, location, false, false);
                    else
                        OnMove(location);
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

                base.OnLostFocus();
            }

            /// <inheritdoc />
            public override bool OnMouseDown(Float2 location, MouseButton button)
            {
                if (base.OnMouseDown(location, button))
                {
                    // Clear flags
                    _isMovingSelection = false;
                    _rightMouseDown = false;
                    _leftMouseDown = false;
                    return true;
                }

                // Cache data
                _isMovingSelection = false;
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
                    _movingViewLastPos = location;
                }

                // Check if any node is under the mouse
                var underMouse = GetChildAt(location);
                if (underMouse is KeyframePoint keyframe)
                {
                    if (_leftMouseDown)
                    {
                        if (Root.GetKey(KeyboardKeys.Control))
                        {
                            // Toggle selection
                            keyframe.IsSelected = !keyframe.IsSelected;
                        }
                        else if (Root.GetKey(KeyboardKeys.Shift))
                        {
                            // Select range
                            keyframe.IsSelected = true;
                            int selectionStart = 0;
                            for (; selectionStart < _editor._points.Count; selectionStart++)
                            {
                                if (_editor._points[selectionStart].IsSelected)
                                    break;
                            }
                            int selectionEnd = _editor._points.Count - 1;
                            for (; selectionEnd > selectionStart; selectionEnd--)
                            {
                                if (_editor._points[selectionEnd].IsSelected)
                                    break;
                            }
                            selectionStart++;
                            for (; selectionStart < selectionEnd; selectionStart++)
                                _editor._points[selectionStart].IsSelected = true;
                        }
                        else if (!keyframe.IsSelected)
                        {
                            // Select node
                            if (_editor.KeyframesEditorContext != null)
                                _editor.KeyframesEditorContext.OnKeyframesDeselect(_editor);
                            else
                                _editor.ClearSelection();
                            keyframe.IsSelected = true;
                        }
                        StartMouseCapture();
                        Focus();
                        Tooltip?.Hide();
                        return true;
                    }
                }
                else
                {
                    if (_leftMouseDown)
                    {
                        // Start selecting
                        StartMouseCapture();
                        if (_editor.KeyframesEditorContext != null)
                            _editor.KeyframesEditorContext.OnKeyframesDeselect(_editor);
                        else
                            _editor.ClearSelection();
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
            public override bool OnMouseUp(Float2 location, MouseButton button)
            {
                _mousePos = location;

                if (_leftMouseDown && button == MouseButton.Left)
                {
                    _leftMouseDown = false;
                    EndMouseCapture();
                    Cursor = CursorType.Default;

                    // Moving keyframes
                    if (_isMovingSelection)
                    {
                        if (_editor.KeyframesEditorContext != null)
                            _editor.KeyframesEditorContext.OnKeyframesMove(_editor, this, location, false, true);
                        else
                            OnMoveEnd(location);
                    }

                    _isMovingSelection = false;
                    _movedKeyframes = false;
                }
                if (_rightMouseDown && button == MouseButton.Right)
                {
                    _rightMouseDown = false;
                    EndMouseCapture();
                    Cursor = CursorType.Default;

                    // Check if no move has been made at all
                    if (Float2.Distance(ref location, ref _rightMouseDownPos) < 2.0f)
                    {
                        var selectionCount = _editor.SelectionCount;
                        var point = GetChildAt(location) as KeyframePoint;
                        if (selectionCount == 0 && point != null)
                        {
                            // Select node
                            selectionCount = 1;
                            point.IsSelected = true;
                        }

                        var viewRect = _editor._mainPanel.GetClientArea();
                        _cmShowPos = PointToKeyframes(location, ref viewRect);

                        var cm = new ContextMenu.ContextMenu();
                        cm.AddButton("Add keyframe", () => _editor.AddKeyframe(_cmShowPos)).Enabled = _editor.Keyframes.Count < _editor.MaxKeyframes && _editor.DefaultValue != null;
                        if (selectionCount > 0 && _editor.EnableKeyframesValueEdit)
                        {
                            cm.AddButton(selectionCount == 1 ? "Edit keyframe" : "Edit keyframes", () => _editor.EditKeyframes(this, location));
                        }
                        var totalSelectionCount = _editor.KeyframesEditorContext?.OnKeyframesSelectionCount() ?? selectionCount;
                        if (totalSelectionCount > 0)
                        {
                            cm.AddButton(totalSelectionCount == 1 ? "Remove keyframe" : "Remove keyframes", _editor.RemoveKeyframes);
                            cm.AddButton(totalSelectionCount == 1 ? "Copy keyframe" : "Copy keyframes", () => _editor.CopyKeyframes(point));
                        }
                        cm.AddButton("Paste keyframes", () => KeyframesEditorUtils.Paste(_editor, point?.Time ?? _cmShowPos.X)).Enabled = KeyframesEditorUtils.CanPaste();
                        cm.AddSeparator();
                        if (_editor.EnableKeyframesValueEdit)
                            cm.AddButton("Edit all keyframes", () => _editor.EditAllKeyframes(this, location));
                        cm.AddButton("Select all keyframes", _editor.SelectAll).Enabled = _editor._points.Count > 0;
                        cm.AddButton("Deselect all keyframes", _editor.DeselectAll).Enabled = _editor._points.Count > 0;
                        cm.AddButton("Copy all keyframes", () =>
                        {
                            _editor.SelectAll();
                            _editor.CopyKeyframes(point);
                        }).Enabled = _editor.DefaultValue != null;
                        if (_editor.EnableZoom && _editor.EnablePanning)
                        {
                            cm.AddSeparator();
                            cm.AddButton("Show whole keyframes", _editor.ShowWholeKeyframes);
                            cm.AddButton("Reset view", _editor.ResetView);
                        }
                        cm.Show(this, location);
                    }
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
            public override bool OnMouseWheel(Float2 location, float delta)
            {
                if (base.OnMouseWheel(location, delta))
                    return true;

                // Zoom in/out
                if (_editor.EnableZoom && IsMouseOver && !_leftMouseDown)
                {
                    // TODO: preserve the view center point for easier zooming
                    _editor.ViewScale += delta * 0.1f;
                    return true;
                }

                return false;
            }

            /// <inheritdoc />
            protected override void SetScaleInternal(ref Float2 scale)
            {
                base.SetScaleInternal(ref scale);

                _editor.UpdateKeyframes();
            }

            /// <summary>
            /// Converts the input point from editor contents control space into the keyframes time/value coordinates.
            /// </summary>
            /// <param name="point">The point.</param>
            /// <param name="keyframesContentAreaBounds">The keyframes contents area bounds.</param>
            /// <returns>The result.</returns>
            private Float2 PointToKeyframes(Float2 point, ref Rectangle keyframesContentAreaBounds)
            {
                // Contents -> Keyframes
                return new Float2(
                                   (point.X + Location.X) / UnitsPerSecond,
                                   (point.Y + Location.Y - keyframesContentAreaBounds.Height) / -UnitsPerSecond
                                  );
            }
        }

        /// <summary>
        /// The single keyframe control.
        /// </summary>
        private class KeyframePoint : Control
        {
            /// <summary>
            /// The parent keyframes editor.
            /// </summary>
            public KeyframesEditor Editor;

            /// <summary>
            /// The keyframe index.
            /// </summary>
            public int Index;

            /// <summary>
            /// Flag for selected keyframes.
            /// </summary>
            public bool IsSelected;

            /// <summary>
            /// Gets the time of the keyframe.
            /// </summary>
            public float Time => Editor._keyframes[Index].Time;

            /// <inheritdoc />
            public override void Draw()
            {
                var rect = new Rectangle(Float2.Zero, Size);
                var color = Color.Gray;
                if (IsSelected)
                    color = Editor.ContainsFocus ? Color.YellowGreen : Color.Lerp(Color.Gray, Color.YellowGreen, 0.4f);
                if (IsMouseOver)
                    color *= 1.1f;
                Render2D.FillRectangle(rect, color);
            }

            /// <inheritdoc />
            protected override void OnLocationChanged()
            {
                base.OnLocationChanged();

                UpdateTooltip();
            }

            /// <inheritdoc />
            public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
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
            /// Updates the tooltip.
            /// </summary>
            public void UpdateTooltip()
            {
                var k = Editor._keyframes[Index];
                TooltipText = string.Format("Time: {0}, Value: {1}", k.Time, k.Value);
            }
        }

        /// <summary>
        /// The timeline units per second (on time axis).
        /// </summary>
        public static readonly float UnitsPerSecond = 100.0f;

        /// <summary>
        /// The keyframes size.
        /// </summary>
        private static readonly Float2 KeyframesSize = new Float2(7.0f);

        private Contents _contents;
        private Panel _mainPanel;
        private readonly List<KeyframePoint> _points = new List<KeyframePoint>();
        private bool _refreshAfterEdit;
        private Popup _popup;
        private float? _fps;

        /// <summary>
        /// The keyframes collection.
        /// </summary>
        protected readonly List<Keyframe> _keyframes = new List<Keyframe>();

        /// <summary>
        /// Occurs when keyframes collection gets changed (keyframe added or removed).
        /// </summary>
        public event Action KeyframesChanged;

        /// <summary>
        /// Gets the keyframes collection (read-only).
        /// </summary>
        public IReadOnlyList<Keyframe> Keyframes => _keyframes;

        /// <summary>
        /// Gets or sets the view offset (via scroll bars).
        /// </summary>
        public Float2 ViewOffset
        {
            get => _mainPanel.ViewOffset;
            set => _mainPanel.ViewOffset = value;
        }

        /// <summary>
        /// Gets or sets the view scale.
        /// </summary>
        public Float2 ViewScale
        {
            get => _contents.Scale;
            set => _contents.Scale = Float2.Clamp(value, new Float2(0.0001f), new Float2(1000.0f));
        }

        /// <summary>
        /// Occurs when keyframes data gets edited.
        /// </summary>
        public event Action Edited;

        /// <summary>
        /// Occurs when keyframes data editing starts (via UI).
        /// </summary>
        public event Action EditingStart;

        /// <summary>
        /// Occurs when keyframes data editing ends (via UI).
        /// </summary>
        public event Action EditingEnd;

        /// <summary>
        /// The function for custom view panning. Gets input movement delta (in keyframes editor control space) and returns the renaming input delta to process by keyframes editor itself.
        /// </summary>
        public Func<Float2, Float2> CustomViewPanning;

        /// <summary>
        /// The maximum amount of keyframes to use.
        /// </summary>
        public int MaxKeyframes = ushort.MaxValue;

        /// <summary>
        /// True if enable view zooming. Otherwise user won't be able to zoom in or out.
        /// </summary>
        public bool EnableZoom = true;

        /// <summary>
        /// True if enable view panning. Otherwise user won't be able to move the view area.
        /// </summary>
        public bool EnablePanning = true;

        /// <summary>
        /// True if enable keyframes values editing (via popup).
        /// </summary>
        public bool EnableKeyframesValueEdit = true;

        /// <summary>
        /// True if enable view panning. Otherwise user won't be able to move the view area.
        /// </summary>
        public bool Enable = true;

        /// <summary>
        /// Gets a value indicating whether user is editing the keyframes.
        /// </summary>
        public bool IsUserEditing => _popup != null || _contents._leftMouseDown;

        /// <summary>
        /// Gets or sets the scroll bars usage.
        /// </summary>
        public ScrollBars ScrollBars
        {
            get => _mainPanel.ScrollBars;
            set => _mainPanel.ScrollBars = value;
        }

        /// <summary>
        /// The default value.
        /// </summary>
        public object DefaultValue;

        /// <summary>
        /// The amount of frames per second of the keyframes animation (optional). Can be used to restrict the keyframes time values to the given time quantization rate.
        /// </summary>
        public float? FPS
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

        /// <summary>
        /// Initializes a new instance of the <see cref="KeyframesEditor"/> class.
        /// </summary>
        public KeyframesEditor()
        {
            _mainPanel = new Panel(ScrollBars.Both)
            {
                ScrollMargin = new Margin(150.0f),
                AlwaysShowScrollbars = true,
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                Parent = this
            };
            _contents = new Contents(this)
            {
                ClipChildren = false,
                AutoFocus = false,
                Parent = _mainPanel
            };

            UpdateKeyframes();
        }

        private void OnEdited()
        {
            Edited?.Invoke();
        }

        private void OnEditingStart()
        {
            EditingStart?.Invoke();
        }

        private void OnEditingEnd()
        {
            EditingEnd?.Invoke();
        }

        /// <summary>
        /// Evaluates the keyframe value at the specified time.
        /// </summary>
        /// <param name="time">The time to evaluate the keyframe value.</param>
        /// <returns>The evaluated value.</returns>
        public object Evaluate(float time)
        {
            if (_keyframes.Count == 0)
                return DefaultValue;

            // Find the keyframe at time
            int start = 0;
            int searchLength = _keyframes.Count;
            while (searchLength > 0)
            {
                int half = searchLength >> 1;
                int mid = start + half;

                if (time < _keyframes[mid].Time)
                {
                    searchLength = half;
                }
                else
                {
                    start = mid + 1;
                    searchLength -= (half + 1);
                }
            }
            int leftKey = Mathf.Max(0, start - 1);

            return _keyframes[leftKey].Value;
        }

        /// <summary>
        /// Resets the keyframes collection.
        /// </summary>
        public void ResetKeyframes()
        {
            if (_keyframes.Count == 0)
                return;

            _keyframes.Clear();

            UpdateFPS();

            OnKeyframesChanged();
        }

        /// <summary>
        /// Sets the keyframes collection.
        /// </summary>
        /// <param name="keyframes">The keyframes.</param>
        public void SetKeyframes(IEnumerable<Keyframe> keyframes)
        {
            if (keyframes == null)
                throw new ArgumentNullException(nameof(keyframes));
            var keyframesArray = keyframes as Keyframe[] ?? keyframes.ToArray();
            if (_keyframes.SequenceEqual(keyframesArray))
                return;
            if (keyframesArray.Length > MaxKeyframes)
            {
                var tmp = keyframesArray;
                keyframesArray = new Keyframe[MaxKeyframes];
                Array.Copy(tmp, keyframesArray, MaxKeyframes);
            }

            _keyframes.Clear();
            _keyframes.AddRange(keyframesArray);
            _keyframes.Sort((a, b) => a.Time.CompareTo(b.Time));

            UpdateFPS();
            OnKeyframesChanged();
        }

        private void UpdateFPS()
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
        /// Called when keyframes collection gets changed (keyframe added or removed).
        /// </summary>
        protected virtual void OnKeyframesChanged()
        {
            while (_points.Count > _keyframes.Count)
            {
                var last = _points.Count - 1;
                _points[last].Dispose();
                _points.RemoveAt(last);
            }

            while (_points.Count < _keyframes.Count)
            {
                _points.Add(new KeyframePoint
                {
                    AutoFocus = false,
                    Size = KeyframesSize,
                    Editor = this,
                    Index = _points.Count,
                    Parent = _contents,
                });

                _refreshAfterEdit = true;
            }

            UpdateKeyframes();

            KeyframesChanged?.Invoke();
        }

        /// <summary>
        /// Adds the new keyframe.
        /// </summary>
        /// <param name="k">The keyframe to add.</param>
        public void AddKeyframe(Keyframe k)
        {
            var eps = Mathf.Epsilon;
            if (FPS.HasValue)
            {
                float fps = FPS.Value;
                k.Time = Mathf.Floor(k.Time * fps) / fps;
                eps = 1.0f / fps;
            }

            int pos = 0;
            while (pos < _keyframes.Count && _keyframes[pos].Time < k.Time)
                pos++;
            if (_keyframes.Count > pos && Mathf.Abs(_keyframes[pos].Time - k.Time) < eps)
                _keyframes[pos] = k;
            else
                _keyframes.Insert(pos, k);

            OnKeyframesChanged();
            OnEdited();
        }

        /// <summary>
        /// Sets the existing keyframe value as boxed value.
        /// </summary>
        /// <param name="index">The keyframe index.</param>
        /// <param name="value">The keyframe value.</param>
        public void SetKeyframe(int index, object value)
        {
            var k = _keyframes[index];
            k.Value = value;
            _keyframes[index] = k;

            OnKeyframesChanged();
            OnEdited();
        }

        private void AddKeyframe(Float2 keyframesPos)
        {
            var k = new Keyframe
            {
                Time = keyframesPos.X,
                Value = Utilities.Utils.CloneValue(DefaultValue),
            };
            OnEditingStart();
            AddKeyframe(k);
            OnEditingEnd();
        }

        class Popup : ContextMenuBase
        {
            private CustomEditorPresenter _presenter;
            private KeyframesEditor _editor;
            private List<int> _keyframeIndices;
            private bool _isDirty;

            public Popup(KeyframesEditor editor, object[] selection, List<int> keyframeIndices = null, float height = 140.0f)
            : this(editor, height)
            {
                _presenter.Select(selection);
                _presenter.OpenAllGroups();
                _keyframeIndices = keyframeIndices;
                if (keyframeIndices != null && selection.Length != keyframeIndices.Count)
                    throw new Exception();
            }

            private Popup(KeyframesEditor editor, float height = 140.0f)
            {
                _editor = editor;
                const float width = 280.0f;
                Size = new Float2(width, height);
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
                        var keyframe = (Keyframe)_presenter.Selection[i];
                        var index = _keyframeIndices[i];
                        _editor._keyframes[index] = keyframe;
                    }
                }
                else if (_presenter.Selection[0] is AllKeyframesProxy proxy)
                {
                    _editor.SetKeyframes(proxy.Keyframes);
                }

                _editor.UpdateFPS();
                _editor.UpdateKeyframes();
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
                _keyframeIndices = null;
                _editor = null;

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

            /// <inheritdoc />
            public override void OnDestroy()
            {
                _editor = null;

                base.OnDestroy();
            }
        }

        sealed class AllKeyframesProxy
        {
            [HideInEditor, NoSerialize]
            public KeyframesEditor Editor;

            [Collection(CanReorderItems = false, Spacing = 10)]
            public Keyframe[] Keyframes;
        }

        private void EditAllKeyframes(Control control, Float2 pos)
        {
            _popup = new Popup(this, new object[] { new AllKeyframesProxy { Editor = this, Keyframes = _keyframes.ToArray(), } }, null, 400.0f);
            _popup.Show(control, pos);
        }

        private void EditKeyframes(Control control, Float2 pos)
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

        private void EditKeyframes(Control control, Float2 pos, List<int> keyframeIndices)
        {
            var selection = new object[keyframeIndices.Count];
            for (int i = 0; i < keyframeIndices.Count; i++)
                selection[i] = _keyframes[keyframeIndices[i]];
            _popup = new Popup(this, selection, keyframeIndices);
            _popup.Show(control, pos);
        }

        private void RemoveKeyframes()
        {
            if (KeyframesEditorContext != null)
                KeyframesEditorContext.OnKeyframesDelete(this);
            else
                RemoveKeyframesInner();
        }

        private void CopyKeyframes(KeyframePoint point = null)
        {
            float? timeOffset = null;
            if (point != null)
            {
                timeOffset = -point.Time;
            }
            else
            {
                for (int i = 0; i < _points.Count; i++)
                {
                    if (_points[i].IsSelected)
                    {
                        timeOffset = -_points[i].Time;
                        break;
                    }
                }
            }
            KeyframesEditorUtils.Copy(this, timeOffset);
        }

        private void RemoveKeyframesInner()
        {
            if (SelectionCount == 0)
                return;
            var keyframes = new Dictionary<int, Keyframe>(_keyframes.Count);
            for (int i = 0; i < _points.Count; i++)
            {
                var p = _points[i];
                if (!p.IsSelected)
                {
                    keyframes[p.Index] = _keyframes[p.Index];
                }
                else
                {
                    p.IsSelected = false;
                }
            }

            OnEditingStart();
            _keyframes.Clear();
            _keyframes.AddRange(keyframes.Values);
            OnKeyframesChanged();
            OnEdited();
            OnEditingEnd();
        }

        /// <summary>
        /// Shows the whole keyframes UI.
        /// </summary>
        public void ShowWholeKeyframes()
        {
            ViewScale = _mainPanel.Size / _contents.Size;
            ViewOffset = -_mainPanel.ControlsBounds.Location;
            UpdateKeyframes();
        }

        /// <summary>
        /// Resets the view.
        /// </summary>
        public void ResetView()
        {
            ViewScale = Float2.One;
            ViewOffset = Float2.Zero;
            UpdateKeyframes();
        }

        /// <summary>
        /// Updates the keyframes positioning.
        /// </summary>
        public virtual void UpdateKeyframes()
        {
            if (_points.Count == 0)
            {
                // No keyframes
                _contents.Bounds = Rectangle.Empty;
                return;
            }

            _mainPanel.LockChildrenRecursive();

            // Place keyframes
            var viewScale = ViewScale;
            for (int i = 0; i < _points.Count; i++)
            {
                var p = _points[i];
                var k = _keyframes[p.Index];

                p.Size = new Float2(4.0f / viewScale.X, Height - 2.0f);
                p.Location = new Float2(k.Time * UnitsPerSecond - p.Width * 0.5f, 1.0f);
                p.UpdateTooltip();
            }

            // Calculate bounds
            var bounds = _points[0].Bounds;
            for (var i = 1; i < _points.Count; i++)
            {
                bounds = Rectangle.Union(bounds, _points[i].Bounds);
            }

            // Adjust contents bounds to fill the keyframes area
            if (EnablePanning && !_contents._isMovingSelection)
            {
                _contents.Bounds = bounds;
            }

            // Offset the keyframes (parent container changed its location)
            var posOffset = _contents.Location;
            for (var i = 0; i < _points.Count; i++)
            {
                _points[i].Location -= posOffset;
            }

            _mainPanel.UnlockChildrenRecursive();
            _mainPanel.PerformLayout();
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

        /// <summary>
        /// Clears the selection.
        /// </summary>
        public void ClearSelection()
        {
            for (int i = 0; i < _points.Count; i++)
            {
                _points[i].IsSelected = false;
            }
        }

        private void BulkSelectUpdate(bool select = true)
        {
            for (int i = 0; i < _points.Count; i++)
            {
                _points[i].IsSelected = select;
            }
        }

        /// <summary>
        /// Selects all keyframes.
        /// </summary>
        public void SelectAll()
        {
            BulkSelectUpdate(true);
        }

        /// <summary>
        /// Deselects all keyframes.
        /// </summary>
        public void DeselectAll()
        {
            BulkSelectUpdate(false);
        }

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
            var rect = new Rectangle(Float2.Zero, Size);

            // Draw selection rectangle
            if (_contents._leftMouseDown && !_contents._isMovingSelection)
            {
                var selectionRect = Rectangle.FromPoints
                (
                 _mainPanel.PointToParent(_contents.PointToParent(_contents._leftMouseDownPos)),
                 _mainPanel.PointToParent(_contents.PointToParent(_contents._mousePos))
                );
                Render2D.FillRectangle(selectionRect, style.Selection);
                Render2D.DrawRectangle(selectionRect, style.SelectionBorder);
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

            InputOptions options = Editor.Instance.Options.Options.Input;
            if (options.SelectAll.Process(this))
            {
                SelectAll();
                return true;
            }
            else if (options.DeselectAll.Process(this))
            {
                DeselectAll();
                return true;
            }
            else if (options.Delete.Process(this))
            {
                RemoveKeyframes();
                return true;
            }
            else if (options.Copy.Process(this))
            {
                CopyKeyframes();
                return true;
            }
            else if (options.Paste.Process(this))
            {
                KeyframesEditorUtils.Paste(this);
                return true;
            }

            return false;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            // Clear references to the controls
            _mainPanel = null;
            _contents = null;
            _popup = null;

            // Cleanup
            _points.Clear();
            _keyframes.Clear();
            KeyframesEditorContext = null;

            base.OnDestroy();
        }

        /// <inheritdoc />
        public IKeyframesEditorContext KeyframesEditorContext { get; set; }

        /// <inheritdoc />
        public void OnKeyframesDeselect(IKeyframesEditor editor)
        {
            ClearSelection();
        }

        /// <inheritdoc />
        public void OnKeyframesSelection(IKeyframesEditor editor, ContainerControl control, Rectangle selection)
        {
            if (_keyframes.Count == 0)
                return;
            var selectionRect = Rectangle.FromPoints(_contents.PointFromParent(control, selection.UpperLeft), _contents.PointFromParent(control, selection.BottomRight));
            _contents.UpdateSelection(ref selectionRect);
        }

        /// <inheritdoc />
        public int OnKeyframesSelectionCount()
        {
            return SelectionCount;
        }

        /// <inheritdoc />
        public void OnKeyframesDelete(IKeyframesEditor editor)
        {
            RemoveKeyframesInner();
        }

        /// <inheritdoc />
        public void OnKeyframesMove(IKeyframesEditor editor, ContainerControl control, Float2 location, bool start, bool end)
        {
            if (SelectionCount == 0)
                return;
            location = _contents.PointFromParent(control, location);
            if (start)
                _contents.OnMoveStart(location);
            else if (end)
                _contents.OnMoveEnd(location);
            else
                _contents.OnMove(location);
        }

        /// <inheritdoc />
        public void OnKeyframesCopy(IKeyframesEditor editor, float? timeOffset, StringBuilder data)
        {
            if (SelectionCount == 0 || DefaultValue == null)
                return;
            var offset = timeOffset ?? 0.0f;
            data.AppendLine(KeyframesEditorUtils.CopyPrefix);
            data.AppendLine(DefaultValue.GetType().FullName);
            for (int i = 0; i < _keyframes.Count; i++)
            {
                if (!_points[i].IsSelected)
                    continue;
                var k = _keyframes[i];
                data.AppendLine((k.Time + offset).ToString(CultureInfo.InvariantCulture));
                data.AppendLine(JsonSerializer.Serialize(k.Value).RemoveNewLine());
            }
        }

        /// <inheritdoc />
        public void OnKeyframesPaste(IKeyframesEditor editor, float? timeOffset, string[] datas, ref int index)
        {
            if (DefaultValue == null)
                return;
            if (index == -1)
            {
                if (editor == this)
                    index = 0;
                else
                    return;
            }
            else if (index >= datas.Length)
                return;
            var data = datas[index];
            var offset = timeOffset ?? 0.0f;
            var eps = FPS.HasValue ? 0.5f / FPS.Value : Mathf.Epsilon;
            try
            {
                var lines = data.Split(new[] { "\r\n", "\r", "\n" }, StringSplitOptions.RemoveEmptyEntries);
                if (lines.Length < 3 || lines.Length % 2 == 0)
                    return;
                var type = TypeUtils.GetManagedType(lines[0]);
                if (type == null)
                    throw new Exception($"Unknown type {lines[0]}.");
                if (type != DefaultValue.GetType())
                    throw new Exception($"Mismatching keyframes data type {type.FullName} when pasting into {DefaultValue.GetType().FullName}.");
                var count = (lines.Length - 1) / 2;
                var modified = false;
                index++;
                for (int i = 0; i < count; i++)
                {
                    var k = new Keyframe
                    {
                        Time = float.Parse(lines[i * 2 + 1], CultureInfo.InvariantCulture) + offset,
                        Value = JsonSerializer.Deserialize(lines[i * 2 + 2], type),
                    };
                    if (FPS.HasValue)
                    {
                        float fps = FPS.Value;
                        k.Time = Mathf.Floor(k.Time * fps) / fps;
                    }
                    int pos = 0;
                    while (pos < _keyframes.Count && _keyframes[pos].Time < k.Time)
                        pos++;
                    if (_keyframes.Count > pos && Mathf.Abs(_keyframes[pos].Time - k.Time) < eps)
                    {
                        // Skip if the keyframe value won't change
                        if (JsonSerializer.ValueEquals(_keyframes[pos].Value, k.Value))
                            continue;
                    }

                    if (!modified)
                    {
                        modified = true;
                        OnEditingStart();
                    }

                    AddKeyframe(k);
                    _points[pos].IsSelected = true;
                }
                if (modified)
                    OnEditingEnd();
            }
            catch (Exception ex)
            {
                Editor.LogWarning("Failed to paste keyframes.");
                Editor.LogWarning(ex.Message);
                Editor.LogWarning(ex);
            }
        }

        /// <inheritdoc />
        public void OnKeyframesGet(string trackName, Action<string, float, object> get)
        {
            for (int i = 0; i < _keyframes.Count; i++)
            {
                var k = _keyframes[i];
                get(trackName, k.Time, k);
            }
        }

        /// <inheritdoc />
        public void OnKeyframesSet(List<KeyValuePair<float, object>> keyframes)
        {
            OnEditingStart();
            _keyframes.Clear();
            if (keyframes != null)
            {
                foreach (var e in keyframes)
                {
                    var k = (Keyframe)e.Value;
                    _keyframes.Add(new Keyframe(e.Key, k.Value));
                }
            }
            OnKeyframesChanged();
            OnEdited();
            OnEditingEnd();
        }
    }
}
