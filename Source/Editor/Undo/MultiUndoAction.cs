// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using FlaxEngine;

namespace FlaxEditor
{
    /// <summary>
    /// Implementation of <see cref="IUndoAction"/> that contains one or more child actions performed at once. Allows to merge different actions.
    /// </summary>
    /// <seealso cref="FlaxEditor.IUndoAction" />
    [Serializable]
    [HideInEditor]
    public class MultiUndoAction : IUndoAction
    {
        /// <summary>
        /// The child actions.
        /// </summary>
        [Serialize]
        public readonly IUndoAction[] Actions;

        /// <summary>
        /// Initializes a new instance of the <see cref="MultiUndoAction"/> class.
        /// </summary>
        /// <param name="actions">The actions to include within this multi action.</param>
        public MultiUndoAction(params IUndoAction[] actions)
        {
            Actions = actions?.ToArray() ?? throw new ArgumentNullException();
            if (Actions.Length == 0)
                throw new ArgumentException("Empty actions collection.");
            ActionString = Actions[0].ActionString;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="MultiUndoAction"/> class.
        /// </summary>
        /// <param name="actions">The actions to include within this multi action.</param>
        /// <param name="actionString">The action string.</param>
        public MultiUndoAction(IEnumerable<IUndoAction> actions, string actionString = null)
        {
            Actions = actions?.ToArray() ?? throw new ArgumentNullException();
            if (Actions.Length == 0)
                throw new ArgumentException("Empty actions collection.");
            ActionString = actionString ?? Actions[0].ActionString;
        }

        /// <inheritdoc />
        public string ActionString { get; }

        /// <inheritdoc />
        public void Do()
        {
            for (int i = 0; i < Actions.Length; i++)
            {
                Actions[i].Do();
            }
        }

        /// <inheritdoc />
        public void Undo()
        {
            for (int i = Actions.Length - 1; i >= 0; i--)
            {
                Actions[i].Undo();
            }
        }

        /// <inheritdoc />
        public void Dispose()
        {
            for (int i = 0; i < Actions.Length; i++)
            {
                Actions[i].Dispose();
            }
        }
    }
}
