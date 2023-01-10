// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.SceneGraph.Actors
{
    /// <summary>
    /// Scene tree node for <see cref="Foliage"/> actor type.
    /// </summary>
    [HideInEditor]
    public sealed class FoliageNode : ActorNode
    {
        /// <inheritdoc />
        public FoliageNode(Actor actor)
        : base(actor)
        {
        }
    }
}
