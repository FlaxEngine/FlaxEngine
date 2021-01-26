// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.SceneGraph.Actors
{
    /// <summary>
    /// Scene tree node for <see cref="Spline"/> actor type.
    /// </summary>
    [HideInEditor]
    public sealed class SplineNode : ActorNode
    {
        /// <inheritdoc />
        public SplineNode(Actor actor)
        : base(actor)
        {
        }
    }
}
