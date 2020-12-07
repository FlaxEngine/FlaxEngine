// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.States
{
    /// <summary>
    /// In this state user may edit scene and use editor normally.
    /// </summary>
    /// <seealso cref="FlaxEditor.States.EditorState" />
    [HideInEditor]
    public sealed class EditingSceneState : EditorState
    {
        internal string AutoSaveStatus;

        /// <inheritdoc />
        public override bool CanUseToolbox => true;

        /// <inheritdoc />
        public override bool CanUseUndoRedo => true;

        /// <inheritdoc />
        public override bool CanChangeScene => true;

        /// <inheritdoc />
        public override bool CanEditScene => true;

        /// <inheritdoc />
        public override bool CanEnterPlayMode => true;

        /// <inheritdoc />
        public override bool CanReloadScripts => true;

        /// <inheritdoc />
        public override string Status => AutoSaveStatus;

        internal EditingSceneState(Editor editor)
        : base(editor)
        {
            UpdateFPS();
        }
    }
}
