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
        private FoliageInstance instance;
        /// <summary>
        /// The selected instance index
        /// </summary>
        public int SelectedInstanceIndex;

        /// <inheritdoc />
        public FoliageNode(Foliage actor, int selectedInstanceIndex = -1)
        : base(actor)
        {
            SelectedInstanceIndex = selectedInstanceIndex;
            if (selectedInstanceIndex != -1)
            {
                instance = actor.GetInstance(selectedInstanceIndex);
            }
        }
    }
}
