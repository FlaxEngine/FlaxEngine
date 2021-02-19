// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.SceneGraph.Actors
{
    /// <summary>
    /// Scene tree node for <see cref="Terrain"/> actor type.
    /// </summary>
    [HideInEditor]
    public sealed class TerrainNode : ActorNode
    {
        /// <inheritdoc />
        public TerrainNode(Actor actor)
        : base(actor)
        {
        }

        /// <inheritdoc />
        public override bool AffectsNavigation => true;
    }
}
