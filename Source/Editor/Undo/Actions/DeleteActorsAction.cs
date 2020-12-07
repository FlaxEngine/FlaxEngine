// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.SceneGraph;
using FlaxEngine;

namespace FlaxEditor.Actions
{
    /// <summary>
    /// Implementation of <see cref="IUndoAction"/> used to delete a selection of <see cref="ActorNode"/>.
    /// </summary>
    /// <seealso cref="FlaxEditor.IUndoAction" />
    [Serializable]
    class DeleteActorsAction : IUndoAction
    {
        [Serialize]
        private byte[] _data;

        [Serialize]
        private Guid[] _prefabIds;

        [Serialize]
        private Guid[] _prefabObjectIds;

        [Serialize]
        private bool _isInverted;

        /// <summary>
        /// The node parents.
        /// </summary>
        [Serialize]
        protected List<ActorNode> _nodeParents;

        /// <summary>
        /// Initializes a new instance of the <see cref="DeleteActorsAction"/> class.
        /// </summary>
        /// <param name="objects">The objects.</param>
        /// <param name="isInverted">If set to <c>true</c> action will be inverted - instead of delete it will be create actors.</param>
        internal DeleteActorsAction(List<SceneGraphNode> objects, bool isInverted = false)
        {
            _isInverted = isInverted;
            _nodeParents = new List<ActorNode>(objects.Count);
            var actorNodes = new List<ActorNode>(objects.Count);
            var actors = new List<Actor>(objects.Count);
            for (int i = 0; i < objects.Count; i++)
            {
                if (objects[i] is ActorNode node)
                {
                    actorNodes.Add(node);
                    actors.Add(node.Actor);
                }
            }
            actorNodes.BuildNodesParents(_nodeParents);

            _data = Actor.ToBytes(actors.ToArray());

            _prefabIds = new Guid[actors.Count];
            _prefabObjectIds = new Guid[actors.Count];
            for (int i = 0; i < actors.Count; i++)
            {
                _prefabIds[i] = actors[i].PrefabID;
                _prefabObjectIds[i] = actors[i].PrefabObjectID;
            }
        }

        /// <inheritdoc />
        public string ActionString => _isInverted ? "Create actors" : "Delete actors";

        /// <inheritdoc />
        public void Do()
        {
            if (_isInverted)
                Create();
            else
                Delete();
        }

        /// <inheritdoc />
        public void Undo()
        {
            if (_isInverted)
                Delete();
            else
                Create();
        }

        /// <inheritdoc />
        public void Dispose()
        {
            _data = null;
            _prefabIds = null;
            _prefabObjectIds = null;
        }

        /// <summary>
        /// Deletes the objects.
        /// </summary>
        protected virtual void Delete()
        {
            // Remove objects
            for (int i = 0; i < _nodeParents.Count; i++)
            {
                var node = _nodeParents[i];
                Editor.Instance.Scene.MarkSceneEdited(node.ParentScene);
                node.Delete();
            }
            _nodeParents.Clear();
        }

        /// <summary>
        /// Gets the node.
        /// </summary>
        /// <param name="id">The actor id.</param>
        /// <returns>The scene graph node.</returns>
        protected virtual SceneGraphNode GetNode(Guid id)
        {
            return SceneGraphFactory.FindNode(id);
        }

        /// <summary>
        /// Creates the removed objects (from data).
        /// </summary>
        protected virtual void Create()
        {
            // Restore objects
            var actors = Actor.FromBytes(_data);
            if (actors == null)
                return;
            for (int i = 0; i < actors.Length; i++)
            {
                Guid prefabId = _prefabIds[i];
                if (prefabId != Guid.Empty)
                {
                    Actor.Internal_LinkPrefab(FlaxEngine.Object.GetUnmanagedPtr(actors[i]), ref prefabId, ref _prefabObjectIds[i]);
                }
            }
            var actorNodes = new List<ActorNode>(actors.Length);
            for (int i = 0; i < actors.Length; i++)
            {
                var foundNode = GetNode(actors[i].ID);
                if (foundNode is ActorNode node)
                {
                    actorNodes.Add(node);
                }
            }
            actorNodes.BuildNodesParents(_nodeParents);
            for (int i = 0; i < _nodeParents.Count; i++)
            {
                Editor.Instance.Scene.MarkSceneEdited(_nodeParents[i].ParentScene);
            }
        }
    }
}
