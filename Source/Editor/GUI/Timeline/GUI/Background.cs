// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Globalization;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Timeline.GUI
{
    /// <summary>
    /// The timeline background control.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    class Background : ContainerControl
    {
        private readonly Timeline _timeline;
        private double[] _tickSteps;
        private float[] _tickStrengths;
        private bool _isSelecting;
        private Float2 _selectingStartPos = Float2.Minimum;
        private Float2 _mousePos = Float2.Minimum;

        /// <summary>
        /// Initializes a new instance of the <see cref="Background"/> class.
        /// </summary>
        /// <param name="timeline">The timeline.</param>
        public Background(Timeline timeline)
        {
            _timeline = timeline;
            _tickSteps = Utilities.Utils.CurveTickSteps;
        }

        private void UpdateSelectionRectangle()
        {
            var selectionRect = Rectangle.FromPoints(_selectingStartPos, _mousePos);
            _timeline.OnKeyframesSelection(null, this, selectionRect);
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (base.OnMouseDown(location, button))
                return true;

            _mousePos = location;
            if (button == MouseButton.Left)
            {
                // Start selecting
                _isSelecting = true;
                _selectingStartPos = location;
                _timeline.OnKeyframesDeselect(null);
                Focus();
                StartMouseCapture();
                return true;
            }

            return false;
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            _mousePos = location;
            if (_isSelecting && button == MouseButton.Left)
            {
                // End selecting
                _isSelecting = false;
                EndMouseCapture();
                return true;
            }

            if (base.OnMouseUp(location, button))
                return true;

            return true;
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            _mousePos = location;

            // Selecting
            if (_isSelecting)
            {
                UpdateSelectionRectangle();
                return;
            }

            base.OnMouseMove(location);
        }

        /// <inheritdoc />
        public override void OnLostFocus()
        {
            if (_isSelecting)
            {
                _isSelecting = false;
                EndMouseCapture();
            }

            base.OnLostFocus();
        }

        /// <inheritdoc />
        public override void OnEndMouseCapture()
        {
            _isSelecting = false;
            EndMouseCapture();

            base.OnEndMouseCapture();
        }

        /// <inheritdoc />
        public override bool IntersectsContent(ref Float2 locationParent, out Float2 location)
        {
            // Pass all events
            location = PointFromParent(ref locationParent);
            return true;
        }

        /// <inheritdoc />
        public override void Draw()
        {
            var style = Style.Current;
            var mediaBackground = _timeline.MediaBackground;
            var tracks = _timeline.Tracks;
            var linesColor = style.BackgroundNormal;
            var areaLeft = -X;
            var areaRight = Parent.Width + mediaBackground.ControlsBounds.BottomRight.X;
            var height = Height;

            // Calculate the timeline range in the view to optimize background drawing
            Render2D.PeekClip(out var globalClipping);
            Render2D.PeekTransform(out var globalTransform);
            var globalRect = new Rectangle(globalTransform.M31 + areaLeft, globalTransform.M32, areaRight * globalTransform.M11, height * globalTransform.M22);
            var globalMask = Rectangle.Shared(globalClipping, globalRect);
            var globalTransformInv = Matrix3x3.Invert(globalTransform);
            var localRect = Rectangle.FromPoints(Matrix3x3.Transform2D(globalMask.UpperLeft, globalTransformInv), Matrix3x3.Transform2D(globalMask.BottomRight, globalTransformInv));
            var localRectMin = localRect.UpperLeft;
            var localRectMax = localRect.BottomRight;

            // Draw lines between tracks
            Render2D.DrawLine(new Float2(areaLeft, 0.5f), new Float2(areaRight, 0.5f), linesColor);
            for (int i = 0; i < tracks.Count; i++)
            {
                var track = tracks[i];
                if (track.Visible)
                {
                    var top = track.Bottom + 0.5f;
                    Render2D.DrawLine(new Float2(areaLeft, top), new Float2(areaRight, top), linesColor);
                }
            }

            // Highlight selected tracks
            for (int i = 0; i < tracks.Count; i++)
            {
                var track = tracks[i];
                if (track.Visible && _timeline.SelectedTracks.Contains(track) && _timeline.ContainsFocus)
                {
                    Render2D.FillRectangle(new Rectangle(areaLeft, track.Top, areaRight, track.Height), style.BackgroundSelected.RGBMultiplied(0.4f));
                }
            }

            // Setup time axis ticks
            var minDistanceBetweenTicks = 50.0f;
            var maxDistanceBetweenTicks = 100.0f;
            var zoom = Timeline.UnitsPerSecond * _timeline.Zoom;
            var left = Float2.Min(localRectMin, localRectMax).X;
            var right = Float2.Max(localRectMin, localRectMax).X;
            var leftFrame = Mathf.Floor((left - Timeline.StartOffset) / zoom) * _timeline.FramesPerSecond;
            var rightFrame = Mathf.Ceil((right - Timeline.StartOffset) / zoom) * _timeline.FramesPerSecond;
            var min = leftFrame;
            var max = rightFrame;

            // Draw vertical lines for time axis
            var pixelsInRange = _timeline.Zoom;
            var pixelRange = pixelsInRange * (max - min);
            var tickRange = Utilities.Utils.DrawCurveTicks((decimal tick, double step, float strength) =>
            {
                var time = (float)tick / _timeline.FramesPerSecond;
                var x = time * zoom + Timeline.StartOffset;
                var lineColor = style.ForegroundDisabled.RGBMultiplied(0.7f).AlphaMultiplied(strength);
                Render2D.FillRectangle(new Rectangle(x - 0.5f, 0, 1.0f, height), lineColor);
            }, _tickSteps, ref _tickStrengths, min, max, pixelRange, minDistanceBetweenTicks, maxDistanceBetweenTicks);
            var smallestTick = tickRange.X;
            var biggestTick = tickRange.Y;
            var tickLevels = biggestTick - smallestTick + 1;

            // Draw selection rectangle
            if (_isSelecting)
            {
                var selectionRect = Rectangle.FromPoints(_selectingStartPos, _mousePos);
                Render2D.FillRectangle(selectionRect, style.Selection);
                Render2D.DrawRectangle(selectionRect, style.SelectionBorder);
            }

            DrawChildren();

            // Disabled overlay
            for (int i = 0; i < tracks.Count; i++)
            {
                var track = tracks[i];
                if (track.DrawDisabled && track.IsExpandedAll)
                {
                    Render2D.FillRectangle(new Rectangle(areaLeft, track.Top, areaRight, track.Height), new Color(0, 0, 0, 100));
                }
            }

            // Darken area outside the duration
            {
                var outsideDurationAreaColor = new Color(0, 0, 0, 100);
                var leftSideMin = PointFromParent(Float2.Zero);
                var leftSideMax = BottomLeft;
                var rightSideMin = UpperRight;
                var rightSideMax = PointFromParent(Parent.BottomRight) + mediaBackground.ControlsBounds.BottomRight;
                Render2D.FillRectangle(new Rectangle(leftSideMin, leftSideMax.X - leftSideMin.X, height), outsideDurationAreaColor);
                Render2D.FillRectangle(new Rectangle(rightSideMin, rightSideMax.X - rightSideMin.X, height), outsideDurationAreaColor);
            }

            // Draw time axis header
            var timeAxisHeaderOffset = -_timeline.MediaBackground.ViewOffset.Y;
            var verticalLinesHeaderExtend = Timeline.HeaderTopAreaHeight * 0.5f;
            var timeShowMode = _timeline.TimeShowMode;
            Render2D.FillRectangle(new Rectangle(areaLeft, timeAxisHeaderOffset - Timeline.HeaderTopAreaHeight, areaRight - areaLeft, Timeline.HeaderTopAreaHeight), style.Background.RGBMultiplied(0.7f));
            for (int level = 0; level < tickLevels; level++)
            {
                float strength = _tickStrengths[smallestTick + level];
                if (strength <= Mathf.Epsilon)
                    continue;

                // Draw all ticks
                int l = Mathf.Clamp(smallestTick + level, 0, _tickSteps.Length - 2);
                var lStep = _tickSteps[l];
                var lNextStep = _tickSteps[l + 1];
                var startTick = Mathd.FloorToInt(min / lStep);
                var endTick = Mathd.CeilToInt(max / lStep);
                Color lineColor = style.Foreground.RGBMultiplied(0.8f).AlphaMultiplied(strength);
                Color labelColor = style.ForegroundDisabled.AlphaMultiplied(strength);
                for (var i = startTick; i <= endTick; i++)
                {
                    if (l < biggestTick && (i % Mathd.RoundToInt(lNextStep / lStep) == 0))
                        continue;
                    var tick = (decimal)lStep * i;
                    var time = (double)tick / _timeline.FramesPerSecond;
                    var x = (float)time * zoom + Timeline.StartOffset;

                    // Header line
                    var lineRect = new Rectangle((float)x - 0.5f, -verticalLinesHeaderExtend * 0.6f + timeAxisHeaderOffset, 1.0f, verticalLinesHeaderExtend * 0.6f);
                    Render2D.FillRectangle(lineRect, lineColor);

                    // Time label
                    string labelText;
                    switch (timeShowMode)
                    {
                    case Timeline.TimeShowModes.Frames:
                        labelText = tick.ToString("###0", CultureInfo.InvariantCulture);
                        break;
                    case Timeline.TimeShowModes.Seconds:
                        labelText = time.ToString("###0.##'s'", CultureInfo.InvariantCulture);
                        break;
                    case Timeline.TimeShowModes.Time:
                        labelText = TimeSpan.FromSeconds(time).ToString("g");
                        break;
                    default: throw new ArgumentOutOfRangeException();
                    }
                    var labelRect = new Rectangle(x + 2, -verticalLinesHeaderExtend * 0.8f + timeAxisHeaderOffset, 50, verticalLinesHeaderExtend);
                    Render2D.DrawText(style.FontSmall, labelText, labelRect, labelColor, TextAlignment.Near, TextAlignment.Center, TextWrapping.NoWrap, 1.0f, 0.8f);
                }
            }
        }

        /// <inheritdoc />
        public override bool OnMouseWheel(Float2 location, float delta)
        {
            if (base.OnMouseWheel(location, delta))
                return true;

            // Zoom in/out
            if (IsMouseOver && Root.GetKey(KeyboardKeys.Control))
            {
                var locationTimeOld = _timeline.MediaBackground.PointFromParent(_timeline, _timeline.Size * 0.5f).X;
                var frame = (locationTimeOld - Timeline.StartOffset * 2.0f) / _timeline.Zoom / Timeline.UnitsPerSecond * _timeline.FramesPerSecond;

                _timeline.Zoom += delta * 0.1f;

                var locationTimeNew = frame / _timeline.FramesPerSecond * Timeline.UnitsPerSecond * _timeline.Zoom + Timeline.StartOffset * 2.0f;
                var locationTimeDelta = locationTimeNew - locationTimeOld;
                var scroll = _timeline.MediaBackground.HScrollBar;
                if (scroll.Visible && scroll.Enabled)
                    scroll.TargetValue += locationTimeDelta;
                return true;
            }

            // Scroll view horizontally
            if (IsMouseOver && Root.GetKey(KeyboardKeys.Shift))
            {
                var scroll = _timeline.MediaBackground.HScrollBar;
                if (scroll.Visible && scroll.Enabled)
                {
                    scroll.TargetValue -= delta * Timeline.UnitsPerSecond / _timeline.Zoom;
                    return true;
                }
            }

            return false;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            // Cleanup
            _tickSteps = null;
            _tickStrengths = null;

            base.OnDestroy();
        }
    }
}
