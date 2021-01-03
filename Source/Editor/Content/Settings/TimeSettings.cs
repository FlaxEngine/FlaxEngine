// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.ComponentModel;
using FlaxEngine;

namespace FlaxEditor.Content.Settings
{
    /// <summary>
    /// The time settings asset archetype. Allows to edit asset via editor.
    /// </summary>
    public sealed class TimeSettings : SettingsBase
    {
        /// <summary>
        /// The target amount of the game logic updates per second (script updates frequency). Use 0 for infinity.
        /// </summary>
        [DefaultValue(30.0f)]
        [EditorOrder(1), Limit(0, 1000), EditorDisplay(null, "Update FPS"), Tooltip("Target amount of the game logic updates per second (script updates frequency). Use 0 for infinity.")]
        public float UpdateFPS = 30.0f;

        /// <summary>
        /// The target amount of the physics simulation updates per second (also fixed updates frequency). Use 0 for infinity.
        /// </summary>
        [DefaultValue(60.0f)]
        [EditorOrder(2), Limit(0, 1000), EditorDisplay(null, "Physics FPS"), Tooltip("Target amount of the physics simulation updates per second (also fixed updates frequency). Use 0 for infinity.")]
        public float PhysicsFPS = 60.0f;

        /// <summary>
        /// The target amount of the frames rendered per second (actual game FPS). Use 0 for infinity.
        /// </summary>
        [DefaultValue(60.0f)]
        [EditorOrder(3), Limit(0, 1000), EditorDisplay(null, "Draw FPS"), Tooltip("Target amount of the frames rendered per second (actual game FPS). Use 0 for infinity.")]
        public float DrawFPS = 60.0f;

        /// <summary>
        /// The game time scale factor. Default is 1.
        /// </summary>
        [DefaultValue(1.0f)]
        [EditorOrder(10), Limit(0, 1000.0f, 0.1f), Tooltip("Game time scaling factor. Default is 1 for real-time simulation.")]
        public float TimeScale = 1.0f;

        /// <summary>
        /// The maximum allowed delta time (in seconds) for the game logic update step.
        /// </summary>
        [DefaultValue(1.0f / 10.0f)]
        [EditorOrder(20), Limit(0.1f, 1000.0f, 0.01f), Tooltip("The maximum allowed delta time (in seconds) for the game logic update step.")]
        public float MaxUpdateDeltaTime = 1.0f / 10.0f;
    }
}
