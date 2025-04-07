// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;

namespace FlaxEditor.Tools.Foliage.Undo
{
    /// <summary>
    /// The foliage instance edit action.
    /// </summary>
    /// <seealso cref="FlaxEditor.IUndoAction" />
    [Serializable]
    sealed class EditInstanceAction : IUndoAction
    {
        [Serialize]
        private readonly Guid _foliageId;

        [Serialize]
        private int _index;

        [Serialize]
        private Transform _before;

        [Serialize]
        private Transform _after;

        /// <summary>
        /// Initializes a new instance of the <see cref="EditInstanceAction"/> class.
        /// </summary>
        /// <param name="foliage">The foliage.</param>
        /// <param name="index">The instance index.</param>
        public EditInstanceAction(FlaxEngine.Foliage foliage, int index)
        {
            _foliageId = foliage.ID;
            _index = index;
            _before = foliage.GetInstance(_index).Transform;
        }

        /// <summary>
        /// Called when foliage editing ends. Records the `after` state of the instance. Marks foliage actor parent scene edited.
        /// </summary>
        public void RecordEnd()
        {
            var foliageId = _foliageId;
            var foliage = FlaxEngine.Object.Find<FlaxEngine.Foliage>(ref foliageId);
            _after = foliage.GetInstance(_index).Transform;
            Editor.Instance.Scene.MarkSceneEdited(foliage.Scene);
        }

        /// <inheritdoc />
        public string ActionString => "Edit foliage instance";

        /// <inheritdoc />
        public void Do()
        {
            var foliageId = _foliageId;
            var foliage = FlaxEngine.Object.Find<FlaxEngine.Foliage>(ref foliageId);
            foliage.SetInstanceTransform(_index, ref _after);
            foliage.RebuildClusters();
            Editor.Instance.Scene.MarkSceneEdited(foliage.Scene);
        }

        /// <inheritdoc />
        public void Undo()
        {
            var foliageId = _foliageId;
            var foliage = FlaxEngine.Object.Find<FlaxEngine.Foliage>(ref foliageId);
            foliage.SetInstanceTransform(_index, ref _before);
            foliage.RebuildClusters();
            Editor.Instance.Scene.MarkSceneEdited(foliage.Scene);
        }

        /// <inheritdoc />
        public void Dispose()
        {
        }
    }
}
