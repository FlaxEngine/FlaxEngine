// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.GUI.Timeline.Undo;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Timeline
{
    /// <summary>
    /// Timeline track media event (range-based). Can be added to the timeline track.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    public abstract class Media : ContainerControl
    {
        /// <summary>
        /// The base class for media properties proxy objects.
        /// </summary>
        /// <typeparam name="TTrack">The type of the track.</typeparam>
        /// <typeparam name="TMedia">The type of the media.</typeparam>
        public abstract class ProxyBase<TTrack, TMedia>
        where TTrack : Track
        where TMedia : Media
        {
            /// <summary>
            /// The track reference/
            /// </summary>
            [HideInEditor, NoSerialize]
            public TTrack Track;

            /// <summary>
            /// The media reference.
            /// </summary>
            [HideInEditor, NoSerialize]
            public TMedia Media;

            /// <summary>
            /// Gets or sets the start frame of the media event (in frames).
            /// </summary>
            [EditorDisplay("General"), EditorOrder(-10010), VisibleIf(nameof(UseFrames)), Tooltip("Start frame of the media event (in frames).")]
            public int StartFrame
            {
                get => Media.StartFrame;
                set => Media.StartFrame = value;
            }

            /// <summary>
            /// Gets or sets the start frame of the media event (in seconds).
            /// </summary>
            [EditorDisplay("General"), EditorOrder(-10010), VisibleIf(nameof(UseFrames), true), Tooltip("Start frame of the media event (in seconds).")]
            public float Start
            {
                get => Media.Start;
                set => Media.Start = value;
            }

            /// <summary>
            /// Gets or sets the total duration of the media event (in frames).
            /// </summary>
            [EditorDisplay("General"), EditorOrder(-1000), Limit(1), VisibleIf(nameof(UseFrames)), Tooltip("Total duration of the media event (in frames).")]
            public int DurationFrames
            {
                get => Media.DurationFrames;
                set
                {
                    if (Media.CanResize)
                        Media.DurationFrames = value;
                }
            }

            /// <summary>
            /// Gets or sets the total duration of the timeline (in seconds).
            /// </summary>
            [EditorDisplay("General"), EditorOrder(-1000), Limit(0.0f, float.MaxValue, 0.001f), VisibleIf(nameof(UseFrames), true), Tooltip("Total duration of the timeline (in seconds).")]
            public float Duration
            {
                get => Media.Duration;
                set
                {
                    if (Media.CanResize)
                        Media.Duration = value;
                }
            }

            private bool UseFrames => Media.Timeline.TimeShowMode == Timeline.TimeShowModes.Frames;

            /// <summary>
            /// Initializes a new instance of the <see cref="ProxyBase{TTrack,TMedia}"/> class.
            /// </summary>
            /// <param name="track">The track.</param>
            /// <param name="media">The media.</param>
            protected ProxyBase(TTrack track, TMedia media)
            {
                Track = track;
                Media = media ?? throw new ArgumentNullException(nameof(media));
            }
        }

        private Timeline _timeline;
        private Track _tack;
        private int _startFrame, _durationFrames;
        private Float2 _mouseLocation = Float2.Minimum;
        internal bool _isMoving;
        private Float2 _startMoveLocation;
        private int _startMoveStartFrame;
        private int _startMoveDuration;
        private bool _startMoveLeftEdge;
        private bool _startMoveRightEdge;

        /// <summary>
        /// Gets or sets the start frame of the media event.
        /// </summary>
        public int StartFrame
        {
            get => _startFrame;
            set
            {
                if (_startFrame == value)
                    return;

                _startFrame = value;
                if (_timeline != null)
                {
                    OnTimelineZoomChanged();
                }
                OnStartFrameChanged();
            }
        }

        /// <summary>
        /// Occurs when start frame gets changed.
        /// </summary>
        public event Action StartFrameChanged;

        /// <summary>
        /// Gets the end frame of the media (start + duration).
        /// </summary>
        public int EndFrame => _startFrame + _durationFrames;

        /// <summary>
        /// Gets or sets the total duration of the media event in the timeline sequence frames amount.
        /// </summary>
        public int DurationFrames
        {
            get => _durationFrames;
            set
            {
                value = Math.Max(value, 1);
                if (_durationFrames == value)
                    return;

                _durationFrames = value;
                if (_timeline != null)
                {
                    OnTimelineZoomChanged();
                }
                OnDurationFramesChanged();
            }
        }

        /// <summary>
        /// Occurs when media duration gets changed.
        /// </summary>
        public event Action DurationFramesChanged;

        /// <summary>
        /// Get or sets the media start time in seconds.
        /// </summary>
        /// <seealso cref="StartFrame"/>
        public float Start
        {
            get => _startFrame / _timeline.FramesPerSecond;
            set => StartFrame = (int)(value * _timeline.FramesPerSecond);
        }

        /// <summary>
        /// Get or sets the media duration in seconds.
        /// </summary>
        /// <seealso cref="DurationFrames"/>
        public float Duration
        {
            get => _durationFrames / _timeline.FramesPerSecond;
            set => DurationFrames = (int)(value * _timeline.FramesPerSecond);
        }

        /// <summary>
        /// Gets the parent timeline.
        /// </summary>
        public Timeline Timeline => _timeline;

        /// <summary>
        /// Gets the track.
        /// </summary>
        public Track Track => _tack;

        private Rectangle MoveLeftEdgeRect => new Rectangle(-5, -5, 10, Height + 10);

        private Rectangle MoveRightEdgeRect => new Rectangle(Width - 5, -5, 10, Height + 10);

        /// <summary>
        /// The track properties editing proxy object. Assign it to add media properties editing support.
        /// </summary>
        public object PropertiesEditObject;

        /// <summary>
        /// Gets a value indicating whether this media can be split.
        /// </summary>
        public bool CanSplit;

        /// <summary>
        /// Gets a value indicating whether this media can be removed.
        /// </summary>
        public bool CanDelete;

        /// <summary>
        /// Gets a value indicating whether this media can be resized (duration changed).
        /// </summary>
        public bool CanResize = true;

        /// <summary>
        /// Initializes a new instance of the <see cref="Media"/> class.
        /// </summary>
        protected Media()
        {
            AutoFocus = false;
        }

        /// <summary>
        /// Called when showing timeline context menu to the user. Can be used to add custom buttons.
        /// </summary>
        /// <param name="menu">The menu.</param>
        /// <param name="time">The time (in seconds) at which context menu is shown (user clicked on a timeline).</param>
        /// <param name="controlUnderMouse">The found control under the mouse cursor.</param>
        public virtual void OnTimelineContextMenu(ContextMenu.ContextMenu menu, float time, Control controlUnderMouse)
        {
            if (CanDelete && Track.Media.Count > Track.MinMediaCount)
                menu.AddButton("Delete media", Delete);
        }

        /// <summary>
        /// Called when parent track gets changed.
        /// </summary>
        /// <param name="track">The track.</param>
        public virtual void OnTimelineChanged(Track track)
        {
            _timeline = track?.Timeline;
            _tack = track;
            Parent = _timeline?.MediaPanel;
            if (_timeline != null)
            {
                OnTimelineZoomChanged();
            }
        }

        /// <summary>
        /// Called when timeline FPS gets changed.
        /// </summary>
        /// <param name="before">The before value.</param>
        /// <param name="after">The after value.</param>
        public virtual void OnTimelineFpsChanged(float before, float after)
        {
            StartFrame = (int)((_startFrame / before) * after);
            DurationFrames = (int)((_durationFrames / before) * after);
        }

        /// <summary>
        /// Called when undo action gets reverted or applied for this media parent track data. Can be used to update UI.
        /// </summary>
        public virtual void OnUndo()
        {
        }

        /// <summary>
        /// Called when media gets removed by the user.
        /// </summary>
        public virtual void OnDeleted()
        {
            Dispose();
        }

        /// <summary>
        /// Called when media start frame gets changed.
        /// </summary>
        protected virtual void OnStartFrameChanged()
        {
            StartFrameChanged?.Invoke();
        }

        /// <summary>
        /// Called when media duration in frames gets changed.
        /// </summary>
        protected virtual void OnDurationFramesChanged()
        {
            DurationFramesChanged?.Invoke();
        }

        /// <summary>
        /// Called when timeline zoom gets changed.
        /// </summary>
        public virtual void OnTimelineZoomChanged()
        {
            X = Start * Timeline.UnitsPerSecond * _timeline.Zoom + Timeline.StartOffset;
            Width = Duration * Timeline.UnitsPerSecond * _timeline.Zoom;
        }

        /// <summary>
        /// Splits the media at the specified frame.
        /// </summary>
        /// <param name="frame">The frame to split at.</param>
        /// <returns>The another media created after this media split.</returns>
        public virtual Media Split(int frame)
        {
            var clone = (Media)Activator.CreateInstance(GetType());
            clone.StartFrame = frame;
            clone.DurationFrames = EndFrame - frame;
            DurationFrames = DurationFrames - clone.DurationFrames;
            Track?.AddMedia(clone);
            Timeline?.MarkAsEdited();
            return clone;
        }

        /// <summary>
        /// Deletes this media.
        /// </summary>
        public void Delete()
        {
            _timeline.Delete(this);
        }

        /// <inheritdoc />
        public override void GetDesireClientArea(out Rectangle rect)
        {
            base.GetDesireClientArea(out rect);

            // Add some margin to allow to drag media edges
            rect = rect.MakeExpanded(-6.0f);
        }

        /// <inheritdoc />
        public override void Draw()
        {
            var style = Style.Current;
            var bounds = new Rectangle(Float2.Zero, Size);

            var fillColor = BackgroundColor.A > 0.0f ? BackgroundColor : style.Background * 1.5f;
            Render2D.FillRectangle(bounds, fillColor);

            var isMovingWholeMedia = _isMoving && !_startMoveRightEdge && !_startMoveLeftEdge;
            var borderHighlightColor = style.BorderHighlighted;
            var moveColor = style.SelectionBorder;
            var selectedColor = style.BackgroundSelected;
            var moveThickness = 2.0f;
            var borderColor = isMovingWholeMedia ? moveColor : (Timeline.SelectedMedia.Contains(this) ? selectedColor : (IsMouseOver ? borderHighlightColor : style.BorderNormal));
            Render2D.DrawRectangle(bounds, borderColor, isMovingWholeMedia ? moveThickness : 1.0f);
            if (_startMoveLeftEdge)
            {
                Render2D.DrawLine(bounds.UpperLeft, bounds.BottomLeft, moveColor, moveThickness);
            }
            else if (IsMouseOver && CanResize && MoveLeftEdgeRect.Contains(ref _mouseLocation))
            {
                Render2D.DrawLine(bounds.UpperLeft, bounds.BottomLeft, Color.Yellow);
            }
            if (_startMoveRightEdge)
            {
                Render2D.DrawLine(bounds.UpperRight, bounds.BottomRight, moveColor, moveThickness);
            }
            else if (IsMouseOver && CanResize && MoveRightEdgeRect.Contains(ref _mouseLocation))
            {
                Render2D.DrawLine(bounds.UpperRight, bounds.BottomRight, Color.Yellow);
            }

            DrawChildren();
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (base.OnMouseDown(location, button))
                return true;

            if (button == MouseButton.Left)
            {
                _isMoving = true;
                _startMoveLocation = Root.MousePosition;
                _startMoveStartFrame = StartFrame;
                _startMoveDuration = DurationFrames;
                _startMoveLeftEdge = MoveLeftEdgeRect.Contains(ref location) && CanResize;
                _startMoveRightEdge = MoveRightEdgeRect.Contains(ref location) && CanResize;
                StartMouseCapture(true);
                if (_startMoveLeftEdge || _startMoveRightEdge)
                    return true;

                if (Root.GetKey(KeyboardKeys.Control))
                {
                    // Add/Remove selection
                    if (_timeline.SelectedMedia.Contains(this))
                        _timeline.Deselect(this);
                    else
                        _timeline.Select(this, true);
                }
                else
                {
                    // Select
                    if (!_timeline.SelectedMedia.Contains(this))
                        _timeline.Select(this);
                }

                _timeline.OnKeyframesMove(null, this, location, true, false);
                return true;
            }

            return false;
        }

        /// <inheritdoc />
        public override void OnMouseMove(Float2 location)
        {
            _mouseLocation = location;

            if (_isMoving)
            {
                var moveLocation = Root.MousePosition;
                var moveLocationDelta = moveLocation - _startMoveLocation;
                var moveDelta = (int)(moveLocationDelta.X / (Timeline.UnitsPerSecond * _timeline.Zoom) * _timeline.FramesPerSecond);
                var startFrame = StartFrame;
                var durationFrames = DurationFrames;

                if (_startMoveLeftEdge)
                {
                    StartFrame = _startMoveStartFrame + moveDelta;
                    DurationFrames = _startMoveDuration - moveDelta;
                }
                else if (_startMoveRightEdge)
                {
                    DurationFrames = _startMoveDuration + moveDelta;
                }
                else
                {
                    // Move with global timeline selection
                    _timeline.OnKeyframesMove(null, this, location, false, false);
                }

                if (StartFrame != startFrame || DurationFrames != durationFrames)
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
                if (!_startMoveLeftEdge && !_startMoveRightEdge && !Root.GetKey(KeyboardKeys.Control))
                {
                    var moveLocationDelta = Root.MousePosition - _startMoveLocation;
                    if (moveLocationDelta.Length < 4.0f)
                    {
                        // No move so just select itself
                        _timeline.Select(this);
                    }
                }
                EndMoving();
                return true;
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override bool OnMouseDoubleClick(Float2 location, MouseButton button)
        {
            if (base.OnMouseDoubleClick(location, button))
                return true;

            if (PropertiesEditObject != null)
            {
                Timeline.ShowEditPopup(PropertiesEditObject, PointToParent(Timeline, location), Track);
                return true;
            }
            return false;
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

        /// <inheritdoc />
        public override void OnMouseEnter(Float2 location)
        {
            base.OnMouseEnter(location);

            _mouseLocation = location;
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            base.OnMouseLeave();

            _mouseLocation = Float2.Minimum;
        }

        private void EndMoving()
        {
            _isMoving = false;
            if (_startMoveLeftEdge || _startMoveRightEdge)
            {
                _startMoveLeftEdge = false;
                _startMoveRightEdge = false;

                // Re-assign the media start/duration inside the undo recording block
                if (_startMoveStartFrame != _startFrame || _startMoveDuration != _durationFrames)
                {
                    var endMoveStartFrame = _startFrame;
                    var endMoveDuration = _durationFrames;
                    _startFrame = _startMoveStartFrame;
                    _durationFrames = _startMoveDuration;
                    using (new TrackUndoBlock(_tack))
                    {
                        _startFrame = endMoveStartFrame;
                        _durationFrames = endMoveDuration;
                    }
                }
            }
            else
            {
                // Global timeline selection moving end
                _timeline.OnKeyframesMove(null, this, _mouseLocation, false, true);
            }

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
            _tack = null;

            base.OnDestroy();
        }
    }
}
