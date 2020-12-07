// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System.IO;
using FlaxEngine;

namespace FlaxEditor.GUI.Timeline
{
    /// <summary>
    /// Create a new track object.
    /// </summary>
    /// <param name="options">The options.</param>
    /// <returns>The created track object.</returns>
    public delegate Track CreateTrackDelegate(TrackCreateOptions options);

    /// <summary>
    /// Loads a track data.
    /// </summary>
    /// <param name="version">The serialized data version.</param>
    /// <param name="track">The track.</param>
    /// <param name="stream">The input stream.</param>
    public delegate void LoadTrackDelegate(int version, Track track, BinaryReader stream);

    /// <summary>
    /// Saves a track data.
    /// </summary>
    /// <param name="track">The track.</param>
    /// <param name="stream">The output stream.</param>
    public delegate void SaveTrackDelegate(Track track, BinaryWriter stream);

    /// <summary>
    /// Defines the track type.
    /// </summary>
    [HideInEditor]
    public struct TrackArchetype
    {
        /// <summary>
        /// The track type identifier (must match C++ implementation).
        /// </summary>
        public int TypeId;

        /// <summary>
        /// The name of the track type (for UI).
        /// </summary>
        public string Name;

        /// <summary>
        /// True if hide track archetype from spawning via GUI.
        /// </summary>
        public bool DisableSpawnViaGUI;

        /// <summary>
        /// The icon of the track type (for UI).
        /// </summary>
        public SpriteHandle Icon;

        /// <summary>
        /// The track create factory callback.
        /// </summary>
        public CreateTrackDelegate Create;

        /// <summary>
        /// The track data loading callback.
        /// </summary>
        public LoadTrackDelegate Load;

        /// <summary>
        /// The track data saving callback.
        /// </summary>
        public SaveTrackDelegate Save;
    }
}
