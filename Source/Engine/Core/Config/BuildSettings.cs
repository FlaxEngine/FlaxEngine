// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;

namespace FlaxEditor.Content.Settings
{
    partial class BuildSettings
    {
#if FLAX_EDITOR
        /// <summary>
        /// The build presets.
        /// </summary>
        [EditorOrder(5000), EditorDisplay("Presets", EditorDisplayAttribute.InlineStyle), Tooltip("Build presets configuration")]
        public BuildPreset[] Presets =
        {
            new BuildPreset
            {
                Name = "Development",
                Targets = new[]
                {
                    new BuildTarget
                    {
                        Name = "Windows",
                        Output = "Output\\Windows",
                        Platform = BuildPlatform.Windows64,
                        Mode = BuildConfiguration.Development,
                    },
                }
            },
            new BuildPreset
            {
                Name = "Release",
                Targets = new[]
                {
                    new BuildTarget
                    {
                        Name = "Windows",
                        Output = "Output\\Windows",
                        Platform = BuildPlatform.Windows64,
                        Mode = BuildConfiguration.Release,
                    },
                }
            },
        };

        /// <summary>
        /// Gets the preset of the given name (ignore case search) or returns null if cannot find it.
        /// </summary>
        /// <param name="name">The preset name.</param>
        /// <returns>Found preset or null if is missing.</returns>
        public BuildPreset GetPreset(string name)
        {
            if (Presets != null)
            {
                for (int i = 0; i < Presets.Length; i++)
                {
                    if (string.Equals(Presets[i].Name, name, StringComparison.OrdinalIgnoreCase))
                        return Presets[i];
                }
            }
            return null;
        }
#endif
    }
}
