// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Globalization;
using System.Text;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Json;
using FlaxEngine.Utilities;

namespace FlaxEditor.GUI
{
    partial class CurveEditor<T>
    {
        /// <summary>
        /// The curve contents container control.
        /// </summary>
        /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
        protected class ContentsBase : ContainerControl
        {
            private readonly CurveEditor<T> _editor;
            internal bool _leftMouseDown;
            private bool _rightMouseDown;
            internal Float2 _leftMouseDownPos = Float2.Minimum;
            private Float2 _rightMouseDownPos = Float2.Minimum;
            private Float2 _movingViewLastPos;
            internal Float2 _mousePos = Float2.Minimum;
            internal bool _isMovingSelection;
            internal bool _isMovingTangent;
            internal bool _movedView;
            internal bool _movedKeyframes;
            internal bool _toggledSelection;
            private TangentPoint _movingTangent;
            private Float2 _movingSelectionStart;
            private Float2 _movingSelectionStartPosLock;
            private Float2[] _movingSelectionOffsets;
            private Float2 _cmShowPos;

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
                if (_editor.KeyframesEditorContext != null)
                    _editor.KeyframesEditorContext.OnKeyframesSelection(_editor, this, selectionRect);
                else
                    UpdateSelection(ref selectionRect);
            }

            internal void UpdateSelection(ref Rectangle selectionRect)
            {
                // Find controls to select
                var children = _children;
                for (int i = 0; i < children.Count; i++)
                {
                    if (children[i] is KeyframePoint p)
                        p.IsSelected = p.Bounds.Intersects(ref selectionRect);
                }
                _editor.UpdateTangents();
            }

            internal void OnMoveStart(Float2 location)
            {
                // Start moving selected keyframes
                _isMovingSelection = true;
                _movedKeyframes = false;
                var viewRect = _editor._mainPanel.GetClientArea();
                _movingSelectionStartPosLock = location;
                _movingSelectionStart = PointToKeyframes(location, ref viewRect);
                if (_movingSelectionOffsets == null || _movingSelectionOffsets.Length != _editor._points.Count)
                    _movingSelectionOffsets = new Float2[_editor._points.Count];
                for (int i = 0; i < _movingSelectionOffsets.Length; i++)
                    _movingSelectionOffsets[i] = _editor._points[i].Point - _movingSelectionStart;
                _editor.OnEditingStart();
            }

            internal void OnMove(Float2 location)
            {
                // Skip updating keyframes until move actual starts to be meaningful
                if (Float2.Distance(ref _movingSelectionStartPosLock, ref location) < 1.5f)
                    return;
                _movingSelectionStartPosLock = Float2.Minimum;

                var viewRect = _editor._mainPanel.GetClientArea();
                var locationKeyframes = PointToKeyframes(location, ref viewRect);
                var accessor = _editor.Accessor;
                var components = accessor.GetCurveComponents();
                var snapEnabled = Root.GetKey(KeyboardKeys.Control);
                var snapGrid = snapEnabled ? _editor.GetGridSnap() : Float2.One;
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

                        var offset = _movingSelectionOffsets[i];

                        if (!_editor.ShowCollapsed)
                        {
                            // Move on value axis
                            value = locationKeyframes.Y + offset.Y;
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
                            time = locationKeyframes.X + offset.X;
                        }

                        if (snapEnabled)
                        {
                            // Snap to the grid
                            var key = new Float2(time, value);
                            key = Float2.SnapToGrid(key, snapGrid);
                            time = key.X;
                            value = key.Y;
                        }

                        // Clamp and snap time to the valid range
                        if (isFirstSelected)
                        {
                            if (_editor.FPS.HasValue)
                            {
                                float fps = _editor.FPS.Value;
                                time = Mathf.Floor(time * fps) / fps;
                            }
                            time = Mathf.Clamp(time, minTime, maxTime);
                        }

                        _editor.SetKeyframeInternal(p.Index, time, value, p.Component);
                    }
                    _editor.UpdateKeyframes();
                    _editor.UpdateTooltips();
                    if (_editor.EnablePanning == UseMode.On)
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
                    var movingViewPos = Parent.PointToParent(PointToParent(location));
                    var delta = movingViewPos - _movingViewLastPos;
                    if (_editor.CustomViewPanning != null)
                        delta = _editor.CustomViewPanning(delta);
                    delta *= GetUseModeMask(_editor.EnablePanning);
                    if (delta.LengthSquared > 0.01f)
                    {
                        _editor._mainPanel.ViewOffset += delta;
                        _movingViewLastPos = movingViewPos;
                        _movedView = true;
                        if (_editor.CustomViewPanning != null)
                        {
                            if (Cursor == CursorType.Default)
                                Cursor = CursorType.SizeAll;
                        }
                        else
                        {
                            switch (_editor.EnablePanning)
                            {
                            case UseMode.Vertical: Cursor = CursorType.SizeNS; break;
                            case UseMode.Horizontal: Cursor = CursorType.SizeWE; break;
                            case UseMode.On: Cursor = CursorType.SizeAll; break;
                            }
                        }
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
                // Moving tangent
                else if (_isMovingTangent)
                {
                    var viewRect = _editor._mainPanel.GetClientArea();
                    var k = _editor.GetKeyframe(_movingTangent.Index);
                    var kv = _editor.GetKeyframeValue(k);
                    var value = _editor.Accessor.GetCurveValue(ref kv, _movingTangent.Component);
                    var tangent = PointToKeyframes(location, ref viewRect).Y - value;
                    if (Root.GetKey(KeyboardKeys.Control))
                        tangent = Float2.SnapToGrid(new Float2(0, tangent), _editor.GetGridSnap()).Y; // Snap tangent over Y axis
                    tangent = tangent * _editor.ViewScale.X * 2;
                    _movingTangent.TangentValue = tangent;
                    _editor.UpdateTangents();
                    Cursor = CursorType.SizeNS;
                    _movedKeyframes = true;
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
            public override bool OnMouseDown(Float2 location, MouseButton button)
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
                _toggledSelection = false;
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
                    _movedView = false;
                    _movingViewLastPos = Parent.PointToParent(PointToParent(location));
                }

                // Check if any node is under the mouse
                var underMouse = GetChildAt(location);
                if (underMouse is KeyframePoint keyframe)
                {
                    if (_leftMouseDown)
                    {
                        if (Root.GetKey(KeyboardKeys.Shift))
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
                            _editor.UpdateTangents();
                        }
                        else if (!keyframe.IsSelected)
                        {
                            // Select node
                            if (!Root.GetKey(KeyboardKeys.Control))
                            {
                                if (_editor.KeyframesEditorContext != null)
                                    _editor.KeyframesEditorContext.OnKeyframesDeselect(_editor);
                                else
                                    _editor.ClearSelection();
                            }
                            _toggledSelection = true;
                            keyframe.IsSelected = true;
                            _editor.UpdateTangents();
                        }
                        if (_editor.ShowCollapsed)
                        {
                            // Synchronize selection for curve points when collapsed so all points fo the keyframe are selected
                            for (var i = 0; i < _editor._points.Count; i++)
                            {
                                var p = _editor._points[i];
                                if (p.Index == keyframe.Index)
                                    p.IsSelected = keyframe.IsSelected;
                            }
                        }
                        StartMouseCapture();
                        Focus();
                        Tooltip?.Hide();
                        return true;
                    }
                }
                else if (underMouse is TangentPoint tangent && tangent.Visible)
                {
                    if (_leftMouseDown)
                    {
                        // Start moving tangent
                        StartMouseCapture();
                        _isMovingTangent = true;
                        _movedKeyframes = false;
                        _movingTangent = tangent;
                        _editor.OnEditingStart();
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
            public override bool OnMouseUp(Float2 location, MouseButton button)
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
                        if (_movedKeyframes)
                        {
                            _editor.OnEdited();
                            _editor.OnEditingEnd();
                            _editor.UpdateKeyframes();
                        }
                    }
                    // Moving keyframes
                    else if (_isMovingSelection)
                    {
                        if (_editor.KeyframesEditorContext != null)
                            _editor.KeyframesEditorContext.OnKeyframesMove(_editor, this, location, false, true);
                        else
                            OnMoveEnd(location);
                    }
                    // Toggle selection
                    else if (!_toggledSelection && Root.GetKey(KeyboardKeys.Control) && GetChildAt(location) is KeyframePoint keyframe)
                    {
                        keyframe.IsSelected = !keyframe.IsSelected;
                        _editor.UpdateTangents();
                    }

                    _isMovingSelection = false;
                    _isMovingTangent = false;
                    _movedKeyframes = false;
                }
                if (_rightMouseDown && button == MouseButton.Right)
                {
                    _rightMouseDown = false;
                    EndMouseCapture();
                    Cursor = CursorType.Default;

                    // Check if no move has been made at all
                    if (!_movedView)
                    {
                        var selectionCount = _editor.SelectionCount;
                        var point = GetChildAt(location) as KeyframePoint;
                        if (selectionCount == 0 && point != null)
                        {
                            // Select node
                            selectionCount = 1;
                            point.IsSelected = true;
                            if (_editor.ShowCollapsed)
                            {
                                for (int i = 0; i < _editor._points.Count; i++)
                                {
                                    var p = _editor._points[i];
                                    if (p.Index == point.Index)
                                        p.IsSelected = point.IsSelected;
                                }
                            }
                            _editor.UpdateTangents();
                        }

                        var viewRect = _editor._mainPanel.GetClientArea();
                        _cmShowPos = PointToKeyframes(location, ref viewRect);

                        var cm = new ContextMenu.ContextMenu();
                        cm.AddButton("Add keyframe", () => _editor.AddKeyframe(_cmShowPos)).Enabled = _editor.KeyframesCount < _editor.MaxKeyframes;
                        if (selectionCount > 0)
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
                        cm.AddButton("Edit all keyframes", () => _editor.EditAllKeyframes(this, location));
                        cm.AddButton("Select all keyframes", _editor.SelectAll);
                        cm.AddButton("Deselect all keyframes", _editor.DeselectAll);
                        cm.AddButton("Copy all keyframes", () =>
                        {
                            _editor.SelectAll();
                            _editor.CopyKeyframes(point);
                        });
                        if (_editor.EnableZoom != UseMode.Off || _editor.EnablePanning != UseMode.Off)
                        {
                            cm.AddSeparator();
                            cm.AddButton("Show whole curve", _editor.ShowWholeCurve);
                            cm.AddButton("Reset view", _editor.ResetView);
                        }
                        _editor.OnShowContextMenu(cm, selectionCount);
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
            public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
            {
                if (base.OnMouseDoubleClick(location, button))
                    return true;

                // Add keyframe on double click
                var child = GetChildAt(location);
                if (child is not KeyframePoint &&
                    child is not TangentPoint &&
                    _editor.KeyframesCount < _editor.MaxKeyframes)
                {
                    var viewRect = _editor._mainPanel.GetClientArea();
                    var pos = PointToKeyframes(location, ref viewRect);
                    _editor.AddKeyframe(pos);
                    return true;
                }

                return false;
            }

            /// <inheritdoc />
            public override bool OnMouseWheel(Float2 location, float delta)
            {
                if (base.OnMouseWheel(location, delta))
                    return true;

                // Zoom in/out
                var zoom = RootWindow.GetKey(KeyboardKeys.Control);
                var zoomAlt = RootWindow.GetKey(KeyboardKeys.Shift);
                if (_editor.EnableZoom != UseMode.Off && IsMouseOver && !_leftMouseDown && (zoom || zoomAlt))
                {
                    // Cache mouse location in curve-space
                    var viewRect = _editor._mainPanel.GetClientArea();
                    var locationInKeyframes = PointToKeyframes(location, ref viewRect);
                    var locationInEditorBefore = _editor.PointFromKeyframes(locationInKeyframes, ref viewRect);

                    // Scale relative to the curve size
                    var scale = new Float2(delta * 0.1f);
                    _editor._mainPanel.GetDesireClientArea(out var mainPanelArea);
                    var curveScale = mainPanelArea.Size / _editor._contents.Size;
                    scale *= curveScale;
                    if (zoomAlt)
                        scale.X = 0; // Scale Y axis only
                    scale *= GetUseModeMask(_editor.EnableZoom); // Mask scale depending on allowed usage
                    _editor.ViewScale += scale;

                    // Zoom towards the mouse position
                    var locationInEditorAfter = _editor.PointFromKeyframes(locationInKeyframes, ref viewRect);
                    var locationInEditorDelta = locationInEditorAfter - locationInEditorBefore;
                    _editor.ViewOffset -= locationInEditorDelta;
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
            /// Converts the input point from curve editor contents control space into the keyframes time/value coordinates.
            /// </summary>
            /// <param name="point">The point.</param>
            /// <param name="curveContentAreaBounds">The curve contents area bounds.</param>
            /// <returns>The result.</returns>
            private Float2 PointToKeyframes(Float2 point, ref Rectangle curveContentAreaBounds)
            {
                // Contents -> Keyframes
                return new Float2(
                                  (point.X + Location.X) / UnitsPerSecond,
                                  (point.Y + Location.Y - curveContentAreaBounds.Height) / -UnitsPerSecond
                                 );
            }
        }

        /// <inheritdoc />
        public override void OnKeyframesDeselect(IKeyframesEditor editor)
        {
            ClearSelection();
        }

        /// <inheritdoc />
        public override void OnKeyframesSelection(IKeyframesEditor editor, ContainerControl control, Rectangle selection)
        {
            if (_points.Count == 0)
                return;
            var selectionRect = Rectangle.FromPoints(_contents.PointFromParent(control, selection.UpperLeft), _contents.PointFromParent(control, selection.BottomRight));
            _contents.UpdateSelection(ref selectionRect);
        }

        /// <inheritdoc />
        public override int OnKeyframesSelectionCount()
        {
            return SelectionCount;
        }

        /// <inheritdoc />
        public override void OnKeyframesDelete(IKeyframesEditor editor)
        {
            RemoveKeyframesInner();
        }

        /// <inheritdoc />
        public override void OnKeyframesMove(IKeyframesEditor editor, ContainerControl control, Float2 location, bool start, bool end)
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
        public override void OnKeyframesCopy(IKeyframesEditor editor, float? timeOffset, StringBuilder data)
        {
            List<int> selectedIndices = null;
            for (int i = 0; i < _points.Count; i++)
            {
                var p = _points[i];
                if (p.IsSelected)
                {
                    if (selectedIndices == null)
                        selectedIndices = new List<int>();
                    if (!selectedIndices.Contains(p.Index))
                        selectedIndices.Add(p.Index);
                }
            }
            if (selectedIndices == null)
                return;
            var offset = timeOffset ?? 0.0f;
            data.AppendLine(KeyframesEditorUtils.CopyPrefix);
            data.AppendLine(ValueType.FullName);
            for (int i = 0; i < selectedIndices.Count; i++)
            {
                GetKeyframe(selectedIndices[i], out var time, out var value, out var tangentIn, out var tangentOut);
                data.AppendLine((time + offset).ToString(CultureInfo.InvariantCulture));
                data.AppendLine(JsonSerializer.Serialize(value).RemoveNewLine());
                data.AppendLine(JsonSerializer.Serialize(tangentIn).RemoveNewLine());
                data.AppendLine(JsonSerializer.Serialize(tangentOut).RemoveNewLine());
            }
        }

        /// <inheritdoc />
        public override void OnKeyframesPaste(IKeyframesEditor editor, float? timeOffset, string[] datas, ref int index)
        {
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
            try
            {
                var lines = data.Split(new[] { "\r\n", "\r", "\n" }, StringSplitOptions.RemoveEmptyEntries);
                if (lines.Length < 4)
                    return;
                var type = TypeUtils.GetManagedType(lines[0]);
                if (type == null)
                    throw new Exception($"Unknown type {lines[0]}.");
                if (type != DefaultValue.GetType())
                    throw new Exception($"Mismatching keyframes data type {type.FullName} when pasting into {DefaultValue.GetType().FullName}.");
                var count = (lines.Length - 1) / 4;
                OnEditingStart();
                index++;
                for (int i = 0; i < count; i++)
                {
                    var time = float.Parse(lines[i * 4 + 1], CultureInfo.InvariantCulture) + offset;
                    var value = JsonSerializer.Deserialize(lines[i * 4 + 2], type);
                    var tangentIn = JsonSerializer.Deserialize(lines[i * 4 + 3], type);
                    var tangentOut = JsonSerializer.Deserialize(lines[i * 4 + 4], type);
                    if (FPS.HasValue)
                    {
                        float fps = FPS.Value;
                        time = Mathf.Floor(time * fps) / fps;
                    }

                    var pos = AddKeyframe(time, value, tangentIn, tangentOut);
                    for (int j = 0; j < _points.Count; j++)
                    {
                        var p = _points[j];
                        if (p.Index == pos)
                            p.IsSelected = true;
                    }
                }
                OnEditingEnd();
                UpdateKeyframes();
                UpdateTangents();
            }
            catch (Exception ex)
            {
                Editor.LogWarning("Failed to paste keyframes.");
                Editor.LogWarning(ex.Message);
                Editor.LogWarning(ex);
            }
        }
    }
}
