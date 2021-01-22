// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using FlaxEditor.CustomEditors;
using FlaxEditor.GUI.ContextMenu;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI
{
    /// <summary>
    /// The generic keyframes animation editor control.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    [HideInEditor]
    public class KeyframesEditor : ContainerControl
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
            internal Vector2 _leftMouseDownPos = Vector2.Minimum;
            private Vector2 _rightMouseDownPos = Vector2.Minimum;
            internal Vector2 _mousePos = Vector2.Minimum;
            private float _mouseMoveAmount;
            internal bool _isMovingSelection;
            private Vector2 _movingSelectionViewPos;
            private Vector2 _cmShowPos;

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

                // Find controls to select
                for (int i = 0; i < Children.Count; i++)
                {
                    if (Children[i] is KeyframePoint p)
                    {
                        p.IsSelected = p.Bounds.Intersects(ref selectionRect);
                    }
                }
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
                    if (delta.LengthSquared > 0.01f && _editor.EnablePanning)
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
                        for (var i = 0; i < _editor._points.Count; i++)
                        {
                            var p = _editor._points[i];
                            if (p.IsSelected)
                            {
                                var k = _editor._keyframes[p.Index];

                                float minTime = p.Index != 0 ? _editor._keyframes[p.Index - 1].Time : float.MinValue;
                                float maxTime = p.Index != _editor._keyframes.Count - 1 ? _editor._keyframes[p.Index + 1].Time : float.MaxValue;

                                k.Time += keyframeDelta.X;
                                if (_editor.FPS.HasValue)
                                {
                                    float fps = _editor.FPS.Value;
                                    k.Time = Mathf.Floor(k.Time * fps) / fps;
                                }
                                k.Time = Mathf.Clamp(k.Time, minTime, maxTime);

                                // TODO: snapping keyframes to grid when moving

                                _editor._keyframes[p.Index] = k;
                            }
                        }
                        _editor.UpdateKeyframes();
                        if (_editor.EnablePanning)
                            _editor._mainPanel.ScrollViewTo(PointToParent(location));
                        _leftMouseDownPos = location;
                        Cursor = CursorType.SizeAll;
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

                base.OnLostFocus();
            }

            /// <inheritdoc />
            public override bool OnMouseDown(Vector2 location, MouseButton button)
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
                        }
                        // Check if node isn't selected
                        else if (!keyframe.IsSelected)
                        {
                            // Select node
                            _editor.ClearSelection();
                            keyframe.Select();
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
                else
                {
                    if (_leftMouseDown)
                    {
                        // Start selecting
                        StartMouseCapture();
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
            public override bool OnMouseUp(Vector2 location, MouseButton button)
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
                        }

                        var viewRect = _editor._mainPanel.GetClientArea();
                        _cmShowPos = PointToKeyframes(location, ref viewRect);

                        var cm = new ContextMenu.ContextMenu();
                        cm.AddButton("Add keyframe", () => _editor.AddKeyframe(_cmShowPos)).Enabled = _editor.Keyframes.Count < _editor.MaxKeyframes;
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
                        if (_editor.EnableZoom && _editor.EnablePanning)
                        {
                            cm.AddSeparator();
                            cm.AddButton("Show whole keyframes", _editor.ShowWholeKeyframes);
                            cm.AddButton("Reset view", _editor.ResetView);
                        }
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
                if (_editor.EnableZoom && IsMouseOver && !_leftMouseDown)
                {
                    // TODO: preserve the view center point for easier zooming
                    _editor.ViewScale += delta * 0.1f;
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
            /// Converts the input point from editor contents control space into the keyframes time/value coordinates.
            /// </summary>
            /// <param name="point">The point.</param>
            /// <param name="keyframesContentAreaBounds">The keyframes contents area bounds.</param>
            /// <returns>The result.</returns>
            private Vector2 PointToKeyframes(Vector2 point, ref Rectangle keyframesContentAreaBounds)
            {
                // Contents -> Keyframes
                return new Vector2(
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

            /// <inheritdoc />
            public override void Draw()
            {
                var rect = new Rectangle(Vector2.Zero, Size);
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

            public void Select()
            {
                IsSelected = true;
            }

            public void Deselect()
            {
                IsSelected = false;
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
        private static readonly Vector2 KeyframesSize = new Vector2(5.0f);

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
        public Vector2 ViewOffset
        {
            get => _mainPanel.ViewOffset;
            set => _mainPanel.ViewOffset = value;
        }

        /// <summary>
        /// Gets or sets the view scale.
        /// </summary>
        public Vector2 ViewScale
        {
            get => _contents.Scale;
            set => _contents.Scale = Vector2.Clamp(value, new Vector2(0.0001f), new Vector2(1000.0f));
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
        /// The amount of frames per second of the keyframes animation (optional). Can be sued to restrict the keyframes time values to the given time quantization rate.
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

        private void AddKeyframe(Vector2 keyframesPos)
        {
            var k = new Keyframe
            {
                Time = keyframesPos.X,
                Value = DefaultValue,
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

        private void EditAllKeyframes(Control control, Vector2 pos)
        {
            _popup = new Popup(this, new object[] { new AllKeyframesProxy { Editor = this, Keyframes = _keyframes.ToArray(), } }, null, 400.0f);
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
            for (int i = 0; i < keyframeIndices.Count; i++)
                selection[i] = _keyframes[keyframeIndices[i]];
            _popup = new Popup(this, selection, keyframeIndices);
            _popup.Show(control, pos);
        }

        private void RemoveKeyframes()
        {
            bool edited = false;
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
                    p.Deselect();
                    edited = true;
                }
            }
            if (!edited)
                return;

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
            ViewScale = Vector2.One;
            ViewOffset = Vector2.Zero;
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

                p.Size = new Vector2(4.0f / viewScale.X, Height - 2.0f);
                p.Location = new Vector2(k.Time * UnitsPerSecond - p.Width * 0.5f, 1.0f);
                p.UpdateTooltip();
            }

            // Calculate bounds
            var bounds = _points[0].Bounds;
            for (var i = 1; i < _points.Count; i++)
            {
                bounds = Rectangle.Union(bounds, _points[i].Bounds);
            }

            // Adjust contents bounds to fill the keyframes area
            if (EnablePanning)
                _contents.Bounds = bounds;

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

            // Draw selection rectangle
            if (_contents._leftMouseDown && !_contents._isMovingSelection)
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
                    return true;
                }
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

            base.OnDestroy();
        }
    }
}
