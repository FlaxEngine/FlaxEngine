// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.CustomEditors;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Timeline.Undo;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Timeline
{
    partial class Timeline
    {
        private sealed class TimeIntervalsHeader : ContainerControl
        {
            private Timeline _timeline;
            private bool _isLeftMouseButtonDown;

            public TimeIntervalsHeader(Timeline timeline)
            {
                _timeline = timeline;
            }

            /// <inheritdoc />
            public override bool OnMouseDown(Float2 location, MouseButton button)
            {
                if (base.OnMouseDown(location, button))
                    return true;

                if (button == MouseButton.Left)
                {
                    _isLeftMouseButtonDown = true;
                    _timeline._isMovingPositionHandle = true;
                    StartMouseCapture();
                    Seek(ref location);
                    Focus();
                    return true;
                }

                return false;
            }

            /// <inheritdoc />
            public override void OnMouseMove(Float2 location)
            {
                base.OnMouseMove(location);

                if (_isLeftMouseButtonDown)
                {
                    Seek(ref location);
                }
            }

            /// <inheritdoc />
            public override void OnMouseEnter(Float2 location)
            {
                base.OnMouseEnter(location);
                Cursor = CursorType.Hand;
            }

            /// <inheritdoc />
            public override void OnMouseLeave()
            {
                Cursor = CursorType.Default;
                base.OnMouseLeave();
            }

            /// <inheritdoc />
            public override void Defocus()
            {
                Cursor = CursorType.Default;
                base.Defocus();
            }

            private void Seek(ref Float2 location)
            {
                if (_timeline.PlaybackState == PlaybackStates.Disabled)
                    return;

                var locationTimeline = PointToParent(_timeline, location);
                var locationTime = _timeline._backgroundArea.PointFromParent(_timeline, locationTimeline);
                var frame = (locationTime.X - StartOffset * 2.0f) / _timeline.Zoom / UnitsPerSecond * _timeline.FramesPerSecond;
                _timeline.OnSeek((int)frame);
            }

            /// <inheritdoc />
            public override bool OnMouseUp(Float2 location, MouseButton button)
            {
                if (base.OnMouseUp(location, button))
                    return true;

                if (button == MouseButton.Left && _isLeftMouseButtonDown)
                {
                    Seek(ref location);
                    EndMouseCapture();
                    return true;
                }

                return false;
            }

            /// <inheritdoc />
            public override void OnEndMouseCapture()
            {
                _isLeftMouseButtonDown = false;
                _timeline._isMovingPositionHandle = false;

                base.OnEndMouseCapture();
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                _timeline = null;

                base.OnDestroy();
            }
        }

        class PropertiesEditPopup : ContextMenuBase
        {
            private Timeline _timeline;
            private bool _isDirty;
            private byte[] _beforeData;
            private object _undoContext;

            public PropertiesEditPopup(Timeline timeline, object obj, object undoContext)
            {
                const float width = 280.0f;
                const float height = 160.0f;
                Size = new Float2(width, height);

                var panel1 = new Panel(ScrollBars.Vertical)
                {
                    Bounds = new Rectangle(0, 0.0f, width, height),
                    Parent = this
                };
                var editor = new CustomEditorPresenter(null);
                editor.Panel.AnchorPreset = AnchorPresets.HorizontalStretchTop;
                editor.Panel.IsScrollable = true;
                editor.Panel.Parent = panel1;
                editor.Modified += OnModified;

                editor.Select(obj);

                _timeline = timeline;
                if (timeline.Undo != null && timeline.Undo.Enabled && undoContext != null)
                {
                    _undoContext = undoContext;
                    if (undoContext is Track track)
                        _beforeData = EditTrackAction.CaptureData(track);
                    else if (undoContext is Timeline)
                        _beforeData = EditTimelineAction.CaptureData(timeline);
                }
            }

            private void OnModified()
            {
                _isDirty = true;
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
                    if (_beforeData != null)
                    {
                        if (_undoContext is Track track)
                        {
                            var after = EditTrackAction.CaptureData(track);
                            if (!Utils.ArraysEqual(_beforeData, after))
                                _timeline.Undo.AddAction(new EditTrackAction(_timeline, track, _beforeData, after));
                        }
                        else if (_undoContext is Timeline)
                        {
                            var after = EditTimelineAction.CaptureData(_timeline);
                            if (!Utils.ArraysEqual(_beforeData, after))
                                _timeline.Undo.AddAction(new EditTimelineAction(_timeline, _beforeData, after));
                        }
                    }
                    _timeline.MarkAsEdited();
                }

                base.Hide();
            }

            /// <inheritdoc />
            public override bool OnKeyDown(KeyboardKeys key)
            {
                if (key == KeyboardKeys.Escape)
                {
                    Hide();
                    return true;
                }

                return base.OnKeyDown(key);
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                _timeline = null;
                _beforeData = null;
                _undoContext = null;

                base.OnDestroy();
            }
        }

        class TracksPanelArea : Panel
        {
            private DragDropEffect _currentDragEffect = DragDropEffect.None;
            private Timeline _timeline;
            private bool _needSetup = true;

            public TracksPanelArea(Timeline timeline)
            : base(ScrollBars.Vertical)
            {
                _timeline = timeline;
            }

            /// <inheritdoc />
            public override DragDropEffect OnDragEnter(ref Float2 location, DragData data)
            {
                var result = base.OnDragEnter(ref location, data);
                if (result == DragDropEffect.None)
                {
                    if (_needSetup)
                    {
                        _needSetup = false;
                        _timeline.SetupDragDrop();
                    }
                    for (int i = 0; i < _timeline.DragHandlers.Count; i++)
                    {
                        var dragHelper = _timeline.DragHandlers[i].Helper;
                        if (dragHelper.OnDragEnter(data))
                        {
                            result = dragHelper.Effect;
                            break;
                        }
                    }
                    _currentDragEffect = result;
                }

                return result;
            }

            /// <inheritdoc />
            public override DragDropEffect OnDragMove(ref Float2 location, DragData data)
            {
                var result = base.OnDragEnter(ref location, data);
                if (result == DragDropEffect.None)
                {
                    result = _currentDragEffect;
                }

                return result;
            }

            /// <inheritdoc />
            public override void OnDragLeave()
            {
                _currentDragEffect = DragDropEffect.None;
                _timeline.DragHandlers.ForEach(x => x.Helper.OnDragLeave());

                base.OnDragLeave();
            }

            /// <inheritdoc />
            public override DragDropEffect OnDragDrop(ref Float2 location, DragData data)
            {
                var result = base.OnDragDrop(ref location, data);
                if (result == DragDropEffect.None && _currentDragEffect != DragDropEffect.None)
                {
                    for (int i = 0; i < _timeline.DragHandlers.Count; i++)
                    {
                        var e = _timeline.DragHandlers[i];
                        if (e.Helper.HasValidDrag)
                        {
                            e.Action(_timeline, e.Helper);
                        }
                    }
                }

                return result;
            }

            /// <inheritdoc />
            public override void Draw()
            {
                if (IsDragOver && _currentDragEffect != DragDropEffect.None)
                {
                    var style = Style.Current;
                    var bounds = new Rectangle(Float2.Zero, Size);
                    Render2D.FillRectangle(bounds, style.Selection);
                    Render2D.DrawRectangle(bounds, style.SelectionBorder);
                }

                base.Draw();
            }

            /// <inheritdoc />
            public override void OnDestroy()
            {
                _timeline = null;

                base.OnDestroy();
            }
        }
    }
}
