// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.SceneGraph.Actors
{
    /// <summary>
    /// Actor node for <see cref="NavModifierVolume"/>.
    /// </summary>
    /// <seealso cref="BoxVolumeNode" />
    [HideInEditor]
    public sealed class NavModifierVolumeNode : BoxVolumeNode
    {
        /// <inheritdoc />
        public NavModifierVolumeNode(Actor actor)
        : base(actor)
        {
        }

        /// <inheritdoc />
        public override bool AffectsNavigation => true;
    }
}
