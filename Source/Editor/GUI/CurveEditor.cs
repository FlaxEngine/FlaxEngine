// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using FlaxEditor.CustomEditors;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.Options;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI
{
    /// <summary>
    /// The generic curve editor control.
    /// </summary>
    /// <typeparam name="T">The keyframe value type.</typeparam>
    /// <seealso cref="CurveEditorBase" />
    public abstract partial class CurveEditor<T> : CurveEditorBase where T : new()
    {
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

            /// <summary>
            /// Gets the point time and value on a curve.
            /// </summary>
            public Float2 Point => Editor.GetKeyframePoint(Index, Component);

            /// <summary>
            /// Gets the time of the keyframe point.
            /// </summary>
            public float Time => Editor.GetKeyframeTime(Index);

            /// <inheritdoc />
            public override void Draw()
            {
                var style = Style.Current;
                var rect = new Rectangle(Float2.Zero, Size);
                var color = Editor.ShowCollapsed ? style.ForegroundDisabled : Editor.Colors[Component];
                if (IsSelected)
                    color = Editor.ContainsFocus ? style.SelectionBorder : Color.Lerp(style.ForegroundDisabled, style.SelectionBorder, 0.4f);
                if (IsMouseOver)
                    color *= 1.1f;
                Render2D.FillRectangle(rect, color);
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

            /// <inheritdoc />
            protected override bool ShowTooltip => base.ShowTooltip && !Editor._contents._isMovingSelection;

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

            internal float TangentOffset => 50.0f / Editor.ViewScale.X;

            /// <inheritdoc />
            public override void Draw()
            {
                var style = Style.Current;
                var thickness = 6.0f / Mathf.Max(Editor.ViewScale.X, 1.0f);
                var size = Size;
                var pointPos = PointFromParent(Point.Center);
                Render2D.DrawLine(size * 0.5f, pointPos, style.ForegroundDisabled, thickness);

                var rect = new Rectangle(Float2.Zero, size);
                var color = style.BorderSelected;
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
        protected static readonly Float2 KeyframesSize = new Float2(7.0f);

        /// <summary>
        /// The colors for the keyframe points.
        /// </summary>
        protected Color[] Colors = Utilities.Utils.CurveKeyframesColors;

        /// <summary>
        /// The curve time/value axes tick steps.
        /// </summary>
        protected double[] TickSteps = Utilities.Utils.CurveTickSteps;

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
        public override Float2 ViewOffset
        {
            get => _mainPanel.ViewOffset;
            set
            {
                _mainPanel.ViewOffset = value;
                _mainPanel.FastScroll();
            }
        }

        /// <inheritdoc />
        public override Float2 ViewScale
        {
            get => _contents.Scale;
            set => _contents.Scale = Float2.Clamp(value, new Float2(0.0001f), new Float2(1000.0f));
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
                {
                    // Synchronize selection for curve points when collapsed so all points fo the keyframe are selected
                    for (var i = 0; i < _points.Count; i++)
                    {
                        var p = _points[i];
                        if (p.IsSelected)
                        {
                            for (var j = 0; j < _points.Count; j++)
                            {
                                var q = _points[j];
                                if (q.Index == p.Index)
                                    q.IsSelected = true;
                            }
                        }
                    }

                    ShowWholeCurve();
                }
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
            Accessor.GetDefaultValue(out DefaultValue);

            var style = Style.Current;
            _contentsColor = style.Background.RGBMultiplied(0.7f);
            _linesColor = style.ForegroundDisabled.RGBMultiplied(0.7f);
            _labelsColor = style.ForegroundDisabled;
            _labelsFont = style.FontSmall;

            _mainPanel = new Panel(ScrollBars.Both)
            {
                ScrollMargin = new Margin(150.0f),
                AlwaysShowScrollbars = false,
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
        /// Gets the time of the keyframe.
        /// </summary>
        /// <param name="index">The keyframe index.</param>
        /// <returns>The keyframe time.</returns>
        protected abstract float GetKeyframeTime(int index);

        /// <summary>
        /// Gets the time of the keyframe.
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
        protected abstract void AddKeyframe(Float2 keyframesPos);

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

        private void EditAllKeyframes(Control control, Float2 pos)
        {
            _popup = new Popup(this, new object[] { GetAllKeyframesEditingProxy() }, null, 400.0f);
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
            var keyframes = GetKeyframes();
            for (int i = 0; i < keyframeIndices.Count; i++)
                selection[i] = keyframes[keyframeIndices[i]];
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
            var indicesToRemove = new HashSet<int>();
            for (int i = 0; i < _points.Count; i++)
            {
                var p = _points[i];
                if (p.IsSelected)
                {
                    p.IsSelected = false;
                    indicesToRemove.Add(p.Index);
                }
            }

            OnEditingStart();
            RemoveKeyframesInternal(indicesToRemove);
            OnKeyframesChanged();
            OnEdited();
            OnEditingEnd();
        }

        private void ShowCurve(bool selectedOnly)
        {
            if (_points.Count == 0)
                return;
            int pass = 1;
            REDO:

            // Get curve bounds in Keyframes (time and value)
            Float2 posMin = Float2.Maximum, posMax = Float2.Minimum;
            // TODO: include bezier curve bounds calculation to handle curve outside the bounds made out of points
            foreach (var point in _points)
            {
                if (selectedOnly && !point.IsSelected)
                    continue;
                var pos = point.Point;
                Float2.Min(ref posMin, ref pos, out posMin);
                Float2.Max(ref posMax, ref pos, out posMax);
            }

            // Apply margin around the area
            var posMargin = (posMax - posMin) * 0.05f;
            posMin -= posMargin;
            posMax += posMargin;

            // Convert from Keyframes to Contents
            _mainPanel.GetDesireClientArea(out var viewRect);
            PointFromKeyframesToContents(ref posMin, ref viewRect);
            PointFromKeyframesToContents(ref posMax, ref viewRect);
            var tmp = posMin;
            Float2.Min(ref posMin, ref posMax, out posMin);
            Float2.Max(ref posMax, ref tmp, out posMax);
            var contentsSize = posMax - posMin;

            // Convert from Contents to Main Panel
            posMin = _contents.PointToParent(posMin);
            posMax = _contents.PointToParent(posMax);
            tmp = posMin;
            Float2.Min(ref posMin, ref posMax, out posMin);
            Float2.Max(ref posMax, ref tmp, out posMax);

            // Update zoom (leave unchanged when focusing a single point)
            var zoomMask = EnableZoom;
            if (Mathf.IsZero(posMargin.X))
                zoomMask &= ~UseMode.Horizontal;
            if (Mathf.IsZero(posMargin.Y))
                zoomMask &= ~UseMode.Vertical;
            ViewScale = ApplyUseModeMask(zoomMask, viewRect.Size / contentsSize, ViewScale);

            // Update scroll (attempt to center the area when it's smaller than the view)
            Float2 viewOffset = -posMin;
            Float2 viewSize = _mainPanel.Size;
            Float2 viewSizeLeft = viewSize - Float2.Clamp(posMax - posMin, Float2.Zero, viewSize);
            viewOffset += viewSizeLeft * 0.5f;
            viewOffset = ApplyUseModeMask(EnablePanning, viewOffset, _mainPanel.ViewOffset);
            _mainPanel.ViewOffset = viewOffset;

            // Do it multiple times so the view offset can be properly calculate once the view scale gets changes
            if (pass++ <= 2)
                goto REDO;

            UpdateKeyframes();
        }

        /// <summary>
        /// Focuses the view on the selected keyframes.
        /// </summary>
        public void FocusSelection()
        {
            // Fallback to showing whole curve if nothing is selected
            ShowCurve(SelectionCount != 0);
        }

        /// <inheritdoc />
        public override void ShowWholeCurve()
        {
            ShowCurve(false);
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
                if (ShowCollapsed)
                {
                    for (int i = 0; i < _points.Count; i++)
                        if (_points[i].Component == 0 && _points[i].IsSelected)
                            result++;
                }
                else
                {
                    for (int i = 0; i < _points.Count; i++)
                        if (_points[i].IsSelected)
                            result++;
                }
                return result;
            }
        }

        /// <inheritdoc />
        public override void ClearSelection()
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

        /// <summary>
        /// Converts the input point from curve editor control space into the keyframes time/value coordinates.
        /// </summary>
        /// <param name="point">The point.</param>
        /// <param name="curveContentAreaBounds">The curve contents area bounds.</param>
        /// <returns>The result.</returns>
        protected Float2 PointToKeyframes(Float2 point, ref Rectangle curveContentAreaBounds)
        {
            // Curve Editor -> Main Panel
            point = _mainPanel.PointFromParent(point);

            // Main Panel -> Contents
            point = _contents.PointFromParent(point);

            // Contents -> Keyframes
            return PointFromContentsToKeyframes(ref point, ref curveContentAreaBounds);
        }

        /// <summary>
        /// Converts the input point from the keyframes time/value coordinates into the curve editor control space.
        /// </summary>
        /// <param name="point">The point.</param>
        /// <param name="curveContentAreaBounds">The curve contents area bounds.</param>
        /// <returns>The result.</returns>
        protected Float2 PointFromKeyframes(Float2 point, ref Rectangle curveContentAreaBounds)
        {
            // Keyframes -> Contents
            PointFromKeyframesToContents(ref point, ref curveContentAreaBounds);

            // Contents -> Main Panel
            point = _contents.PointToParent(point);

            // Main Panel -> Curve Editor
            return _mainPanel.PointToParent(point);
        }

        internal Float2 PointFromContentsToKeyframes(ref Float2 point, ref Rectangle curveContentAreaBounds)
        {
            return new Float2(
                              (point.X + _contents.Location.X) / UnitsPerSecond,
                              (point.Y + _contents.Location.Y - curveContentAreaBounds.Height) / -UnitsPerSecond
                             );
        }

        internal void PointFromKeyframesToContents(ref Float2 point, ref Rectangle curveContentAreaBounds)
        {
            point = new Float2(
                               point.X * UnitsPerSecond - _contents.Location.X,
                               point.Y * -UnitsPerSecond + curveContentAreaBounds.Height - _contents.Location.Y
                              );
        }

        private void DrawAxis(Float2 axis, Rectangle viewRect, float min, float max, float pixelRange)
        {
            Utilities.Utils.DrawCurveTicks((decimal tick, double step, float strength) =>
            {
                var p = PointFromKeyframes(axis * (float)tick, ref viewRect);

                // Draw line
                var lineRect = new Rectangle
                (
                 viewRect.Location + (p - 0.5f) * axis,
                 Float2.Lerp(viewRect.Size, Float2.One, axis)
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
            }, TickSteps, ref _tickStrengths, min, max, pixelRange);
        }

        private void SetupGrid(out Float2 min, out Float2 max, out Float2 pixelRange)
        {
            var viewRect = _mainPanel.GetClientArea();
            var upperLeft = PointToKeyframes(viewRect.Location, ref viewRect);
            var bottomRight = PointToKeyframes(viewRect.Size, ref viewRect);

            min = Float2.Min(upperLeft, bottomRight);
            max = Float2.Max(upperLeft, bottomRight);
            pixelRange = (max - min) * ViewScale * UnitsPerSecond;
        }

        private Float2 GetGridSnap()
        {
            SetupGrid(out var min, out var max, out var pixelRange);
            return new Float2(Utilities.Utils.GetCurveGridSnap(TickSteps, ref _tickStrengths, min.X, max.X, pixelRange.X),
                              Utilities.Utils.GetCurveGridSnap(TickSteps, ref _tickStrengths, min.Y, max.Y, pixelRange.Y));
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
            var rect = new Rectangle(Float2.Zero, Size);
            var viewRect = _mainPanel.GetClientArea();

            // Draw background
            if (ShowBackground)
            {
                Render2D.FillRectangle(rect, _contentsColor);
            }

            // Draw time and values axes
            if (ShowAxes != UseMode.Off)
            {
                SetupGrid(out var min, out var max, out var pixelRange);

                Render2D.PushClip(ref viewRect);

                if ((ShowAxes & UseMode.Vertical) == UseMode.Vertical)
                    DrawAxis(Float2.UnitX, viewRect, min.X, max.X, pixelRange.X);
                if ((ShowAxes & UseMode.Horizontal) == UseMode.Horizontal)
                    DrawAxis(Float2.UnitY, viewRect, min.Y, max.Y, pixelRange.Y);

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
                UpdateTangents();
                return true;
            }
            else if (options.DeselectAll.Process(this))
            {
                DeselectAll();
                UpdateTangents();
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
            else if (options.FocusSelection.Process(this))
            {
                FocusSelection();
                return true;
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
            KeyframesEditorContext = null;

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
        /// <returns>The index of the keyframe.</returns>
        public int AddKeyframe(LinearCurve<T>.Keyframe k)
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
            return pos;
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
        private Float2 GetKeyframePoint(ref LinearCurve<T>.Keyframe k, int component)
        {
            return new Float2(k.Time, Accessor.GetCurveValue(ref k.Value, component));
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
        protected override float GetKeyframeTime(int index)
        {
            return _keyframes[index].Time;
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
        protected override void AddKeyframe(Float2 keyframesPos)
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
        public override int AddKeyframe(float time, object value)
        {
            return AddKeyframe(new LinearCurve<T>.Keyframe(time, (T)value));
        }

        /// <inheritdoc />
        public override int AddKeyframe(float time, object value, object tangentIn, object tangentOut)
        {
            return AddKeyframe(new LinearCurve<T>.Keyframe(time, (T)value));
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
        public override void SetKeyframeValue(int index, object value, object tangentIn, object tangentOut)
        {
            var k = _keyframes[index];
            k.Value = (T)value;
            _keyframes[index] = k;

            UpdateKeyframes();
            UpdateTooltips();
            OnEdited();
        }

        /// <inheritdoc />
        public override Float2 GetKeyframePoint(int index, int component)
        {
            var k = _keyframes[index];
            return new Float2(k.Time, Accessor.GetCurveValue(ref k.Value, component));
        }

        /// <inheritdoc />
        public override void OnKeyframesGet(string trackName, Action<string, float, object> get)
        {
            for (int i = 0; i < _keyframes.Count; i++)
            {
                var k = _keyframes[i];
                get(trackName, k.Time, k);
            }
        }

        /// <inheritdoc />
        public override void OnKeyframesSet(List<KeyValuePair<float, object>> keyframes)
        {
            OnEditingStart();
            _keyframes.Clear();
            if (keyframes != null)
            {
                foreach (var e in keyframes)
                {
                    var k = (LinearCurve<T>.Keyframe)e.Value;
                    _keyframes.Add(new LinearCurve<T>.Keyframe(e.Key, k.Value));
                }
            }
            OnKeyframesChanged();
            OnEdited();
            OnEditingEnd();
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
                var point = new Float2
                (
                 location.X * UnitsPerSecond - p.Width * 0.5f,
                 location.Y * -UnitsPerSecond - p.Height * 0.5f + curveContentAreaBounds.Height
                );

                if (_showCollapsed)
                {
                    point.Y = 1.0f;
                    p.Size = new Float2(KeyframesSize.X / viewScale.X, Height - 2.0f);
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
                bounds = Rectangle.Union(bounds, _points[i].Bounds);

            // Adjust contents bounds to fill the curve area
            if (EnablePanning != UseMode.Off || !ShowCollapsed)
            {
                bounds.Width = Mathf.Max(bounds.Width, 1.0f);
                bounds.Height = Mathf.Max(bounds.Height, 1.0f);
                bounds.Location = ApplyUseModeMask(EnablePanning, bounds.Location, _contents.Location);
                bounds.Size = ApplyUseModeMask(EnablePanning, bounds.Size, _contents.Size);
                if (!_contents._isMovingSelection)
                    _contents.Bounds = bounds;
            }
            else if (_contents.Bounds == Rectangle.Empty)
            {
                _contents.Bounds = Rectangle.Union(bounds, new Rectangle(Float2.Zero, Float2.One));
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
        /// <returns>The index of the keyframe.</returns>
        public int AddKeyframe(BezierCurve<T>.Keyframe k)
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
            return pos;
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
                    slope = -slope;
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
        private Float2 GetKeyframePoint(ref BezierCurve<T>.Keyframe k, int component)
        {
            return new Float2(k.Time, Accessor.GetCurveValue(ref k.Value, component));
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
        protected override float GetKeyframeTime(int index)
        {
            return _keyframes[index].Time;
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
        protected override void AddKeyframe(Float2 keyframesPos)
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
        public override int AddKeyframe(float time, object value)
        {
            return AddKeyframe(new BezierCurve<T>.Keyframe(time, (T)value));
        }

        /// <inheritdoc />
        public override int AddKeyframe(float time, object value, object tangentIn, object tangentOut)
        {
            return AddKeyframe(new BezierCurve<T>.Keyframe(time, (T)value, (T)tangentIn, (T)tangentOut));
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
        public override void SetKeyframeValue(int index, object value, object tangentIn, object tangentOut)
        {
            var k = _keyframes[index];
            k.Value = (T)value;
            k.TangentIn = (T)tangentIn;
            k.TangentOut = (T)tangentOut;
            _keyframes[index] = k;

            UpdateKeyframes();
            UpdateTooltips();
            OnEdited();
        }

        /// <inheritdoc />
        public override Float2 GetKeyframePoint(int index, int component)
        {
            var k = _keyframes[index];
            return new Float2(k.Time, Accessor.GetCurveValue(ref k.Value, component));
        }

        /// <inheritdoc />
        public override void OnKeyframesGet(string trackName, Action<string, float, object> get)
        {
            for (int i = 0; i < _keyframes.Count; i++)
            {
                var k = _keyframes[i];
                get(trackName, k.Time, k);
            }
        }

        /// <inheritdoc />
        public override void OnKeyframesSet(List<KeyValuePair<float, object>> keyframes)
        {
            OnEditingStart();
            _keyframes.Clear();
            if (keyframes != null)
            {
                foreach (var e in keyframes)
                {
                    var k = (BezierCurve<T>.Keyframe)e.Value;
                    _keyframes.Add(new BezierCurve<T>.Keyframe(e.Key, k.Value, k.TangentIn, k.TangentOut));
                }
            }
            OnKeyframesChanged();
            OnEdited();
            OnEditingEnd();
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
            var pointsSize = _showCollapsed ? new Float2(4.0f / viewScale.X, Height - 2.0f) : KeyframesSize / viewScale;
            for (int i = 0; i < _points.Count; i++)
            {
                var p = _points[i];
                var k = _keyframes[p.Index];
                var point = GetKeyframePoint(ref k, p.Component);
                var location = new Float2
                (
                 point.X * UnitsPerSecond - pointsSize.X * 0.5f,
                 point.Y * -UnitsPerSecond - pointsSize.Y * 0.5f + curveContentAreaBounds.Height
                );
                if (_showCollapsed)
                {
                    location.Y = 1.0f;
                    p.Visible = p.Component == 0;
                }
                else
                {
                    p.Visible = true;
                }
                p.Bounds = new Rectangle(location, pointsSize);
            }

            // Calculate bounds
            var bounds = _points[0].Bounds;
            for (int i = 1; i < _points.Count; i++)
                bounds = Rectangle.Union(bounds, _points[i].Bounds);

            // Adjust contents bounds to fill the curve area
            if (EnablePanning != UseMode.Off || !ShowCollapsed)
            {
                bounds.Width = Mathf.Max(bounds.Width, 1.0f);
                bounds.Height = Mathf.Max(bounds.Height, 1.0f);
                bounds.Location = ApplyUseModeMask(EnablePanning, bounds.Location, _contents.Location);
                bounds.Size = ApplyUseModeMask(EnablePanning, bounds.Size, _contents.Size);
                if (!_contents._isMovingSelection)
                    _contents.Bounds = bounds;
            }
            else if (_contents.Bounds == Rectangle.Empty)
            {
                _contents.Bounds = Rectangle.Union(bounds, new Rectangle(Float2.Zero, Float2.One));
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
                    var offset = t.TangentOffset;
                    var location = GetKeyframePoint(ref k, selectedComponent);
                    t.Size = KeyframesSize / ViewScale;
                    t.Location = new Float2
                    (
                     location.X * UnitsPerSecond - t.Width * 0.5f + offset * direction,
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
        protected override void SetScaleInternal(ref Float2 scale)
        {
            base.SetScaleInternal(ref scale);

            if (!_showCollapsed)
            {
                // Refresh keyframes when zooming (their size depends on the scale)
                UpdateKeyframes();
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

                    var tangentScale = (endK.Time - startK.Time) / 3.0f;
                    var p1 = PointFromKeyframes(start, ref viewRect);
                    var p2 = PointFromKeyframes(start + new Float2(0, startTangent * tangentScale), ref viewRect);
                    var p3 = PointFromKeyframes(end + new Float2(0, endTangent * tangentScale), ref viewRect);
                    var p4 = PointFromKeyframes(end, ref viewRect);

                    Render2D.DrawSpline(p1, p2, p3, p4, color);
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
