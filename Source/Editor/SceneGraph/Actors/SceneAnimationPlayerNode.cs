// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.SceneGraph.Actors
{
    /// <summary>
    /// Scene tree node for <see cref="SceneAnimationPlayer"/> actor type.
    /// </summary>
    /// <seealso cref="ActorNodeWithIcon" />
    [HideInEditor]
    public sealed class SceneAnimationPlayerNode : ActorNodeWithIcon
    {
        /// <inheritdoc />
        public SceneAnimationPlayerNode(Actor actor)
        : base(actor)
        {
        }
    }
}
