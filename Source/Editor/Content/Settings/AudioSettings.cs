// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using System.ComponentModel;
using FlaxEngine;

namespace FlaxEditor.Content.Settings
{
    /// <summary>
    /// The audio payback engine settings container. Allows to edit asset via editor.
    /// </summary>
    public sealed class AudioSettings : SettingsBase
    {
        /// <summary>
        /// If checked, audio playback will be disabled in build game. Can be used if game uses custom audio playback engine.
        /// </summary>
        [DefaultValue(false)]
        [EditorOrder(0), EditorDisplay("General"), Tooltip("If checked, audio playback will be disabled in build game. Can be used if game uses custom audio playback engine.")]
        public bool DisableAudio;

        /// <summary>
        /// The doppler doppler effect factor. Scale for source and listener velocities. Default is 1.
        /// </summary>
        [DefaultValue(1.0f)]
        [EditorOrder(100), EditorDisplay("General"), Limit(0, 10.0f, 0.01f), Tooltip("The doppler doppler effect factor. Scale for source and listener velocities. Default is 1.")]
        public float DopplerFactor = 1.0f;

        /// <summary>
        /// True if mute all audio playback when game has no use focus.
        /// </summary>
        [DefaultValue(true)]
        [EditorOrder(200), EditorDisplay("General", "Mute On Focus Loss"), Tooltip("If checked, engine will mute all audio playback when game has no use focus.")]
        public bool MuteOnFocusLoss = true;
    }
}
