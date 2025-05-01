// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;

namespace FlaxEditor.Tools.Foliage.Undo
{
    /// <summary>
    /// The foliage editing action that handles changing selected foliage actor instance index.
    /// </summary>
    /// <seealso cref="FlaxEditor.IUndoAction" />
    [Serializable]
    public sealed class EditSelectedInstanceIndexAction : IUndoAction
    {
        [Serialize]
        private int _before;

        [Serialize]
        private int _after;

        /// <summary>
        /// Initializes a new instance of the <see cref="EditSelectedInstanceIndexAction"/> class.
        /// </summary>
        /// <param name="before">The selected index before.</param>
        /// <param name="after">The selected index after.</param>
        public EditSelectedInstanceIndexAction(int before, int after)
        {
            _before = before;
            _after = after;
        }

        /// <inheritdoc />
        public string ActionString => "Edit selected foliage instance index";

        /// <inheritdoc />
        public void Dispose()
        {
        }

        /// <inheritdoc />
        public void Do()
        {
            Editor.Instance.Windows.ToolboxWin.Foliage.Edit.Mode.SelectedInstanceIndex = _after;
        }

        /// <inheritdoc />
        public void Undo()
        {
            Editor.Instance.Windows.ToolboxWin.Foliage.Edit.Mode.SelectedInstanceIndex = _before;
        }
    }
}
