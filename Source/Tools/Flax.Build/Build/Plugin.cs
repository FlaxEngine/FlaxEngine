// Copyright (c) Wojciech Figat. All rights reserved.

namespace Flax.Build
{
    /// <summary>
    /// Defines a build system plugin that can customize building process or implement a custom functionalities like scripting language integration or automated process for game project.
    /// </summary>
    public class Plugin
    {
        /// <summary>
        /// The plugin name.
        /// </summary>
        public string Name;

        /// <summary>
        /// Initializes a new instance of the <see cref="Plugin"/> class.
        /// </summary>
        public Plugin()
        {
            var type = GetType();
            Name = type.Name;

            if (Configuration.PrintPlugins)
            {
                Log.Info("Plugin: " + Name);
            }
        }

        /// <summary>
        /// Initializes the plugin.
        /// </summary>
        public virtual void Init()
        {
        }
    }
}
