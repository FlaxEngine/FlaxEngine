// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEditor.History;

namespace FlaxEditor
{
    /// <summary>
    /// Interface for <see cref="Undo"/> actions.
    /// </summary>
    /// <seealso cref="FlaxEditor.History.IHistoryAction" />
    public interface IUndoAction : IHistoryAction
    {
        /// <summary>
        /// Performs this action.
        /// </summary>
        void Do();

        /// <summary>
        /// Undoes this action.
        /// </summary>
        void Undo();
    }
}
