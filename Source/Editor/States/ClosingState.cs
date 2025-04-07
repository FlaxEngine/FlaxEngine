// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;
using FlaxEngine.Utilities;

namespace FlaxEditor.States
{
    /// <summary>
    /// In this state editor is performing closing actions and will shutdown. This is last state and cannot leave it.
    /// </summary>
    /// <seealso cref="FlaxEditor.States.EditorState" />
    [HideInEditor]
    public sealed class ClosingState : EditorState
    {
        /// <inheritdoc />
        public override bool CanEditContent => false;

        /// <inheritdoc />
        public override bool IsEditorReady => false;

        /// <inheritdoc />
        public override string Status => "Closing...";

        internal ClosingState(Editor editor)
        : base(editor)
        {
        }

        /// <inheritdoc />
        public override bool CanExit(State nextState)
        {
            // Disable exit from Closing state
            return false;
        }

        /// <inheritdoc />
        public override void OnEnter()
        {
            Editor.CloseSplashScreen();

            // Cleanup selection
            Editor.Instance.SceneEditing.Deselect();

            base.OnEnter();
        }
    }
}
