// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using FlaxEditor.GUI.Timeline.Undo;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Timeline.GUI
{
    /// <summary>
    /// Timeline ending edge control that can be used to modify timeline duration with a mouse.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    class TimelineEdge : Control
    {
        private Timeline _timeline;
        private bool _isMoving;
        private Float2 _startMoveLocation;
        private int _startMoveDuration;
        private bool _isStart;
        private bool _canEdit;

        /// <summary>
        /// Initializes a new instance of the <see cref="TimelineEdge"/> class.
        /// </summary>
        /// <param name="timeline">The parent timeline.</param>
        /// <param name="isStart">True if edge edits the timeline start, otherwise it's for the ending cap.</param>
        /// <param name="canEdit">True if can edit the edge.</param>
        public TimelineEdge(Timeline timeline, bool isStart, bool canEdit)
        {
            AutoFocus = false;
            _timeline = timeline;
            _isStart = isStart;
            _canEdit = canEdit;
        }

        /// <inheritdoc />
        public override void Draw()
        {
            var style = Style.Current;
            var timeAxisOverlap = Timeline.HeaderTopAreaHeight * 0.5f;
            var timeAxisHeaderOffset = -_timeline.MediaBackground.ViewOffset.Y - timeAxisOverlap;

            var moveColor = style.ProgressNormal;
            var thickness = 2.0f;
            var borderColor = _isMoving ? moveColor : (IsMouseOver && _canEdit ? Color.Yellow : style.BorderNormal);
            Render2D.FillRectangle(new Rectangle((Width - thickness) * 0.5f, timeAxisHeaderOffset, thickness, Height - timeAxisHeaderOffset), borderColor);
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (base.OnMouseDown(location, button))
                return true;

            if (button == MouseButton.Left && _canEdit)
            {
                _isMoving = true;
                _startMoveLocation = Root.MousePosition;
                _startMoveDuration = _timeline.DurationFrames;

                StartMouseCapture(true);

                return true;
            }

            return false;
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            if (_isMoving && !_timeline.RootWindow.Window.IsMouseFlippingHorizontally)
            {
                var moveLocation = Root.MousePosition;
                var moveLocationDelta = moveLocation - _startMoveLocation + _timeline.Root.TrackingMouseOffset.X;
                var moveDelta = (int)(moveLocationDelta.X / (Timeline.UnitsPerSecond * _timeline.Zoom) * _timeline.FramesPerSecond);
                var durationFrames = _timeline.DurationFrames;

                if (_isStart)
                {
                    // TODO: editing timeline start frame?
                }
                else
                {
                    _timeline.DurationFrames = _startMoveDuration + moveDelta;
                    _timeline.MediaBackground.HScrollBar.Value = _timeline.MediaBackground.HScrollBar.Maximum;
                }

                if (_timeline.DurationFrames != durationFrames)
                {
                    _timeline.MarkAsEdited();
                }
            }
            else
            {
                base.OnMouseMove(location);
            }
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Left && _isMoving)
            {
                EndMoving();
                return true;
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override void OnEndMouseCapture()
        {
            if (_isMoving)
            {
                EndMoving();
            }

            base.OnEndMouseCapture();
        }

        /// <inheritdoc />
        public override void OnLostFocus()
        {
            if (_isMoving)
            {
                EndMoving();
            }

            base.OnLostFocus();
        }

        private void EndMoving()
        {
            var duration = _timeline.DurationFrames;
            if (duration != _startMoveDuration)
            {
                _timeline.DurationFrames = _startMoveDuration;
                using (new TimelineUndoBlock(_timeline))
                    _timeline.DurationFrames = duration;
            }
            _isMoving = false;
            _timeline.MediaBackground.HScrollBar.Value = _timeline.MediaBackground.HScrollBar.Maximum;

            EndMouseCapture();
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (_isMoving)
            {
                EndMoving();
            }
            _timeline = null;

            base.OnDestroy();
        }
    }
}
