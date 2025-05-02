// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.SceneGraph.Actors
{
    /// <summary>
    /// Scene tree node for <see cref="BoneSocket"/> actor type.
    /// </summary>
    /// <seealso cref="ActorNode" />
    [HideInEditor]
    public sealed class BoneSocketNode : ActorNode
    {
        /// <inheritdoc />
        public BoneSocketNode(Actor actor)
        : base(actor)
        {
        }
    }
}
