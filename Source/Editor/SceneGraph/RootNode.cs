// Copyright (c) Wojciech Figat. All rights reserved.

#if USE_LARGE_WORLDS
using Real = System.Double;
#else
using Real = System.Single;
#endif

using System;
using System.Collections.Generic;
using FlaxEditor.SceneGraph.Actors;
using FlaxEngine;

namespace FlaxEditor.SceneGraph
{
    /// <summary>
    /// Represents root node of the whole scene graph.
    /// </summary>
    /// <seealso cref="ActorNode" />
    [HideInEditor]
    public abstract class RootNode : ActorNode
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="RootNode"/> class.
        /// </summary>
        protected RootNode()
        : base(null, Guid.NewGuid())
        {
            _treeNode.AutoFocus = false;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="RootNode"/> class.
        /// </summary>
        /// <param name="id">The node id.</param>
        protected RootNode(Guid id)
        : base(null, id)
        {
            _treeNode.AutoFocus = false;
        }

        /// <summary>
        /// Called when actor child nodes get released.
        /// </summary>
        public event Action<ActorNode> ActorChildNodesDispose;

        /// <summary>
        /// Called when actor child nodes get released.
        /// </summary>
        /// <param name="node">The node.</param>
        public virtual void OnActorChildNodesDispose(ActorNode node)
        {
            ActorChildNodesDispose?.Invoke(node);
        }

        /// <inheritdoc />
        public override string Name => "Root";

        /// <inheritdoc />
        public override SceneNode ParentScene => null;

        /// <inheritdoc />
        public override RootNode Root => this;

        /// <inheritdoc />
        public override bool CanCopyPaste => false;

        /// <inheritdoc />
        public override bool CanDuplicate => false;

        /// <inheritdoc />
        public override bool CanDelete => false;

        /// <inheritdoc />
        public override bool CanDrag => false;

        /// <inheritdoc />
        public override bool IsActive => true;

        /// <inheritdoc />
        public override bool IsActiveInHierarchy => true;

        /// <inheritdoc />
        public override Transform Transform
        {
            get => Transform.Identity;
            set { }
        }

        /// <summary>
        /// Performs raycasting over nodes hierarchy trying to get the closest object hit by the given ray.
        /// </summary>
        /// <param name="ray">The ray.</param>
        /// <param name="view">The view.</param>
        /// <param name="distance">The result distance.</param>
        /// <param name="flags">The raycasting flags.</param>
        /// <returns>Hit object or null if there is no intersection at all.</returns>
        public SceneGraphNode RayCast(ref Ray ray, ref Ray view, out Real distance, RayCastData.FlagTypes flags = RayCastData.FlagTypes.None)
        {
            var data = new RayCastData
            {
                Ray = ray,
                View = view,
                Flags = flags
            };
            return RayCast(ref data, out distance, out _);
        }

        /// <summary>
        /// Performs raycasting over nodes hierarchy trying to get the closest object hit by the given ray.
        /// </summary>
        /// <param name="ray">The ray.</param>
        /// <param name="view">The view.</param>
        /// <param name="distance">The result distance.</param>
        /// <param name="normal">The result intersection surface normal vector.</param>
        /// <param name="flags">The raycasting flags.</param>
        /// <returns>Hit object or null if there is no intersection at all.</returns>
        public SceneGraphNode RayCast(ref Ray ray, ref Ray view, out Real distance, out Vector3 normal, RayCastData.FlagTypes flags = RayCastData.FlagTypes.None)
        {
            var data = new RayCastData
            {
                Ray = ray,
                View = view,
                Flags = flags
            };
            return RayCast(ref data, out distance, out normal);
        }

        internal static Quaternion RaycastNormalRotation(ref Vector3 normal)
        {
            Quaternion rotation;
            if (normal == Vector3.Down)
                rotation = Quaternion.RotationZ(Mathf.Pi);
            else
                rotation = Quaternion.LookRotation(Vector3.Cross(Vector3.Cross(normal, Vector3.Forward), normal), normal);
            return rotation;
        }

        /// <inheritdoc />
        public override bool RayCastSelf(ref RayCastData ray, out Real distance, out Vector3 normal)
        {
            distance = 0;
            normal = Vector3.Up;
            return false;
        }

        /// <inheritdoc />
        public override void OnDebugDraw(ViewportDebugDrawData data)
        {
        }

        /// <inheritdoc />
        public override void Delete()
        {
        }

        /// <summary>
        /// Spawns the specified actor.
        /// </summary>
        /// <param name="actor">The actor.</param>
        /// <param name="parent">The parent.</param>
        /// <param name="orderInParent">Custom index in parent to use. Value -1 means the default one (the last one).</param>
        public abstract void Spawn(Actor actor, Actor parent, int orderInParent = -1);

        /// <summary>
        /// Gets the undo.
        /// </summary>
        public abstract Undo Undo { get; }

        /// <summary>
        /// Gets the list of selected scene graph nodes in the editor context.
        /// [Deprecated in v1.10]
        /// </summary>
        [Obsolete("Use SceneContext.Selection instead.")]
        public List<SceneGraphNode> Selection => SceneContext.Selection;

        /// <summary>
        /// Gets the scene editing context.
        /// </summary>
        public abstract ISceneEditingContext SceneContext { get; }
    }
}
