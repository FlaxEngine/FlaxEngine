// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using FlaxEditor.CustomEditors;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Drag;
using FlaxEditor.GUI.Input;
using FlaxEditor.GUI.Timeline.GUI;
using FlaxEditor.GUI.Timeline.Tracks;
using FlaxEditor.GUI.Timeline.Undo;
using FlaxEngine;
using FlaxEngine.GUI;

namespace FlaxEditor.GUI.Timeline
{
    /// <summary>
    /// The timeline control that contains tracks section and headers. Can be used to create time-based media interface for camera tracks editing, audio mixing and events tracking.
    /// </summary>
    /// <seealso cref="FlaxEngine.GUI.ContainerControl" />
    [HideInEditor]
    public class Timeline : ContainerControl
    {
        private static readonly KeyValuePair<float, string>[] FPSValues =
        {
            new KeyValuePair<float, string>(12f, "12 fps"),
            new KeyValuePair<float, string>(15f, "15 fps"),
            new KeyValuePair<float, string>(23.976f, "23.97 (NTSC)"),
            new KeyValuePair<float, string>(24f, "24 fps"),
            new KeyValuePair<float, string>(25f, "25 (PAL)"),
            new KeyValuePair<float, string>(30f, "30 fps"),
            new KeyValuePair<float, string>(48f, "48 fps"),
            new KeyValuePair<float, string>(50f, "50 (PAL)"),
            new KeyValuePair<float, string>(60f, "60 fps"),
            new KeyValuePair<float, string>(100f, "100 fps"),
            new KeyValuePair<float, string>(120f, "120 fps"),
            new KeyValuePair<float, string>(240f, "240 fps"),
            new KeyValuePair<float, string>(0, "Custom"),
        };

        internal const int FormatVersion = 3;

        private sealed class TimeIntervalsHeader : ContainerControl
        {
            private Timeline _timeline;
            private bool _isLeftMouseButtonDown;

            public TimeIntervalsHeader(Timeline timeline)
            {
                _timeline = timeline;
            }

            /// <inheritdoc />
            public override bool OnMouseDown(Vector2 location, MouseButton button)
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
            public override void OnMouseMove(Vector2 location)
            {
                base.OnMouseMove(location);

                if (_isLeftMouseButtonDown)
                {
                    Seek(ref location);
                }
            }

            private void Seek(ref Vector2 location)
            {
                if (_timeline.PlaybackState == PlaybackStates.Disabled)
                    return;

                var locationTimeline = PointToParent(_timeline, location);
                var locationTime = _timeline._backgroundArea.PointFromParent(_timeline, locationTimeline);
                var frame = (locationTime.X - StartOffset * 2.0f) / _timeline.Zoom / UnitsPerSecond * _timeline.FramesPerSecond;
                _timeline.OnSeek((int)frame);
            }

            /// <inheritdoc />
            public override bool OnMouseUp(Vector2 location, MouseButton button)
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

        /// <summary>
        /// The base class for timeline properties proxy objects.
        /// </summary>
        /// <typeparam name="TTimeline">The type of the timeline.</typeparam>
        public abstract class ProxyBase<TTimeline>
        where TTimeline : Timeline
        {
            /// <summary>
            /// The timeline reference.
            /// </summary>
            [HideInEditor, NoSerialize]
            public TTimeline Timeline;

            /// <summary>
            /// Gets or sets the total duration of the timeline in the frames amount.
            /// </summary>
            [EditorDisplay("General"), EditorOrder(10), Limit(1), Tooltip("Total duration of the timeline event the frames amount.")]
            public int DurationFrames
            {
                get => Timeline.DurationFrames;
                set => Timeline.DurationFrames = value;
            }

            /// <summary>
            /// Initializes a new instance of the <see cref="ProxyBase{TTimeline}"/> class.
            /// </summary>
            /// <param name="timeline">The timeline.</param>
            protected ProxyBase(TTimeline timeline)
            {
                Timeline = timeline ?? throw new ArgumentNullException(nameof(timeline));
            }
        }

        /// <summary>
        /// The time axis value formatting modes.
        /// </summary>
        public enum TimeShowModes
        {
            /// <summary>
            /// The frame numbers.
            /// </summary>
            Frames,

            /// <summary>
            /// The seconds amount.
            /// </summary>
            Seconds,

            /// <summary>
            /// The time.
            /// </summary>
            Time,
        }

        /// <summary>
        /// The timeline animation playback states.
        /// </summary>
        public enum PlaybackStates
        {
            /// <summary>
            /// The timeline animation feature is disabled.
            /// </summary>
            Disabled,

            /// <summary>
            /// The timeline animation feature is disabled except for current frame seeking.
            /// </summary>
            Seeking,

            /// <summary>
            /// The timeline animation is stopped.
            /// </summary>
            Stopped,

            /// <summary>
            /// The timeline animation is playing.
            /// </summary>
            Playing,

            /// <summary>
            /// The timeline animation is paused.
            /// </summary>
            Paused,
        }

        /// <summary>
        /// The timeline playback buttons types.
        /// </summary>
        [Flags]
        public enum PlaybackButtons
        {
            /// <summary>
            /// No buttons.
            /// </summary>
            None = 0,

            /// <summary>
            /// The play/pause button.
            /// </summary>
            Play = 1,

            /// <summary>
            /// The stop button.
            /// </summary>
            Stop = 2,

            /// <summary>
            /// The current frame navigation buttons (left/right frame, seep begin/end).
            /// </summary>
            Navigation = 4,
        }

        /// <summary>
        /// The header top area height (in pixels).
        /// </summary>
        public static readonly float HeaderTopAreaHeight = 22.0f;

        /// <summary>
        /// The timeline units per second (on time axis).
        /// </summary>
        public static readonly float UnitsPerSecond = 100.0f;

        /// <summary>
        /// The start offset for the timeline (on time axis).
        /// </summary>
        public static readonly float StartOffset = 50.0f;

        private bool _isChangingFps;
        private float _framesPerSecond = 30.0f;
        private int _durationFrames = 30 * 5;
        private int _currentFrame;
        private TimeShowModes _timeShowMode = TimeShowModes.Frames;
        private bool _showPreviewValues = true;
        private PlaybackStates _state = PlaybackStates.Disabled;

        /// <summary>
        /// Flag used to mark modified timeline data.
        /// </summary>
        protected bool _isModified;

        /// <summary>
        /// The tracks collection.
        /// </summary>
        protected readonly List<Track> _tracks = new List<Track>();

        private SplitPanel _splitter;
        private TimeIntervalsHeader _timeIntervalsHeader;
        private ContainerControl _backgroundScroll;
        private Background _background;
        private Panel _backgroundArea;
        private TimelineEdge _leftEdge;
        private TimelineEdge _rightEdge;
        private Button _addTrackButton;
        private ComboBox _fpsComboBox;
        private Button _viewButton;
        private FloatValueBox _fpsCustomValue;
        private TracksPanelArea _tracksPanelArea;
        private VerticalPanel _tracksPanel;
        private Image[] _playbackNavigation;
        private Image _playbackStop;
        private Image _playbackPlay;
        private Label _noTracksLabel;
        private PositionHandle _positionHandle;
        private bool _isRightMouseButtonDown;
        private Vector2 _rightMouseButtonDownPos;
        private float _zoom = 1.0f;
        private bool _isMovingPositionHandle;
        private bool _canPlayPauseStop = true;

        /// <summary>
        /// Gets or sets the current time showing mode.
        /// </summary>
        public TimeShowModes TimeShowMode
        {
            get => _timeShowMode;
            set
            {
                if (_timeShowMode == value)
                    return;

                _timeShowMode = value;
                TimeShowModeChanged?.Invoke();
            }
        }

        /// <summary>
        /// Occurs when current time showing mode gets changed.
        /// </summary>
        public event Action TimeShowModeChanged;

        /// <summary>
        /// Gets or sets the preview values showing option value.
        /// </summary>
        public bool ShowPreviewValues
        {
            get => _showPreviewValues;
            set
            {
                if (_showPreviewValues == value)
                    return;

                _showPreviewValues = value;
                ShowPreviewValuesChanged?.Invoke();
            }
        }

        /// <summary>
        /// Occurs when preview values showing option gets changed.
        /// </summary>
        public event Action ShowPreviewValuesChanged;

        /// <summary>
        /// Gets or sets the current animation playback time position (frame number).
        /// </summary>
        public int CurrentFrame
        {
            get => _currentFrame;
            set
            {
                if (_currentFrame == value)
                    return;

                _currentFrame = value;
                UpdatePositionHandle();
                for (var i = 0; i < _tracks.Count; i++)
                {
                    _tracks[i].OnTimelineCurrentFrameChanged(_currentFrame);
                }
                CurrentFrameChanged?.Invoke();
            }
        }

        /// <summary>
        /// Gets the current animation time position (in seconds).
        /// </summary>
        public float CurrentTime => _currentFrame / _framesPerSecond;

        /// <summary>
        /// Occurs when current playback animation frame gets changed.
        /// </summary>
        public event Action CurrentFrameChanged;

        /// <summary>
        /// Gets or sets the amount of frames per second of the timeline animation.
        /// </summary>
        public float FramesPerSecond
        {
            get => _framesPerSecond;
            set
            {
                value = Mathf.Clamp(value, 0.1f, 1000.0f);
                if (Mathf.NearEqual(_framesPerSecond, value))
                    return;

                Undo?.AddAction(new EditFpsAction(this, _framesPerSecond, value));
                SetFPS(value);
            }
        }

        internal void SetFPS(float value)
        {
            var oldDuration = Duration;
            var oldValue = _framesPerSecond;

            _isChangingFps = true;
            _framesPerSecond = value;
            if (_fpsComboBox != null)
            {
                int index = FPSValues.Length - 1;
                for (int i = 0; i < FPSValues.Length; i++)
                {
                    if (Mathf.NearEqual(FPSValues[i].Key, value))
                    {
                        index = i;
                        break;
                    }
                }
                _fpsComboBox.SelectedIndex = index;
            }
            if (_fpsCustomValue != null)
                _fpsCustomValue.Value = value;
            _isChangingFps = false;
            FramesPerSecondChanged?.Invoke();

            // Preserve media events and duration
            for (var i = 0; i < _tracks.Count; i++)
            {
                _tracks[i].OnTimelineFpsChanged(oldValue, value);
            }
            Duration = oldDuration;

            // Update
            MarkAsEdited();
            ArrangeTracks();
            UpdatePositionHandle();
        }

        /// <summary>
        /// Occurs when frames per second gets changed.
        /// </summary>
        public event Action FramesPerSecondChanged;

        /// <summary>
        /// Gets or sets the timeline animation duration in frames.
        /// </summary>
        /// <seealso cref="FramesPerSecond"/>
        public int DurationFrames
        {
            get => _durationFrames;
            set
            {
                value = Mathf.Max(value, 1);
                if (_durationFrames == value)
                    return;

                _durationFrames = value;
                ArrangeTracks();
                DurationFramesChanged?.Invoke();
            }
        }

        /// <summary>
        /// Gets the timeline animation duration in seconds.
        /// </summary>
        /// <seealso cref="FramesPerSecond"/>
        public float Duration
        {
            get => _durationFrames / FramesPerSecond;
            set => DurationFrames = (int)(value * FramesPerSecond);
        }

        /// <summary>
        /// Occurs when timeline duration gets changed.
        /// </summary>
        public event Action DurationFramesChanged;

        /// <summary>
        /// Gets the collection of the tracks added to this timeline (read-only list).
        /// </summary>
        public IReadOnlyList<Track> Tracks => _tracks;

        /// <summary>
        /// Occurs when tracks collection gets changed.
        /// </summary>
        public event Action TracksChanged;

        /// <summary>
        /// Gets a value indicating whether this timeline was modified by the user (needs saving and flushing with data source).
        /// </summary>
        public bool IsModified => _isModified;

        /// <summary>
        /// Occurs when timeline gets modified (track edited, media moved, etc.).
        /// </summary>
        public event Action Modified;

        /// <summary>
        /// Occurs when timeline starts playing animation.
        /// </summary>
        public event Action Play;

        /// <summary>
        /// Occurs when timeline pauses playing animation.
        /// </summary>
        public event Action Pause;

        /// <summary>
        /// Occurs when timeline stops playing animation.
        /// </summary>
        public event Action Stop;

        /// <summary>
        /// Gets the splitter.
        /// </summary>
        public SplitPanel Splitter => _splitter;

        /// <summary>
        /// The track archetypes.
        /// </summary>
        public readonly List<TrackArchetype> TrackArchetypes = new List<TrackArchetype>(32);

        /// <summary>
        /// The selected tracks.
        /// </summary>
        public readonly List<Track> SelectedTracks = new List<Track>();

        /// <summary>
        /// The selected media events.
        /// </summary>
        public readonly List<Media> SelectedMedia = new List<Media>();

        /// <summary>
        /// Occurs when any collection of the selected objects in the timeline gets changed.
        /// </summary>
        public event Action SelectionChanged;

        /// <summary>
        /// Gets the media controls background panel (with scroll bars).
        /// </summary>
        public Panel MediaBackground => _backgroundArea;

        /// <summary>
        /// Gets the media controls parent panel.
        /// </summary>
        public Background MediaPanel => _background;

        /// <summary>
        /// Gets the track controls parent panel.
        /// </summary>
        public VerticalPanel TracksPanel => _tracksPanel;

        /// <summary>
        /// Gets the state of the timeline animation playback.
        /// </summary>
        public PlaybackStates PlaybackState
        {
            get => _state;
            protected set
            {
                if (_state == value)
                    return;
                _state = value;
                UpdatePlaybackButtons();
            }
        }

        /// <summary>
        /// The timeline properties editing proxy object. Assign it to add timeline properties editing support.
        /// </summary>
        public object PropertiesEditObject;

        /// <summary>
        /// Gets or sets the timeline view zoom.
        /// </summary>
        public float Zoom
        {
            get => _zoom;
            set
            {
                value = Mathf.Clamp(value, 0.0001f, 1000.0f);
                if (Mathf.NearEqual(_zoom, value))
                    return;

                _zoom = value;

                foreach (var track in _tracks)
                {
                    track.OnTimelineZoomChanged();
                }

                ArrangeTracks();
                UpdatePositionHandle();
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether user can sue Play/Pause/Stop buttons, otherwise those should be disabled.
        /// </summary>
        public bool CanPlayPauseStop
        {
            get => _canPlayPauseStop;
            set
            {
                if (_canPlayPauseStop == value)
                    return;
                _canPlayPauseStop = value;
                UpdatePlaybackButtons();
            }
        }

        /// <summary>
        /// Gets a value indicating whether user is moving position handle (seeking).
        /// </summary>
        public bool IsMovingPositionHandle => _isMovingPositionHandle;

        /// <summary>
        /// The drag and drop handler.
        /// </summary>
        public struct DragHandler
        {
            /// <summary>
            /// The drag and drop handler.
            /// </summary>
            public DragHelper Helper;

            /// <summary>
            /// The action.
            /// </summary>
            public Action<Timeline, DragHelper> Action;

            /// <summary>
            /// Initializes a new instance of the <see cref="DragHandler"/> struct.
            /// </summary>
            /// <param name="helper">The helper.</param>
            /// <param name="action">The action.</param>
            public DragHandler(DragHelper helper, Action<Timeline, DragHelper> action)
            {
                Helper = helper;
                Action = action;
            }
        }

        /// <summary>
        /// The drag handlers pairs of drag helper and the function that creates a track on drag drop.
        /// </summary>
        public readonly List<DragHandler> DragHandlers = new List<DragHandler>();

        /// <summary>
        /// The camera cut thumbnail renderer.
        /// </summary>
        public CameraCutThumbnailRenderer CameraCutThumbnailRenderer;

        /// <summary>
        /// The undo system to use for the history actions recording (optional, can be null).
        /// </summary>
        public readonly FlaxEditor.Undo Undo;

        /// <summary>
        /// Initializes a new instance of the <see cref="Timeline"/> class.
        /// </summary>
        /// <param name="playbackButtons">The playback buttons to use.</param>
        /// <param name="undo">The undo/redo to use for the history actions recording. Optional, can be null to disable undo support.</param>
        /// <param name="canChangeFps">True if user can modify the timeline FPS, otherwise it will be fixed or controlled from the code.</param>
        /// <param name="canAddTracks">True if user can add new tracks.</param>
        public Timeline(PlaybackButtons playbackButtons, FlaxEditor.Undo undo = null, bool canChangeFps = true, bool canAddTracks = true)
        {
            Undo = undo;
            AutoFocus = false;
            _splitter = new SplitPanel(Orientation.Horizontal, ScrollBars.None, ScrollBars.None)
            {
                SplitterValue = 0.4f,
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                Parent = this
            };

            var headerTopArea = new ContainerControl
            {
                AutoFocus = false,
                BackgroundColor = Style.Current.LightBackground,
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                Offsets = new Margin(0, 0, 0, HeaderTopAreaHeight),
                Parent = _splitter.Panel1
            };
            if (canAddTracks)
            {
                _addTrackButton = new Button(2, 2, 40.0f, 18.0f)
                {
                    TooltipText = "Add new tracks to the timeline",
                    Text = "Add",
                    Parent = headerTopArea
                };
                _addTrackButton.Clicked += OnAddTrackButtonClicked;
            }
            _viewButton = new Button((_addTrackButton?.Right ?? 0) + 2, 2, 40.0f, 18.0f)
            {
                TooltipText = "Change timeline view options",
                Text = "View",
                Parent = headerTopArea
            };
            _viewButton.Clicked += OnViewButtonClicked;

            if (canChangeFps)
            {
                var changeFpsWidth = 70.0f;
                _fpsComboBox = new ComboBox(_viewButton.Right + 2, 2, changeFpsWidth)
                {
                    TooltipText = "Change timeline frames per second",
                    Parent = headerTopArea
                };
                for (int i = 0; i < FPSValues.Length; i++)
                {
                    _fpsComboBox.AddItem(FPSValues[i].Value);
                    if (Mathf.NearEqual(_framesPerSecond, FPSValues[i].Key))
                        _fpsComboBox.SelectedIndex = i;
                }
                if (_fpsComboBox.SelectedIndex == -1)
                    _fpsComboBox.SelectedIndex = FPSValues.Length - 1;
                _fpsComboBox.SelectedIndexChanged += OnFpsSelectedIndexChanged;
                _fpsComboBox.PopupShowing += OnFpsPopupShowing;
            }

            var playbackButtonsSize = 0.0f;
            if (playbackButtons != PlaybackButtons.None)
            {
                playbackButtonsSize = 24.0f;
                var icons = Editor.Instance.Icons;
                var playbackButtonsArea = new ContainerControl
                {
                    AutoFocus = false,
                    ClipChildren = false,
                    BackgroundColor = Style.Current.LightBackground,
                    AnchorPreset = AnchorPresets.HorizontalStretchBottom,
                    Offsets = new Margin(0, 0, -playbackButtonsSize, playbackButtonsSize),
                    Parent = _splitter.Panel1
                };
                var playbackButtonsPanel = new ContainerControl
                {
                    AutoFocus = false,
                    ClipChildren = false,
                    AnchorPreset = AnchorPresets.VerticalStretchCenter,
                    Offsets = Margin.Zero,
                    Parent = playbackButtonsArea,
                };
                if ((playbackButtons & PlaybackButtons.Navigation) == PlaybackButtons.Navigation)
                {
                    _playbackNavigation = new Image[6];

                    _playbackNavigation[0] = new Image(playbackButtonsPanel.Width, 0, playbackButtonsSize, playbackButtonsSize)
                    {
                        TooltipText = "Rewind to timeline start (Home)",
                        Brush = new SpriteBrush(icons.Step32),
                        Enabled = false,
                        Visible = false,
                        Rotation = 180.0f,
                        Parent = playbackButtonsPanel
                    };
                    _playbackNavigation[0].Clicked += (image, button) => OnSeek(0);
                    playbackButtonsPanel.Width += playbackButtonsSize;

                    _playbackNavigation[1] = new Image(playbackButtonsPanel.Width, 0, playbackButtonsSize, playbackButtonsSize)
                    {
                        TooltipText = "Seek back to the previous keyframe (Page Down)",
                        Brush = new SpriteBrush(icons.Next32),
                        Enabled = false,
                        Visible = false,
                        Rotation = 180.0f,
                        Parent = playbackButtonsPanel
                    };
                    _playbackNavigation[1].Clicked += (image, button) =>
                    {
                        if (GetPreviousKeyframeFrame(CurrentTime, out var time))
                            OnSeek(time);
                    };
                    playbackButtonsPanel.Width += playbackButtonsSize;

                    _playbackNavigation[2] = new Image(playbackButtonsPanel.Width, 0, playbackButtonsSize, playbackButtonsSize)
                    {
                        TooltipText = "Move one frame back (Left Arrow)",
                        Brush = new SpriteBrush(icons.ArrowLeft32),
                        Enabled = false,
                        Visible = false,
                        Parent = playbackButtonsPanel
                    };
                    _playbackNavigation[2].Clicked += (image, button) => OnSeek(CurrentFrame - 1);
                    playbackButtonsPanel.Width += playbackButtonsSize;
                }
                if ((playbackButtons & PlaybackButtons.Stop) == PlaybackButtons.Stop)
                {
                    _playbackStop = new Image(playbackButtonsPanel.Width, 0, playbackButtonsSize, playbackButtonsSize)
                    {
                        TooltipText = "Stop playback",
                        Brush = new SpriteBrush(icons.Stop32),
                        Visible = false,
                        Enabled = false,
                        Parent = playbackButtonsPanel
                    };
                    _playbackStop.Clicked += OnStopClicked;
                    playbackButtonsPanel.Width += playbackButtonsSize;
                }
                if ((playbackButtons & PlaybackButtons.Play) == PlaybackButtons.Play)
                {
                    _playbackPlay = new Image(playbackButtonsPanel.Width, 0, playbackButtonsSize, playbackButtonsSize)
                    {
                        TooltipText = "Play/pause playback (Space)",
                        Brush = new SpriteBrush(icons.Play32),
                        Visible = false,
                        Tag = false, // Set to true if image is set to Pause, false if Play
                        Parent = playbackButtonsPanel
                    };
                    _playbackPlay.Clicked += OnPlayClicked;
                    playbackButtonsPanel.Width += playbackButtonsSize;
                }
                if ((playbackButtons & PlaybackButtons.Navigation) == PlaybackButtons.Navigation)
                {
                    _playbackNavigation[3] = new Image(playbackButtonsPanel.Width, 0, playbackButtonsSize, playbackButtonsSize)
                    {
                        TooltipText = "Move one frame forward (Right Arrow)",
                        Brush = new SpriteBrush(icons.ArrowRight32),
                        Enabled = false,
                        Visible = false,
                        Parent = playbackButtonsPanel
                    };
                    _playbackNavigation[3].Clicked += (image, button) => OnSeek(CurrentFrame + 1);
                    playbackButtonsPanel.Width += playbackButtonsSize;

                    _playbackNavigation[4] = new Image(playbackButtonsPanel.Width, 0, playbackButtonsSize, playbackButtonsSize)
                    {
                        TooltipText = "Seek to the next keyframe (Page Up)",
                        Brush = new SpriteBrush(icons.Next32),
                        Enabled = false,
                        Visible = false,
                        Parent = playbackButtonsPanel
                    };
                    _playbackNavigation[4].Clicked += (image, button) =>
                    {
                        if (GetNextKeyframeFrame(CurrentTime, out var time))
                            OnSeek(time);
                    };
                    playbackButtonsPanel.Width += playbackButtonsSize;

                    _playbackNavigation[5] = new Image(playbackButtonsPanel.Width, 0, playbackButtonsSize, playbackButtonsSize)
                    {
                        TooltipText = "Rewind to timeline end (End)",
                        Brush = new SpriteBrush(icons.Step32),
                        Enabled = false,
                        Visible = false,
                        Parent = playbackButtonsPanel
                    };
                    _playbackNavigation[5].Clicked += (image, button) => OnSeek(DurationFrames);
                    playbackButtonsPanel.Width += playbackButtonsSize;
                }
                playbackButtonsPanel.X = (playbackButtonsPanel.Parent.Width - playbackButtonsPanel.Width) * 0.5f;
            }

            _tracksPanelArea = new TracksPanelArea(this)
            {
                AutoFocus = false,
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, HeaderTopAreaHeight, playbackButtonsSize),
                Parent = _splitter.Panel1,
            };
            _tracksPanel = new VerticalPanel
            {
                AutoFocus = false,
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                Offsets = Margin.Zero,
                IsScrollable = true,
                BottomMargin = 40.0f,
                Parent = _tracksPanelArea
            };
            _noTracksLabel = new Label
            {
                AnchorPreset = AnchorPresets.MiddleCenter,
                Offsets = Margin.Zero,
                TextColor = Color.Gray,
                TextColorHighlighted = Color.Gray * 1.1f,
                Text = "No tracks",
                Parent = _tracksPanelArea
            };

            _timeIntervalsHeader = new TimeIntervalsHeader(this)
            {
                AutoFocus = false,
                BackgroundColor = Style.Current.Background.RGBMultiplied(0.9f),
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                Offsets = new Margin(0, 0, 0, HeaderTopAreaHeight),
                Parent = _splitter.Panel2
            };
            _backgroundArea = new Panel(ScrollBars.Both)
            {
                AutoFocus = false,
                ClipChildren = false,
                BackgroundColor = Style.Current.Background.RGBMultiplied(0.7f),
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, HeaderTopAreaHeight, 0),
                Parent = _splitter.Panel2
            };
            _backgroundScroll = new ContainerControl
            {
                AutoFocus = false,
                ClipChildren = false,
                Offsets = Margin.Zero,
                Parent = _backgroundArea
            };
            _background = new Background(this)
            {
                AutoFocus = false,
                ClipChildren = false,
                Offsets = Margin.Zero,
                Parent = _backgroundArea
            };
            _leftEdge = new TimelineEdge(this, true, false)
            {
                Offsets = Margin.Zero,
                Parent = _backgroundArea
            };
            _rightEdge = new TimelineEdge(this, false, true)
            {
                Offsets = Margin.Zero,
                Parent = _backgroundArea
            };
            _positionHandle = new PositionHandle(this)
            {
                ClipChildren = false,
                Visible = false,
                Parent = _backgroundArea,
            };
            UpdatePositionHandle();
            PlaybackState = PlaybackStates.Disabled;
        }

        private void UpdatePositionHandle()
        {
            var handleWidth = 12.0f;
            _positionHandle.Bounds = new Rectangle(
                                                   StartOffset * 2.0f - handleWidth * 0.5f + _currentFrame / _framesPerSecond * UnitsPerSecond * Zoom,
                                                   HeaderTopAreaHeight * -0.5f,
                                                   handleWidth,
                                                   HeaderTopAreaHeight * 0.5f
                                                  );
        }

        private void OnFpsPopupShowing(ComboBox comboBox)
        {
            if (_fpsCustomValue == null)
            {
                _fpsCustomValue = new FloatValueBox(_framesPerSecond, 63, 295, 45.0f, 0.1f, 1000.0f, 0.1f)
                {
                    Parent = comboBox.Popup
                };
                _fpsCustomValue.ValueChanged += OnFpsCustomValueChanged;
            }
            _fpsCustomValue.Value = FramesPerSecond;
        }

        private void OnFpsCustomValueChanged()
        {
            if (_isChangingFps || _fpsComboBox.SelectedIndex != FPSValues.Length - 1)
                return;

            // Custom value
            FramesPerSecond = _fpsCustomValue.Value;
        }

        private void OnFpsSelectedIndexChanged(ComboBox comboBox)
        {
            if (_isChangingFps)
                return;

            if (comboBox.SelectedIndex == FPSValues.Length - 1)
            {
                // Custom value
                FramesPerSecond = _fpsCustomValue.Value;
            }
            else
            {
                // Predefined value
                FramesPerSecond = FPSValues[comboBox.SelectedIndex].Key;
            }
        }

        private void OnAddTrackButtonClicked()
        {
            // TODO: maybe cache context menu object?
            var menu = new ContextMenu.ContextMenu();
            for (int i = 0; i < TrackArchetypes.Count; i++)
            {
                var archetype = TrackArchetypes[i];
                if (archetype.DisableSpawnViaGUI)
                    continue;

                var button = menu.AddButton(archetype.Name, OnAddTrackOptionClicked);
                button.Tag = archetype;
                button.Icon = archetype.Icon;
            }
            menu.Show(_addTrackButton.Parent, _addTrackButton.BottomLeft);
        }

        private void OnAddTrackOptionClicked(ContextMenuButton button)
        {
            var archetype = (TrackArchetype)button.Tag;
            AddTrack(archetype);
        }

        private void OnViewButtonClicked()
        {
            var menu = new ContextMenu.ContextMenu();

            var showTimeAs = menu.AddChildMenu("Show time as");
            showTimeAs.ContextMenu.AddButton("Frames", () => TimeShowMode = TimeShowModes.Frames).Checked = TimeShowMode == TimeShowModes.Frames;
            showTimeAs.ContextMenu.AddButton("Seconds", () => TimeShowMode = TimeShowModes.Seconds).Checked = TimeShowMode == TimeShowModes.Seconds;
            showTimeAs.ContextMenu.AddButton("Time", () => TimeShowMode = TimeShowModes.Time).Checked = TimeShowMode == TimeShowModes.Time;

            menu.AddButton("Show preview values", () => ShowPreviewValues = !ShowPreviewValues).Checked = ShowPreviewValues;

            OnShowViewContextMenu(menu);

            menu.Show(_viewButton.Parent, _viewButton.BottomLeft);
        }

        /// <summary>
        /// Occurs when timeline shows the View context menu. Can be sued to add custom options.
        /// </summary>
        public event Action<ContextMenu.ContextMenu> ShowViewContextMenu;

        /// <summary>
        /// Called when timeline shows the View context menu. Can be sued to add custom options.
        /// </summary>
        /// <param name="menu">The menu.</param>
        protected virtual void OnShowViewContextMenu(ContextMenu.ContextMenu menu)
        {
            ShowViewContextMenu?.Invoke(menu);
        }

        private void OnStopClicked(Image stop, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                OnStop();
            }
        }

        private void OnPlayClicked(Image play, MouseButton button)
        {
            if (button == MouseButton.Left)
            {
                if ((bool)play.Tag)
                    OnPause();
                else
                    OnPlay();
            }
        }

        /// <summary>
        /// Called when animation should stop.
        /// </summary>
        public virtual void OnStop()
        {
            Stop?.Invoke();
            PlaybackState = PlaybackStates.Stopped;
        }

        /// <summary>
        /// Called when animation should play.
        /// </summary>
        public virtual void OnPlay()
        {
            Play?.Invoke();
            PlaybackState = PlaybackStates.Playing;
        }

        /// <summary>
        /// Called when animation should pause.
        /// </summary>
        public virtual void OnPause()
        {
            Pause?.Invoke();
            PlaybackState = PlaybackStates.Paused;
        }

        /// <summary>
        /// Called when animation playback position should be changed.
        /// </summary>
        /// <param name="frame">The frame.</param>
        public virtual void OnSeek(int frame)
        {
        }

        /// <summary>
        /// Gets the frame of the next keyframe from all tracks (if found).
        /// </summary>
        /// <param name="time">The start time.</param>
        /// <param name="result">The result value.</param>
        /// <returns>True if found next keyframe, otherwise false.</returns>
        public bool GetNextKeyframeFrame(float time, out int result)
        {
            bool hasValid = false;
            int closestFrame = 0;
            for (int i = 0; i < _tracks.Count; i++)
            {
                if (_tracks[i].GetNextKeyframeFrame(time, out var frame) && (!hasValid || closestFrame > frame))
                {
                    hasValid = true;
                    closestFrame = frame;
                }
            }
            result = closestFrame;
            return hasValid;
        }

        /// <summary>
        /// Gets the frame of the previous keyframe (if found).
        /// </summary>
        /// <param name="time">The start time.</param>
        /// <param name="result">The result value.</param>
        /// <returns>True if found previous keyframe, otherwise false.</returns>
        public bool GetPreviousKeyframeFrame(float time, out int result)
        {
            bool hasValid = false;
            int closestFrame = 0;
            for (int i = 0; i < _tracks.Count; i++)
            {
                if (_tracks[i].GetPreviousKeyframeFrame(time, out var frame) && (!hasValid || closestFrame < frame))
                {
                    hasValid = true;
                    closestFrame = frame;
                }
            }
            result = closestFrame;
            return hasValid;
        }

        private void UpdatePlaybackButtons()
        {
            // Update buttons UI
            var icons = Editor.Instance.Icons;
            // TODO: cleanup this UI code
            switch (_state)
            {
            case PlaybackStates.Disabled:
                if (_playbackNavigation != null)
                {
                    foreach (var e in _playbackNavigation)
                    {
                        e.Enabled = false;
                        e.Visible = false;
                    }
                }
                if (_playbackStop != null)
                {
                    _playbackStop.Visible = false;
                }
                if (_playbackPlay != null)
                {
                    _playbackPlay.Visible = false;
                }
                if (_positionHandle != null)
                {
                    _positionHandle.Visible = false;
                }
                break;
            case PlaybackStates.Seeking:
                if (_playbackNavigation != null)
                {
                    foreach (var e in _playbackNavigation)
                    {
                        e.Enabled = false;
                        e.Visible = false;
                    }
                }
                if (_playbackStop != null)
                {
                    _playbackStop.Visible = false;
                }
                if (_playbackPlay != null)
                {
                    _playbackPlay.Visible = false;
                }
                if (_positionHandle != null)
                {
                    _positionHandle.Visible = true;
                }
                break;
            case PlaybackStates.Stopped:
                if (_playbackNavigation != null)
                {
                    foreach (var e in _playbackNavigation)
                    {
                        e.Enabled = true;
                        e.Visible = true;
                    }
                }
                if (_playbackStop != null)
                {
                    _playbackStop.Visible = true;
                    _playbackStop.Enabled = false;
                }
                if (_playbackPlay != null)
                {
                    _playbackPlay.Visible = true;
                    _playbackPlay.Enabled = _canPlayPauseStop;
                    _playbackPlay.Brush = new SpriteBrush(icons.Play32);
                    _playbackPlay.Tag = false;
                }
                if (_positionHandle != null)
                {
                    _positionHandle.Visible = true;
                }
                break;
            case PlaybackStates.Playing:
                if (_playbackNavigation != null)
                {
                    foreach (var e in _playbackNavigation)
                    {
                        e.Enabled = true;
                        e.Visible = true;
                    }
                }
                if (_playbackStop != null)
                {
                    _playbackStop.Visible = true;
                    _playbackStop.Enabled = _canPlayPauseStop;
                }
                if (_playbackPlay != null)
                {
                    _playbackPlay.Visible = true;
                    _playbackPlay.Enabled = _canPlayPauseStop;
                    _playbackPlay.Brush = new SpriteBrush(icons.Pause32);
                    _playbackPlay.Tag = true;
                }
                if (_positionHandle != null)
                {
                    _positionHandle.Visible = true;
                }
                break;
            case PlaybackStates.Paused:
                if (_playbackNavigation != null)
                {
                    foreach (var e in _playbackNavigation)
                    {
                        e.Enabled = true;
                        e.Visible = true;
                    }
                }
                if (_playbackStop != null)
                {
                    _playbackStop.Visible = true;
                    _playbackStop.Enabled = _canPlayPauseStop;
                }
                if (_playbackPlay != null)
                {
                    _playbackPlay.Visible = true;
                    _playbackPlay.Enabled = _canPlayPauseStop;
                    _playbackPlay.Brush = new SpriteBrush(icons.Play32);
                    _playbackPlay.Tag = false;
                }
                if (_positionHandle != null)
                {
                    _positionHandle.Visible = true;
                }
                break;
            default: throw new ArgumentOutOfRangeException();
            }
        }

        /// <summary>
        /// Creates a new track object that can be later added using <see cref="AddTrack(FlaxEditor.GUI.Timeline.Track,bool)"/>.
        /// </summary>
        /// <param name="archetype">The archetype.</param>
        /// <returns>The created track object.</returns>
        public Track NewTrack(TrackArchetype archetype)
        {
            var options = new TrackCreateOptions
            {
                Archetype = archetype,
                Mute = false,
            };
            return archetype.Create(options);
        }

        /// <summary>
        /// Loads the timeline data.
        /// </summary>
        /// <param name="version">The version.</param>
        /// <param name="stream">The input stream.</param>
        protected virtual void LoadTimelineData(int version, BinaryReader stream)
        {
        }

        /// <summary>
        /// Saves the timeline data.
        /// </summary>
        /// <param name="stream">The output stream.</param>
        protected virtual void SaveTimelineData(BinaryWriter stream)
        {
        }

        /// <summary>
        /// Loads the timeline data after reading the timeline tracks.
        /// </summary>
        /// <param name="version">The version.</param>
        /// <param name="stream">The input stream.</param>
        protected virtual void LoadTimelineCustomData(int version, BinaryReader stream)
        {
        }

        /// <summary>
        /// Saves the timeline data after saving the timeline tracks.
        /// </summary>
        /// <param name="stream">The output stream.</param>
        protected virtual void SaveTimelineCustomData(BinaryWriter stream)
        {
        }

        /// <summary>
        /// Loads the timeline from the specified data.
        /// </summary>
        /// <param name="data">The data.</param>
        public virtual void Load(byte[] data)
        {
            Profiler.BeginEvent("Clear");
            Clear();
            LockChildrenRecursive();
            Profiler.EndEvent();

            using (var memory = new MemoryStream(data))
            using (var stream = new BinaryReader(memory))
            {
                Profiler.BeginEvent("LoadData");
                int version = stream.ReadInt32();
                switch (version)
                {
                case 1:
                {
                    // [Deprecated on 23.07.2019, expires on 27.04.2021]

                    // Load properties
                    FramesPerSecond = stream.ReadSingle();
                    DurationFrames = stream.ReadInt32();
                    LoadTimelineData(version, stream);

                    // Load tracks
                    int tracksCount = stream.ReadInt32();
                    _tracks.Capacity = Math.Max(_tracks.Capacity, tracksCount);
                    for (int i = 0; i < tracksCount; i++)
                    {
                        var type = stream.ReadByte();
                        var flag = stream.ReadByte();
                        Track track = null;
                        var mute = (flag & 1) == 1;
                        for (int j = 0; j < TrackArchetypes.Count; j++)
                        {
                            if (TrackArchetypes[j].TypeId == type)
                            {
                                var options = new TrackCreateOptions
                                {
                                    Archetype = TrackArchetypes[j],
                                    Mute = mute,
                                };
                                track = TrackArchetypes[j].Create(options);
                                break;
                            }
                        }
                        if (track == null)
                            throw new Exception("Unknown timeline track type " + type);
                        int parentIndex = stream.ReadInt32();
                        int childrenCount = stream.ReadInt32();
                        track.Name = Utilities.Utils.ReadStr(stream, -13);
                        track.Tag = parentIndex;

                        track.Archetype.Load(version, track, stream);

                        AddLoadedTrack(track);
                    }
                    break;
                }
                case 2:
                case 3:
                {
                    // Load properties
                    FramesPerSecond = stream.ReadSingle();
                    DurationFrames = stream.ReadInt32();
                    LoadTimelineData(version, stream);

                    // Load tracks
                    int tracksCount = stream.ReadInt32();
                    _tracks.Capacity = Math.Max(_tracks.Capacity, tracksCount);
                    for (int i = 0; i < tracksCount; i++)
                    {
                        var type = stream.ReadByte();
                        var flag = stream.ReadByte();
                        Track track = null;
                        var mute = (flag & 1) == 1;
                        var loop = (flag & 2) == 2;
                        for (int j = 0; j < TrackArchetypes.Count; j++)
                        {
                            if (TrackArchetypes[j].TypeId == type)
                            {
                                var options = new TrackCreateOptions
                                {
                                    Archetype = TrackArchetypes[j],
                                    Mute = mute,
                                    Loop = loop,
                                };
                                track = TrackArchetypes[j].Create(options);
                                break;
                            }
                        }
                        if (track == null)
                            throw new Exception("Unknown timeline track type " + type);
                        int parentIndex = stream.ReadInt32();
                        int childrenCount = stream.ReadInt32();
                        track.Name = Utilities.Utils.ReadStr(stream, -13);
                        track.Tag = parentIndex;
                        track.Color = stream.ReadColor32();

                        Profiler.BeginEvent("LoadTack");
                        track.Archetype.Load(version, track, stream);
                        Profiler.EndEvent();

                        AddLoadedTrack(track);
                    }
                    break;
                }
                default: throw new Exception("Unknown timeline version " + version);
                }
                LoadTimelineCustomData(version, stream);
                Profiler.EndEvent();

                Profiler.BeginEvent("ParentTracks");
                for (int i = 0; i < _tracks.Count; i++)
                {
                    var parentIndex = (int)_tracks[i].Tag;
                    _tracks[i].Tag = null;
                    if (parentIndex != -1)
                        _tracks[i].ParentTrack = _tracks[parentIndex];
                }
                Profiler.EndEvent();
                Profiler.BeginEvent("SetupTracks");
                for (int i = 0; i < _tracks.Count; i++)
                {
                    _tracks[i].OnLoaded();
                }
                Profiler.EndEvent();
            }

            Profiler.BeginEvent("ArrangeTracks");
            ArrangeTracks();
            PerformLayout(true);
            UnlockChildrenRecursive();
            Profiler.EndEvent();

            ClearEditedFlag();
        }

        /// <summary>
        /// Saves the timeline data.
        /// </summary>
        /// <returns>The saved timeline data.</returns>
        public virtual byte[] Save()
        {
            // Serialize timeline to stream
            using (var memory = new MemoryStream(512))
            using (var stream = new BinaryWriter(memory))
            {
                // Save properties
                stream.Write(FormatVersion);
                stream.Write(FramesPerSecond);
                stream.Write(DurationFrames);
                SaveTimelineData(stream);

                // Save tracks
                int tracksCount = Tracks.Count;
                stream.Write(tracksCount);
                for (int i = 0; i < tracksCount; i++)
                {
                    var track = Tracks[i];

                    stream.Write((byte)track.Archetype.TypeId);
                    byte flag = 0;
                    if (track.Mute)
                        flag |= 1;
                    if (track.Loop)
                        flag |= 2;
                    stream.Write(flag);
                    stream.Write(_tracks.IndexOf(track.ParentTrack));
                    stream.Write(track.SubTracks.Count);
                    Utilities.Utils.WriteStr(stream, track.Name, -13);
                    stream.Write((Color32)track.Color);
                    track.Archetype.Save(track, stream);
                }

                SaveTimelineCustomData(stream);

                return memory.ToArray();
            }
        }

        /// <summary>
        /// Finds the track by the name.
        /// </summary>
        /// <param name="name">The name.</param>
        /// <returns>The found track or null.</returns>
        public Track FindTrack(string name)
        {
            for (int i = 0; i < _tracks.Count; i++)
            {
                if (_tracks[i].Name == name)
                    return _tracks[i];
            }
            return null;
        }

        /// <summary>
        /// Adds the track.
        /// </summary>
        /// <param name="archetype">The archetype.</param>
        public void AddTrack(TrackArchetype archetype)
        {
            AddTrack(NewTrack(archetype));
        }

        /// <summary>
        /// Adds the track.
        /// </summary>
        /// <param name="track">The track.</param>
        /// <param name="withUndo">True if use undo/redo action for track adding.</param>
        public virtual void AddTrack(Track track, bool withUndo = true)
        {
            // Ensure name is unique
            int idx = 1;
            var name = track.Name;
            while (!IsTrackNameValid(track.Name))
                track.Name = string.Format("{0} {1}", name, idx++);

            // Add it to the timeline
            _tracks.Add(track);
            track.OnTimelineChanged(this);
            track.Parent = _tracksPanel;

            // Update
            OnTracksChanged();
            if (track.ParentTrack != null)
                OnTracksOrderChanged();
            track.OnSpawned();
            _tracksPanelArea.ScrollViewTo(track);
            MarkAsEdited();
            if (withUndo)
                Undo?.AddAction(new AddRemoveTrackAction(this, track, true));
        }

        /// <summary>
        /// Adds the loaded track. Does not handle any UI updates.
        /// </summary>
        /// <param name="track">The track.</param>
        protected void AddLoadedTrack(Track track)
        {
            _tracks.Add(track);
            track.OnTimelineChanged(this);
            track.Parent = _tracksPanel;
        }

        /// <summary>
        /// Removes the track.
        /// </summary>
        /// <param name="track">The track.</param>
        public virtual void RemoveTrack(Track track)
        {
            track.Parent = null;
            track.OnTimelineChanged(null);
            _tracks.Remove(track);

            OnTracksChanged();
        }

        /// <summary>
        /// Called when collection of the tracks gets changed.
        /// </summary>
        protected virtual void OnTracksChanged()
        {
            TracksChanged?.Invoke();
            ArrangeTracks();
        }

        /// <summary>
        /// Selects the specified track.
        /// </summary>
        /// <param name="track">The track.</param>
        /// <param name="addToSelection">If set to <c>true</c> track will be added to selection, otherwise will clear selection before.</param>
        public void Select(Track track, bool addToSelection = false)
        {
            if (SelectedTracks.Contains(track) && (addToSelection || (SelectedTracks.Count == 1 && SelectedMedia.Count == 0)))
                return;

            if (!addToSelection)
            {
                SelectedTracks.Clear();
                SelectedMedia.Clear();
            }
            SelectedTracks.Add(track);
            OnSelectionChanged();
        }

        /// <summary>
        /// Deselects the specified track.
        /// </summary>
        /// <param name="track">The track.</param>
        public void Deselect(Track track)
        {
            if (!SelectedTracks.Contains(track))
                return;

            SelectedTracks.Remove(track);
            OnSelectionChanged();
        }

        /// <summary>
        /// Selects the specified media event.
        /// </summary>
        /// <param name="media">The media.</param>
        /// <param name="addToSelection">If set to <c>true</c> track will be added to selection, otherwise will clear selection before.</param>
        public void Select(Media media, bool addToSelection = false)
        {
            if (SelectedMedia.Contains(media) && (addToSelection || (SelectedTracks.Count == 0 && SelectedMedia.Count == 1)))
                return;

            if (!addToSelection)
            {
                SelectedTracks.Clear();
                SelectedMedia.Clear();
            }
            SelectedMedia.Add(media);
            OnSelectionChanged();
        }

        /// <summary>
        /// Deselects the specified media event.
        /// </summary>
        /// <param name="media">The media.</param>
        public void Deselect(Media media)
        {
            if (!SelectedMedia.Contains(media))
                return;

            SelectedMedia.Remove(media);
            OnSelectionChanged();
        }

        /// <summary>
        /// Deselects all media and tracks.
        /// </summary>
        public void Deselect()
        {
            if (SelectedMedia.Count == 0 && SelectedTracks.Count == 0)
                return;

            SelectedTracks.Clear();
            SelectedMedia.Clear();
            OnSelectionChanged();
        }

        /// <summary>
        /// Called when selection gets changed.
        /// </summary>
        protected virtual void OnSelectionChanged()
        {
            SelectionChanged?.Invoke();
        }

        private void GetTracks(Track track, List<Track> tracks)
        {
            if (tracks.Contains(track))
                return;
            tracks.Add(track);
            foreach (var subTrack in track.SubTracks)
                GetTracks(subTrack, tracks);
        }

        /// <summary>
        /// Deletes the selected tracks/media events.
        /// </summary>
        /// <param name="withUndo">True if use undo/redo action for track removing.</param>
        public void DeleteSelection(bool withUndo = true)
        {
            if (SelectedMedia.Count > 0)
            {
                throw new NotImplementedException("TODO: removing selected media events");
            }

            if (SelectedTracks.Count > 0)
            {
                // Delete selected tracks
                var tracks = new List<Track>(SelectedTracks.Count);
                for (int i = 0; i < SelectedTracks.Count; i++)
                {
                    GetTracks(SelectedTracks[i], tracks);
                }
                SelectedTracks.Clear();
                if (withUndo & Undo != null)
                {
                    if (tracks.Count == 1)
                    {
                        Undo.AddAction(new AddRemoveTrackAction(this, tracks[0], false));
                    }
                    else
                    {
                        var actions = new List<IUndoAction>();
                        for (int i = tracks.Count - 1; i >= 0; i--)
                            actions.Add(new AddRemoveTrackAction(this, tracks[i], false));
                        Undo.AddAction(new MultiUndoAction(actions, "Remove tracks"));
                    }
                }
                for (int i = tracks.Count - 1; i >= 0; i--)
                {
                    tracks[i].ParentTrack = null;
                    OnDeleteTrack(tracks[i]);
                }
                OnTracksChanged();
                MarkAsEdited();
            }
        }

        /// <summary>
        /// Deletes the tracks.
        /// </summary>
        /// <param name="track">The track to delete (and its sub tracks).</param>
        /// <param name="withUndo">True if use undo/redo action for track removing.</param>
        public void Delete(Track track, bool withUndo = true)
        {
            if (track == null)
                throw new ArgumentNullException();

            // Delete tracks
            var tracks = new List<Track>(4);
            GetTracks(track, tracks);
            if (withUndo & Undo != null)
            {
                if (tracks.Count == 1)
                {
                    Undo.AddAction(new AddRemoveTrackAction(this, tracks[0], false));
                }
                else
                {
                    var actions = new List<IUndoAction>();
                    for (int i = tracks.Count - 1; i >= 0; i--)
                        actions.Add(new AddRemoveTrackAction(this, tracks[i], false));
                    Undo.AddAction(new MultiUndoAction(actions, "Remove tracks"));
                }
            }
            for (int i = tracks.Count - 1; i >= 0; i--)
            {
                tracks[i].ParentTrack = null;
                OnDeleteTrack(tracks[i]);
            }
            OnTracksChanged();
            MarkAsEdited();
        }

        /// <summary>
        /// Called to delete track.
        /// </summary>
        /// <param name="track">The track.</param>
        protected virtual void OnDeleteTrack(Track track)
        {
            SelectedTracks.Remove(track);
            _tracks.Remove(track);
            track.OnDeleted();
        }

        /// <summary>
        /// Called once to setup the drag drop handling for the timeline (lazy init on first drag action).
        /// </summary>
        protected virtual void SetupDragDrop()
        {
        }

        /// <summary>
        /// Mark timeline as edited.
        /// </summary>
        public void MarkAsEdited()
        {
            _isModified = true;

            Modified?.Invoke();
        }

        /// <summary>
        /// Clears this instance. Removes all tracks, parameters and state.
        /// </summary>
        public void Clear()
        {
            Deselect();

            // Remove all tracks
            var tracks = new List<Track>(_tracks);
            for (int i = 0; i < tracks.Count; i++)
            {
                OnDeleteTrack(tracks[i]);
            }
            OnTracksChanged();

            ClearEditedFlag();
        }

        /// <summary>
        /// Clears the modification flag.
        /// </summary>
        public void ClearEditedFlag()
        {
            if (_isModified)
            {
                _isModified = false;
                Modified?.Invoke();
            }
        }

        internal void ChangeTrackIndex(Track track, int newIndex)
        {
            int oldIndex = _tracks.IndexOf(track);
            _tracks.RemoveAt(oldIndex);

            // Check if index is invalid
            if (newIndex < 0 || newIndex >= _tracks.Count)
            {
                // Append at the end
                _tracks.Add(track);
            }
            else
            {
                // Change order
                _tracks.Insert(newIndex, track);
            }
        }

        private void CollectTracks(Track track)
        {
            track.Parent = _tracksPanel;
            _tracks.Add(track);

            for (int i = 0; i < track.SubTracks.Count; i++)
            {
                CollectTracks(track.SubTracks[i]);
            }
        }

        /// <summary>
        /// Called when tracks order gets changed.
        /// </summary>
        public void OnTracksOrderChanged()
        {
            _tracksPanel.IsLayoutLocked = true;

            for (int i = 0; i < _tracks.Count; i++)
            {
                _tracks[i].Parent = null;
            }

            var rootTracks = new List<Track>();
            foreach (var track in _tracks)
            {
                if (track.ParentTrack == null)
                    rootTracks.Add(track);
            }
            _tracks.Clear();
            foreach (var track in rootTracks)
            {
                CollectTracks(track);
            }

            ArrangeTracks();

            _tracksPanel.IsLayoutLocked = false;
            _tracksPanel.PerformLayout();
            ArrangeTracks();
        }

        /// <summary>
        /// Determines whether the specified track name is valid.
        /// </summary>
        /// <param name="name">The name.</param>
        /// <returns> <c>true</c> if is track name is valid; otherwise, <c>false</c>.</returns>
        public bool IsTrackNameValid(string name)
        {
            name = name?.Trim();
            return !string.IsNullOrEmpty(name) && _tracks.All(x => x.Name != name);
        }

        /// <summary>
        /// Arranges the tracks.
        /// </summary>
        public void ArrangeTracks()
        {
            if (_noTracksLabel != null)
            {
                _noTracksLabel.Visible = _tracks.Count == 0;
            }

            for (int i = 0; i < _tracks.Count; i++)
            {
                _tracks[i].OnTimelineArrange();
            }

            if (_background != null)
            {
                float height = _tracksPanel.Height;

                _background.Visible = _tracks.Count > 0;
                _background.Bounds = new Rectangle(StartOffset, 0, Duration * UnitsPerSecond * Zoom, height);

                var edgeWidth = 6.0f;
                _leftEdge.Bounds = new Rectangle(_background.Left - edgeWidth * 0.5f + StartOffset, HeaderTopAreaHeight * -0.5f, edgeWidth, height + HeaderTopAreaHeight * 0.5f);
                _rightEdge.Bounds = new Rectangle(_background.Right - edgeWidth * 0.5f + StartOffset, HeaderTopAreaHeight * -0.5f, edgeWidth, height + HeaderTopAreaHeight * 0.5f);

                _backgroundScroll.Bounds = new Rectangle(0, 0, _background.Width + 5 * StartOffset, height);
            }
        }

        /// <inheritdoc />
        protected override void PerformLayoutBeforeChildren()
        {
            base.PerformLayoutBeforeChildren();

            ArrangeTracks();
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);

            // Synchronize scroll vertical bars for tracks and media panels to keep the view in sync
            var scroll1 = _tracksPanelArea.VScrollBar;
            var scroll2 = _backgroundArea.VScrollBar;
            if (scroll1.IsThumbClicked || _tracksPanelArea.IsMouseOver)
                scroll2.TargetValue = scroll1.Value;
            else
                scroll1.TargetValue = scroll2.Value;
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Vector2 location, MouseButton button)
        {
            if (base.OnMouseDown(location, button))
                return true;

            if (button == MouseButton.Right)
            {
                _isRightMouseButtonDown = true;
                _rightMouseButtonDownPos = location;
                Focus();

                return true;
            }

            if (!ContainsFocus)
            {
                Focus();
                return true;
            }

            return false;
        }

        /// <inheritdoc />
        public override bool OnMouseUp(Vector2 location, MouseButton button)
        {
            if (button == MouseButton.Right && _isRightMouseButtonDown)
            {
                _isRightMouseButtonDown = false;

                if (Vector2.Distance(ref location, ref _rightMouseButtonDownPos) < 4.0f)
                {
                    if (!ContainsFocus)
                        Focus();

                    var controlUnderMouse = GetChildAtRecursive(location);
                    var mediaUnderMouse = controlUnderMouse;
                    while (mediaUnderMouse != null && !(mediaUnderMouse is Media))
                    {
                        mediaUnderMouse = mediaUnderMouse.Parent;
                    }

                    var menu = new ContextMenu.ContextMenu();
                    if (mediaUnderMouse is Media media)
                    {
                        media.OnTimelineShowContextMenu(menu, controlUnderMouse);
                        if (media.PropertiesEditObject != null)
                        {
                            menu.AddButton("Edit media", () => ShowEditPopup(media.PropertiesEditObject, ref location, media.Track));
                        }
                    }
                    if (PropertiesEditObject != null)
                    {
                        menu.AddButton("Edit timeline", () => ShowEditPopup(PropertiesEditObject, ref location, this));
                    }
                    menu.AddSeparator();
                    menu.AddButton("Reset zoom", () => Zoom = 1.0f);
                    menu.AddButton("Show whole timeline", ShowWholeTimeline);
                    OnShowContextMenu(menu);
                    menu.Show(this, location);
                }
            }

            return base.OnMouseUp(location, button);
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            if (base.OnKeyDown(key))
                return true;

            switch (key)
            {
            case KeyboardKeys.ArrowLeft:
                OnSeek(CurrentFrame - 1);
                return true;
            case KeyboardKeys.ArrowRight:
                OnSeek(CurrentFrame + 1);
                return true;
            case KeyboardKeys.Home:
                OnSeek(0);
                return true;
            case KeyboardKeys.End:
                OnSeek(DurationFrames);
                return true;
            case KeyboardKeys.PageUp:
                if (GetNextKeyframeFrame(CurrentTime, out var nextKeyframeTime))
                {
                    OnSeek(nextKeyframeTime);
                    return true;
                }
                break;
            case KeyboardKeys.PageDown:
                if (GetPreviousKeyframeFrame(CurrentTime, out var prevKeyframeTime))
                {
                    OnSeek(prevKeyframeTime);
                    return true;
                }
                break;
            case KeyboardKeys.Spacebar:
                if (CanPlayPauseStop)
                {
                    if (PlaybackState == PlaybackStates.Playing)
                        OnPause();
                    else
                        OnPlay();
                    return true;
                }
                break;
            }

            return false;
        }

        /// <summary>
        /// Shows the whole timeline.
        /// </summary>
        public void ShowWholeTimeline()
        {
            var viewWidth = Width;
            var timelineWidth = Duration * UnitsPerSecond * Zoom + 8 * StartOffset;
            _backgroundArea.ViewOffset = Vector2.Zero;
            Zoom = viewWidth / timelineWidth;
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
                Size = new Vector2(width, height);

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
                if (timeline.Undo != null && undoContext != null)
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
            public override DragDropEffect OnDragEnter(ref Vector2 location, DragData data)
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
            public override DragDropEffect OnDragMove(ref Vector2 location, DragData data)
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
            public override DragDropEffect OnDragDrop(ref Vector2 location, DragData data)
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
                    Render2D.FillRectangle(new Rectangle(Vector2.Zero, Size), style.BackgroundSelected * 0.4f);
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

        /// <summary>
        /// Shows the timeline object editing popup.
        /// </summary>
        /// <param name="obj">The object.</param>
        /// <param name="location">The show location (in timeline space).</param>
        /// <param name="undoContext">The undo context object.</param>
        protected virtual void ShowEditPopup(object obj, ref Vector2 location, object undoContext = null)
        {
            var popup = new PropertiesEditPopup(this, obj, undoContext);
            popup.Show(this, location);
        }

        /// <summary>
        /// Called when showing context menu to the user. Can be used to add custom buttons.
        /// </summary>
        /// <param name="menu">The menu.</param>
        protected virtual void OnShowContextMenu(ContextMenu.ContextMenu menu)
        {
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            if (CameraCutThumbnailRenderer != null)
            {
                CameraCutThumbnailRenderer.Dispose();
                CameraCutThumbnailRenderer = null;
            }

            // Clear references to the controls
            _splitter = null;
            _timeIntervalsHeader = null;
            _backgroundScroll = null;
            _background = null;
            _backgroundArea = null;
            _leftEdge = null;
            _rightEdge = null;
            _addTrackButton = null;
            _fpsComboBox = null;
            _viewButton = null;
            _fpsCustomValue = null;
            _tracksPanelArea = null;
            _tracksPanel = null;
            _playbackNavigation = null;
            _playbackStop = null;
            _playbackPlay = null;
            _noTracksLabel = null;
            _positionHandle = null;
            DragHandlers.Clear();

            base.OnDestroy();
        }
    }
}
