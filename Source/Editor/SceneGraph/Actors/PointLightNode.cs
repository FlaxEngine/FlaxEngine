// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.SceneGraph.Actors
{
    /// <summary>
    /// Scene tree node for <see cref="PointLight"/> actor type.
    /// </summary>
    /// <seealso cref="ActorNodeWithIcon" />
    [HideInEditor]
    public sealed class PointLightNode : ActorNodeWithIcon
    {
        /// <inheritdoc />
        public PointLightNode(Actor actor)
        : base(actor)
        {
        }
    }
}
