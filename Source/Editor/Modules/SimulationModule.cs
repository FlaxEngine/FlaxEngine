// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Threading;
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
        /// Checks if play mode should start only with single frame update and then enter step mode.
        /// </summary>
        public bool ShouldPlayModeStartWithStep => Editor.UI.IsPauseButtonChecked;

        /// <summary>
        /// Returns true if play mode has been requested.
        /// </summary>
        public bool IsPlayModeRequested => _isPlayModeRequested;

        /// <summary>
        /// Requests start playing in editor.
        /// </summary>
        public void RequestStartPlay()
        {
            if (Editor.StateMachine.IsEditMode)
            {
                Editor.Log("[PlayMode] Start");

                if (Editor.Options.Options.General.AutoReloadScriptsOnMainWindowFocus)
                    ScriptsBuilder.CheckForCompile();
                _isPlayModeRequested = true;
                Editor.UI.UpdateToolstrip();
            }
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
        /// Requests the playing start or stop in editor.
        /// </summary>
        public void RequestPlayOrStopPlay()
        {
            if (Editor.StateMachine.IsPlayMode)
                RequestStopPlay();
            else
                RequestStartPlay();
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
            if (gameWin != null && gameWin.FocusOnPlay)
            {
                if (!gameWin.IsDocked)
                {
                    gameWin.ShowFloating();
                }
                else if (!gameWin.IsSelected)
                {
                    gameWin.SelectTab(false);
                    gameWin.RootWindow?.Window?.Focus();
                    FlaxEngine.GUI.RootControl.GameRoot.Focus();
                }
            }

            Editor.Log("[PlayMode] Enter");
        }

        /// <inheritdoc />
        public override void OnPlayEnd()
        {
            // Restore focused window before play mode
            if (_enterPlayFocusedWindow != null)
            {
                _enterPlayFocusedWindow.FocusOrShow();
                _enterPlayFocusedWindow = null;
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
                    if ((ScriptsBuilder.IsReady || !Editor.Options.Options.General.AutoReloadScriptsOnMainWindowFocus) && !Level.IsAnyActionPending)
                    {
                        // Clear flag
                        _isPlayModeRequested = false;

                        // Enter play mode
                        Editor.StateMachine.GoToState<PlayingState>();

                        // Check if move just by one frame
                        if (ShouldPlayModeStartWithStep)
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
