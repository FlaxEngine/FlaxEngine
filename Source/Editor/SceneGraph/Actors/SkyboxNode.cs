// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.SceneGraph.Actors
{
    /// <summary>
    /// Scene tree node for <see cref="Skybox"/> actor type.
    /// </summary>
    /// <seealso cref="ActorNodeWithIcon" />
    [HideInEditor]
    public sealed class SkyboxNode : ActorNodeWithIcon
    {
        /// <inheritdoc />
        public SkyboxNode(Actor actor)
        : base(actor)
        {
        }
    }
}
