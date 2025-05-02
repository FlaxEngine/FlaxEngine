// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;

namespace FlaxEditor.Tools.Foliage.Undo
{
    /// <summary>
    /// The foliage instance delete action that can restore it.
    /// </summary>
    /// <seealso cref="FlaxEditor.IUndoAction" />
    [Serializable]
    sealed class DeleteInstanceAction : IUndoAction
    {
        [Serialize]
        private readonly Guid _foliageId;

        [Serialize]
        private int _index;

        [Serialize]
        private FoliageInstance _instance;

        /// <summary>
        /// Initializes a new instance of the <see cref="DeleteInstanceAction"/> class.
        /// </summary>
        /// <param name="foliage">The foliage.</param>
        /// <param name="index">The instance index.</param>
        public DeleteInstanceAction(FlaxEngine.Foliage foliage, int index)
        {
            _foliageId = foliage.ID;
            _index = index;
        }

        /// <inheritdoc />
        public string ActionString => "Delete foliage instance";

        /// <inheritdoc />
        public void Do()
        {
            var foliageId = _foliageId;
            var foliage = FlaxEngine.Object.Find<FlaxEngine.Foliage>(ref foliageId);

            _instance = foliage.GetInstance(_index);
            foliage.RemoveInstance(_index);
            foliage.RebuildClusters();

            Editor.Instance.Scene.MarkSceneEdited(foliage.Scene);
        }

        /// <inheritdoc />
        public void Undo()
        {
            var foliageId = _foliageId;
            var foliage = FlaxEngine.Object.Find<FlaxEngine.Foliage>(ref foliageId);

            _index = foliage.InstancesCount;
            foliage.AddInstance(ref _instance);
            foliage.RebuildClusters();

            Editor.Instance.Scene.MarkSceneEdited(foliage.Scene);
        }

        /// <inheritdoc />
        public void Dispose()
        {
        }
    }
}
