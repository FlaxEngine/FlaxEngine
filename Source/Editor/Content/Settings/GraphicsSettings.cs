// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using FlaxEngine;

namespace FlaxEditor.Content.Settings
{
    /// <summary>
    /// The graphics rendering settings container. Allows to edit asset via editor. To modify those settings at runtime use <see cref="GraphicsSettings"/>.
    /// </summary>
    /// <seealso cref="FlaxEngine.Graphics"/>
    public sealed class GraphicsSettings : SettingsBase
    {
        /// <summary>
        /// Enables rendering synchronization with the refresh rate of the display device to avoid "tearing" artifacts.
        /// </summary>
        [EditorOrder(20), EditorDisplay("General", "Use V-Sync"), Tooltip("Enables rendering synchronization with the refresh rate of the display device to avoid \"tearing\" artifacts.")]
        public bool UseVSync = false;

        /// <summary>
        /// Anti Aliasing quality setting.
        /// </summary>
        [EditorOrder(1000), EditorDisplay("Quality", "AA Quality"), Tooltip("Anti Aliasing quality.")]
        public Quality AAQuality = Quality.Medium;

        /// <summary>
        /// Screen Space Reflections quality.
        /// </summary>
        [EditorOrder(1100), EditorDisplay("Quality", "SSR Quality"), Tooltip("Screen Space Reflections quality.")]
        public Quality SSRQuality = Quality.Medium;

        /// <summary>
        /// Screen Space Ambient Occlusion quality setting.
        /// </summary>
        [EditorOrder(1200), EditorDisplay("Quality", "SSAO Quality"), Tooltip("Screen Space Ambient Occlusion quality setting.")]
        public Quality SSAOQuality = Quality.Medium;

        /// <summary>
        /// Volumetric Fog quality setting.
        /// </summary>
        [EditorOrder(1250), EditorDisplay("Quality", "Volumetric Fog Quality"), Tooltip("Volumetric Fog quality setting.")]
        public Quality VolumetricFogQuality = Quality.High;

        /// <summary>
        /// The shadows quality.
        /// </summary>
        [EditorOrder(1300), EditorDisplay("Quality", "Shadows Quality"), Tooltip("The shadows quality.")]
        public Quality ShadowsQuality = Quality.Medium;

        /// <summary>
        /// The shadow maps quality (textures resolution).
        /// </summary>
        [EditorOrder(1310), EditorDisplay("Quality", "Shadow Maps Quality"), Tooltip("The shadow maps quality (textures resolution).")]
        public Quality ShadowMapsQuality = Quality.Medium;

        /// <summary>
        /// Enables cascades splits blending for directional light shadows.
        /// </summary>
        [EditorOrder(1320), EditorDisplay("Quality", "Allow CSM Blending"), Tooltip("Enables cascades splits blending for directional light shadows.")]
        public bool AllowCSMBlending = false;
    }
}
