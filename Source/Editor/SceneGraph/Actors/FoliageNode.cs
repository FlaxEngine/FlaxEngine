// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.SceneGraph.Actors
{
    /// <summary>
    /// Scene tree node for <see cref="Foliage"/> actor type.
    /// </summary>
    [HideInEditor]
    public sealed class FoliageNode : ActorNode
    {
        /// <inheritdoc />
        public FoliageNode(Actor actor)
        : base(actor)
        {
        }
    }

    /// <summary>
    /// Scene tree node for instance of <see cref="Foliage"/>.
    /// </summary>
    [HideInEditor]
    public sealed class FoliageInstanceNode : SceneGraphNode
    {
        /// <summary>
        /// The foliage actor that owns this instance.
        /// </summary>
        public Foliage Actor;

        /// <summary>
        /// Index of the foliage instance.
        /// </summary>
        public int Index;

        /// <inheritdoc />
        public FoliageInstanceNode(Foliage actor, int index)
        : base(GetSubID(actor.ID, index))
        {
            Actor = actor;
            Index = index;
        }

        /// <inheritdoc />
        public override string Name => "Foliage Instance";

        /// <inheritdoc />
        public override SceneNode ParentScene
        {
            get
            {
                var scene = Actor ? Actor.Scene : null;
                return scene != null ? SceneGraphFactory.FindNode(scene.ID) as SceneNode : null;
            }
        }

        /// <inheritdoc />
        public override Transform Transform
        {
            get => Actor.GetInstance(Index).Transform;
            set => Actor.SetInstanceTransform(Index, ref value);
        }

        /// <inheritdoc />
        public override bool IsActive => Actor.IsActive;

        /// <inheritdoc />
        public override bool IsActiveInHierarchy => Actor.IsActiveInHierarchy;

        /// <inheritdoc />
        public override int OrderInParent
        {
            get => Index;
            set { }
        }
    }
}
