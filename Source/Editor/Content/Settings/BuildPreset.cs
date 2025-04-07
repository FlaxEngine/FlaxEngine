// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using FlaxEngine;

namespace FlaxEditor.Content.Settings
{
    /// <summary>
    /// A <see cref="GameCooker"/> game building preset with set of build targets.
    /// </summary>
    [Serializable]
    public class BuildPreset
    {
        /// <summary>
        /// The name of the preset.
        /// </summary>
        [EditorOrder(10), Tooltip("Name of the preset")]
        public string Name;

        /// <summary>
        /// The target configurations.
        /// </summary>
        [EditorOrder(20), Tooltip("Target configurations")]
        public BuildTarget[] Targets;

        /// <summary>
        /// Gets the target of the given name (ignore case search) or returns null if cannot find it.
        /// </summary>
        /// <param name="name">The target name.</param>
        /// <returns>Found target or null if is missing.</returns>
        public BuildTarget GetTarget(string name)
        {
            if (Targets != null)
            {
                for (int i = 0; i < Targets.Length; i++)
                {
                    if (string.Equals(Targets[i].Name, name, StringComparison.OrdinalIgnoreCase))
                        return Targets[i];
                }
            }
            return null;
        }
    }
}
