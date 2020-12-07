// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System;
using FlaxEditor.SceneGraph.Actors;
using FlaxEngine;

namespace FlaxEditor.SceneGraph
{
    /// <summary>
    /// Helper base class for actor sub nodes (eg. link points, child parts).
    /// </summary>
    /// <seealso cref="FlaxEditor.SceneGraph.SceneGraphNode" />
    /// <seealso cref="FlaxEditor.SceneGraph.ActorNode" />
    [HideInEditor]
    public abstract class ActorChildNode : SceneGraphNode
    {
        /// <summary>
        /// The node index.
        /// </summary>
        public readonly int Index;

        /// <summary>
        /// Initializes a new instance of the <see cref="ActorChildNode"/> class.
        /// </summary>
        /// <param name="id">The child id.</param>
        /// <param name="index">The child index.</param>
        protected ActorChildNode(Guid id, int index)
        : base(id)
        {
            Index = index;
        }

        /// <inheritdoc />
        public override string Name => ParentNode.Name + "." + Index;

        /// <inheritdoc />
        public override SceneNode ParentScene => ParentNode.ParentScene;

        /// <inheritdoc />
        public override bool CanTransform => ParentNode.CanTransform;

        /// <inheritdoc />
        public override bool IsActive => ParentNode.IsActive;

        /// <inheritdoc />
        public override bool IsActiveInHierarchy => ParentNode.IsActiveInHierarchy;

        /// <inheritdoc />
        public override int OrderInParent
        {
            get => Index;
            set { }
        }

        /// <inheritdoc />
        public override bool CanDelete => false;

        /// <inheritdoc />
        public override bool CanCopyPaste => false;

        /// <inheritdoc />
        public override bool CanDrag => false;

        /// <inheritdoc />
        public override object EditableObject => ParentNode.EditableObject;

        /// <inheritdoc />
        public override object UndoRecordObject => ParentNode.UndoRecordObject;
    }

    /// <summary>
    /// Helper base class for actor sub nodes (eg. link points, child parts).
    /// </summary>
    /// <typeparam name="T">The parent actor type.</typeparam>
    /// <seealso cref="FlaxEditor.SceneGraph.SceneGraphNode" />
    /// <seealso cref="FlaxEditor.SceneGraph.ActorNode" />
    public abstract class ActorChildNode<T> : ActorChildNode where T : ActorNode
    {
        /// <summary>
        /// The actor.
        /// </summary>
        protected readonly T _actor;

        /// <summary>
        /// Initializes a new instance of the <see cref="ActorChildNode{T}"/> class.
        /// </summary>
        /// <param name="actor">The parent actor.</param>
        /// <param name="id">The child id.</param>
        /// <param name="index">The child index.</param>
        protected ActorChildNode(T actor, Guid id, int index)
        : base(id, index)
        {
            _actor = actor;
        }
    }
}
