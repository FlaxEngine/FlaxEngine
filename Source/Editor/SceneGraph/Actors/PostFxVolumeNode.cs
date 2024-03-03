// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.SceneGraph.Actors
{
    /// <summary>
    /// Actor node for <see cref="PostFxVolume"/>.
    /// </summary>
    /// <seealso cref="ActorNode" />
    [HideInEditor]
    public sealed class PostFxVolumeNode : BoxVolumeNode
    {
        /// <inheritdoc />
        public PostFxVolumeNode(Actor actor)
        : base(actor)
        {
        }
    }
}
