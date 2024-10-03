// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Xml;
using FlaxEditor.Content;
using FlaxEditor.CustomEditors;
using FlaxEditor.CustomEditors.Editors;
using FlaxEditor.GUI;
using FlaxEditor.GUI.Timeline;
using FlaxEditor.Scripting;
using FlaxEngine;
using FlaxEngine.GUI;
using Object = FlaxEngine.Object;

namespace FlaxEditor.Windows.Assets
{
    /// <summary>
    /// Scene Animation window allows to view and edit <see cref="SceneAnimation"/> asset.
    /// Note: it uses ClonedAssetEditorWindowBase which is creating cloned asset to edit/preview.
    /// </summary>
    /// <seealso cref="SceneAnimation" />
    /// <seealso cref="FlaxEditor.Windows.Assets.AssetEditorWindow" />
    public sealed class SceneAnimationWindow : ClonedAssetEditorWindowBase<SceneAnimation>
    {
        struct StagingTexture
        {
            public GPUTexture Texture;
            public int AnimationFrame;
            public int TaskFrame;
        }

        abstract class VideoOutput
        {
            public abstract void RenderFrame(GPUContext context, ref StagingTexture frame, RenderOptions options, GPUTexture output);

            public abstract void CollectFrame(GPUContext context, ref StagingTexture frame, RenderOptions options);
        }

        abstract class VideoOutputImage : VideoOutput
        {
            protected abstract string Extension { get; }

            public override void RenderFrame(GPUContext context, ref StagingTexture frame, RenderOptions options, GPUTexture output)
            {
                // Copy texture back to the staging texture
                context.CopyTexture(frame.Texture, 0, 0, 0, 0, output, 0);
            }

            public override void CollectFrame(GPUContext context, ref StagingTexture frame, RenderOptions options)
            {
                // Save the staging texture to the file
                var path = options.GetOutputPath(ref frame) + Extension;
                Screenshot.Capture(frame.Texture, path);
            }
        }

        class VideoOutputPng : VideoOutputImage
        {
            protected override string Extension => ".png";
        }

        class VideoOutputBmp : VideoOutputImage
        {
            protected override string Extension => ".bmp";
        }

        class VideoOutputJpg : VideoOutputImage
        {
            protected override string Extension => ".jpg";
        }

        sealed class VideoOutputEditor : ObjectSwitcherEditor
        {
            public override DisplayStyle Style => DisplayStyle.InlineIntoParent;

            protected override OptionType[] Options => new[]
            {
                new OptionType("Image (.png)", typeof(VideoOutputPng)),
                new OptionType("Image (.bmp)", typeof(VideoOutputBmp)),
                new OptionType("Image (.jpg)", typeof(VideoOutputJpg)),
            };

            protected override string TypeComboBoxName => "Video Output Type";
        }

        enum ResolutionModes
        {
            [EditorDisplay(null, "640x480 (VGA)")]
            _640x480,

            [EditorDisplay(null, "1280x720 (HD)")]
            _1280x720,

            [EditorDisplay(null, "1920x1080 (Full HD)")]
            _1920x1080,

            [EditorDisplay(null, "2560x1440 (2K)")]
            _2560x1440,

            [EditorDisplay(null, "3840x2160 (4K)")]
            _3840x2160,

            [EditorDisplay(null, "7680x4320 (8K)")]
            _7680x4320,

            Custom,
        }

        [CustomEditor(typeof(RenderOptionsEditor))]
        sealed class RenderOptions
        {
            [HideInEditor, NoSerialize]
            public SceneAnimation Animation;

            [EditorDisplay("Options"), EditorOrder(0)]
            [Tooltip("The output video format type and options.")]
            [CustomEditor(typeof(VideoOutputEditor))]
            public VideoOutput VideoOutputFormat = new VideoOutputJpg();

            [EditorDisplay("Options"), EditorOrder(10)]
            [Tooltip("The output video resolution (in pixels). Use Custom option to specify it manually.")]
            public ResolutionModes Resolution = ResolutionModes._1920x1080;

            public bool IsCustomResolution => Resolution == ResolutionModes.Custom;

            [EditorDisplay("Options"), EditorOrder(20)]
            [Tooltip("The custom output video width (in pixels).")]
            [VisibleIf(nameof(IsCustomResolution)), Limit(1, 8192)]
            public int Width = 1280;

            [EditorDisplay("Options"), EditorOrder(21)]
            [Tooltip("The custom output video height (in pixels).")]
            [VisibleIf(nameof(IsCustomResolution)), Limit(1, 8192)]
            public int Height = 720;

            [EditorDisplay("Options"), EditorOrder(40)]
            [Tooltip("The animation update rate (amount of frames per second).")]
            [Limit(10, 240)]
            public float FrameRate = 60.0f;

            [EditorDisplay("Options"), EditorOrder(50)]
            [Tooltip("The animation playback start (in seconds). Can be used to trim rendered movie.")]
            [Limit(0)]
            public float StartTime = 0.0f;

            [EditorDisplay("Options"), EditorOrder(60)]
            [Tooltip("The animation playback end time (in seconds). Can be used to trim rendered movie.")]
            [Limit(0)]
            public float EndTime;

            [EditorDisplay("Options", "Warm Up Time"), EditorOrder(70)]
            [Tooltip("The rendering warmup time to wait before starting recording (in seconds). Can be used to pre-render few frames for the initial camera shot so all temporal visual effects are having enough history samples to give better-looking results.")]
            [Limit(0, 10)]
            public float WarmUpTime = 0.4f;

            [EditorDisplay("Options"), EditorOrder(80)]
            [Tooltip("The animation playback speed ratio. Can be used to slow down or speed up animation.")]
            [Limit(0, 100)]
            public float PlaybackSpeed = 1.0f;

            [EditorDisplay("Output"), EditorOrder(100)]
            [Tooltip("The output folder for the rendering process artifact files. Relative to the project folder or absolute path.")]
            public string OutputDirectory = "Output/Render";

            [EditorDisplay("Output"), EditorOrder(110)]
            [Tooltip("The output file name format for the rendering process artifact files. Can be fixed or use following format arguments: {animation}, {fps}, {frame}, {width}, {height}.")]
            public string Filename = "{animation}_{frame}";

            public Int2 GetResolution()
            {
                switch (Resolution)
                {
                case ResolutionModes._640x480: return new Int2(640, 480);
                case ResolutionModes._1280x720: return new Int2(1280, 720);
                case ResolutionModes._1920x1080: return new Int2(1920, 1080);
                case ResolutionModes._2560x1440: return new Int2(2560, 1440);
                case ResolutionModes._3840x2160: return new Int2(3840, 2160);
                case ResolutionModes._7680x4320: return new Int2(7680, 4320);
                case ResolutionModes.Custom: return new Int2(Width, Height);
                default: throw new ArgumentOutOfRangeException();
                }
            }

            public string GetOutputPath(ref StagingTexture texture)
            {
                var dir = Path.IsPathRooted(OutputDirectory) ? OutputDirectory : Path.Combine(Globals.ProjectFolder, OutputDirectory);
                if (!Directory.Exists(dir))
                    Directory.CreateDirectory(dir);
                var resolution = GetResolution();
                var filename = Filename;
                filename = filename.Replace("{animation}", Path.GetFileNameWithoutExtension(Animation.Path).Replace(' ', '_'));
                filename = filename.Replace("{fps}", FrameRate.ToString(CultureInfo.InvariantCulture));
                filename = filename.Replace("{frame}", texture.AnimationFrame.ToString());
                filename = filename.Replace("{width}", resolution.X.ToString());
                filename = filename.Replace("{height}", resolution.Y.ToString());
                return Path.Combine(dir, filename);
            }
        }

        sealed class RenderOptionsEditor : GenericEditor
        {
            public override void Initialize(LayoutElementsContainer layout)
            {
                layout.Label("Scene Animation Rendering Utility", TextAlignment.Center);
                layout.Space(10.0f);

                base.Initialize(layout);
            }
        }

        sealed class RenderProgress : Progress.ProgressHandler
        {
            public void Start()
            {
                OnStart();
                OnUpdate(0.0f, "Warming up...");
            }

            public void Update(float progress, int frame)
            {
                OnUpdate(progress, string.Format("Rendering scene animation (frame {0})...", frame));
            }

            public void End()
            {
                OnEnd();
            }
        }

        sealed class RenderEditorState : States.EditorState
        {
            private readonly RenderPopup _popup;

            public RenderEditorState(Editor editor, RenderPopup popup)
            : base(editor)
            {
                _popup = popup;
            }

            public override bool CanChangeScene => false;

            public override bool CanEditContent => false;

            public override bool CanEditScene => false;

            public override bool CanEnterPlayMode => false;

            public override bool CanReloadScripts => false;

            public override bool CanUseToolbox => false;

            public override bool CanUseUndoRedo => false;

            public override void UpdateFPS()
            {
                Time.UpdateFPS = Time.DrawFPS = _popup.Options.FrameRate;
                Time.PhysicsFPS = 0;
            }
        }

        sealed class RenderPopup : Panel
        {
            private enum States
            {
                Warmup,
                Render,
                Update,
                Staging,
            }

            private const int FrameLatency = 3;

            private SceneAnimationWindow _window;
            private RenderOptions _options;
            private CustomEditorPresenter _presenter;
            private bool _isRendering;
            private float _warmUpTimeLeft;
            private float _dt;
            private int _animationFrame;
            private bool _wasGamePaused, _wasTickEnabled;
            private SceneAnimationPlayer _player;
            private States _state;
            private readonly StagingTexture[] _stagingTextures = new StagingTexture[FrameLatency + 1];
            private RenderProgress _progress;
            private RenderEditorState _editorState;
            private GameWindow _gameWindow;

            public RenderOptions Options => _options;

            public RenderPopup(SceneAnimationWindow window)
            : base(ScrollBars.Vertical)
            {
                _window = window;
                _options = new RenderOptions
                {
                    Animation = window.OriginalAsset,
                    EndTime = window.Timeline.Duration,
                };
                _presenter = new CustomEditorPresenter(null);
                _presenter.Panel.AnchorPreset = AnchorPresets.HorizontalStretchTop;
                _presenter.Panel.IsScrollable = true;
                _presenter.Panel.Parent = this;
                _presenter.AfterLayout += OnAfterLayout;
                _presenter.Select(_options);
                var cancelButton = new Button
                {
                    Bounds = new Rectangle(4, 4, 60, 24),
                    Text = "Cancel",
                    TooltipText = "Cancel the rendering",
                    Parent = this
                };
                cancelButton.Clicked += Close;

                _window.Timeline.Enabled = false;
                _window.Timeline.Visible = false;
                _window._toolstrip.Enabled = false;

                BackgroundColor = Style.Current.Background;
                Offsets = Margin.Zero;
                AnchorPreset = AnchorPresets.StretchAll;
                Parent = _window;
            }

            private void OnAfterLayout(LayoutElementsContainer layout)
            {
                layout.Space(10);
                var button = layout.Button("Render").Button;
                button.TooltipText = "Start the Scene Animation rendering using the specified options";
                button.Clicked += StartRendering;
            }

            private void StartRendering()
            {
                if (_isRendering)
                    return;

                var editor = Editor.Instance;

                // Check if save the asset before rendering
                if (_window.IsEdited)
                {
                    var result = MessageBox.Show("Save scene animation asset to file before rendering?", "Save?", MessageBoxButtons.YesNoCancel, MessageBoxIcon.Question);
                    if (result == DialogResult.Yes)
                        _window.Save();
                    else if (result != DialogResult.No)
                        return;
                }

                // Update UI
                _isRendering = true;
                _presenter.Panel.Enabled = false;
                _presenter.BuildLayoutOnUpdate();

                // Start rendering
                Editor.Log("Starting scene animation rendering " + _options.Animation);
                _dt = 1.0f / _options.FrameRate;
                _player = new SceneAnimationPlayer
                {
                    Animation = _options.Animation,
                    HideFlags = HideFlags.FullyHidden,
                    RestoreStateOnStop = true,
                    PlayOnStart = false,
                    RandomStartTime = false,
                    StartTime = _options.StartTime,
                    UpdateMode = SceneAnimationPlayer.UpdateModes.Manual,
                };
                FlaxEngine.Scripting.Update += Tick;
                _wasGamePaused = Time.GamePaused;
                _wasTickEnabled = Level.TickEnabled;
                Time.GamePaused = false;
                Time.SetFixedDeltaTime(true, _dt);
                Time.UpdateFPS = Time.DrawFPS = _options.FrameRate;
                if (!Editor.IsPlayMode)
                {
                    // Don't simulate physics and don't tick game when rendering at edit time
                    Time.PhysicsFPS = 0;
                    Level.TickEnabled = false;
                }
                Level.SpawnActor(_player);
                var gameWin = editor.Windows.GameWin;
                var resolution = _options.GetResolution();
                gameWin.Viewport.CustomResolution = resolution;
                gameWin.Viewport.BackgroundColor = Color.Black;
                gameWin.Viewport.KeepAspectRatio = true;
                gameWin.Viewport.Task.PostRender += OnPostRender;
                if (!gameWin.Visible)
                    gameWin.Show();
                else if (!gameWin.IsFocused)
                    gameWin.Focus();
                _gameWindow = gameWin;
                _warmUpTimeLeft = _options.WarmUpTime;
                _animationFrame = 0;
                var stagingTextureDesc = GPUTextureDescription.New2D(resolution.X, resolution.Y, gameWin.Viewport.Task.Output.Format, GPUTextureFlags.None);
                stagingTextureDesc.Usage = GPUResourceUsage.StagingReadback;
                for (int i = 0; i < _stagingTextures.Length; i++)
                {
                    _stagingTextures[i].Texture = new GPUTexture();
                    _stagingTextures[i].Texture.Init(ref stagingTextureDesc);
                    _stagingTextures[i].AnimationFrame = -1;
                    _stagingTextures[i].TaskFrame = -1;
                }
                _player.Play();
                if (!_player.IsPlaying)
                {
                    Editor.LogError("Scene Animation Player failed to start playing.");
                    CancelRendering();
                    return;
                }
                if (_warmUpTimeLeft > 0.0f)
                {
                    // Start warmup time
                    _player.Tick(0.0f);
                    _player.Pause();
                    _state = States.Warmup;
                }
                else
                {
                    // Render first frame
                    _state = States.Render;
                }
                if (!Editor.IsPlayMode)
                {
                    _editorState = new RenderEditorState(editor, this);
                    editor.StateMachine.AddState(_editorState);
                    editor.StateMachine.GoToState(_editorState);
                }
                _progress = new RenderProgress();
                editor.ProgressReporting.RegisterHandler(_progress);
                _progress.Start();
            }

            private void Close()
            {
                CancelRendering();
                Dispose();
            }

            private void CancelRendering()
            {
                if (!_isRendering)
                    return;

                var editor = Editor.Instance;

                // End rendering
                Editor.Log("Ending scene animation rendering");
                FlaxEngine.Scripting.Update -= Tick;
                Time.SetFixedDeltaTime(false, 0.0f);
                Time.GamePaused = _wasGamePaused;
                Level.TickEnabled = _wasTickEnabled;
                if (_player)
                {
                    _player.Stop();
                    _player.Parent = null;
                    Object.Destroy(ref _player);
                }
                _window.Editor.StateMachine.CurrentState.UpdateFPS();
                for (int i = 0; i < _stagingTextures.Length; i++)
                {
                    var texture = _stagingTextures[i].Texture;
                    if (texture)
                    {
                        texture.ReleaseGPU();
                        Object.Destroy(ref texture);
                    }
                }
                if (_progress != null)
                {
                    _progress.End();
                    editor.ProgressReporting.UnregisterHandler(_progress);
                }
                if (_editorState != null)
                {
                    editor.StateMachine.GoToState(editor.StateMachine.EditingSceneState);
                    editor.StateMachine.RemoveState(_editorState);
                }

                // Update UI
                var gameWin = editor.Windows.GameWin;
                gameWin.Viewport.CustomResolution = null;
                gameWin.Viewport.BackgroundColor = Color.Transparent;
                gameWin.Viewport.KeepAspectRatio = false;
                gameWin.Viewport.Task.PostRender -= OnPostRender;
                _gameWindow = null;
                _isRendering = false;
                _presenter.Panel.Enabled = true;
                _presenter.BuildLayoutOnUpdate();
            }

            private void Tick()
            {
                switch (_state)
                {
                case States.Warmup:
                    _warmUpTimeLeft -= _dt;
                    if (_warmUpTimeLeft <= 0.0f)
                    {
                        // Render first frame
                        _player.Play();
                        if (!_player.IsPlaying)
                        {
                            Editor.LogError("Scene Animation Player failed to start playing.");
                            CancelRendering();
                        }
                        _warmUpTimeLeft = -1;
                        _state = States.Render;
                    }
                    break;
                case States.Update:
                    // Update scene animation with a fixed delta-time
                    if (!_player.IsStopped)
                    {
                        var speed = _player.Speed * (_window?._timeline.Player?.Speed ?? 1.0f) * _options.PlaybackSpeed;
                        if (speed <= 0.001f)
                        {
                            Editor.LogError("Scene Animation Player speed was nearly zero. Cannot continue rendering.");
                            CancelRendering();
                            break;
                        }
                        var dt = _dt * speed;
                        _player.Tick(dt);
                        _animationFrame++;
                        Editor.Log("Tick anim by " + dt + " to frame " + _animationFrame + " into time " + _player.Time);
                        _progress.Update((_player.Time - _options.StartTime) / (_options.EndTime - _options.StartTime), _animationFrame);
                    }
                    if (_player.IsStopped || _player.Time >= _options.EndTime)
                    {
                        // End rendering but perform remaining copies of the staging textures so all data is captured (from GPU to CPU)
                        _state = States.Staging;
                        break;
                    }

                    // Now wait for the next frame to be rendered
                    _state = States.Render;
                    break;
                }
            }

            private void OnPostRender(GPUContext context, ref RenderContext renderContext)
            {
                var task = _gameWindow.Viewport.Task;
                var taskFrame = task.FrameCount;

                // Check all staging textures for finished GPU to CPU transfers
                Array.Sort(_stagingTextures, (x, y) => x.AnimationFrame.CompareTo(y.AnimationFrame));
                for (int i = 1; i < _stagingTextures.Length; i++)
                {
                    ref var stagingTexture = ref _stagingTextures[i];
                    if (stagingTexture.TaskFrame != -1 && taskFrame - stagingTexture.TaskFrame >= FrameLatency)
                    {
                        _options.VideoOutputFormat.CollectFrame(context, ref stagingTexture, _options);
                        stagingTexture.AnimationFrame = -1;
                        stagingTexture.TaskFrame = -1;
                    }
                }

                switch (_state)
                {
                case States.Render:
                    // Find the first unused staging texture
                    int textureIdx = -1;
                    for (int i = 1; i < _stagingTextures.Length; i++)
                    {
                        if (_stagingTextures[i].AnimationFrame == -1)
                        {
                            textureIdx = i;
                            break;
                        }
                    }
                    if (textureIdx == -1)
                        throw new Exception("Failed to get unused staging texture buffer.");

                    // Copy the backbuffer (GPU) to the staging (CPU)
                    ref var stagingTexture = ref _stagingTextures[textureIdx];
                    stagingTexture.AnimationFrame = _animationFrame;
                    stagingTexture.TaskFrame = taskFrame;
                    _options.VideoOutputFormat.RenderFrame(context, ref stagingTexture, _options, _gameWindow.Viewport.Task.Output);

                    // Now wait for the next animation frame to be updated
                    _state = States.Update;
                    break;
                case States.Staging:
                    // Check if all staging textures has been synchronized
                    if (_stagingTextures.All(x => x.AnimationFrame == -1))
                    {
                        CancelRendering();
                    }
                    break;
                }
            }

            public override void OnDestroy()
            {
                CancelRendering();
                _window.Timeline.Enabled = true;
                _window.Timeline.Visible = true;
                _window._toolstrip.Enabled = true;
                _window._popup = null;
                _window = null;
                _presenter = null;
                _options = null;

                base.OnDestroy();
            }
        }

        private SceneAnimationTimeline _timeline;
        private ToolStripButton _saveButton;
        private ToolStripButton _undoButton;
        private ToolStripButton _redoButton;
        private ToolStripButton _previewButton;
        private ToolStripButton _renderButton;
        private FlaxObjectRefPickerControl _previewPlayerPicker;
        private SceneAnimationPlayer _previewPlayer;
        private Undo _undo;
        private bool _tmpSceneAnimationIsDirty;
        private bool _isWaitingForTimelineLoad;
        private Guid _cachedPlayerId;
        private RenderPopup _popup;

        /// <summary>
        /// Gets the timeline editor.
        /// </summary>
        public SceneAnimationTimeline Timeline => _timeline;

        /// <summary>
        /// Gets the undo history context for this window.
        /// </summary>
        public Undo Undo => _undo;

        /// <inheritdoc />
        public SceneAnimationWindow(Editor editor, AssetItem item)
        : base(editor, item)
        {
            var inputOptions = Editor.Options.Options.Input;

            // Undo
            _undo = new Undo();
            _undo.UndoDone += OnUndoRedo;
            _undo.RedoDone += OnUndoRedo;
            _undo.ActionDone += OnUndoRedo;

            Level.ActorDeleted += OnActorDeleted;

            // Timeline
            _timeline = new SceneAnimationTimeline(_undo)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = new Margin(0, 0, _toolstrip.Bottom, 0),
                Parent = this,
                CanPlayPause = Editor.IsPlayMode,
                CanPlayStop = Editor.IsPlayMode,
                Enabled = false,
            };
            _timeline.Modified += OnTimelineModified;
            _timeline.PlayerChanged += OnTimelinePlayerChanged;
            _timeline.SetNoTracksText("Loading...");

            // Toolstrip
            _saveButton = _toolstrip.AddButton(Editor.Icons.Save64, Save).LinkTooltip("Save", ref inputOptions.Save);
            _toolstrip.AddSeparator();
            _undoButton = _toolstrip.AddButton(Editor.Icons.Undo64, _undo.PerformUndo).LinkTooltip("Undo", ref inputOptions.Undo);
            _redoButton = _toolstrip.AddButton(Editor.Icons.Redo64, _undo.PerformRedo).LinkTooltip("Redo", ref inputOptions.Redo);
            _toolstrip.AddSeparator();
            _previewButton = (ToolStripButton)_toolstrip.AddButton(Editor.Icons.Refresh64, OnPreviewButtonClicked).SetAutoCheck(true).LinkTooltip("If checked, enables live-preview of the animation on a scene while editing");
            _renderButton = (ToolStripButton)_toolstrip.AddButton(Editor.Icons.Build64, OnRenderButtonClicked).LinkTooltip("Open the scene animation rendering utility...");
            _toolstrip.AddSeparator();
            _toolstrip.AddButton(editor.Icons.Docs64, () => Platform.OpenUrl(Utilities.Constants.DocsUrl + "manual/animation/scene-animations/index.html")).LinkTooltip("See documentation to learn more");

            // Preview player picker
            var previewPlayerPickerContainer = new ContainerControl();
            var previewPlayerPickerLabel = new Label
            {
                AnchorPreset = AnchorPresets.VerticalStretchLeft,
                VerticalAlignment = TextAlignment.Center,
                HorizontalAlignment = TextAlignment.Far,
                Parent = previewPlayerPickerContainer,
                Size = new Float2(60.0f, _toolstrip.Height),
                Text = "Player:",
                TooltipText = "The current scene animation player actor to preview. Pick the player to debug it's playback.",
            };
            _previewPlayerPicker = new FlaxObjectRefPickerControl
            {
                Location = new Float2(previewPlayerPickerLabel.Right + 4.0f, 8.0f),
                Width = 140.0f,
                Type = new ScriptType(typeof(SceneAnimationPlayer)),
                Parent = previewPlayerPickerContainer,
            };
            previewPlayerPickerContainer.Width = _previewPlayerPicker.Right + 2.0f;
            previewPlayerPickerContainer.Size = new Float2(_previewPlayerPicker.Right + 2.0f, _toolstrip.Height);
            previewPlayerPickerContainer.Parent = _toolstrip;
            _previewPlayerPicker.CheckValid = OnCheckValid;
            _previewPlayerPicker.ValueChanged += OnPreviewPlayerPickerChanged;

            // Setup input actions
            InputActions.Add(options => options.Undo, _undo.PerformUndo);
            InputActions.Add(options => options.Redo, _undo.PerformRedo);
        }

        private void OnUndoRedo(IUndoAction action)
        {
            MarkAsEdited();
            UpdateToolstrip();
        }

        private void OnPreviewButtonClicked()
        {
            if (_previewButton.Checked)
            {
                // Use utility player actor for live-preview
                if (!_previewPlayer)
                {
                    _previewPlayer = new SceneAnimationPlayer
                    {
                        Animation = Asset,
                        HideFlags = HideFlags.FullyHidden,
                        RestoreStateOnStop = true,
                        PlayOnStart = false,
                        RandomStartTime = false,
                        UseTimeScale = false,
                        UpdateMode = SceneAnimationPlayer.UpdateModes.Manual,
                    };
                    Level.SpawnActor(_previewPlayer);

                    // Live-preview player is in pause or play mode (stopped on the usage end)
                    _previewPlayer.Play();
                    _previewPlayer.Pause();
                }
                var time = _timeline.CurrentTime;
                _timeline.Player = _previewPlayer;
                _previewPlayerPicker.Value = null;
                _cachedPlayerId = Guid.Empty;
                _previewPlayer.Time = time;
            }
            else
            {
                if (_timeline.Player == _previewPlayer)
                {
                    _timeline.Player = null;
                    _previewPlayerPicker.Value = null;
                    _cachedPlayerId = Guid.Empty;
                }
                if (_previewPlayer)
                {
                    _previewPlayer.Stop();
                    Object.Destroy(_previewPlayer);
                    _previewPlayer = null;
                }
            }
            _previewPlayerPicker.Parent.Visible = !_previewButton.Checked;
            _timeline.CanPlayPause = _previewButton.Checked || Editor.IsPlayMode;
        }

        private void OnRenderButtonClicked()
        {
            if (_popup != null)
                return;
            _popup = new RenderPopup(this);
        }

        private bool OnCheckValid(Object obj, ScriptType type)
        {
            return obj is SceneAnimationPlayer player && player.Animation == OriginalAsset;
        }

        /// <inheritdoc />
        public override void OnSceneUnloading(Scene scene, Guid sceneId)
        {
            base.OnSceneUnloading(scene, sceneId);

            if (scene == _timeline.Player?.Scene)
            {
                var id = _timeline.Player.ID;
                _timeline.Player = null;
                _cachedPlayerId = id;
            }
        }

        private void OnActorDeleted(Actor actor)
        {
            if (actor == _timeline.Player)
            {
                var id = actor.ID;
                _timeline.Player = null;
                _cachedPlayerId = id;
            }
            if (actor == _previewPlayer)
            {
                _previewPlayer = null;
                if (_previewButton.Checked)
                {
                    _previewButton.Checked = false;
                    OnPreviewButtonClicked();
                }
            }
        }

        private void OnTimelinePlayerChanged()
        {
            if (_previewButton.Checked)
                return;
            _previewPlayerPicker.Value = _timeline.Player != null ? _timeline.Player : null;
            _cachedPlayerId = _timeline.Player?.ID ?? Guid.Empty;
        }

        private void OnPreviewPlayerPickerChanged()
        {
            if (_previewButton.Checked)
                return;
            _timeline.Player = _previewPlayerPicker.Value as SceneAnimationPlayer;
        }

        private void OnTimelineModified()
        {
            _tmpSceneAnimationIsDirty = true;
            MarkAsEdited();
        }

        /// <summary>
        /// Refreshes temporary asset to see changes live when editing the timeline.
        /// </summary>
        /// <returns>True if cannot refresh it, otherwise false.</returns>
        public bool RefreshTempAsset()
        {
            if (_asset == null || _isWaitingForTimelineLoad)
                return true;

            if (_timeline.IsModified)
            {
                var time = _timeline.CurrentTime;
                var isPlaying = _previewPlayer != null ? _previewPlayer.IsPlaying : false;
                _timeline.Save(_asset);
                if (_previewButton.Checked && _previewPlayer != null)
                {
                    // Preserve playback time in live-preview mode when editing the asset
                    _asset.WaitForLoaded();
                    _previewPlayer.Play();
                    if (!isPlaying)
                        _previewPlayer.Pause();
                    _previewPlayer.Time = time;
                    _timeline.CurrentTime = time;
                }
            }

            return false;
        }

        /// <inheritdoc />
        public override void Save()
        {
            if (!IsEdited)
                return;

            if (RefreshTempAsset())
                return;
            if (SaveToOriginal())
                return;

            ClearEditedFlag();
            _item.RefreshThumbnail();
        }

        /// <inheritdoc />
        protected override void UpdateToolstrip()
        {
            _saveButton.Enabled = IsEdited;
            _undoButton.Enabled = _undo.CanUndo;
            _redoButton.Enabled = _undo.CanRedo;
            _previewButton.Enabled = Level.IsAnySceneLoaded && Editor.StateMachine.IsEditMode;
            _renderButton.Enabled = Level.IsAnySceneLoaded && (Editor.IsPlayMode || Editor.StateMachine.IsEditMode);

            base.UpdateToolstrip();
        }

        /// <inheritdoc />
        protected override void UnlinkItem()
        {
            _isWaitingForTimelineLoad = false;

            base.UnlinkItem();
        }

        /// <inheritdoc />
        protected override void OnAssetLinked()
        {
            _isWaitingForTimelineLoad = true;

            base.OnAssetLinked();
        }

        /// <inheritdoc />
        public override void OnSceneLoaded(Scene scene, Guid sceneId)
        {
            base.OnSceneLoaded(scene, sceneId);

            UpdateToolstrip();
        }

        /// <inheritdoc />
        public override void OnSceneUnloaded(Scene scene, Guid sceneId)
        {
            base.OnSceneUnloaded(scene, sceneId);

            UpdateToolstrip();
        }

        /// <inheritdoc />
        public override void OnPlayBegin()
        {
            base.OnPlayBegin();

            UpdateToolstrip();
            _timeline.CanPlayPause = true;
            _timeline.CanPlayStop = true;
        }

        /// <inheritdoc />
        public override void OnPlayEnd()
        {
            base.OnPlayEnd();

            UpdateToolstrip();
            _timeline.CanPlayPause = _previewButton.Checked;
            _timeline.CanPlayStop = false;
        }

        /// <inheritdoc />
        public override void OnEditorStateChanged()
        {
            base.OnEditorStateChanged();

            UpdateToolstrip();
        }

        /// <inheritdoc />
        public override void Update(float deltaTime)
        {
            base.Update(deltaTime);

            // Check if temporary asset need to be updated
            if (_tmpSceneAnimationIsDirty)
            {
                _tmpSceneAnimationIsDirty = false;
                RefreshTempAsset();
            }

            // Check if need to load timeline
            if (_isWaitingForTimelineLoad && _asset.IsLoaded)
            {
                _isWaitingForTimelineLoad = false;
                _timeline._id = OriginalAsset.ID;
                _timeline.Load(_asset);
                _undo.Clear();
                _timeline.SetNoTracksText(null);
                _timeline.Enabled = true;
                ClearEditedFlag();
            }

            // Try to reassign the player
            if (_timeline.Player == null && _cachedPlayerId != Guid.Empty)
            {
                var obj = Object.TryFind<SceneAnimationPlayer>(ref _cachedPlayerId);
                if (obj)
                {
                    _cachedPlayerId = Guid.Empty;
                    if (obj.Animation == OriginalAsset)
                        _timeline.Player = obj;
                }
            }

            // Manually tick the live-preview animation player
            if (_previewButton.Checked && _previewPlayer != null)
            {
                if (_previewPlayer.IsPlaying)
                {
                    // Preview is playing
                    _previewPlayer.Tick(Time.UnscaledDeltaTime);
                }
                else if (Mathf.NearEqual(_previewPlayer.Time, _timeline.CurrentFrame))
                {
                    // Preview is paused
                    _previewPlayer.Time = _timeline.CurrentTime;
                }
                else
                {
                    // User is seeking
                    _previewPlayer.Time = _timeline.CurrentTime;
                    _previewPlayer.Tick(0.0f);
                }
            }
        }

        /// <inheritdoc />
        public override bool UseLayoutData => true;

        /// <inheritdoc />
        public override void OnLayoutSerialize(XmlWriter writer)
        {
            LayoutSerializeSplitter(writer, "TimelineSplitter", _timeline.Splitter);
            writer.WriteAttributeString("TimeShowMode", _timeline.TimeShowMode.ToString());
            var id = _previewButton.Checked ? Guid.Empty : (_timeline.Player?.ID ?? _cachedPlayerId);
            writer.WriteAttributeString("SelectedPlayer", id.ToString());
            writer.WriteAttributeString("ShowPreviewValues", _timeline.ShowPreviewValues.ToString());
            writer.WriteAttributeString("ShowSelected3dTrack", _timeline.ShowSelected3dTrack.ToString());
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize(XmlElement node)
        {
            LayoutDeserializeSplitter(node, "TimelineSplitter", _timeline.Splitter);
            if (Guid.TryParse(node.GetAttribute("SelectedPlayer"), out Guid value2))
                _cachedPlayerId = value2;
            if (Enum.TryParse(node.GetAttribute("TimeShowMode"), out Timeline.TimeShowModes value3))
                _timeline.TimeShowMode = value3;
            if (bool.TryParse(node.GetAttribute("ShowPreviewValues"), out bool value4))
                _timeline.ShowPreviewValues = value4;
            if (bool.TryParse(node.GetAttribute("ShowSelected3dTrack"), out value4))
                _timeline.ShowSelected3dTrack = value4;
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize()
        {
            _timeline.Splitter.SplitterValue = 0.2f;
        }

        /// <inheritdoc />
        public override void OnDestroy()
        {
            Level.ActorDeleted -= OnActorDeleted;

            if (_previewButton.Checked)
            {
                _previewButton.Checked = false;
                OnPreviewButtonClicked();
            }
            if (_popup != null)
            {
                _popup.Dispose();
                _popup = null;
            }

            if (_undo != null)
            {
                _undo.Enabled = false;
                _undo.Clear();
                _undo = null;
            }

            _timeline = null;
            _saveButton = null;
            _undoButton = null;
            _redoButton = null;
            _renderButton = null;
            _previewPlayerPicker = null;

            base.OnDestroy();
        }
    }
}
