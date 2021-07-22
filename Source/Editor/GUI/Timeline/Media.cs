// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
            /// Gets or sets the start frame of the media event.
            /// </summary>
            [EditorDisplay("General"), EditorOrder(-10010), Tooltip("Start frame of the media event.")]
            public int StartFrame
            {
                get => Media.StartFrame;
                set => Media.StartFrame = value;
            }

            /// <summary>
            /// Gets or sets the total duration of the media event in the timeline sequence frames amount.
            /// </summary>
            [EditorDisplay("General"), EditorOrder(-1000), Limit(1), Tooltip("Total duration of the media event in the timeline sequence frames amount.")]
            public int DurationFrames
            {
                get => Media.DurationFrames;
                set => Media.DurationFrames = value;
            }

            /// <summary>
            /// Initializes a new instance of the <see cref="ProxyBase{TTrack,TMedia}"/> class.
            /// </summary>
            /// <param name="track">The track.</param>
            /// <param name="media">The media.</param>
            protected ProxyBase(TTrack track, TMedia media)
            {
                Track = track ?? throw new ArgumentNullException(nameof(track));
                Media = media ?? throw new ArgumentNullException(nameof(media));
            }
        }

        private Timeline _timeline;
        private Track _tack;
        private int _startFrame, _durationFrames;
        private Vector2 _mouseLocation = Vector2.Minimum;
        private bool _isMoving;
        private Vector2 _startMoveLocation;
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
        /// Gets the media start time in seconds.
        /// </summary>
        /// <seealso cref="StartFrame"/>
        public float Start => _startFrame / _timeline.FramesPerSecond;

        /// <summary>
        /// Get the media duration in seconds.
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
        /// <param name="controlUnderMouse">The found control under the mouse cursor.</param>
        public virtual void OnTimelineShowContextMenu(ContextMenu.ContextMenu menu, Control controlUnderMouse)
        {
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
            var bounds = new Rectangle(Vector2.Zero, Size);

            var fillColor = style.Background * 1.5f;
            Render2D.FillRectangle(bounds, fillColor);

            var isMovingWholeMedia = _isMoving && !_startMoveRightEdge && !_startMoveLeftEdge;
            var borderHighlightColor = style.BorderHighlighted;
            var moveColor = style.ProgressNormal;
            var moveThickness = 2.0f;
            var borderColor = isMovingWholeMedia ? moveColor : (IsMouseOver || IsFocused ? borderHighlightColor : style.BorderNormal);
            Render2D.DrawRectangle(bounds, borderColor, isMovingWholeMedia ? moveThickness : 1.0f);
            if (_startMoveLeftEdge)
            {
                Render2D.DrawLine(bounds.UpperLeft, bounds.BottomLeft, moveColor, moveThickness);
            }
            else if ((IsMouseOver || IsFocused) && MoveLeftEdgeRect.Contains(ref _mouseLocation))
            {
                Render2D.DrawLine(bounds.UpperLeft, bounds.BottomLeft, Color.Yellow);
            }
            if (_startMoveRightEdge)
            {
                Render2D.DrawLine(bounds.UpperRight, bounds.BottomRight, moveColor, moveThickness);
            }
            else if ((IsMouseOver || IsFocused) && MoveRightEdgeRect.Contains(ref _mouseLocation))
            {
                Render2D.DrawLine(bounds.UpperRight, bounds.BottomRight, Color.Yellow);
            }

            DrawChildren();
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Vector2 location, MouseButton button)
        {
            if (base.OnMouseDown(location, button))
                return true;

            if (button == MouseButton.Left)
            {
                _isMoving = true;
                _startMoveLocation = Root.MousePosition;
                _startMoveStartFrame = StartFrame;
                _startMoveDuration = DurationFrames;
                _startMoveLeftEdge = MoveLeftEdgeRect.Contains(ref location);
                _startMoveRightEdge = MoveRightEdgeRect.Contains(ref location);

                StartMouseCapture(true);

                return true;
            }

            return false;
        }

        /// <inheritdoc />
        public override void OnMouseMove(Vector2 location)
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
                    StartFrame = _startMoveStartFrame + moveDelta;
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
        public override bool OnMouseUp(Vector2 location, MouseButton button)
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

        /// <inheritdoc />
        public override void OnMouseEnter(Vector2 location)
        {
            base.OnMouseEnter(location);

            _mouseLocation = location;
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            base.OnMouseLeave();

            _mouseLocation = Vector2.Minimum;
        }

        private void EndMoving()
        {
            _isMoving = false;
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
