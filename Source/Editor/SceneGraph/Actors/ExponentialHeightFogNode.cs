// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.SceneGraph.Actors
{
    /// <summary>
    /// Scene tree node for <see cref="ExponentialHeightFog"/> actor type.
    /// </summary>
    /// <seealso cref="ActorNodeWithIcon" />
    [HideInEditor]
    public sealed class ExponentialHeightFogNode : ActorNodeWithIcon
    {
        /// <inheritdoc />
        public ExponentialHeightFogNode(Actor actor)
        : base(actor)
        {
        }
    }
}
