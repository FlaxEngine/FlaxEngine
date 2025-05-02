// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEngine;
using FlaxEngine.Utilities;

namespace FlaxEditor.States
{
    /// <summary>
    /// Flax Editor states machine.
    /// </summary>
    /// <seealso cref="FlaxEngine.Utilities.StateMachine" />
    [HideInEditor]
    public sealed class EditorStateMachine : StateMachine
    {
        private readonly Queue<EditorState> _pendingStates = new Queue<EditorState>();

        /// <summary>
        /// Gets the current state.
        /// </summary>
        public new EditorState CurrentState => currentState as EditorState;

        /// <summary>
        /// Checks if editor is in playing mode
        /// </summary>
        public bool IsPlayMode => CurrentState == PlayingState;

        /// <summary>
        /// Checks if editor is in editing mode
        /// </summary>
        public bool IsEditMode => CurrentState == EditingSceneState;

        /// <summary>
        /// Editor loading state.
        /// </summary>
        public readonly LoadingState LoadingState;

        /// <summary>
        /// Editor closing state.
        /// </summary>
        public readonly ClosingState ClosingState;

        /// <summary>
        /// Editor editing scene state.
        /// </summary>
        public readonly EditingSceneState EditingSceneState;

        /// <summary>
        /// Editor changing scenes state.
        /// </summary>
        public readonly ChangingScenesState ChangingScenesState;

        /// <summary>
        /// Editor playing state.
        /// </summary>
        public readonly PlayingState PlayingState;

        /// <summary>
        /// Editor reloading scripts state.
        /// </summary>
        public readonly ReloadingScriptsState ReloadingScriptsState;

        /// <summary>
        /// Editor building lighting state.
        /// </summary>
        public readonly BuildingLightingState BuildingLightingState;

        /// <summary>
        /// Editor building scenes data state.
        /// </summary>
        public readonly BuildingScenesState BuildingScenesState;

        internal EditorStateMachine(Editor editor)
        {
            // Register all in-build states
            AddState(LoadingState = new LoadingState(editor));
            AddState(ClosingState = new ClosingState(editor));
            AddState(EditingSceneState = new EditingSceneState(editor));
            AddState(ChangingScenesState = new ChangingScenesState(editor));
            AddState(PlayingState = new PlayingState(editor));
            AddState(ReloadingScriptsState = new ReloadingScriptsState(editor));
            AddState(BuildingLightingState = new BuildingLightingState(editor));
            AddState(BuildingScenesState = new BuildingScenesState(editor));

            // Set initial state
            GoToState(LoadingState);
        }

        internal void Update()
        {
            // Changing states
            while (_pendingStates.Count > 0)
            {
                GoToState(_pendingStates.Dequeue());
            }

            // State update
            if (CurrentState != null)
            {
                CurrentState.Update();
                CurrentState.UpdateFPS();
            }
        }

        /// <inheritdoc />
        protected override void SwitchState(State nextState)
        {
            Editor.Log($"Changing editor state from {currentState} to {nextState}");

            base.SwitchState(nextState);
        }
    }
}
