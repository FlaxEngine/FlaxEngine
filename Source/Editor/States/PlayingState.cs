// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.Content.Settings;
using FlaxEditor.SceneGraph;
using FlaxEditor.Utilities;
using FlaxEngine;
using FlaxEngine.Utilities;

namespace FlaxEditor.States
{
    /// <summary>
    /// In this state editor is simulating game.
    /// </summary>
    /// <seealso cref="FlaxEditor.States.EditorState" />
    [HideInEditor]
    public sealed class PlayingState : EditorState
    {
        private readonly DuplicateScenes _duplicateScenes = new DuplicateScenes();
        private readonly List<Guid> _selectedObjects = new List<Guid>();

        /// <summary>
        /// Gets a value indicating whether any scene was dirty before entering the play mode.
        /// </summary>
        public bool WasDirty => _duplicateScenes.WasDirty;

        /// <inheritdoc />
        public override bool CanEditScene => true;

        /// <inheritdoc />
        public override bool CanChangeScene => true;

        /// <inheritdoc />
        public override bool CanUseUndoRedo => false;

        /// <inheritdoc />
        public override bool CanEnterPlayMode => true;

        /// <summary>
        /// Occurs when play mode is starting (before scene duplicating).
        /// </summary>
        public event Action SceneDuplicating;

        /// <summary>
        /// Occurs when play mode is starting (after scene duplicating).
        /// </summary>
        public event Action SceneDuplicated;

        /// <summary>
        /// Occurs when play mode is applying game settings. Can be used to cache the editor local state of some settings.
        /// </summary>
        public event Action GameSettingsApplying;

        /// <summary>
        /// Occurs when play mode applied game settings. Can be used to preserve the editor local state of some settings.
        /// </summary>
        public event Action GameSettingsApplied;

        /// <summary>
        /// Occurs when play mode is ending (before scene restoring).
        /// </summary>
        public event Action SceneRestoring;

        /// <summary>
        /// Occurs when play mode is ending (after scene restoring).
        /// </summary>
        public event Action SceneRestored;

        /// <summary>
        /// Gets or sets a value indicating whether game logic is paused.
        /// </summary>
        public bool IsPaused
        {
            get => Time.GamePaused;
            set
            {
                if (!IsActive)
                    throw new InvalidOperationException();
                Time.GamePaused = value;
            }
        }

        /// <summary>
        /// True if play mode is starting.
        /// </summary>
        public bool IsPlayModeStarting;

        /// <summary>
        /// True if play mode is ending.
        /// </summary>
        public bool IsPlayModeEnding;

        internal PlayingState(Editor editor)
        : base(editor)
        {
            SetupEditorEnvOptions();
        }

        private void CacheSelection()
        {
            Profiler.BeginEvent("PlayingState.CacheSelection");
            _selectedObjects.Clear();
            var selection = Editor.SceneEditing.Selection;
            if (selection.Count != 0)
            {
                for (int i = 0; i < selection.Count; i++)
                    _selectedObjects.Add(selection[i].ID);
                selection.Clear();
                Editor.SceneEditing.OnSelectionChanged();
            }
            Profiler.EndEvent();
        }

        private void RestoreSelection()
        {
            Profiler.BeginEvent("PlayingState.RestoreSelection");
            var selection = Editor.SceneEditing.Selection;
            var count = selection.Count;
            selection.Clear();
            for (int i = 0; i < _selectedObjects.Count; i++)
            {
                var node = SceneGraphFactory.FindNode(_selectedObjects[i]);
                if (node != null)
                    selection.Add(node);
            }
            if (selection.Count != count)
                Editor.SceneEditing.OnSelectionChanged();
            Profiler.EndEvent();
        }

        /// <inheritdoc />
        public override string Status => "Play Mode!";

        /// <inheritdoc />
        public override void OnEnter()
        {
            Profiler.BeginEvent("PlayingState.OnEnter");
            IsPlayModeStarting = true;
            Editor.OnPlayBeginning();

            CacheSelection();

            // Remove references to the scene objects
            Editor.Scene.ClearRefsToSceneObjects();

            // Apply game settings (user may modify them before the gameplay)
            GameSettingsApplying?.Invoke();
            GameSettings.Apply();
            GameSettingsApplied?.Invoke();

            // Duplicate editor scene for simulation
            SceneDuplicating?.Invoke();
            _duplicateScenes.GatherSceneData();
            Editor.Internal_SetPlayMode(true);
            IsPaused = false;
            PluginManager.Internal_InitializeGamePlugins();
            _duplicateScenes.CreateScenes();
            SceneDuplicated?.Invoke();
            RestoreSelection();

            Editor.OnPlayBegin();
            IsPlayModeStarting = false;
            Profiler.EndEvent();

            Time.Synchronize();
        }

        private void SetupEditorEnvOptions()
        {
            Time.TimeScale = 1.0f;
            Physics.DefaultScene.AutoSimulation = true;
            Screen.CursorVisible = true;
            Screen.CursorLock = CursorLockMode.None;
        }

        /// <inheritdoc />
        public override void UpdateFPS()
        {
            // Game drives the Time settings
        }

        /// <inheritdoc />
        public override void OnExit(State nextState)
        {
            Profiler.BeginEvent("PlayingState.OnExit");
            IsPlayModeEnding = true;
            Editor.OnPlayEnding();
            IsPaused = true;

            // Remove references to the scene objects
            Editor.Scene.ClearRefsToSceneObjects();

            // Restore editor scene
            SceneRestoring?.Invoke();
            _duplicateScenes.UnloadScenes();
            PluginManager.Internal_DeinitializeGamePlugins();
            FlaxEngine.Scripting.FlushRemovedObjects();
            Editor.WipeOutLeftoverSceneObjects();
            Editor.Internal_SetPlayMode(false);
            _duplicateScenes.RestoreSceneData();
            SceneRestored?.Invoke();

            // Restore game settings and state for editor environment
            SetupEditorEnvOptions();
            var win = Editor.Windows.GameWin?.Root;
            if (win != null)
                win.Cursor = CursorType.Default;
            IsPaused = true;
            RestoreSelection();

            Editor.OnPlayEnd();
            IsPlayModeEnding = false;
            Profiler.EndEvent();

            Time.Synchronize();
        }
    }
}
