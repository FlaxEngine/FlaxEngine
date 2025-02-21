// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Xml;
using FlaxEditor.Gizmo;
using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Input;
using FlaxEditor.Options;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Json;

namespace FlaxEditor.Windows
{
    /// <summary>
    /// Provides in-editor play mode simulation.
    /// </summary>
    /// <seealso cref="FlaxEditor.Windows.EditorWindow" />
    public class GameWindow : EditorWindow
    {
        private readonly RenderOutputControl _viewport;
        private readonly GameRoot _guiRoot;
        private bool _showGUI = true;
        private bool _showDebugDraw = false;
        private bool _audioMuted = false;
        private float _audioVolume = 1;
        private bool _isMaximized = false, _isUnlockingMouse = false;
        private bool _isFloating = false, _isBorderless = false;
        private bool _cursorVisible = true;
        private float _gameStartTime;
        private GUI.Docking.DockState _maximizeRestoreDockState;
        private GUI.Docking.DockPanel _maximizeRestoreDockTo;
        private CursorLockMode _cursorLockMode = CursorLockMode.None;

        // Viewport scaling variables
        private List<ViewportScaleOptions> _defaultViewportScaling = new List<ViewportScaleOptions>();
        private List<ViewportScaleOptions> _customViewportScaling = new List<ViewportScaleOptions>();
        private float _viewportAspectRatio = 1;
        private float _windowAspectRatio = 1;
        private bool _useAspect = false;
        private bool _freeAspect = true;

        private List<PlayModeFocusOptions> _focusOptions = new List<PlayModeFocusOptions>()
        {
            new PlayModeFocusOptions
            {
                Name = "None",
                Tooltip = "Don't change focus.",
                FocusOption = InterfaceOptions.PlayModeFocus.None,
            },
            new PlayModeFocusOptions
            {
                Name = "Game Window",
                Tooltip = "Focus the Game Window.",
                FocusOption = InterfaceOptions.PlayModeFocus.GameWindow,
            },
            new PlayModeFocusOptions
            {
                Name = "Game Window Then Restore",
                Tooltip = "Focus the Game Window. On play mode end restore focus to the previous window.",
                FocusOption = InterfaceOptions.PlayModeFocus.GameWindowThenRestore,
            },
        };

        /// <summary>
        /// Gets the viewport.
        /// </summary>
        public RenderOutputControl Viewport => _viewport;

        /// <summary>
        /// Gets or sets a value indicating whether show game GUI in the view or keep it hidden.
        /// </summary>
        public bool ShowGUI
        {
            get => _showGUI;
            set
            {
                if (value != _showGUI)
                {
                    _showGUI = value;
                    _guiRoot.Visible = value;
                }
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether show Debug Draw shapes in the view or keep it hidden.
        /// </summary>
        public bool ShowDebugDraw
        {
            get => _showDebugDraw;
            set => _showDebugDraw = value;
        }


        /// <summary>
        /// Gets or set a value indicating whether Audio is muted.
        /// </summary>
        public bool AudioMuted
        {
            get => _audioMuted;
            set
            {
                Audio.MasterVolume = value ? 0 : AudioVolume;
                _audioMuted = value;
            }
        }

        /// <summary>
        /// Gets or sets a value that set the audio volume.
        /// </summary>
        public float AudioVolume
        {
            get => _audioVolume;
            set
            {
                if (!AudioMuted)
                    Audio.MasterVolume = value;

                _audioVolume = value;
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether the game window is maximized (only in play mode).
        /// </summary>
        private bool IsMaximized
        {
            get => _isMaximized;
            set
            {
                if (_isMaximized == value)
                    return;
                _isMaximized = value;
                if (value)
                {
                    IsFloating = true;
                    var rootWindow = RootWindow;
                    rootWindow.Maximize();
                }
                else
                {
                    IsFloating = false;
                }
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether the game window is floating (popup, only in play mode).
        /// </summary>
        private bool IsFloating
        {
            get => _isFloating;
            set
            {
                if (_isFloating == value)
                    return;
                _isFloating = value;
                var rootWindow = RootWindow;
                if (value)
                {
                    _maximizeRestoreDockTo = _dockedTo;
                    _maximizeRestoreDockState = _dockedTo.TryGetDockState(out _);
                    if (_maximizeRestoreDockState != GUI.Docking.DockState.Float)
                    {
                        var monitorBounds = Platform.GetMonitorBounds(PointToScreen(Size * 0.5f));
                        var size = DefaultSize;
                        var location = monitorBounds.Location + monitorBounds.Size * 0.5f - size * 0.5f;
                        ShowFloating(location, size, WindowStartPosition.Manual);
                    }
                }
                else
                {
                    // Restore
                    if (rootWindow != null)
                        rootWindow.Restore();
                    if (_maximizeRestoreDockTo != null && _maximizeRestoreDockTo.IsDisposing)
                        _maximizeRestoreDockTo = null;
                    Show(_maximizeRestoreDockState, _maximizeRestoreDockTo);
                }
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether the game window is borderless (only in play mode).
        /// </summary>
        private bool IsBorderless
        {
            get => _isBorderless;
            set
            {
                if (_isBorderless == value)
                    return;
                _isBorderless = value;
                if (value)
                {
                    IsFloating = true;
                    var rootWindow = RootWindow;
                    var monitorBounds = Platform.GetMonitorBounds(rootWindow.RootWindow.Window.ClientPosition);
                    rootWindow.Window.Position = monitorBounds.Location;
                    rootWindow.Window.SetBorderless(true);
                    rootWindow.Window.ClientSize = monitorBounds.Size;
                }
                else
                {
                    IsFloating = false;
                }
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether center mouse position on window focus in play mode. Helps when working with games that lock mouse cursor.
        /// </summary>
        public bool CenterMouseOnFocus { get; set; }

        /// <summary>
        /// Gets or sets a value indicating what panel should be focused when play mode start.
        /// </summary>
        public InterfaceOptions.PlayModeFocus FocusOnPlayOption { get; set; }

        private enum ViewportScaleType
        {
            Resolution = 0,
            Aspect = 1,
        }

        private class ViewportScaleOptions
        {
            /// <summary>
            /// The name.
            /// </summary>
            public string Label;

            /// <summary>
            /// The Type of scaling to do.
            /// </summary>
            public ViewportScaleType ScaleType;

            /// <summary>
            /// The width and height to scale by.
            /// </summary>
            public Int2 Size;

            /// <summary>
            /// If the scaling is active.
            /// </summary>
            public bool Active;
        }

        private class PlayModeFocusOptions
        {
            /// <summary>
            /// The name.
            /// </summary>
            public string Name;

            /// <summary>
            /// The tooltip.
            /// </summary>
            public string Tooltip;

            /// <summary>
            /// The type of focus.
            /// </summary>
            public InterfaceOptions.PlayModeFocus FocusOption;

            /// <summary>
            /// If the option is active.
            /// </summary>
            public bool Active;
        }

        /// <summary>
        /// Root control for game UI preview in Editor. Supports basic UI editing via <see cref="UIEditorRoot"/>.
        /// </summary>
        private class GameRoot : UIEditorRoot
        {
            public override bool EnableInputs => !Time.GamePaused && Editor.IsPlayMode;
            public override bool EnableSelecting => !Editor.IsPlayMode || Time.GamePaused;
            public override TransformGizmo TransformGizmo => Editor.Instance.MainTransformGizmo;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="GameWindow"/> class.
        /// </summary>
        /// <param name="editor">The editor.</param>
        public GameWindow(Editor editor)
        : base(editor, true, ScrollBars.None)
        {
            Title = "Game";
            AutoFocus = true;

            var task = MainRenderTask.Instance;

            // Setup viewport
            _viewport = new RenderOutputControl(task)
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                AutoFocus = false,
                Parent = this
            };
            task.PostRender += OnPostRender;

            // Override the game GUI root
            _guiRoot = new GameRoot
            {
                Parent = _viewport
            };
            RootControl.GameRoot = _guiRoot.UIRoot;

            SizeChanged += control => { ResizeViewport(); };

            Editor.StateMachine.PlayingState.SceneDuplicating += PlayingStateOnSceneDuplicating;
            Editor.StateMachine.PlayingState.SceneRestored += PlayingStateOnSceneRestored;

            // Link editor options
            Editor.Options.OptionsChanged += OnOptionsChanged;
            OnOptionsChanged(Editor.Options.Options);

            InputActions.Add(options => options.TakeScreenshot, () => Screenshot.Capture(string.Empty));
            InputActions.Add(options => options.DebuggerUnlockMouse, UnlockMouseInPlay);
            InputActions.Add(options => options.ToggleFullscreen, () => { if (Editor.IsPlayMode) IsMaximized = !IsMaximized; });
            InputActions.Add(options => options.Play, Editor.Instance.Simulation.DelegatePlayOrStopPlayInEditor);
            InputActions.Add(options => options.Pause, Editor.Instance.Simulation.RequestResumeOrPause);
            InputActions.Add(options => options.StepFrame, Editor.Instance.Simulation.RequestPlayOneFrame);
            InputActions.Add(options => options.ProfilerStartStop, () =>
            {
                bool recording = !Editor.Instance.Windows.ProfilerWin.LiveRecording;
                Editor.Instance.Windows.ProfilerWin.LiveRecording = recording;
                Editor.Instance.UI.AddStatusMessage($"Profiling {(recording ? "started" : "stopped")}.");
            });
            InputActions.Add(options => options.ProfilerClear, () =>
            {
                Editor.Instance.Windows.ProfilerWin.Clear();
                Editor.Instance.UI.AddStatusMessage($"Profiling results cleared.");
            });
            InputActions.Add(options => options.Save, () =>
            {
                if (Editor.IsPlayMode)
                    return;
                Editor.Instance.SaveAll();
            });
            InputActions.Add(options => options.Undo, () =>
            {
                if (Editor.IsPlayMode)
                    return;
                Editor.Instance.PerformUndo();
                Focus();
            });
            InputActions.Add(options => options.Redo, () =>
            {
                if (Editor.IsPlayMode)
                    return;
                Editor.Instance.PerformRedo();
                Focus();
            });
            InputActions.Add(options => options.Cut, () =>
            {
                if (Editor.IsPlayMode)
                    return;
                Editor.Instance.SceneEditing.Cut();
            });
            InputActions.Add(options => options.Copy, () =>
            {
                if (Editor.IsPlayMode)
                    return;
                Editor.Instance.SceneEditing.Copy();
            });
            InputActions.Add(options => options.Paste, () =>
            {
                if (Editor.IsPlayMode)
                    return;
                Editor.Instance.SceneEditing.Paste();
            });
            InputActions.Add(options => options.Duplicate, () =>
            {
                if (Editor.IsPlayMode)
                    return;
                Editor.Instance.SceneEditing.Duplicate();
            });
            InputActions.Add(options => options.Delete, () =>
            {
                if (Editor.IsPlayMode)
                    return;
                Editor.Instance.SceneEditing.Delete();
            });
        }

        private void ChangeViewportRatio(ViewportScaleOptions v)
        {
            if (v == null)
                return;

            if (v.Size.Y <= 0 || v.Size.X <= 0)
            {
                return;
            }

            if (string.Equals(v.Label, "Free Aspect", StringComparison.Ordinal) && v.Size == new Int2(1, 1))
            {
                _freeAspect = true;
                _useAspect = true;
            }
            else
            {
                switch (v.ScaleType)
                {
                case ViewportScaleType.Aspect:
                    _useAspect = true;
                    _freeAspect = false;
                    break;
                case ViewportScaleType.Resolution:
                    _useAspect = false;
                    _freeAspect = false;
                    break;
                }
            }

            _viewportAspectRatio = (float)v.Size.X / v.Size.Y;

            if (!_freeAspect)
            {
                if (!_useAspect)
                {
                    _viewport.KeepAspectRatio = true;
                    _viewport.CustomResolution = new Int2(v.Size.X, v.Size.Y);
                }
                else
                {
                    _viewport.CustomResolution = new Int2?();
                    _viewport.KeepAspectRatio = true;
                }
            }
            else
            {
                _viewport.CustomResolution = new Int2?();
                _viewport.KeepAspectRatio = false;
            }

            ResizeViewport();
        }

        private void ResizeViewport()
        {
            if (!_freeAspect)
            {
                _windowAspectRatio = Width / Height;
            }
            else
            {
                _windowAspectRatio = 1;
            }

            var scaleWidth = _viewportAspectRatio / _windowAspectRatio;
            var scaleHeight = _windowAspectRatio / _viewportAspectRatio;

            if (scaleHeight < 1)
            {
                _viewport.Bounds = new Rectangle(0, Height * (1 - scaleHeight) / 2, Width, Height * scaleHeight);
            }
            else
            {
                _viewport.Bounds = new Rectangle(Width * (1 - scaleWidth) / 2, 0, Width * scaleWidth, Height);
            }
            _viewport.SyncBackbufferSize();
            PerformLayout();
        }

        private void OnPostRender(GPUContext context, ref RenderContext renderContext)
        {
            // Debug Draw shapes
            if (_showDebugDraw)
            {
                var task = _viewport.Task;

                // Draw actors debug shapes manually if editor viewport is hidden (game viewport task is always rendered before editor viewports)
                var editWindowViewport = Editor.Windows.EditWin.Viewport;
                if (editWindowViewport.Task.LastUsedFrame != Engine.FrameCount)
                {
                    var drawDebugData = editWindowViewport.DebugDrawData;
                    drawDebugData.Clear();
                    var selectedParents = editWindowViewport.TransformGizmo.SelectedParents;
                    if (selectedParents.Count > 0)
                    {
                        for (int i = 0; i < selectedParents.Count; i++)
                        {
                            if (selectedParents[i].IsActiveInHierarchy)
                                selectedParents[i].OnDebugDraw(drawDebugData);
                        }
                    }
                    unsafe
                    {
                        fixed (IntPtr* actors = drawDebugData.ActorsPtrs)
                        {
                            DebugDraw.DrawActors(new IntPtr(actors), drawDebugData.ActorsCount, true);
                        }
                    }
                }

                DebugDraw.Draw(ref renderContext, task.OutputView);
            }
        }

        private void OnOptionsChanged(EditorOptions options)
        {
            CenterMouseOnFocus = options.Interface.CenterMouseOnGameWinFocus;
            FocusOnPlayOption = options.Interface.FocusOnPlayMode;
        }

        private void PlayingStateOnSceneDuplicating()
        {
            // Remove remaining GUI controls so loaded scene can add own GUI
            //_guiRoot.DisposeChildren();

            // Show GUI
            //_guiRoot.Visible = _showGUI;
        }

        private void PlayingStateOnSceneRestored()
        {
            // Remove remaining GUI controls so loaded scene can add own GUI
            //_guiRoot.DisposeChildren();

            // Hide GUI
            //_guiRoot.Visible = false;
        }

        /// <inheritdoc />
        protected override bool CanOpenContentFinder => !Editor.IsPlayMode;

        /// <inheritdoc />
        protected override bool CanUseNavigation => false;

        /// <inheritdoc />
        public override void OnPlayBegin()
        {
            _gameStartTime = Time.UnscaledGameTime;
        }

        /// <inheritdoc />
        public override void OnPlayEnd()
        {
            IsFloating = false;
            IsMaximized = false;
            IsBorderless = false;
            Cursor = CursorType.Default;
            Screen.CursorLock = CursorLockMode.None;
            if (Screen.MainWindow.IsMouseTracking)
                Screen.MainWindow.EndTrackingMouse();
            RootControl.GameRoot.EndMouseCapture();
        }

        /// <inheritdoc />
        public override void OnMouseLeave()
        {
            base.OnMouseLeave();

            // Remove focus from game window when mouse moves out and the cursor is hidden during game
            if (ContainsFocus && Parent != null && Editor.IsPlayMode && !Screen.CursorVisible && Screen.CursorLock == CursorLockMode.None)
            {
                Parent.Focus();
            }
        }

        /// <inheritdoc />
        public override void OnShowContextMenu(ContextMenu menu)
        {
            base.OnShowContextMenu(menu);

            // Focus on play
            {
                var pfMenu = menu.AddChildMenu("Focus On Play Override").ContextMenu;

                GenerateFocusOptionsContextMenu(pfMenu);

                pfMenu.AddSeparator();

                var button = pfMenu.AddButton("Remove override");
                button.TooltipText = "Reset the override to the value set in the editor options.";
                button.Clicked += () => FocusOnPlayOption = Editor.Instance.Options.Options.Interface.FocusOnPlayMode;
            }

            menu.AddSeparator();

            // Viewport Brightness
            {
                var brightness = menu.AddButton("Viewport Brightness");
                brightness.CloseMenuOnClick = false;
                var brightnessValue = new FloatValueBox(_viewport.Brightness, 140, 2, 50.0f, 0.001f, 10.0f, 0.001f)
                {
                    Parent = brightness
                };
                brightnessValue.ValueChanged += () => _viewport.Brightness = brightnessValue.Value;
            }

            // Viewport Resolution
            {
                var resolution = menu.AddButton("Viewport Resolution");
                resolution.CloseMenuOnClick = false;
                var resolutionValue = new FloatValueBox(_viewport.ResolutionScale, 140, 2, 50.0f, 0.1f, 4.0f, 0.001f)
                {
                    Parent = resolution
                };
                resolutionValue.ValueChanged += () => _viewport.ResolutionScale = resolutionValue.Value;
            }

            // Viewport aspect ratio
            {
                // Create default scaling options if they dont exist from deserialization.
                if (_defaultViewportScaling.Count == 0)
                {
                    _defaultViewportScaling.Add(new ViewportScaleOptions
                    {
                        Label = "Free Aspect",
                        ScaleType = ViewportScaleType.Aspect,
                        Size = new Int2(1, 1),
                        Active = true,
                    });
                    _defaultViewportScaling.Add(new ViewportScaleOptions
                    {
                        Label = "16:9 Aspect",
                        ScaleType = ViewportScaleType.Aspect,
                        Size = new Int2(16, 9),
                        Active = false,
                    });
                    _defaultViewportScaling.Add(new ViewportScaleOptions
                    {
                        Label = "16:10 Aspect",
                        ScaleType = ViewportScaleType.Aspect,
                        Size = new Int2(16, 10),
                        Active = false,
                    });
                    _defaultViewportScaling.Add(new ViewportScaleOptions
                    {
                        Label = "1920x1080 Resolution (Full HD)",
                        ScaleType = ViewportScaleType.Resolution,
                        Size = new Int2(1920, 1080),
                        Active = false,
                    });
                    _defaultViewportScaling.Add(new ViewportScaleOptions
                    {
                        Label = "2560x1440 Resolution (2K)",
                        ScaleType = ViewportScaleType.Resolution,
                        Size = new Int2(2560, 1440),
                        Active = false,
                    });
                }

                var vsMenu = menu.AddChildMenu("Viewport Size").ContextMenu;

                CreateViewportSizingContextMenu(vsMenu);
            }

            // Take Screenshot
            {
                var takeScreenshot = menu.AddButton("Take Screenshot");
                takeScreenshot.Clicked += TakeScreenshot;
            }

            menu.AddSeparator();

            // Show GUI
            {
                var button = menu.AddButton("Show GUI");
                button.CloseMenuOnClick = false;
                var checkbox = new CheckBox(140, 2, ShowGUI) { Parent = button };
                checkbox.StateChanged += x => ShowGUI = x.Checked;
            }

            // Show Debug Draw
            {
                var button = menu.AddButton("Show Debug Draw");
                button.CloseMenuOnClick = false;
                var checkbox = new CheckBox(140, 2, ShowDebugDraw) { Parent = button };
                checkbox.StateChanged += x => ShowDebugDraw = x.Checked;
            }

            menu.AddSeparator();

            // Mute Audio
            {
                var button = menu.AddButton("Mute Audio");
                button.CloseMenuOnClick = false;
                var checkbox = new CheckBox(140, 2, AudioMuted) { Parent = button };
                checkbox.StateChanged += x => AudioMuted = x.Checked;
            }

            // Audio Volume
            {
                var button = menu.AddButton("Audio Volume");
                button.CloseMenuOnClick = false;
                var slider = new FloatValueBox(AudioVolume, 140, 2, 50, 0, 1) { Parent = button };
                slider.ValueChanged += () => AudioVolume = slider.Value;
            }

            menu.MinimumWidth = 200;
            menu.AddSeparator();
        }

        private void GenerateFocusOptionsContextMenu(ContextMenu pfMenu)
        {
            foreach (PlayModeFocusOptions f in _focusOptions)
            {
                f.Active = f.FocusOption == FocusOnPlayOption;

                var button = pfMenu.AddButton(f.Name);
                button.CloseMenuOnClick = false;
                button.Tag = f;
                button.TooltipText = f.Tooltip;
                button.Icon = f.Active ? Style.Current.CheckBoxTick : SpriteHandle.Invalid;
                button.Clicked += () =>
                {
                    foreach (var child in pfMenu.Items)
                    {
                        if (child is ContextMenuButton cmb && cmb.Tag is PlayModeFocusOptions p)
                        {
                            if (cmb == button)
                            {
                                p.Active = true;
                                button.Icon = Style.Current.CheckBoxTick;
                                FocusOnPlayOption = p.FocusOption;
                            }
                            else if (p.Active)
                            {
                                cmb.Icon = SpriteHandle.Invalid;
                                p.Active = false;
                            }
                        }
                    }
                };
            }
        }

        private void CreateViewportSizingContextMenu(ContextMenu vsMenu)
        {
            // Add default viewport sizing options
            for (int i = 0; i < _defaultViewportScaling.Count; i++)
            {
                var viewportScale = _defaultViewportScaling[i];
                var button = vsMenu.AddButton(viewportScale.Label);
                button.CloseMenuOnClick = false;
                button.Icon = viewportScale.Active ? Style.Current.CheckBoxTick : SpriteHandle.Invalid;
                button.Tag = viewportScale;
                if (viewportScale.Active)
                    ChangeViewportRatio(viewportScale);

                button.Clicked += () =>
                {
                    if (button.Tag == null)
                        return;

                    // Reset selected icon on all buttons
                    foreach (var child in vsMenu.Items)
                    {
                        if (child is ContextMenuButton cmb && cmb.Tag is ViewportScaleOptions v)
                        {
                            if (cmb == button)
                            {
                                v.Active = true;
                                button.Icon = Style.Current.CheckBoxTick;
                                ChangeViewportRatio(v);
                            }
                            else if (v.Active)
                            {
                                cmb.Icon = SpriteHandle.Invalid;
                                v.Active = false;
                            }
                        }
                    }
                };
            }
            if (_defaultViewportScaling.Count != 0)
                vsMenu.AddSeparator();

            // Add custom viewport options
            for (int i = 0; i < _customViewportScaling.Count; i++)
            {
                var viewportScale = _customViewportScaling[i];
                var childCM = vsMenu.AddChildMenu(viewportScale.Label);
                childCM.CloseMenuOnClick = false;
                childCM.Icon = viewportScale.Active ? Style.Current.CheckBoxTick : SpriteHandle.Invalid;
                childCM.Tag = viewportScale;
                if (viewportScale.Active)
                    ChangeViewportRatio(viewportScale);

                var applyButton = childCM.ContextMenu.AddButton("Apply");
                applyButton.Tag = childCM.Tag = viewportScale;
                applyButton.CloseMenuOnClick = false;
                applyButton.Clicked += () =>
                {
                    if (childCM.Tag == null)
                        return;

                    // Reset selected icon on all buttons
                    foreach (var child in vsMenu.Items)
                    {
                        if (child is ContextMenuButton cmb && cmb.Tag is ViewportScaleOptions v)
                        {
                            if (child == childCM)
                            {
                                v.Active = true;
                                childCM.Icon = Style.Current.CheckBoxTick;
                                ChangeViewportRatio(v);
                            }
                            else if (v.Active)
                            {
                                cmb.Icon = SpriteHandle.Invalid;
                                v.Active = false;
                            }
                        }
                    }
                };

                var deleteButton = childCM.ContextMenu.AddButton("Delete");
                deleteButton.CloseMenuOnClick = false;
                deleteButton.Clicked += () =>
                {
                    if (childCM.Tag == null)
                        return;

                    var v = (ViewportScaleOptions)childCM.Tag;
                    if (v.Active)
                    {
                        v.Active = false;
                        _defaultViewportScaling[0].Active = true;
                        ChangeViewportRatio(_defaultViewportScaling[0]);
                    }
                    _customViewportScaling.Remove(v);
                    vsMenu.DisposeAllItems();
                    CreateViewportSizingContextMenu(vsMenu);
                    vsMenu.PerformLayout();
                };
            }
            if (_customViewportScaling.Count != 0)
                vsMenu.AddSeparator();

            // Add button
            var add = vsMenu.AddButton("Add...");
            add.CloseMenuOnClick = false;
            add.Clicked += () =>
            {
                var popup = new ContextMenuBase
                {
                    Size = new Float2(230, 125),
                    ClipChildren = false,
                    CullChildren = false,
                };
                popup.Show(add, new Float2(add.Width, 0));

                var nameLabel = new Label
                {
                    Parent = popup,
                    AnchorPreset = AnchorPresets.TopLeft,
                    Text = "Name",
                    HorizontalAlignment = TextAlignment.Near,
                };
                nameLabel.LocalX += 10;
                nameLabel.LocalY += 10;

                var nameTextBox = new TextBox
                {
                    Parent = popup,
                    AnchorPreset = AnchorPresets.TopLeft,
                    IsMultiline = false,
                };
                nameTextBox.LocalX += 100;
                nameTextBox.LocalY += 10;

                var typeLabel = new Label
                {
                    Parent = popup,
                    AnchorPreset = AnchorPresets.TopLeft,
                    Text = "Type",
                    HorizontalAlignment = TextAlignment.Near,
                };
                typeLabel.LocalX += 10;
                typeLabel.LocalY += 35;

                var typeDropdown = new Dropdown
                {
                    Parent = popup,
                    AnchorPreset = AnchorPresets.TopLeft,
                    Items = { "Aspect", "Resolution" },
                    SelectedItem = "Aspect",
                    Width = nameTextBox.Width
                };
                typeDropdown.LocalY += 35;
                typeDropdown.LocalX += 100;

                var whLabel = new Label
                {
                    Parent = popup,
                    AnchorPreset = AnchorPresets.TopLeft,
                    Text = "Width & Height",
                    HorizontalAlignment = TextAlignment.Near,
                };
                whLabel.LocalX += 10;
                whLabel.LocalY += 60;

                var wValue = new IntValueBox(16)
                {
                    Parent = popup,
                    AnchorPreset = AnchorPresets.TopLeft,
                    MinValue = 1,
                    Width = 55,
                };
                wValue.LocalY += 60;
                wValue.LocalX += 100;

                var hValue = new IntValueBox(9)
                {
                    Parent = popup,
                    AnchorPreset = AnchorPresets.TopLeft,
                    MinValue = 1,
                    Width = 55,
                };
                hValue.LocalY += 60;
                hValue.LocalX += 165;

                var submitButton = new Button
                {
                    Parent = popup,
                    AnchorPreset = AnchorPresets.TopLeft,
                    Text = "Submit",
                    Width = 70,
                };
                submitButton.LocalX += 40;
                submitButton.LocalY += 90;

                submitButton.Clicked += () =>
                {
                    Enum.TryParse(typeDropdown.SelectedItem, out ViewportScaleType type);

                    var combineString = type == ViewportScaleType.Aspect ? ":" : "x";
                    var name = nameTextBox.Text + " (" + wValue.Value + combineString + hValue.Value + ") " + typeDropdown.SelectedItem;

                    var newViewportOption = new ViewportScaleOptions
                    {
                        ScaleType = type,
                        Label = name,
                        Size = new Int2(wValue.Value, hValue.Value),
                    };

                    _customViewportScaling.Add(newViewportOption);
                    vsMenu.DisposeAllItems();
                    CreateViewportSizingContextMenu(vsMenu);
                    vsMenu.PerformLayout();
                };

                var cancelButton = new Button
                {
                    Parent = popup,
                    AnchorPreset = AnchorPresets.TopLeft,
                    Text = "Cancel",
                    Width = 70,
                };
                cancelButton.LocalX += 120;
                cancelButton.LocalY += 90;

                cancelButton.Clicked += () =>
                {
                    nameTextBox.Clear();
                    typeDropdown.SelectedItem = "Aspect";
                    hValue.Value = 9;
                    wValue.Value = 16;
                    popup.Hide();
                };
            };
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            if (Camera.MainCamera == null)
            {
                var style = Style.Current;
                Render2D.DrawText(style.FontLarge, "No camera", new Rectangle(Float2.Zero, Size), style.ForegroundDisabled, TextAlignment.Center, TextAlignment.Center);
            }

            // Play mode hints and overlay
            if (Editor.StateMachine.IsPlayMode)
            {
                var style = Style.Current;
                float time = Time.UnscaledGameTime - _gameStartTime;
                float timeout = 3.0f;
                float fadeOutTime = 1.0f;
                float animTime = time - timeout;
                if (animTime < 0)
                {
                    var alpha = Mathf.Saturate(-animTime / fadeOutTime);
                    var rect = new Rectangle(new Float2(6), Size - 12);
                    var text = $"Press {Editor.Options.Options.Input.DebuggerUnlockMouse} to unlock the mouse";
                    Render2D.DrawText(style.FontSmall, text, rect + new Float2(1.0f), style.Background * alpha, TextAlignment.Near, TextAlignment.Far);
                    Render2D.DrawText(style.FontSmall, text, rect, style.Foreground * alpha, TextAlignment.Near, TextAlignment.Far);
                }

                timeout = 1.0f;
                fadeOutTime = 0.6f;
                animTime = time - timeout;
                if (animTime < 0)
                {
                    float alpha = Mathf.Saturate(-animTime / fadeOutTime);
                    Render2D.DrawRectangle(new Rectangle(new Float2(4), Size - 8), style.SelectionBorder * alpha);
                }

                // Add overlay during debugger breakpoint hang
                if (Editor.Simulation.IsDuringBreakpointHang)
                {
                    var bounds = new Rectangle(Float2.Zero, Size);
                    Render2D.FillRectangle(bounds, new Color(0.0f, 0.0f, 0.0f, 0.2f));
                    Render2D.DrawText(Style.Current.FontLarge, "Debugger breakpoint hit...", bounds, Color.White, TextAlignment.Center, TextAlignment.Center);
                }
            }
        }

        /// <summary>
        /// Unlocks the mouse if game window is focused during play mode.
        /// </summary>
        public void UnlockMouseInPlay()
        {
            if (Editor.StateMachine.IsPlayMode)
            {
                // Cache cursor
                _cursorVisible = Screen.CursorVisible;
                _cursorLockMode = Screen.CursorLock;
                Screen.CursorVisible = true;
                if (Screen.CursorLock == CursorLockMode.Clipped)
                    Screen.CursorLock = CursorLockMode.None;

                // Defocus
                _isUnlockingMouse = true;
                Focus(null);
                _isUnlockingMouse = false;
                Editor.Windows.MainWindow.Focus();
                if (Editor.Windows.PropertiesWin.IsDocked)
                    Editor.Windows.PropertiesWin.Focus();
            }
        }

        /// <summary>
        /// Focuses the game viewport. Shows the window if hidden or unselected.
        /// </summary>
        public void FocusGameViewport()
        {
            if (!IsDocked)
            {
                ShowFloating();
            }
            else if (!IsSelected)
            {
                SelectTab(false);
                RootWindow?.Window?.Focus();
            }
            Focus();
        }

        /// <summary>
        /// Apply the selected window mode to the game window.
        /// </summary>
        /// <param name="mode"></param>
        public void SetWindowMode(InterfaceOptions.GameWindowMode mode)
        {
            switch (mode)
            {
            case InterfaceOptions.GameWindowMode.Docked:
                break;
            case InterfaceOptions.GameWindowMode.PopupWindow:
                IsFloating = true;
                break;
            case InterfaceOptions.GameWindowMode.MaximizedWindow:
                IsMaximized = true;
                break;
            case InterfaceOptions.GameWindowMode.BorderlessWindow:
                IsBorderless = true;
                break;
            default: throw new ArgumentOutOfRangeException(nameof(mode), mode, null);
            }
        }

        /// <summary>
        /// Takes the screenshot of the current viewport.
        /// </summary>
        public void TakeScreenshot()
        {
            Screenshot.Capture(_viewport.Task);
        }

        /// <summary>
        /// Takes the screenshot of the current viewport.
        /// </summary>
        /// <param name="path">The output file path.</param>
        public void TakeScreenshot(string path)
        {
            Screenshot.Capture(_viewport.Task, path);
        }

        /// <inheritdoc />
        public override bool OnKeyDown(KeyboardKeys key)
        {
            // Prevent closing the game window tab during a play session
            if (Editor.StateMachine.IsPlayMode && Editor.Options.Options.Input.CloseTab.Process(this, key))
            {
                return true;
            }

            return base.OnKeyDown(key);
        }

        /// <inheritdoc />
        public override void OnStartContainsFocus()
        {
            base.OnStartContainsFocus();

            if (Editor.StateMachine.IsPlayMode && !Editor.StateMachine.PlayingState.IsPaused)
            {
                // Center mouse in play mode
                if (CenterMouseOnFocus)
                {
                    var center = PointToWindow(Size * 0.5f);
                    Root.MousePosition = center;
                }

                // Restore cursor
                if (_cursorLockMode != CursorLockMode.None)
                    Screen.CursorLock = _cursorLockMode;
                if (!_cursorVisible)
                    Screen.CursorVisible = false;
            }
        }

        /// <inheritdoc />
        public override void OnEndContainsFocus()
        {
            base.OnEndContainsFocus();

            if (!_isUnlockingMouse)
            {
                // Cache cursor
                _cursorVisible = Screen.CursorVisible;
                _cursorLockMode = Screen.CursorLock;

                // Restore cursor visibility (could be hidden by the game)
                if (!_cursorVisible)
                    Screen.CursorVisible = true;
            }
        }

        /// <inheritdoc />
        public override Control OnNavigate(NavDirection direction, Float2 location, Control caller, List<Control> visited)
        {
            // Block leaking UI navigation focus outside the game window
            if (IsFocused && caller != this)
            {
                // Pick the first UI control if game UI is not focused yet
                foreach (var child in _guiRoot.Children)
                {
                    if (child.Visible)
                        return child.OnNavigate(direction, Float2.Zero, this, visited);
                }
            }
            return null;
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Float2 location, MouseButton button)
        {
            var result = base.OnMouseDown(location, button);

            // Catch user focus
            if (!ContainsFocus)
                Focus();

            return result;
        }

        /// <inheritdoc />
        public override bool UseLayoutData => true;

        /// <inheritdoc />
        public override void OnLayoutSerialize(XmlWriter writer)
        {
            writer.WriteAttributeString("ShowGUI", ShowGUI.ToString());
            writer.WriteAttributeString("ShowDebugDraw", ShowDebugDraw.ToString());
            writer.WriteAttributeString("DefaultViewportScaling", JsonSerializer.Serialize(_defaultViewportScaling));
            writer.WriteAttributeString("CustomViewportScaling", JsonSerializer.Serialize(_customViewportScaling));
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize(XmlElement node)
        {
            if (bool.TryParse(node.GetAttribute("ShowGUI"), out bool value1))
                ShowGUI = value1;
            if (bool.TryParse(node.GetAttribute("ShowDebugDraw"), out value1))
                ShowDebugDraw = value1;
            if (node.HasAttribute("CustomViewportScaling"))
                _customViewportScaling = JsonSerializer.Deserialize<List<ViewportScaleOptions>>(node.GetAttribute("CustomViewportScaling"));

            for (int i = 0; i < _customViewportScaling.Count; i++)
            {
                if (_customViewportScaling[i].Active)
                    ChangeViewportRatio(_customViewportScaling[i]);
            }

            if (node.HasAttribute("DefaultViewportScaling"))
                _defaultViewportScaling = JsonSerializer.Deserialize<List<ViewportScaleOptions>>(node.GetAttribute("DefaultViewportScaling"));

            for (int i = 0; i < _defaultViewportScaling.Count; i++)
            {
                if (_defaultViewportScaling[i].Active)
                    ChangeViewportRatio(_defaultViewportScaling[i]);
            }
        }

        /// <inheritdoc />
        public override void OnLayoutDeserialize()
        {
            ShowGUI = true;
            ShowDebugDraw = false;
        }
    }
}
