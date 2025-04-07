// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.SceneGraph;

namespace FlaxEditor
{
    /// <summary>
    /// Objects selection change action.
    /// </summary>
    /// <seealso cref="IUndoAction" />
    [Serializable]
    public class SelectionChangeAction : UndoActionBase<SelectionChangeAction.DataStorage>
    {
        /// <summary>
        /// The undo data.
        /// </summary>
        [Serializable]
        public struct DataStorage
        {
            /// <summary>
            /// The 'before' selection.
            /// </summary>
            public SceneGraphNode[] Before;

            /// <summary>
            /// The 'after' selection.
            /// </summary>
            public SceneGraphNode[] After;
        }

        private Action<SceneGraphNode[]> _callback;

        /// <summary>
        /// User selection has changed
        /// </summary>
        /// <param name="before">Previously selected nodes</param>
        /// <param name="after">Newly selected nodes</param>
        /// <param name="callback">Selection change callback</param>
        public SelectionChangeAction(SceneGraphNode[] before, SceneGraphNode[] after, Action<SceneGraphNode[]> callback)
        {
            Data = new DataStorage
            {
                Before = before,
                After = after,
            };
            _callback = callback;
        }

        /// <inheritdoc />
        public override string ActionString => "Selection change";

        /// <inheritdoc />
        public override void Do()
        {
            var data = Data;
            _callback(data.After);
        }

        /// <inheritdoc />
        public override void Undo()
        {
            var data = Data;
            _callback(data.Before);
        }
    }
}
