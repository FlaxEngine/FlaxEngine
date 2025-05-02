// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.SceneGraph.Actors
{
    /// <summary>
    /// Scene tree node for <see cref="EnvironmentProbe"/> actor type.
    /// </summary>
    /// <seealso cref="ActorNodeWithIcon" />
    [HideInEditor]
    public sealed class EnvironmentProbeNode : ActorNodeWithIcon
    {
        /// <inheritdoc />
        public EnvironmentProbeNode(Actor actor)
        : base(actor)
        {
        }
    }
}
