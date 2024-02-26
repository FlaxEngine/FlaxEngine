// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.SceneGraph.Actors
{
    /// <summary>
    /// Actor node for <see cref="NavMeshBoundsVolume"/>.
    /// </summary>
    /// <seealso cref="BoxVolumeNode" />
    [HideInEditor]
    public sealed class NavMeshBoundsVolumeNode : BoxVolumeNode
    {
        /// <inheritdoc />
        public override bool AffectsNavigation => true;

        /// <inheritdoc />
        public NavMeshBoundsVolumeNode(Actor actor)
        : base(actor)
        {
        }
    }
}
