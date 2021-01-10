// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEngine;

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
    }
}
