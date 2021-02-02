// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEditor.GUI.ContextMenu;
using FlaxEditor.GUI.Input;
using FlaxEditor.Options;
using FlaxEngine;
using FlaxEngine.GUI;
using FlaxEngine.Utilities;

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
        private float _gameStartTime;

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

                    // Update root if it's in game
                    if (Editor.StateMachine.IsPlayMode)
                        _guiRoot.Visible = value;
                }
            }
        }

        /// <summary>
        /// Gets or sets a value indicating whether center mouse position on window focus in play mode. Helps when working with games that lock mouse cursor.
        /// </summary>
        public bool CenterMouseOnFocus { get; set; }

        /// <summary>
        /// Gets or sets a value indicating whether auto-focus game window on play mode start.
        /// </summary>
        public bool FocusOnPlay { get; set; }

        private class GameRoot : ContainerControl
        {
            public bool EnableEvents => !Time.GamePaused;

            public override bool OnCharInput(char c)
            {
                if (!EnableEvents)
                    return false;

                return base.OnCharInput(c);
            }

            public override DragDropEffect OnDragDrop(ref Vector2 location, DragData data)
            {
                if (!EnableEvents)
                    return DragDropEffect.None;

                return base.OnDragDrop(ref location, data);
            }

            public override DragDropEffect OnDragEnter(ref Vector2 location, DragData data)
            {
                if (!EnableEvents)
                    return DragDropEffect.None;

                return base.OnDragEnter(ref location, data);
            }

            public override void OnDragLeave()
            {
                if (!EnableEvents)
                    return;

                base.OnDragLeave();
            }

            public override DragDropEffect OnDragMove(ref Vector2 location, DragData data)
            {
                if (!EnableEvents)
                    return DragDropEffect.None;

                return base.OnDragMove(ref location, data);
            }

            public override bool OnKeyDown(KeyboardKeys key)
            {
                if (!EnableEvents)
                    return false;

                return base.OnKeyDown(key);
            }

            public override void OnKeyUp(KeyboardKeys key)
            {
                if (!EnableEvents)
                    return;

                base.OnKeyUp(key);
            }

            public override bool OnMouseDoubleClick(Vector2 location, MouseButton button)
            {
                if (!EnableEvents)
                    return false;

                return base.OnMouseDoubleClick(location, button);
            }

            public override bool OnMouseDown(Vector2 location, MouseButton button)
            {
                if (!EnableEvents)
                    return false;

                return base.OnMouseDown(location, button);
            }

            public override void OnMouseEnter(Vector2 location)
            {
                if (!EnableEvents)
                    return;

                base.OnMouseEnter(location);
            }

            public override void OnMouseLeave()
            {
                if (!EnableEvents)
                    return;

                base.OnMouseLeave();
            }

            public override void OnMouseMove(Vector2 location)
            {
                if (!EnableEvents)
                    return;

                base.OnMouseMove(location);
            }

            public override bool OnMouseUp(Vector2 location, MouseButton button)
            {
                if (!EnableEvents)
                    return false;

                return base.OnMouseUp(location, button);
            }

            public override bool OnMouseWheel(Vector2 location, float delta)
            {
                if (!EnableEvents)
                    return false;

                return base.OnMouseWheel(location, delta);
            }
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

            // Override the game GUI root
            _guiRoot = new GameRoot
            {
                AnchorPreset = AnchorPresets.StretchAll,
                Offsets = Margin.Zero,
                //Visible = false,
                AutoFocus = false,
                Parent = _viewport
            };
            RootControl.GameRoot = _guiRoot;
            Editor.StateMachine.PlayingState.SceneDuplicating += PlayingStateOnSceneDuplicating;
            Editor.StateMachine.PlayingState.SceneRestored += PlayingStateOnSceneRestored;

            // Link editor options
            Editor.Options.OptionsChanged += OnOptionsChanged;
            OnOptionsChanged(Editor.Options.Options);

            InputActions.Add(options => options.Play, Editor.Simulation.RequestPlayOrStopPlay);
            InputActions.Add(options => options.Pause, Editor.Simulation.RequestResumeOrPause);
            InputActions.Add(options => options.StepFrame, Editor.Simulation.RequestPlayOneFrame);
        }

        private void OnOptionsChanged(EditorOptions options)
        {
            CenterMouseOnFocus = options.Interface.CenterMouseOnGameWinFocus;
            FocusOnPlay = options.Interface.FocusGameWinOnPlay;
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
        protected override bool CanOpenContentFinder => false;

        /// <inheritdoc />
        public override void OnPlayBegin()
        {
            _gameStartTime = Time.UnscaledGameTime;
        }

        /// <inheritdoc />
        public override void OnShowContextMenu(ContextMenu menu)
        {
            base.OnShowContextMenu(menu);

            // Viewport Brightness
            {
                var brightness = menu.AddButton("Viewport Brightness");
                var brightnessValue = new FloatValueBox(_viewport.Brightness, 140, 2, 50.0f, 0.001f, 10.0f, 0.001f);
                brightnessValue.Parent = brightness;
                brightnessValue.ValueChanged += () => _viewport.Brightness = brightnessValue.Value;
            }

            // Viewport Resolution
            {
                var resolution = menu.AddButton("Viewport Resolution");
                var resolutionValue = new FloatValueBox(_viewport.ResolutionScale, 140, 2, 50.0f, 0.1f, 4.0f, 0.001f);
                resolutionValue.Parent = resolution;
                resolutionValue.ValueChanged += () => _viewport.ResolutionScale = resolutionValue.Value;
            }

            // Take Screenshot
            {
                var takeScreenshot = menu.AddButton("Take Screenshot");
                takeScreenshot.Clicked += TakeScreenshot;
            }

            menu.MinimumWidth = 200;
            menu.AddSeparator();
        }

        /// <inheritdoc />
        public override void Draw()
        {
            base.Draw();

            if (Camera.MainCamera == null)
            {
                var style = Style.Current;
                Render2D.DrawText(style.FontLarge, "No camera", new Rectangle(Vector2.Zero, Size), style.ForegroundDisabled, TextAlignment.Center, TextAlignment.Center);
            }

            // Selected UI controls outline
            for (var i = 0; i < Editor.Instance.SceneEditing.Selection.Count; i++)
            {
                if (Editor.Instance.SceneEditing.Selection[i].EditableObject is UIControl controlActor && controlActor.Control != null)
                {
                    Render2D.DrawRectangle(controlActor.Control.Bounds, Editor.Instance.Options.Options.Visual.SelectionOutlineColor0, Editor.Instance.Options.Options.Visual.UISelectionOutlineSize);
                }
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
                    var rect = new Rectangle(new Vector2(6), Size - 12);
                    var text = "Press Shift+F11 to unlock the mouse";
                    Render2D.DrawText(style.FontSmall, text, rect + new Vector2(1.0f), style.Background * alpha, TextAlignment.Near, TextAlignment.Far);
                    Render2D.DrawText(style.FontSmall, text, rect, style.Foreground * alpha, TextAlignment.Near, TextAlignment.Far);
                }

                timeout = 1.0f;
                fadeOutTime = 0.6f;
                animTime = time - timeout;
                if (animTime < 0)
                {
                    float alpha = Mathf.Saturate(-animTime / fadeOutTime);
                    Render2D.DrawRectangle(new Rectangle(new Vector2(4), Size - 8), Color.Orange * alpha);
                }

                // Add overlay during debugger breakpoint hang
                if (Editor.Instance.Simulation.IsDuringBreakpointHang)
                {
                    var bounds = new Rectangle(Vector2.Zero, Size);
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
                Screen.CursorVisible = true;
                Focus(null);
                Editor.Windows.MainWindow.Focus();
                if (Editor.Windows.PropertiesWin.IsDocked)
                    Editor.Windows.PropertiesWin.Focus();
                Screen.CursorVisible = true;
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
            switch (key)
            {
            case KeyboardKeys.F12:
                Screenshot.Capture(string.Empty);
                return true;
            case KeyboardKeys.F11:
            {
                if (Root.GetKey(KeyboardKeys.Shift))
                {
                    // Unlock mouse in game mode
                    UnlockMouseInPlay();
                    return true;
                }
                break;
            }
            }

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

            // Center mouse in play mode
            if (CenterMouseOnFocus && Editor.StateMachine.IsPlayMode)
            {
                Vector2 center = PointToWindow(Size * 0.5f);
                Root.MousePosition = center;
            }
        }

        /// <inheritdoc />
        public override void OnEndContainsFocus()
        {
            base.OnEndContainsFocus();

            // Restore cursor visibility (could be hidden by the game)
            Screen.CursorVisible = true;
        }

        /// <inheritdoc />
        public override bool OnMouseDown(Vector2 location, MouseButton button)
        {
            var result = base.OnMouseDown(location, button);

            // Catch user focus
            if (!ContainsFocus)
                Focus();

            return result;
        }
    }
}
