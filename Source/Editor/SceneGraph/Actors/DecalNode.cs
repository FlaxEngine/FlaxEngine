// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.SceneGraph.Actors
{
    /// <summary>
    /// Scene tree node for <see cref="Decal"/> actor type.
    /// </summary>
    /// <seealso cref="ActorNodeWithIcon" />
    [HideInEditor]
    public sealed class DecalNode : ActorNodeWithIcon
    {
        /// <inheritdoc />
        public DecalNode(Actor actor)
        : base(actor)
        {
        }
    }
}
