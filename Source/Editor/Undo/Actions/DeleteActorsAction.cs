// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.SceneGraph;
using FlaxEditor.Scripting;
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
        private byte[] _actorsData;

        [Serialize]
        private List<SceneGraphNode.StateData> _nodesData;

        [Serialize]
        private Guid[] _nodeParentsIDs;

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
        protected List<SceneGraphNode> _nodeParents;

        /// <summary>
        /// Initializes a new instance of the <see cref="DeleteActorsAction"/> class.
        /// </summary>
        /// <param name="nodes">The objects.</param>
        /// <param name="isInverted">If set to <c>true</c> action will be inverted - instead of delete it will be create actors.</param>
        internal DeleteActorsAction(List<SceneGraphNode> nodes, bool isInverted = false)
        {
            _isInverted = isInverted;

            // Collect nodes to delete
            var deleteNodes = new List<SceneGraphNode>(nodes.Count);
            var actors = new List<Actor>(nodes.Count);
            for (int i = 0; i < nodes.Count; i++)
            {
                var node = nodes[i];
                if (node is ActorNode actorNode)
                {
                    deleteNodes.Add(actorNode);
                    actors.Add(actorNode.Actor);
                }
                else
                {
                    deleteNodes.Add(node);
                    if (node.CanUseState)
                    {
                        if (_nodesData == null)
                            _nodesData = new List<SceneGraphNode.StateData>();
                        _nodesData.Add(node.State);
                    }
                }
            }

            // Collect parent nodes to delete
            _nodeParents = new List<SceneGraphNode>(nodes.Count);
            deleteNodes.BuildNodesParents(_nodeParents);
            _nodeParentsIDs = new Guid[_nodeParents.Count];
            for (int i = 0; i < _nodeParentsIDs.Length; i++)
                _nodeParentsIDs[i] = _nodeParents[i].ID;

            // Serialize actors
            _actorsData = Actor.ToBytes(actors.ToArray());

            // Cache actors linkage to prefab objects
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
            _actorsData = null;
            _nodeParentsIDs = null;
            _prefabIds = null;
            _prefabObjectIds = null;
            _nodeParents.Clear();
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
            var nodes = new List<SceneGraphNode>();

            // Restore actors
            var actors = Actor.FromBytes(_actorsData);
            if (actors != null)
            {
                nodes.Capacity = Math.Max(nodes.Capacity, actors.Length);

                // Preserve prefab objects linkage
                for (int i = 0; i < actors.Length; i++)
                {
                    Guid prefabId = _prefabIds[i];
                    if (prefabId != Guid.Empty)
                    {
                        Actor.Internal_LinkPrefab(FlaxEngine.Object.GetUnmanagedPtr(actors[i]), ref prefabId, ref _prefabObjectIds[i]);
                    }
                }
            }

            // Restore nodes state
            if (_nodesData != null)
            {
                for (int i = 0; i < _nodesData.Count; i++)
                {
                    var state = _nodesData[i];
                    var type = TypeUtils.GetManagedType(state.TypeName);
                    if (type == null)
                    {
                        Editor.LogError($"Missing type {state.TypeName} for scene graph node undo state restore.");
                        continue;
                    }
                    var method = type.GetMethod(state.CreateMethodName);
                    if (method == null)
                    {
                        Editor.LogError($"Missing method {state.CreateMethodName} from type {state.TypeName} for scene graph node undo state restore.");
                        continue;
                    }
                    var node = method.Invoke(null, new object[] { state });
                    if (node == null)
                    {
                        Editor.LogError($"Failed to restore scene graph node state via method {state.CreateMethodName} from type {state.TypeName}.");
                        continue;
                    }
                }
            }

            // Cache parent nodes ids
            for (int i = 0; i < _nodeParentsIDs.Length; i++)
            {
                var foundNode = GetNode(_nodeParentsIDs[i]);
                if (foundNode is ActorNode node)
                {
                    nodes.Add(node);
                }
            }
            nodes.BuildNodesParents(_nodeParents);

            // Mark scenes as modified
            for (int i = 0; i < _nodeParents.Count; i++)
            {
                Editor.Instance.Scene.MarkSceneEdited(_nodeParents[i].ParentScene);
            }
        }
    }
}
