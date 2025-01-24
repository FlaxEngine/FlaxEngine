// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#if USE_LARGE_WORLDS
using Real = System.Double;
#else
using Real = System.Single;
#endif

using System;
using System.Collections.Generic;
using FlaxEditor.SceneGraph.Actors;
using FlaxEditor.SceneGraph.GUI;
using FlaxEditor.Windows;
using FlaxEngine;

namespace FlaxEditor.SceneGraph
{
    /// <summary>
    /// A tree node used to visualize scene actors structure in <see cref="SceneTreeWindow"/>. It's a ViewModel object for <see cref="Actor"/>.
    /// It's part of the Scene Graph.
    /// </summary>
    /// <seealso cref="SceneGraphNode" />
    /// <seealso cref="FlaxEngine.Actor" />
    [HideInEditor]
    public class ActorNode : SceneGraphNode
    {
        /// <summary>
        /// The linked actor object.
        /// </summary>
        protected readonly Actor _actor;

        /// <summary>
        /// The tree node used to present hierarchy structure in GUI.
        /// </summary>
        protected readonly ActorTreeNode _treeNode;

        /// <summary>
        /// Gets the actor.
        /// </summary>
        public Actor Actor => _actor;

        /// <summary>
        /// Gets the tree node (part of the GUI).
        /// </summary>
        public ActorTreeNode TreeNode => _treeNode;

        /// <summary>
        /// The actor child nodes used to represent special parts of the actor (meshes, links, surfaces).
        /// </summary>
        public List<ActorChildNode> ActorChildNodes;

        /// <summary>
        /// Initializes a new instance of the <see cref="ActorNode"/> class.
        /// </summary>
        /// <param name="actor">The actor.</param>
        public ActorNode(Actor actor)
        : base(actor.ID)
        {
            _actor = actor;
            _treeNode = new ActorTreeNode();
            _treeNode.LinkNode(this);
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ActorNode"/> class.
        /// </summary>
        /// <param name="actor">The actor.</param>
        /// <param name="treeNode">The custom tree node.</param>
        protected ActorNode(Actor actor, ActorTreeNode treeNode)
        : base(actor.ID)
        {
            _actor = actor;
            _treeNode = treeNode;
            _treeNode.LinkNode(this);
        }

        internal ActorNode(Actor actor, Guid id)
        : base(id)
        {
            _actor = actor;
            _treeNode = new ActorTreeNode();
            _treeNode.LinkNode(this);
        }

        /// <summary>
        /// Gets a value indicating whether this actor affects navigation.
        /// </summary>
        public virtual bool AffectsNavigation => false;

        /// <summary>
        /// Gets a value indicating whether this actor affects navigation or any of its children (recursive).
        /// </summary>
        public bool AffectsNavigationWithChildren
        {
            get
            {
                if (_actor.HasStaticFlag(StaticFlags.Navigation) && AffectsNavigation)
                    return true;
                for (var i = 0; i < ChildNodes.Count; i++)
                {
                    if (ChildNodes[i] is ActorNode actorChild && actorChild.AffectsNavigationWithChildren)
                        return true;
                }
                return false;
            }
        }

        /// <summary>
        /// Tries to find the tree node for the specified actor.
        /// </summary>
        /// <param name="actor">The actor.</param>
        /// <returns>Tree node or null if cannot find it.</returns>
        public ActorNode Find(Actor actor)
        {
            // Check itself
            if (_actor == actor)
                return this;

            // Check deeper
            for (int i = 0; i < ChildNodes.Count; i++)
            {
                if (ChildNodes[i] is ActorNode node)
                {
                    var result = node.Find(actor);
                    if (result != null)
                        return result;
                }
            }

            return null;
        }

        /// <summary>
        /// Adds the child node.
        /// </summary>
        /// <param name="node">The node.</param>
        /// <returns>The node</returns>
        public ActorChildNode AddChildNode(ActorChildNode node)
        {
            if (ActorChildNodes == null)
                ActorChildNodes = new List<ActorChildNode>();
            ActorChildNodes.Add(node);
            node.ParentNode = this;
            return node;
        }

        /// <summary>
        /// Disposes the child nodes.
        /// </summary>
        public void DisposeChildNodes()
        {
            // Send event to root so if any of this child nodes is selected we can handle it
            var root = Root;
            if (root != null)
            {
                root.OnActorChildNodesDispose(this);
            }

            if (ActorChildNodes != null)
            {
                for (int i = 0; i < ActorChildNodes.Count; i++)
                    ActorChildNodes[i].Dispose();
                ActorChildNodes.Clear();
            }
        }

        /// <summary>
        /// Tries to find the tree node for the specified actor in child nodes collection.
        /// </summary>
        /// <param name="actor">The actor.</param>
        /// <returns>Tree node or null if cannot find it.</returns>
        public ActorNode FindChildActor(Actor actor)
        {
            for (int i = 0; i < ChildNodes.Count; i++)
            {
                if (ChildNodes[i] is ActorNode node && node.Actor == actor)
                {
                    return node;
                }
            }

            return null;
        }

        /// <summary>
        /// Gets a value indicating whether this actor can be used to create prefab from it (as a root).
        /// </summary>
        public virtual bool CanCreatePrefab => (_actor.HideFlags & HideFlags.DontSave) != HideFlags.DontSave;

        /// <summary>
        /// Gets a value indicating whether this actor has a valid linkage to the prefab asset.
        /// </summary>
        public virtual bool HasPrefabLink => _actor.HasPrefabLink;

        /// <inheritdoc />
        public override string Name => _actor.Name;

        /// <inheritdoc />
        public override SceneNode ParentScene
        {
            get
            {
                var scene = _actor ? _actor.Scene : null;
                return scene != null ? SceneGraphFactory.FindNode(scene.ID) as SceneNode : null;
            }
        }

        /// <inheritdoc />
        public override bool CanTransform => (_actor.StaticFlags & StaticFlags.Transform) == 0;

        /// <inheritdoc />
        public override bool CanCopyPaste => (_actor.HideFlags & HideFlags.HideInHierarchy) == 0;

        /// <inheritdoc />
        public override bool CanDuplicate => (_actor.HideFlags & HideFlags.HideInHierarchy) == 0;

        /// <inheritdoc />
        public override bool IsActive => _actor?.IsActive ?? false;

        /// <inheritdoc />
        public override bool IsActiveInHierarchy => _actor?.IsActiveInHierarchy ?? false;

        /// <inheritdoc />
        public override int OrderInParent
        {
            get => _actor.OrderInParent;
            set => _actor.OrderInParent = value;
        }

        /// <inheritdoc />
        public override Transform Transform
        {
            get => _actor.Transform;
            set => _actor.Transform = value;
        }

#if false
        /// <inheritdoc />
        public override SceneGraphNode ParentNode
        {
            set
            {
                if (!(value is ActorNode))
                    throw new InvalidOperationException("ActorNode can have only ActorNode as a parent node.");
                base.ParentNode = value;
            }
        }
#endif

        /// <inheritdoc />
        public override object EditableObject => _actor;

        /// <inheritdoc />
        public override SceneGraphNode RayCast(ref RayCastData ray, out Real distance, out Vector3 normal)
        {
            var hit = base.RayCast(ref ray, out distance, out normal);

            // Skip actors that should not be selected
            if (hit != null && _actor != null && (_actor.HideFlags & HideFlags.DontSelect) == HideFlags.DontSelect)
            {
                hit = parentNode;
            }

            return hit;
        }

        /// <inheritdoc />
        public override bool RayCastSelf(ref RayCastData ray, out Real distance, out Vector3 normal)
        {
            return _actor.IntersectsItself(ray.Ray, out distance, out normal);
        }

        /// <inheritdoc />
        public override void GetEditorSphere(out BoundingSphere sphere)
        {
            Editor.GetActorEditorSphere(_actor, out sphere);
        }

        /// <inheritdoc />
        public override void OnDebugDraw(ViewportDebugDrawData data)
        {
            data.Add(_actor);
        }

        /// <inheritdoc />
        public override void Delete()
        {
            FlaxEngine.Object.Destroy(_actor);
        }

        /// <summary>
        /// Action called after spawning actor in editor (via drag to viewport, with toolbox, etc.).
        /// Can be used to tweak default values of the actor.
        /// </summary>
        public virtual void PostSpawn()
        {
        }

        /// <summary>
        /// Action called after pasting actor in editor.
        /// </summary>
        public virtual void PostPaste()
        {
        }

        /// <summary>
        /// Action called after converting actor in editor.
        /// </summary>
        /// <param name="source">The source actor node from which this node was converted.</param>
        public virtual void PostConvert(ActorNode source)
        {
        }

        /// <inheritdoc />
        protected override void OnParentChanged()
        {
            base.OnParentChanged();

            _treeNode.OnParentChanged(_actor, parentNode as ActorNode);
        }

        /// <inheritdoc />
        public override void Dispose()
        {
            _treeNode.Dispose();
            if (ActorChildNodes != null)
            {
                ActorChildNodes.Clear();
                ActorChildNodes = null;
            }

            base.Dispose();
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return _actor ? _actor.ToString() : base.ToString();
        }
    }
}
