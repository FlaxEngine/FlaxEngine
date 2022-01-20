// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Base class for game engine editor plugins.
    /// </summary>
    /// <remarks>
    /// Plugins should have a public and parameter-less constructor.
    /// </remarks>
    public abstract class Plugin
    {
        internal bool _initialized;

        /// <summary>
        /// Gets the description.
        /// </summary>
        /// <remarks>
        /// Plugin description should be a constant part of the plugin created in constructor and valid before calling <see cref="Initialize"/>.
        /// </remarks>
        public virtual PluginDescription Description => new PluginDescription
        {
            Name = GetType().Name,
            Category = "Other",
            Version = new Version(1, 0),
        };

        /// <summary>
        /// Initialization method called when this plugin is loaded to the memory and can be used.
        /// </summary>
        public virtual void Initialize()
        {
        }

        /// <summary>
        /// Cleanup method called when this plugin is being unloaded or reloaded or engine is closing.
        /// </summary>
        public virtual void Deinitialize()
        {
        }
    }
}
