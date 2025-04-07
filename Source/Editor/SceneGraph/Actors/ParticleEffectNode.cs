// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.SceneGraph.Actors
{
    /// <summary>
    /// Scene tree node for <see cref="ParticleEffect"/> actor type.
    /// </summary>
    /// <seealso cref="ActorNodeWithIcon" />
    [HideInEditor]
    public sealed class ParticleEffectNode : ActorNodeWithIcon
    {
        /// <inheritdoc />
        public ParticleEffectNode(Actor actor)
        : base(actor)
        {
        }
    }
}
