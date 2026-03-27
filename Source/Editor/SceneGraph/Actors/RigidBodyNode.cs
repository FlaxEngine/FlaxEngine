// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.SceneGraph.Actors
{
    /// <summary>
    /// Scene tree node for <see cref="RigidBody"/> actor type.
    /// </summary>
    [HideInEditor]
    public sealed class RigidBodyNode : ActorNode
    {
        /// <inheritdoc />
        public RigidBodyNode(Actor actor)
        : base(actor)
        {
        }

        /// <inheritdoc />
        public override void PostSpawn()
        {
            base.PostSpawn();

            if (HasPrefabLink)
                return;
            Actor.StaticFlags = StaticFlags.None;
        }
    }
}
