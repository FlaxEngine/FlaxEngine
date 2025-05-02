// Copyright (c) Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Surface
{
    /// <summary>
    /// Base interface for elements that can be added to the <see cref="SurfaceNode"/>.
    /// </summary>
    [HideInEditor]
    public interface ISurfaceNodeElement
    {
        /// <summary>
        /// Gets the parent node.
        /// </summary>
        SurfaceNode ParentNode { get; }

        /// <summary>
        /// Gets the element archetype.
        /// </summary>
        NodeElementArchetype Archetype { get; }
    }
}
