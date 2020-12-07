// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.GUI.Timeline
{
    /// <summary>
    /// Track creation options.
    /// </summary>
    [HideInEditor]
    public struct TrackCreateOptions
    {
        /// <summary>
        /// The track archetype.
        /// </summary>
        public TrackArchetype Archetype;

        /// <summary>
        /// Create muted track.
        /// </summary>
        public bool Mute;

        /// <summary>
        /// Create looped track.
        /// </summary>
        public bool Loop;
    }
}
