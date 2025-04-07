// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.Utilities;

namespace FlaxEditor.States
{
    /// <summary>
    /// In this state editor is reloading user scripts.
    /// </summary>
    /// <seealso cref="FlaxEditor.States.EditorState" />
    [HideInEditor]
    public sealed class ReloadingScriptsState : EditorState
    {
        /// <inheritdoc />
        public override string Status => "Reloading scripts...";

        internal ReloadingScriptsState(Editor editor)
        : base(editor)
        {
        }

        /// <inheritdoc />
        public override void OnEnter()
        {
            base.OnEnter();

            ScriptsBuilder.ScriptsReloadEnd += OnScriptsReloadEnd;
        }

        /// <inheritdoc />
        public override void OnExit(State nextState)
        {
            ScriptsBuilder.ScriptsReloadEnd -= OnScriptsReloadEnd;

            base.OnExit(nextState);
        }

        private void OnScriptsReloadEnd()
        {
            StateMachine.GoToState<EditingSceneState>();
        }
    }
}
