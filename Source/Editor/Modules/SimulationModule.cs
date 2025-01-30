// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Threading;
using FlaxEditor.GUI.Docking;
using FlaxEditor.States;
using FlaxEditor.Windows;
using FlaxEngine;

namespace FlaxEditor.Modules
{
    /// <summary>
    /// Manages play in-editor feature (game simulation).
    /// </summary>
    /// <seealso cref="FlaxEditor.Modules.EditorModule" />
    public sealed class SimulationModule : EditorModule
    {
        private bool _isPlayModeRequested;
        private bool _isPlayModeStopRequested;
        private bool _stepFrame;
        private bool _updateOrFixedUpdateWasCalled;
        private long _breakpointHangFlag;
        private EditorWindow _enterPlayFocusedWindow;
        private DockWindow _previousWindow;
        private Guid[] _scenesToReload;

        internal SimulationModule(Editor editor)
        : base(editor)
        {
            FlaxEngine.Scripting.FixedUpdate += OnFixedUpdate;
        }

        /// <summary>
        /// Gets a value indicating whether Editor is during breakpoint hit hang (eg. Visual Script breakpoint).
        /// </summary>
        public bool IsDuringBreakpointHang { get; set; }

        /// <summary>
        /// Occurs when breakpoint hang begins (on eg. Visual Script breakpoint hit).
        /// </summary>
        public event Action BreakpointHangBegin;

        /// <summary>
        /// Occurs when breakpoint hang ends (eg. Visual Script debugging tools continue game execution).
        /// </summary>
        public event Action BreakpointHangEnd;

        internal object BreakpointHangTag;

        internal void OnBreakpointHangBegin()
        {
            _breakpointHangFlag = 1;
            IsDuringBreakpointHang = true;
            BreakpointHangBegin?.Invoke();
        }

        internal void StopBreakpointHang()
        {
            Interlocked.Decrement(ref _breakpointHangFlag);
        }

        internal bool ContinueBreakpointHang()
        {
            return Interlocked.Read(ref _breakpointHangFlag) == 1;
        }

        internal void OnBreakpointHangEnd()
        {
            IsDuringBreakpointHang = false;
            BreakpointHangTag = null;
            BreakpointHangEnd?.Invoke();
        }

        /// <summary>
        /// Delegates between playing game and playing scenes in editor based on the user's editor preference.
        /// </summary>
        public void DelegatePlayOrStopPlayInEditor()
        {
            switch (Editor.Options.Options.Interface.PlayButtonAction)
            {
            case Options.InterfaceOptions.PlayAction.PlayGame:
                Editor.Simulation.RequestPlayGameOrStopPlay();
                return;
            case Options.InterfaceOptions.PlayAction.PlayScenes:
                Editor.Simulation.RequestPlayScenesOrStopPlay();
                return;
            }
        }

        /// <summary>
        /// Returns true if play mode has been requested.
        /// </summary>
        public bool IsPlayModeRequested => _isPlayModeRequested;

        /// <summary>
        /// Requests start playing in editor.
        /// </summary>
        public void RequestStartPlayScenes()
        {
            if (Editor.StateMachine.IsEditMode)
            {
                // Show Game window if hidden
                if (Editor.Windows.GameWin.IsHidden)
                    Editor.Windows.GameWin.Show();

                Editor.Log("[PlayMode] Start");

                if (Editor.Options.Options.General.AutoReloadScriptsOnMainWindowFocus)
                    ScriptsBuilder.CheckForCompile();
                _isPlayModeRequested = true;
                Editor.UI.UpdateToolstrip();
            }
        }

        /// <summary>
        /// Requests playing game start or stop in editor from the project's configured FirstScene.
        /// </summary>
        public void RequestPlayGameOrStopPlay()
        {
            if (Editor.StateMachine.IsPlayMode)
            {
                RequestStopPlay();
            }
            else
            {
                RequestStartPlayGame();
            }
        }

        /// <summary>
        /// Requests start playing in editor from the project's configured FirstScene.
        /// </summary>
        public void RequestStartPlayGame()
        {
            if (!Editor.StateMachine.IsEditMode)
                return;

            // Show Game window if hidden
            if (Editor.Windows.GameWin.IsHidden)
                Editor.Windows.GameWin.Show();

            var firstScene = Content.Settings.GameSettings.Load().FirstScene;
            if (firstScene == Guid.Empty)
            {
                Editor.LogWarning("No First Scene assigned in Game Settings.");
                if (Level.IsAnySceneLoaded)
                    Editor.Simulation.RequestStartPlayScenes();
                return;
            }
            if (!FlaxEngine.Content.GetAssetInfo(firstScene.ID, out var info))
            {
                Editor.LogWarning("Invalid First Scene in Game Settings.");
                if (Level.IsAnySceneLoaded)
                    Editor.Simulation.RequestStartPlayScenes();
                return;
            }

            // Save any modified scenes to prevent loosing local changes
            if (Editor.Scene.IsEdited())
                Level.SaveAllScenes();

            // Load scenes after entering the play mode
            _scenesToReload = new Guid[Level.ScenesCount];
            for (int i = 0; i < _scenesToReload.Length; i++)
                _scenesToReload[i] = Level.GetScene(i).ID;
            Level.UnloadAllScenes();
            Level.LoadScene(firstScene);

            Editor.PlayModeEnd += OnPlayGameEnd;
            RequestPlayScenesOrStopPlay();
        }

        private void OnPlayGameEnd()
        {
            Editor.PlayModeEnd -= OnPlayGameEnd;

            Level.UnloadAllScenes();

            foreach (var sceneId in _scenesToReload)
                Level.LoadScene(sceneId);
        }

        /// <summary>
        /// Requests stop playing in editor.
        /// </summary>
        public void RequestStopPlay()
        {
            if (Editor.StateMachine.IsPlayMode)
            {
                Editor.Log("[PlayMode] Stop");

                if (IsDuringBreakpointHang)
                    StopBreakpointHang();
                _isPlayModeStopRequested = true;
                Editor.UI.UpdateToolstrip();
            }
        }

        /// <summary>
        /// Requests the playing scenes start or stop in editor.
        /// </summary>
        public void RequestPlayScenesOrStopPlay()
        {
            if (Editor.StateMachine.IsPlayMode)
                RequestStopPlay();
            else
                RequestStartPlayScenes();
        }

        /// <summary>
        /// Requests the playing mode resume or pause if already running.
        /// </summary>
        public void RequestResumeOrPause()
        {
            if (Editor.StateMachine.PlayingState.IsPaused)
                Editor.Simulation.RequestResumePlay();

            else
                Editor.Simulation.RequestPausePlay();
        }

        /// <summary>
        /// Requests pause in playing.
        /// </summary>
        public void RequestPausePlay()
        {
            if (Editor.StateMachine.IsPlayMode && !Editor.StateMachine.PlayingState.IsPaused)
            {
                Editor.Log("[PlayMode] Pause");

                Editor.StateMachine.PlayingState.IsPaused = true;
                Editor.UI.UpdateToolstrip();
            }
        }

        /// <summary>
        /// Request resume in playing.
        /// </summary>
        public void RequestResumePlay()
        {
            if (Editor.StateMachine.IsPlayMode && Editor.StateMachine.PlayingState.IsPaused)
            {
                Editor.Log("[PlayMode] Resume");

                Editor.StateMachine.PlayingState.IsPaused = false;
                Editor.UI.UpdateToolstrip();
            }
        }

        /// <summary>
        /// Requests playing single frame in advance.
        /// </summary>
        public void RequestPlayOneFrame()
        {
            if (Editor.StateMachine.IsPlayMode && Editor.StateMachine.PlayingState.IsPaused)
            {
                Editor.Log("[PlayMode] Step one frame");

                // Set flag
                _stepFrame = true;
                _updateOrFixedUpdateWasCalled = false;
                Editor.StateMachine.PlayingState.IsPaused = false;

                // Update
                Editor.UI.UpdateToolstrip();
            }
        }

        /// <inheritdoc />
        public override void OnPlayBegin()
        {
            Editor.Windows.FlashMainWindow();

            // Pick focused window to restore it
            var gameWin = Editor.Windows.GameWin;
            var editWin = Editor.Windows.EditWin;
            if (editWin != null && editWin.IsSelected)
                _enterPlayFocusedWindow = editWin;
            else if (gameWin != null && gameWin.IsSelected)
                _enterPlayFocusedWindow = gameWin;

            // Show Game widow if hidden
            if (gameWin != null)
            {
                switch (gameWin.FocusOnPlayOption)
                {
                case Options.InterfaceOptions.PlayModeFocus.None: break;

                case Options.InterfaceOptions.PlayModeFocus.GameWindow:
                    gameWin.FocusGameViewport();
                    break;

                case Options.InterfaceOptions.PlayModeFocus.GameWindowThenRestore:
                    _previousWindow = gameWin.ParentDockPanel.SelectedTab;
                    gameWin.FocusGameViewport();
                    break;
                }

                gameWin.SetWindowMode(Editor.Options.Options.Interface.DefaultGameWindowMode);
            }

            Editor.Log("[PlayMode] Enter");
        }

        /// <inheritdoc />
        public override void OnPlayEnd()
        {
            var gameWin = Editor.Windows.GameWin;

            switch (gameWin.FocusOnPlayOption)
            {
            case Options.InterfaceOptions.PlayModeFocus.None: break;
            case Options.InterfaceOptions.PlayModeFocus.GameWindow: break;
            case Options.InterfaceOptions.PlayModeFocus.GameWindowThenRestore:
                if (_previousWindow != null && !_previousWindow.IsDisposing)
                {
                    if (!Editor.Windows.GameWin.ParentDockPanel.ContainsTab(_previousWindow))
                        break;
                    _previousWindow.Focus();
                }
                break;
            }

            Editor.UI.UncheckPauseButton();

            Editor.Log("[PlayMode] Exit");
        }

        /// <inheritdoc />
        public override void OnUpdate()
        {
            // Check if can enter playing in editor mode
            if (Editor.StateMachine.CurrentState.CanEnterPlayMode)
            {
                // Check if play mode has been requested
                if (_isPlayModeRequested)
                {
                    // Check if editor has been compiled and scripting reloaded (there is no pending reload action)
                    if ((ScriptsBuilder.IsReady || !Editor.Options.Options.General.AutoReloadScriptsOnMainWindowFocus) && !Level.IsAnyActionPending && Level.IsAnySceneLoaded)
                    {
                        // Clear flag
                        _isPlayModeRequested = false;

                        // Enter play mode
                        var shouldPlayModeStartWithStep = Editor.UI.IsPauseButtonChecked;
                        Editor.StateMachine.GoToState<PlayingState>();
                        if (shouldPlayModeStartWithStep)
                        {
                            RequestPausePlay();
                        }
                    }
                }
                // Check if play mode exit has been requested
                else if (_isPlayModeStopRequested)
                {
                    // Clear flag
                    _isPlayModeStopRequested = false;

                    // Exit play mode
                    Editor.StateMachine.GoToState<EditingSceneState>();
                }
                // Check if step one frame
                else if (_stepFrame)
                {
                    if (_updateOrFixedUpdateWasCalled)
                    {
                        // Clear flag and pause
                        _stepFrame = false;
                        Editor.StateMachine.PlayingState.IsPaused = true;
                        Editor.UI.UpdateToolstrip();
                    }
                    else
                    {
                        // Fixed update may not be called but don't allow to call update for more than 2 times during single step
                        _updateOrFixedUpdateWasCalled = true;
                    }
                }
            }
            else
            {
                // Clear flags
                _isPlayModeRequested = false;
                _isPlayModeStopRequested = false;
                _stepFrame = false;
                _updateOrFixedUpdateWasCalled = false;
            }
        }

        private void OnFixedUpdate()
        {
            // Rise the flag so play mode step end will be called after physics update (user see objects movement)
            _updateOrFixedUpdateWasCalled = true;
        }
    }
}
