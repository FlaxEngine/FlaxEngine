// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;

namespace FlaxEditor.Content.Settings
{
    /// <summary>
    /// The base class for all the settings assets.
    /// </summary>
    public abstract class SettingsBase
    {
    }

    partial class GraphicsSettings
    {
        /// <summary>
        /// Renamed UeeHDRProbes into UseHDRProbes
        /// [Deprecated on 12.10.2022, expires on 12.10.2024]
        /// </summary>
        [Serialize, Obsolete, NoUndo]
        private bool UeeHDRProbes
        {
            get => UseHDRProbes;
            set => UseHDRProbes = value;
        }

        /// <summary>
        /// Use DefaultProbeCubemapFormat instead
        /// [Deprecated in v1.13]
        /// </summary>
        [Serialize, Obsolete, NoUndo]
        private bool UseHDRProbes
        {
            get => UseHDRProbes;
            set => DefaultProbeCubemapFormat = value ? ProbeCubemapFormats.R11G11B10 : ProbeCubemapFormats.R8G8B8A8;
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="GraphicsSettings"/>.
        /// </summary>
        public GraphicsSettings()
        {
            // Initialize PostFx settings with default options (C# structs don't support it)
            PostProcessSettings = PostProcessSettings.Default;
        }
    }
}
