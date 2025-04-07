// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using FlaxEngine;

namespace FlaxEditor.Surface
{
    /// <summary>
    /// Surface nodes group archetype description.
    /// </summary>
    [HideInEditor]
    public sealed class GroupArchetype
    {
        /// <summary>
        /// Unique group ID.
        /// </summary>
        public ushort GroupID;

        /// <summary>
        /// The group name.
        /// </summary>
        public string Name;

        /// <summary>
        /// Primary color for the group nodes.
        /// </summary>
        public Color Color;

        /// <summary>
        /// The custom tag.
        /// </summary>
        public object Tag;

        /// <summary>
        /// All nodes descriptions.
        /// </summary>
        public IEnumerable<NodeArchetype> Archetypes;
    }
}
