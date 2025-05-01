// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using FlaxEditor.SceneGraph;
using FlaxEngine;

namespace FlaxEditor.Actions
{
    /// <summary>
    /// Implementation of <see cref="IUndoAction"/> used to paste a set of <see cref="ActorNode"/>.
    /// </summary>
    /// <seealso cref="FlaxEditor.IUndoAction" />
    [Serializable]
    class PasteActorsAction : IUndoAction
    {
        [Serialize]
        private Dictionary<Guid, Guid> _idsMapping;

        [Serialize]
        private byte[] _data;

        [Serialize]
        private Guid _pasteParent;

        /// <summary>
        /// The node parents.
        /// </summary>
        [Serialize]
        protected List<Guid> _nodeParents;

        /// <summary>
        /// Initializes a new instance of the <see cref="PasteActorsAction"/> class.
        /// </summary>
        /// <param name="data">The data.</param>
        /// <param name="objectIds">The object ids.</param>
        /// <param name="pasteParent">The paste parent object id.</param>
        /// <param name="name">The action name.</param>
        protected PasteActorsAction(byte[] data, Guid[] objectIds, ref Guid pasteParent, string name)
        {
            ActionString = name;

            _pasteParent = pasteParent;
            _idsMapping = new Dictionary<Guid, Guid>(objectIds.Length * 4);
            for (int i = 0; i < objectIds.Length; i++)
            {
                _idsMapping[objectIds[i]] = Guid.NewGuid();
            }

            _nodeParents = new List<Guid>(objectIds.Length);
            _data = data;
        }

        internal static PasteActorsAction Paste(byte[] data, Guid pasteParent)
        {
            var objectIds = Actor.TryGetSerializedObjectsIds(data);
            if (objectIds == null)
                return null;

            return new PasteActorsAction(data, objectIds, ref pasteParent, "Paste actors");
        }

        internal static PasteActorsAction Duplicate(byte[] data, Guid pasteParent)
        {
            var objectIds = Actor.TryGetSerializedObjectsIds(data);
            if (objectIds == null)
                return null;

            return new PasteActorsAction(data, objectIds, ref pasteParent, "Duplicate actors");
        }

        /// <summary>
        /// Links the broken parent reference (missing parent). By default links the actor to the first scene.
        /// </summary>
        /// <param name="actor">The actor.</param>
        protected virtual void LinkBrokenParentReference(Actor actor)
        {
            // Link to the first scene root
            if (Level.ScenesCount == 0)
                throw new Exception("Failed to paste actor with a broken reference. No loaded scenes.");
            actor.SetParent(Level.GetScene(0), false);
        }

        /// <inheritdoc />
        public string ActionString { get; }

        /// <summary>
        /// Performs the paste/duplicate action and outputs created objects nodes.
        /// </summary>
        /// <param name="nodes">The nodes.</param>
        /// <param name="nodeParents">The node parents.</param>
        public virtual void Do(out List<ActorNode> nodes, out List<ActorNode> nodeParents)
        {
            // Restore objects
            var actors = Actor.FromBytes(_data, _idsMapping);
            if (actors == null)
            {
                nodes = null;
                nodeParents = null;
                return;
            }
            nodes = new List<ActorNode>(actors.Length);
            for (int i = 0; i < actors.Length; i++)
            {
                var actor = actors[i];

                // Check if has no parent linked (broken reference eg. old parent not existing)
                if (actor.Parent == null)
                {
                    LinkBrokenParentReference(actor);
                }

                var node = GetNode(actor.ID);
                if (node is ActorNode actorNode)
                {
                    nodes.Add(actorNode);
                }
            }

            nodeParents = nodes.BuildNodesParents();

            // Cache pasted nodes ids (parents only)
            _nodeParents.Clear();
            _nodeParents.Capacity = Mathf.Max(_nodeParents.Capacity, nodeParents.Count);
            for (int i = 0; i < nodeParents.Count; i++)
            {
                _nodeParents.Add(nodeParents[i].ID);
            }

            var pasteParentNode = Editor.Instance.Scene.GetActorNode(_pasteParent);
            if (pasteParentNode != null)
            {
                // Move pasted actors to the parent target (if specified and valid)
                for (int i = 0; i < nodeParents.Count; i++)
                {
                    nodeParents[i].Actor.SetParent(pasteParentNode.Actor, false);
                }
            }

            // Store previously looked up names and the results
            Dictionary<string, bool> foundNamesResults = new();
            for (int i = 0; i < nodeParents.Count; i++)
            {
                var node = nodeParents[i];
                var actor = node.Actor;
                var parent = actor != null ? actor.Parent : null;
                if (parent != null)
                {
                    bool IsNameValid(string name)
                    {
                        if (!foundNamesResults.TryGetValue(name, out bool found))
                        {
                            found = parent.GetChild(name) != null;
                            foundNamesResults.Add(name, found);
                        }
                        return !found;
                    }

                    // Fix name collisions
                    var name = actor.Name;
                    var children = parent.Children;
                    for (int j = 0; j < children.Length; j++)
                    {
                        var child = children[j];
                        if (child != actor && child.Name == name)
                        {
                            string newName = Utilities.Utils.IncrementNameNumber(name, IsNameValid);
                            foundNamesResults[newName] = true;
                            actor.Name = newName;
                            // Multiple actors may have the same name, continue
                        }
                    }
                }

                Editor.Instance.Scene.MarkSceneEdited(node.ParentScene);
            }

            for (int i = 0; i < nodeParents.Count; i++)
                nodeParents[i].PostPaste();
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

        /// <inheritdoc />
        public void Do()
        {
            Do(out _, out _);
        }

        /// <inheritdoc />
        public virtual void Undo()
        {
            // Remove objects
            for (int i = 0; i < _nodeParents.Count; i++)
            {
                var node = GetNode(_nodeParents[i]);
                Editor.Instance.Scene.MarkSceneEdited(node.ParentScene);
                node.Delete();
            }
            _nodeParents.Clear();
        }

        /// <inheritdoc />
        public virtual void Dispose()
        {
            _nodeParents?.Clear();
            _idsMapping?.Clear();
            _data = null;
        }
    }
}
