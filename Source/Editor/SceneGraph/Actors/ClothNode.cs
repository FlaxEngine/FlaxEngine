// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.SceneGraph.Actors
{
    /// <summary>
    /// Scene tree node for <see cref="Cloth"/> actor type.
    /// </summary>
    [HideInEditor]
    public sealed class ClothNode : ActorNode
    {
        /// <inheritdoc />
        public ClothNode(Actor actor)
        : base(actor)
        {
        }

        /// <inheritdoc />
        public override void PostSpawn()
        {
            base.PostSpawn();

            // Snap to the parent
            if (!(ParentNode is SceneNode))
                Actor.LocalTransform = Transform.Identity;
        }
    }
}
