// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using FlaxEditor.Modules;
using FlaxEditor.SceneGraph.Actors;
using FlaxEngine;

namespace FlaxEditor.SceneGraph
{
    /// <summary>
    /// Base class for all leaf node objects which belong to scene graph used by the Editor.
    /// Scene Graph is directional graph without cyclic references. It's a tree.
    /// A <see cref="SceneModule"/> class is responsible for Scene Graph management.
    /// </summary>
    [HideInEditor]
    public abstract class SceneGraphNode
    {
        /// <summary>
        /// The parent node.
        /// </summary>
        protected SceneGraphNode parentNode;

        /// <summary>
        /// Gets the children list.
        /// </summary>
        public List<SceneGraphNode> ChildNodes { get; } = new List<SceneGraphNode>();

        /// <summary>
        /// Initializes a new instance of the <see cref="SceneGraphNode"/> class.
        /// </summary>
        /// <param name="id">The unique node identifier. Cannot be changed at runtime.</param>
        protected SceneGraphNode(Guid id)
        {
            ID = id;
            SceneGraphFactory.Nodes.Add(id, this);
        }

        /// <summary>
        /// Gets the name.
        /// </summary>
        public abstract string Name { get; }

        /// <summary>
        /// Gets the identifier. Must be unique and immutable.
        /// </summary>
        public Guid ID { get; }

        /// <summary>
        /// Gets the parent scene.
        /// </summary>
        public abstract SceneNode ParentScene { get; }

        /// <summary>
        /// Gets the root node of the scene graph (if has).
        /// </summary>
        public virtual RootNode Root => ParentNode?.Root;

        /// <inheritdoc />
        public abstract Transform Transform { get; set; }

        /// <summary>
        /// Gets a value indicating whether this instance can be copied or/and pasted.
        /// </summary>
        public virtual bool CanCopyPaste => true;

        /// <summary>
        /// Gets a value indicating whether this instance can be duplicated by the user.
        /// </summary>
        public virtual bool CanDuplicate => true;

        /// <summary>
        /// Gets a value indicating whether this node can be deleted by the user.
        /// </summary>
        public virtual bool CanDelete => true;

        /// <summary>
        /// Gets a value indicating whether this node can be dragged by the user.
        /// </summary>
        public virtual bool CanDrag => true;

        /// <summary>
        /// Gets a value indicating whether this node can be transformed by the user.
        /// </summary>
        public virtual bool CanTransform => true;

        /// <summary>
        /// Gets a value indicating whether this <see cref="SceneGraphNode"/> is active.
        /// </summary>
        public abstract bool IsActive { get; }

        /// <summary>
        /// Gets a value indicating whether this <see cref="SceneGraphNode"/> is active and all parent nodes are also active.
        /// </summary>
        public abstract bool IsActiveInHierarchy { get; }

        /// <summary>
        /// Gets or sets order of the node in the parent container.
        /// </summary>
        public abstract int OrderInParent { get; set; }

        /// <summary>
        /// Gets or sets the parent node.
        /// </summary>
        public virtual SceneGraphNode ParentNode
        {
            get => parentNode;
            set
            {
                if (parentNode != value)
                {
                    if (parentNode != null)
                    {
                        parentNode.ChildNodes.Remove(this);
                    }

                    parentNode = value;

                    if (parentNode != null)
                    {
                        parentNode.ChildNodes.Add(this);
                    }

                    OnParentChanged();
                }
            }
        }

        /// <summary>
        /// Gets the object to edit via properties editor when this node is being selected.
        /// </summary>
        public virtual object EditableObject => this;

        /// <summary>
        /// Gets the object used to record undo changes.
        /// </summary>
        public virtual object UndoRecordObject => EditableObject;

        /// <summary>
        /// Determines whether the specified object is in a hierarchy (one of the children or lower).
        /// </summary>
        /// <param name="node">The node to check,</param>
        /// <returns>True if given actor is part of the hierarchy, otherwise false.</returns>
        public virtual bool ContainsInHierarchy(SceneGraphNode node)
        {
            if (ChildNodes.Contains(node))
                return true;

            return ChildNodes.Any(x => x.ContainsInHierarchy(node));
        }

        /// <summary>
        /// Determines whether the specified object is one of the children.
        /// </summary>
        /// <param name="node">The node to check,</param>
        /// <returns>True if given object is a child, otherwise false.</returns>
        public virtual bool ContainsChild(SceneGraphNode node)
        {
            return ChildNodes.Contains(node);
        }

        /// <summary>
        /// Adds the child node.
        /// </summary>
        /// <param name="node">The node.</param>
        public void AddChild(SceneGraphNode node)
        {
            node.ParentNode = this;
        }

        /// <summary>
        /// The scene graph raycasting data container.
        /// </summary>
        public struct RayCastData
        {
            /// <summary>
            /// The raycasting optional flags.
            /// </summary>
            [Flags]
            public enum FlagTypes
            {
                /// <summary>
                /// The none.
                /// </summary>
                None = 0,

                /// <summary>
                /// The skip colliders flag. Use it to ignore physics colliders intersections detection.
                /// </summary>
                SkipColliders = 1,

                /// <summary>
                /// The skip editor primitives. Use it to ignore editor icons and primitives intersections detection.
                /// </summary>
                SkipEditorPrimitives = 2,
            }

            /// <summary>
            /// The ray (for intersection raycasting).
            /// </summary>
            public Ray Ray;

            /// <summary>
            /// The camera view ray (camera position and direction).
            /// </summary>
            public Ray View;

            /// <summary>
            /// The flags.
            /// </summary>
            public FlagTypes Flags;
        }

        /// <summary>
        /// Performs raycasting over child nodes hierarchy trying to get the closest object hit by the given ray.
        /// </summary>
        /// <param name="ray">The ray casting data.</param>
        /// <param name="distance">The result distance.</param>
        /// <param name="normal">The result intersection surface normal vector.</param>
        /// <returns>Hit object or null if there is no intersection at all.</returns>
        public virtual SceneGraphNode RayCastChildren(ref RayCastData ray, out float distance, out Vector3 normal)
        {
            if (!IsActive)
            {
                distance = 0;
                normal = Vector3.Up;
                return null;
            }

            SceneGraphNode minTarget = null;
            float minDistance = float.MaxValue;
            Vector3 minDistanceNormal = Vector3.Up;

            // Check all children
            for (int i = 0; i < ChildNodes.Count; i++)
            {
                var hit = ChildNodes[i].RayCast(ref ray, out distance, out normal);
                if (hit != null && distance <= minDistance)
                {
                    minTarget = hit;
                    minDistance = distance;
                    minDistanceNormal = normal;
                }
            }

            // Return result
            distance = minDistance;
            normal = minDistanceNormal;
            return minTarget;
        }

        /// <summary>
        /// Performs raycasting over nodes hierarchy trying to get the closest object hit by the given ray.
        /// </summary>
        /// <param name="ray">The ray casting data.</param>
        /// <param name="distance">The result distance.</param>
        /// <param name="normal">The result intersection surface normal vector.</param>
        /// <returns>Hit object or null if there is no intersection at all.</returns>
        public virtual SceneGraphNode RayCast(ref RayCastData ray, out float distance, out Vector3 normal)
        {
            if (!IsActive)
            {
                distance = 0;
                normal = Vector3.Up;
                return null;
            }

            // Check itself
            SceneGraphNode minTarget = null;
            float minDistance = float.MaxValue;
            Vector3 minDistanceNormal = Vector3.Up;
            if (RayCastSelf(ref ray, out distance, out normal))
            {
                minTarget = this;
                minDistance = distance;
                minDistanceNormal = normal;
            }

            // Check all children
            for (int i = 0; i < ChildNodes.Count; i++)
            {
                var hit = ChildNodes[i].RayCast(ref ray, out distance, out normal);
                if (hit != null && distance <= minDistance)
                {
                    minTarget = hit;
                    minDistance = distance;
                    minDistanceNormal = normal;
                }
            }

            // Return result
            distance = minDistance;
            normal = minDistanceNormal;
            return minTarget;
        }

        /// <summary>
        /// Checks if given ray intersects with the node.
        /// </summary>
        /// <param name="ray">The ray casting data.</param>
        /// <param name="distance">The distance.</param>
        /// <param name="normal">The result intersection surface normal vector.</param>
        /// <returns>True ray hits this node, otherwise false.</returns>
        public virtual bool RayCastSelf(ref RayCastData ray, out float distance, out Vector3 normal)
        {
            distance = 0;
            normal = Vector3.Up;
            return false;
        }

        /// <summary>
        /// Called when selected nodes should draw debug shapes using <see cref="DebugDraw"/> interface.
        /// </summary>
        /// <param name="data">The debug draw data.</param>
        public virtual void OnDebugDraw(ViewportDebugDrawData data)
        {
        }

        /// <summary>
        /// Called when scene tree window wants to show the context menu. Allows to add custom options.
        /// </summary>
        public virtual void OnContextMenu(FlaxEditor.GUI.ContextMenu.ContextMenu contextMenu)
        {
        }

        /// <summary>
        /// The scene graph node state container. Used for Editor undo actions (eg. restoring deleted node).
        /// </summary>
        public struct StateData
        {
            /// <summary>
            /// The name of the scene graph node type (full).
            /// </summary>
            public string TypeName;

            /// <summary>
            /// The name of the method (in <see cref="TypeName"/>) that takes this state as a parameter and returns the created scene graph node. Used by the undo actions to restore deleted objects.
            /// </summary>
            public string CreateMethodName;

            /// <summary>
            /// The custom state data (as string).
            /// </summary>
            public string State;

            /// <summary>
            /// The custom state data (as raw bytes).
            /// </summary>
            public byte[] StateRaw;
        }

        /// <summary>
        /// Gets a value indicating whether this node can use <see cref="State"/> property for editor undo operations.
        /// </summary>
        public virtual bool CanUseState => false;

        /// <summary>
        /// Gets or sets the node state.
        /// </summary>
        public virtual StateData State
        {
            get => throw new NotImplementedException();
            set => throw new NotImplementedException();
        }

        /// <summary>
        /// Deletes object represented by this node eg. actor.
        /// </summary>
        public virtual void Delete()
        {
        }

        /// <summary>
        /// Duplicates this object. Valid only if <see cref="CanDuplicate"/> returns true.
        /// </summary>
        /// <param name="undoAction">The undo action that duplicated the object (already performed), null if skip it.</param>
        /// <returns>The duplicated object node.</returns>
        public virtual SceneGraphNode Duplicate(out IUndoAction undoAction)
        {
            undoAction = null;
            return null;
        }

        /// <summary>
        /// Releases the node and the child tree. Disposed all GUI parts and used resources.
        /// </summary>
        public virtual void Dispose()
        {
            OnDispose();

            // Unlink from the parent
            if (parentNode != null)
            {
                parentNode.ChildNodes.Remove(this);
                parentNode = null;
            }
        }

        /// <summary>
        /// Called when node or parent node is disposing.
        /// </summary>
        public virtual void OnDispose()
        {
            // Call deeper
            for (int i = 0; i < ChildNodes.Count; i++)
            {
                ChildNodes[i].OnDispose();
            }

            SceneGraphFactory.Nodes.Remove(ID);
        }

        /// <summary>
        /// Called when parent node gets changed.
        /// </summary>
        protected virtual void OnParentChanged()
        {
        }
    }
}
