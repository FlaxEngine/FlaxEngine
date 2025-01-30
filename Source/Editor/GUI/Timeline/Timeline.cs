// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
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
    [HideInEditor]
    public partial class Timeline : ContainerControl, IKeyframesEditorContext
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

        internal const int FormatVersion = 4;

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
            /// Gets or sets the total duration of the timeline (in frames).
            /// </summary>
            [EditorDisplay("General"), EditorOrder(10), Limit(1), VisibleIf(nameof(UseFrames)), Tooltip("Total duration of the timeline (in frames).")]
            public int DurationFrames
            {
                get => Timeline.DurationFrames;
                set => Timeline.DurationFrames = value;
            }

            /// <summary>
            /// Gets or sets the total duration of the timeline (in seconds).
            /// </summary>
            [EditorDisplay("General"), EditorOrder(10), Limit(0.0f, float.MaxValue, 0.001f), VisibleIf(nameof(UseFrames), true), Tooltip("Total duration of the timeline (in seconds).")]
            public float Duration
            {
                get => Timeline.Duration;
                set => Timeline.Duration = value;
            }

            private bool UseFrames => Timeline.TimeShowMode == TimeShowModes.Frames;

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
        public static readonly float HeaderTopAreaHeight = 40.0f;

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
        /// The Track that is being dragged over. This could have a value when not dragging.
        /// </summary>
        internal Track DraggedOverTrack = null;

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
        private BackgroundArea _backgroundArea;
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
        private ContainerControl _playbackButtonsArea;
        private PositionHandle _positionHandle;
        private bool _isRightMouseButtonDown;
        private Float2 _rightMouseButtonDownPos;
        private Float2 _rightMouseButtonMovePos;
        private Float2 _mediaMoveStartPos;
        private int[] _mediaMoveStartFrames;
        private List<Track> _mediaMoveStartTracks;
        private byte[][] _mediaMoveStartData;
        private float _zoom = 1.0f;
        private bool _isMovingPositionHandle;
        private bool _canPlayPause = true, _canStop = true;
        private List<IUndoAction> _batchedUndoActions;

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
        public float CurrentTime
        {
            get => _currentFrame / _framesPerSecond;
            set => CurrentFrame = (int)(value * _framesPerSecond);
        }

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
        public ContainerControl MediaPanel => _background;

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
                value = Mathf.Clamp(value, 0.00001f, 1000.0f);
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
        /// Gets or sets a value indicating whether user can use Play and Pause buttons, otherwise those should be disabled.
        /// </summary>
        public bool CanPlayPause
        {
            get => _canPlayPause;
            set
            {
                if (_canPlayPause == value)
                    return;
                _canPlayPause = value;
                UpdatePlaybackButtons();
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether user can use Stop button, otherwise those should be disabled.
        /// </summary>
        public bool CanPlayStop
        {
            get => _canStop;
            set
            {
                if (_canStop == value)
                    return;
                _canStop = value;
                UpdatePlaybackButtons();
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether show playback buttons area.
        /// </summary>
        public bool ShowPlaybackButtonsArea
        {
            get => _playbackButtonsArea?.Visible ?? false;
            set
            {
                if (_playbackButtonsArea != null)
                    _playbackButtonsArea.Visible = value;
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

            var style = Style.Current;
            var headerTopArea = new ContainerControl
            {
                AutoFocus = false,
                BackgroundColor = style.LightBackground,
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
            var playbackButtonsMouseOverColor = Color.FromBgra(0xFFBBBBBB);
            if (playbackButtons != PlaybackButtons.None)
            {
                playbackButtonsSize = 24.0f;
                var icons = Editor.Instance.Icons;
                var playbackButtonsArea = new ContainerControl
                {
                    AutoFocus = false,
                    ClipChildren = false,
                    BackgroundColor = style.LightBackground,
                    AnchorPreset = AnchorPresets.HorizontalStretchBottom,
                    Offsets = new Margin(0, 0, -playbackButtonsSize, playbackButtonsSize),
                    Parent = _splitter.Panel1
                };
                _playbackButtonsArea = playbackButtonsArea;
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
                        Brush = new SpriteBrush(icons.Skip64),
                        MouseOverColor = playbackButtonsMouseOverColor,
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
                        Brush = new SpriteBrush(icons.Shift64),
                        MouseOverColor = playbackButtonsMouseOverColor,
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
                        Brush = new SpriteBrush(icons.Left32),
                        MouseOverColor = playbackButtonsMouseOverColor,
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
                        Brush = new SpriteBrush(icons.Stop64),
                        MouseOverColor = playbackButtonsMouseOverColor,
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
                        Brush = new SpriteBrush(icons.Play64),
                        MouseOverColor = playbackButtonsMouseOverColor,
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
                        Brush = new SpriteBrush(icons.Right32),
                        MouseOverColor = playbackButtonsMouseOverColor,
                        Enabled = false,
                        Visible = false,
                        Parent = playbackButtonsPanel
                    };
                    _playbackNavigation[3].Clicked += (image, button) => OnSeek(CurrentFrame + 1);
                    playbackButtonsPanel.Width += playbackButtonsSize;

                    _playbackNavigation[4] = new Image(playbackButtonsPanel.Width, 0, playbackButtonsSize, playbackButtonsSize)
                    {
                        TooltipText = "Seek to the next keyframe (Page Up)",
                        Brush = new SpriteBrush(icons.Shift64),
                        MouseOverColor = playbackButtonsMouseOverColor,
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
                        Brush = new SpriteBrush(icons.Skip64),
                        MouseOverColor = playbackButtonsMouseOverColor,
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
                BackgroundColor = style.Background.RGBMultiplied(0.9f),
                AnchorPreset = AnchorPresets.HorizontalStretchTop,
                Offsets = new Margin(0, 0, 0, HeaderTopAreaHeight),
                Parent = _splitter.Panel2
            };
            _backgroundArea = new BackgroundArea(this)
            {
                AutoFocus = false,
                ClipChildren = false,
                BackgroundColor = style.Background.RGBMultiplied(0.7f),
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
            var bounds = new Rectangle();
            bounds.Location.X = StartOffset * 2.0f - handleWidth * 0.5f + _currentFrame / _framesPerSecond * UnitsPerSecond * Zoom;
            bounds.Location.Y = 0;
            bounds.Size.X = handleWidth;
            bounds.Size.Y = HeaderTopAreaHeight * 0.5f;
            _positionHandle.Bounds = bounds;
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

            {
                var zoom = menu.AddButton("Zoom");
                var zoomValue = new FloatValueBox(Zoom, 140, 2, 50.0f, 0.00001f, 1000.0f, 0.001f);
                zoomValue.Parent = zoom;
                zoomValue.ValueChanged += () => Zoom = zoomValue.Value;
            }

            OnShowViewContextMenu(menu);

            menu.Show(_viewButton.Parent, _viewButton.BottomLeft);
        }

        /// <summary>
        /// Occurs when timeline shows the View context menu. Can be used to add custom options.
        /// </summary>
        public event Action<ContextMenu.ContextMenu> ShowViewContextMenu;

        /// <summary>
        /// Called when timeline shows the View context menu. Can be used to add custom options.
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
                    _playbackPlay.Enabled = false;
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
                    _playbackPlay.Enabled = _canPlayPause;
                    _playbackPlay.Brush = new SpriteBrush(icons.Play64);
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
                    _playbackStop.Enabled = _canStop;
                }
                if (_playbackPlay != null)
                {
                    _playbackPlay.Visible = true;
                    _playbackPlay.Enabled = _canPlayPause;
                    _playbackPlay.Brush = new SpriteBrush(icons.Pause64);
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
                    _playbackStop.Enabled = _canStop;
                }
                if (_playbackPlay != null)
                {
                    _playbackPlay.Visible = true;
                    _playbackPlay.Enabled = _canPlayPause;
                    _playbackPlay.Brush = new SpriteBrush(icons.Play64);
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
            };
            return archetype.Create(options);
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
            track.Name = GetValidTrackName(track.Name);

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
                OnKeyframesDeselect(null);
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
        /// Deletes the selected tracks.
        /// </summary>
        /// <param name="withUndo">True if use undo/redo action for track removing.</param>
        public void DeleteSelectedTracks(bool withUndo = true)
        {
            if (SelectedTracks.Count == 0)
                return;
            var tracks = new List<Track>(SelectedTracks.Count);
            for (int i = 0; i < SelectedTracks.Count; i++)
            {
                GetTracks(SelectedTracks[i], tracks);
            }
            
            // Find the lowest track position for selection
            int lowestTrackLocation = Tracks.Count - 1;
            for (int i = 0; i < tracks.Count; i++)
            {
                var trackToDelete = tracks[i];
                if (trackToDelete.TrackIndex < lowestTrackLocation)
                {
                    lowestTrackLocation = trackToDelete.TrackIndex;
                }
            }
            SelectedTracks.Clear();
            if (withUndo && Undo != null && Undo.Enabled)
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

            // Select track above deleted tracks unless track is first track
            if (Tracks.Count > 0)
            {
                if (lowestTrackLocation - 1 >= 0)
                    Select(Tracks[lowestTrackLocation - 1]);
                else
                    Select(Tracks[0]);
                
                SelectedTracks[0].Focus();
            }

        }

        /// <summary>
        /// Deletes the track.
        /// </summary>
        /// <param name="track">The track to delete (and its sub tracks).</param>
        /// <param name="withUndo">True if use undo/redo action for track removing.</param>
        public void Delete(Track track, bool withUndo = true)
        {
            if (track == null)
                throw new ArgumentNullException();
            var tracks = new List<Track>(4);
            GetTracks(track, tracks);
            if (withUndo && Undo != null && Undo.Enabled)
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
        /// Deletes the media.
        /// </summary>
        /// <param name="media">The media to delete.</param>
        /// <param name="withUndo">True if use undo/redo action for media removing.</param>
        public void Delete(Media media, bool withUndo = true)
        {
            if (media == null)
                throw new ArgumentNullException();
            var track = media.Track;
            if (track == null)
                throw new InvalidOperationException();
            if (withUndo && Undo != null && Undo.Enabled)
            {
                var before = EditTrackAction.CaptureData(track);
                OnDeleteMedia(media);
                var after = EditTrackAction.CaptureData(track);
                Undo.AddAction(new EditTrackAction(this, track, before, after));
            }
            else
            {
                OnDeleteMedia(media);
            }
            MarkAsEdited();
        }

        /// <summary>
        /// Adds the media.
        /// </summary>
        /// <param name="track">The track to add media to.</param>
        /// <param name="media">The media to add.</param>
        /// <param name="withUndo">True if use undo/redo action for media adding.</param>
        public void AddMedia(Track track, Media media, bool withUndo = true)
        {
            if (track == null || media == null)
                throw new ArgumentNullException();
            if (media.Track != null)
                throw new InvalidOperationException();
            if (withUndo && Undo != null && Undo.Enabled)
            {
                var before = EditTrackAction.CaptureData(track);
                track.AddMedia(media);
                var after = EditTrackAction.CaptureData(track);
                Undo.AddAction(new EditTrackAction(this, track, before, after));
            }
            else
            {
                track.AddMedia(media);
            }
            MarkAsEdited();
            Select(media);
        }

        /// <summary>
        /// Called to delete media.
        /// </summary>
        /// <param name="media">The media.</param>
        protected virtual void OnDeleteMedia(Media media)
        {
            SelectedMedia.Remove(media);
            media.Track.RemoveMedia(media);
            media.OnDeleted();
        }

        /// <summary>
        /// Duplicates the selected tracks.
        /// </summary>
        /// <param name="withUndo">True if use undo/redo action for track duplication.</param>
        public void DuplicateSelectedTracks(bool withUndo = true)
        {
            if (SelectedTracks.Count == 0)
                return;
            var tracks = new List<Track>(SelectedTracks.Count);
            for (int i = 0; i < SelectedTracks.Count; i++)
            {
                GetTracks(SelectedTracks[i], tracks);
            }
            var clones = new Track[tracks.Count];
            for (int i = 0; i < tracks.Count; i++)
            {
                var track = tracks[i];
                var options = new TrackCreateOptions
                {
                    Archetype = track.Archetype,
                    Flags = track.Flags,
                };
                var clone = options.Archetype.Create(options);
                clone.Name = track.CanRename ? GetValidTrackName(track.Name) : track.Name;
                clone.Color = track.Color;
                clone.IsExpanded = track.IsExpanded;
                byte[] data;
                using (var memory = new MemoryStream(512))
                using (var stream = new BinaryWriter(memory))
                {
                    // TODO: reuse memory stream to improve tracks duplication performance
                    options.Archetype.Save(track, stream);
                    data = memory.ToArray();
                }
                using (var memory = new MemoryStream(data))
                using (var stream = new BinaryReader(memory))
                {
                    track.Archetype.Load(Timeline.FormatVersion, clone, stream);
                }
                var trackParent = track.ParentTrack;
                var trackIndex = track.TrackIndex + 1;
                if (trackParent != null && tracks.Contains(trackParent))
                {
                    for (int j = 0; j < i; j++)
                    {
                        if (tracks[j] == trackParent)
                        {
                            trackParent = clones[j];
                            break;
                        }
                    }
                    trackIndex--;
                }
                clone.ParentTrack = trackParent;
                clone.TrackIndex = trackIndex;
                track.OnDuplicated(clone);
                AddTrack(clone, false);
                clones[i] = clone;
            }
            OnTracksOrderChanged();
            if (withUndo && Undo != null && Undo.Enabled)
            {
                if (clones.Length == 1)
                {
                    Undo.AddAction(new AddRemoveTrackAction(this, clones[0], true));
                }
                else
                {
                    var actions = new List<IUndoAction>();
                    for (int i = 0; i < clones.Length; i++)
                        actions.Add(new AddRemoveTrackAction(this, clones[i], true));
                    Undo.AddAction(new MultiUndoAction(actions, "Remove tracks"));
                }
            }
            OnTracksChanged();
            MarkAsEdited();
            
            // Deselect and select new clones.
            Deselect();
            foreach (var clone in clones)
            {
                Select(clone, true);
            }

            SelectedTracks[0].Focus();
        }

        /// <summary>
        /// Splits the media (all or selected only) at the given frame.
        /// </summary>
        /// <param name="frame">The frame to split at.</param>
        public void Split(int frame)
        {
            List<IUndoAction> actions = null;
            foreach (var track in _tracks)
            {
                byte[] trackData = null;
                for (int i = track.Media.Count - 1; i >= 0; i--)
                {
                    if (track.Media.Count >= track.MaxMediaCount)
                        break;
                    var media = track.Media[i];
                    if (media.CanSplit && media.StartFrame < frame && media.EndFrame > frame)
                    {
                        if (Undo != null && Undo.Enabled)
                            trackData = EditTrackAction.CaptureData(track);
                        media.Split(frame);
                    }
                }
                if (trackData != null)
                {
                    if (actions == null)
                        actions = new List<IUndoAction>();
                    actions.Add(new EditTrackAction(this, track, trackData, EditTrackAction.CaptureData(track)));
                }
            }
            if (actions != null)
                Undo.AddAction(new MultiUndoAction(actions, "Split"));
        }

        /// <summary>
        /// Called once to setup the drag drop handling for the timeline (lazy init on first drag action).
        /// </summary>
        protected virtual void SetupDragDrop()
        {
        }

        /// <summary>
        /// Custom event for keyframes or curve editor view panning to handle timeline background panning horizontally too.
        /// </summary>
        /// <seealso cref="KeyframesEditor.CustomViewPanning"/>
        /// <seealso cref="CurveEditorBase.CustomViewPanning"/>
        /// <param name="delta">The input delta.</param>
        /// <returns>The result input delta.</returns>
        public Float2 OnKeyframesViewPanning(Float2 delta)
        {
            var area = _backgroundArea;
            var hScroll = area.HScrollBar.Visible && area.HScrollBar.Enabled;
            if (hScroll)
            {
                bool wasLocked = area.IsLayoutLocked;
                area.IsLayoutLocked = true;
                area.HScrollBar.TargetValue -= delta.X;
                delta.X = 0.0f;
                area.IsLayoutLocked = wasLocked;
                area.PerformLayout();
                area.Cursor = CursorType.SizeWE;
            }
            return delta;
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
        /// Gets the name of the track that is valid to use for a timeline.
        /// </summary>
        /// <param name="name">The name.</param>
        /// <returns>The track name.</returns>
        public string GetValidTrackName(string name)
        {
            return Utilities.Utils.IncrementNameNumber(name, IsTrackNameValid);
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
                _leftEdge.Bounds = new Rectangle(_background.Left - edgeWidth * 0.5f + StartOffset, 0, edgeWidth, height);
                _rightEdge.Bounds = new Rectangle(_background.Right - edgeWidth * 0.5f + StartOffset, 0, edgeWidth, height);

                _backgroundScroll.Bounds = new Rectangle(0, 0, _background.Width + 5 * StartOffset, height);
            }
        }

        /// <summary>
        /// Sets the text for the No Tracks UI.
        /// </summary>
        /// <param name="text">The text.</param>
        public void SetNoTracksText(string text)
        {
            if (_noTracksLabel != null)
            {
                _noTracksLabel.Text = text ?? "No tracks";
            }
        }

        /// <summary>
        /// Adds the undo action to be batched (eg. if multiple undo actions is performed in a sequence during single update).
        /// </summary>
        /// <param name="action">The action.</param>
        public void AddBatchedUndoAction(IUndoAction action)
        {
            if (Undo == null || !Undo.Enabled)
                return;
            if (_batchedUndoActions == null)
                _batchedUndoActions = new List<IUndoAction>();
            _batchedUndoActions.Add(action);
        }

        internal void ShowContextMenu(Float2 location)
        {
            if (!ContainsFocus)
                Focus();

            var timelinePos = MediaPanel.PointFromParent(this, location);
            var time = (timelinePos.X - StartOffset) / (UnitsPerSecond * Zoom);

            var controlUnderMouse = GetChildAtRecursive(location);
            var mediaUnderMouse = controlUnderMouse;
            while (mediaUnderMouse != null && !(mediaUnderMouse is Media))
            {
                mediaUnderMouse = mediaUnderMouse.Parent;
            }

            var menu = new ContextMenu.ContextMenu();
            if (mediaUnderMouse is Media media)
            {
                media.OnTimelineContextMenu(menu, time, controlUnderMouse);
                if (media.PropertiesEditObject != null)
                {
                    menu.AddButton("Edit media", () => ShowEditPopup(media.PropertiesEditObject, location, media.Track));
                }
            }
            else
            {
                OnKeyframesDeselect(null);
                foreach (var track in _tracks)
                {
                    if (Mathf.IsInRange(timelinePos.Y, track.Top, track.Bottom))
                    {
                        track.OnTimelineContextMenu(menu, time);
                        break;
                    }
                }
            }
            if (PropertiesEditObject != null)
            {
                menu.AddButton("Edit timeline", () => ShowEditPopup(PropertiesEditObject, location, this));
            }
            if (_tracks.Count > 1)
            {
                menu.AddButton("Sort tracks", SortTracks).TooltipText = "Sorts tracks alphabetically";
            }
            menu.AddSeparator();
            menu.AddButton("Reset zoom", () => Zoom = 1.0f);
            menu.AddButton("Show whole timeline", ShowWholeTimeline);
            OnShowContextMenu(menu);
            menu.Show(this, location);
        }

        private void SortTracks()
        {
            var rootTracks = new List<Track>();
            foreach (var track in _tracks)
            {
                if (track.ParentTrack == null)
                    rootTracks.Add(track);
            }

            // TODO: undo for tracks sorting
            _tracks.Clear();
            rootTracks.Sort();
            foreach (var track in rootTracks)
                CollectTracks(track);

            OnTracksOrderChanged();
            MarkAsEdited();
        }

        internal void SortTrack(Track e, Action sort)
        {
            var rootTracks = new List<Track>();
            foreach (var track in _tracks)
            {
                if (track.ParentTrack == null)
                    rootTracks.Add(track);
            }

            // TODO: undo for tracks sorting
            _tracks.Clear();
            sort();
            foreach (var track in rootTracks)
                CollectTracks(track);

            OnTracksOrderChanged();
            MarkAsEdited();
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

            // Batch undo actions
            if (_batchedUndoActions != null && _batchedUndoActions.Count != 0)
            {
                Undo.AddAction(_batchedUndoActions.Count == 1 ? _batchedUndoActions[0] : new MultiUndoAction(_batchedUndoActions));
                _batchedUndoActions.Clear();
            }
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            if (base.OnMouseDown(location, button))
                return true;

            if (button == MouseButton.Right)
            {
                _isRightMouseButtonDown = true;
                _rightMouseButtonDownPos = location;
                _rightMouseButtonMovePos = location;
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
        public override bool OnMouseUp(Float2 location, MouseButton button)
        {
            if (button == MouseButton.Right && _isRightMouseButtonDown)
            {
                _isRightMouseButtonDown = false;
                if (Float2.Distance(ref location, ref _rightMouseButtonDownPos) < 4.0f)
                    ShowContextMenu(location);
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
                if (CanPlayPause)
                {
                    if (PlaybackState == PlaybackStates.Playing)
                        OnPause();
                    else
                        OnPlay();
                    return true;
                }
                break;
            case KeyboardKeys.S:
                if (!Root.GetKey(KeyboardKeys.Control))
                {
                    Split(CurrentFrame);
                }
                return true;
            case KeyboardKeys.Delete:
                OnKeyframesDelete(null);
                return true;
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
            _backgroundArea.ViewOffset = Float2.Zero;
            Zoom = viewWidth / timelineWidth;
        }

        /// <summary>
        /// Shows the timeline object editing popup.
        /// </summary>
        /// <param name="obj">The object.</param>
        /// <param name="location">The show location (in timeline space).</param>
        /// <param name="undoContext">The undo context object.</param>
        public virtual void ShowEditPopup(object obj, Float2 location, object undoContext = null)
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
            _playbackButtonsArea = null;
            _positionHandle = null;
            DragHandlers.Clear();

            base.OnDestroy();
        }

        /// <inheritdoc />
        public void OnKeyframesDeselect(IKeyframesEditor editor)
        {
            for (int i = 0; i < _tracks.Count; i++)
            {
                if (_tracks[i] is IKeyframesEditorContext trackContext)
                    trackContext.OnKeyframesDeselect(editor);
            }
            if (SelectedMedia.Count != 0)
            {
                SelectedMedia.Clear();
                OnSelectionChanged();
            }
        }

        /// <inheritdoc />
        public void OnKeyframesSelection(IKeyframesEditor editor, ContainerControl control, Rectangle selection)
        {
            var globalControl = _backgroundArea;
            var globalRect = Rectangle.FromPoints(control.PointToParent(globalControl, selection.UpperLeft), control.PointToParent(globalControl, selection.BottomRight));
            var mediaControl = MediaPanel;
            var mediaRect = Rectangle.FromPoints(mediaControl.PointFromParent(globalRect.UpperLeft), mediaControl.PointFromParent(globalRect.BottomRight));
            var selectionChanged = false;
            if (SelectedMedia.Count != 0)
            {
                SelectedMedia.Clear();
                selectionChanged = true;
            }
            for (int i = 0; i < _tracks.Count; i++)
            {
                if (_tracks[i] is IKeyframesEditorContext trackContext)
                    trackContext.OnKeyframesSelection(editor, globalControl, globalRect);

                foreach (var media in _tracks[i].Media)
                {
                    if (media.Bounds.Intersects(ref mediaRect))
                    {
                        SelectedMedia.Add(media);
                        selectionChanged = true;
                    }
                }
            }
            if (selectionChanged)
            {
                OnSelectionChanged();
            }
        }

        /// <inheritdoc />
        public int OnKeyframesSelectionCount()
        {
            int result = SelectedMedia.Count;
            for (int i = 0; i < _tracks.Count; i++)
            {
                if (_tracks[i] is IKeyframesEditorContext trackContext)
                    result += trackContext.OnKeyframesSelectionCount();
            }
            return result;
        }

        /// <inheritdoc />
        public void OnKeyframesDelete(IKeyframesEditor editor)
        {
            for (int i = 0; i < _tracks.Count; i++)
            {
                if (_tracks[i] is IKeyframesEditorContext trackContext)
                    trackContext.OnKeyframesDelete(editor);
            }

            // Delete selected media events
            if (SelectedMedia.Count != 0)
            {
                if (Undo != null && Undo.Enabled)
                {
                    // Undo per-track
                    if (_mediaMoveStartTracks == null)
                        _mediaMoveStartTracks = new List<Track>();
                    else
                        _mediaMoveStartTracks.Clear();
                    for (var i = 0; i < SelectedMedia.Count; i++)
                    {
                        var media = SelectedMedia[i];
                        if (!_mediaMoveStartTracks.Contains(media.Track))
                            _mediaMoveStartTracks.Add(media.Track);
                    }
                    _mediaMoveStartData = new byte[_mediaMoveStartTracks.Count][];
                    for (int i = 0; i < _mediaMoveStartData.Length; i++)
                        _mediaMoveStartData[i] = EditTrackAction.CaptureData(_mediaMoveStartTracks[i]);
                }

                foreach (var media in SelectedMedia.ToArray())
                    OnDeleteMedia(media);

                if (Undo != null && Undo.Enabled)
                {
                    for (int i = 0; i < _mediaMoveStartData.Length; i++)
                    {
                        var track = _mediaMoveStartTracks[i];
                        var before = _mediaMoveStartData[i];
                        var after = EditTrackAction.CaptureData(track);
                        if (!Utils.ArraysEqual(before, after))
                            AddBatchedUndoAction(new EditTrackAction(this, track, before, after));
                    }
                }

                MarkAsEdited();
            }
        }

        /// <inheritdoc />
        public void OnKeyframesMove(IKeyframesEditor editor, ContainerControl control, Float2 location, bool start, bool end)
        {
            location = control.PointToParent(_backgroundArea, location);
            for (int i = 0; i < _tracks.Count; i++)
            {
                if (_tracks[i] is IKeyframesEditorContext trackContext)
                    trackContext.OnKeyframesMove(editor, _backgroundArea, location, start, end);
            }
            if (SelectedMedia.Count != 0)
            {
                location = MediaPanel.PointFromParent(location);
                if (start)
                {
                    // Start moving selected media events
                    _mediaMoveStartPos = location;
                    _mediaMoveStartFrames = new int[SelectedMedia.Count];
                    if (_mediaMoveStartTracks == null)
                        _mediaMoveStartTracks = new List<Track>();
                    else
                        _mediaMoveStartTracks.Clear();
                    for (var i = 0; i < SelectedMedia.Count; i++)
                    {
                        var media = SelectedMedia[i];
                        _mediaMoveStartFrames[i] = media.StartFrame;
                        if (!_mediaMoveStartTracks.Contains(media.Track))
                            _mediaMoveStartTracks.Add(media.Track);
                    }
                    if (Undo != null && Undo.Enabled)
                    {
                        // Undo per-track
                        _mediaMoveStartData = new byte[_mediaMoveStartTracks.Count][];
                        for (int i = 0; i < _mediaMoveStartData.Length; i++)
                            _mediaMoveStartData[i] = EditTrackAction.CaptureData(_mediaMoveStartTracks[i]);
                    }
                }
                else if (end)
                {
                    // End moving selected media events
                    if (_mediaMoveStartData != null)
                    {
                        for (int i = 0; i < _mediaMoveStartData.Length; i++)
                        {
                            var track = _mediaMoveStartTracks[i];
                            var before = _mediaMoveStartData[i];
                            var after = EditTrackAction.CaptureData(track);
                            if (!Utils.ArraysEqual(before, after))
                                AddBatchedUndoAction(new EditTrackAction(this, track, before, after));
                        }
                    }
                    MarkAsEdited();
                    _mediaMoveStartTracks.Clear();
                    _mediaMoveStartFrames = null;
                }
                else
                {
                    // Move selected media events
                    var moveLocationDelta = location - _mediaMoveStartPos;
                    var moveDelta = (int)(moveLocationDelta.X / (UnitsPerSecond * Zoom) * FramesPerSecond);
                    for (var i = 0; i < SelectedMedia.Count; i++)
                        SelectedMedia[i].StartFrame = _mediaMoveStartFrames[i] + moveDelta;
                }
            }
        }

        /// <inheritdoc />
        public void OnKeyframesCopy(IKeyframesEditor editor, float? timeOffset, System.Text.StringBuilder data)
        {
            var area = _backgroundArea;
            var hScroll = area.HScrollBar.Visible && area.HScrollBar.Enabled;
            if (hScroll && !timeOffset.HasValue)
            {
                // Offset copied keyframes relative to the current view start
                timeOffset = (area.HScrollBar.Value - StartOffset * 2.0f) / (UnitsPerSecond * Zoom);
            }
            for (int i = 0; i < _tracks.Count; i++)
            {
                if (_tracks[i] is IKeyframesEditorContext trackContext)
                    trackContext.OnKeyframesCopy(editor, timeOffset, data);
            }
        }

        /// <inheritdoc />
        public void OnKeyframesPaste(IKeyframesEditor editor, float? timeOffset, string[] datas, ref int index)
        {
            var area = _backgroundArea;
            var hScroll = area.HScrollBar.Visible && area.HScrollBar.Enabled;
            if (hScroll && !timeOffset.HasValue)
            {
                // Offset pasted keyframes relative to the current view start
                timeOffset = (area.HScrollBar.Value - StartOffset * 2.0f) / (UnitsPerSecond * Zoom);
            }
            for (int i = 0; i < _tracks.Count; i++)
            {
                if (_tracks[i] is IKeyframesEditorContext trackContext)
                    trackContext.OnKeyframesPaste(editor, timeOffset, datas, ref index);
            }
        }

        /// <inheritdoc />
        public void OnKeyframesGet(Action<string, float, object> get)
        {
            for (int i = 0; i < _tracks.Count; i++)
            {
                if (_tracks[i] is IKeyframesEditorContext trackContext)
                    trackContext.OnKeyframesGet(get);
            }
        }

        /// <inheritdoc />
        public void OnKeyframesSet(List<KeyValuePair<float, object>> keyframes)
        {
        }
    }
}
