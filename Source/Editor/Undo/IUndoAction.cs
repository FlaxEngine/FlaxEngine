// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEditor
{
    /// <summary>
    /// Interface for <see cref="Undo"/> actions.
    /// </summary>
    /// <seealso cref="FlaxEditor.IHistoryAction" />
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
