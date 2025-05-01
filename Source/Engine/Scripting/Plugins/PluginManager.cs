// Copyright (c) Wojciech Figat. All rights reserved.

namespace FlaxEngine
{
    partial class PluginManager
    {
        /// <summary>
        /// Returns the first plugin of the provided type.
        /// </summary>
        /// <typeparam name="T">The plugin type.</typeparam>
        /// <returns>The plugin, or null if not loaded.</returns>
        public static T GetPlugin<T>() where T : Plugin
        {
            return (T)GetPlugin(typeof(T));
        }
    }
}
