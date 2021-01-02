// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.SceneGraph.Actors
{
    /// <summary>
    /// Scene tree node for <see cref="StaticModel"/> actor type.
    /// </summary>
    /// <seealso cref="ActorNode" />
    [HideInEditor]
    public sealed class StaticModelNode : ActorNode
    {
        /// <inheritdoc />
        public StaticModelNode(Actor actor)
        : base(actor)
        {
        }
    }
}
