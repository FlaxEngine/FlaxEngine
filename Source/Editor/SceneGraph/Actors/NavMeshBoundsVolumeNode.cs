// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

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
        public NavMeshBoundsVolumeNode(Actor actor)
        : base(actor)
        {
        }
    }
}
