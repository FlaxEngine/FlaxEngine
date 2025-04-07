// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;

namespace FlaxEditor
{
    /// <summary>
    /// Helper class to record undo operations in a block with <c>using</c> keyword.
    /// <example>
    /// using(new UndoBlock(undo, obj, "Rename"))
    /// {
    ///     obj.Name = "super name";
    /// }
    /// </example>
    /// </summary>
    /// <seealso cref="System.IDisposable" />
    [HideInEditor]
    public class UndoBlock : IDisposable
    {
        private readonly Undo _undo;
        private readonly object _snapshotUndoInternal;
        private readonly IUndoAction _customActionBefore;
        private readonly IUndoAction _customActionAfter;

        /// <summary>
        /// Creates new undo object for recording actions with using pattern.
        /// </summary>
        /// <param name="undo">The undo/redo object.</param>
        /// <param name="snapshotInstance">Instance of an object to record.</param>
        /// <param name="actionString">Name of action to be displayed in undo stack.</param>
        /// <param name="customActionBefore">Custom action to append to the undo block action before recorded modifications apply.</param>
        /// <param name="customActionAfter">Custom action to append to the undo block action after recorded modifications apply.</param>
        public UndoBlock(Undo undo, object snapshotInstance, string actionString, IUndoAction customActionBefore = null, IUndoAction customActionAfter = null)
        {
            if (undo == null)
                return;
            _snapshotUndoInternal = snapshotInstance;
            _undo = undo;
            _undo.RecordBegin(_snapshotUndoInternal, actionString);
            _customActionBefore = customActionBefore;
            _customActionAfter = customActionAfter;
        }

        /// <inheritdoc />
        public void Dispose()
        {
            _undo?.RecordEnd(_snapshotUndoInternal, _customActionBefore, _customActionAfter);
        }
    }
}
