// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Reflection;

namespace FlaxEngine
{
    /// <summary>
    /// Plugin related event delegate type.
    /// </summary>
    /// <param name="plugin">The plugin.</param>
    [HideInEditor]
    public delegate void PluginDelegate(Plugin plugin);

    public static partial class PluginManager
    {
        private static readonly List<GamePlugin> _gamePlugins = new List<GamePlugin>();
        private static readonly List<Plugin> _editorPlugins = new List<Plugin>();

        /// <summary>
        /// Gets the game plugins.
        /// </summary>
        public static IReadOnlyList<GamePlugin> GamePlugins => _gamePlugins;

        /// <summary>
        /// Gets the editor plugins.
        /// </summary>
        public static IReadOnlyList<Plugin> EditorPlugins => _editorPlugins;

        /// <summary>
        /// Occurs before loading plugin.
        /// </summary>
        public static event PluginDelegate PluginLoading;

        /// <summary>
        /// Occurs when plugin gets loaded and initialized.
        /// </summary>
        public static event PluginDelegate PluginLoaded;

        /// <summary>
        /// Occurs before unloading plugin.
        /// </summary>
        public static event PluginDelegate PluginUnloading;

        /// <summary>
        /// Occurs when plugin gets unloaded. It should not be used anymore.
        /// </summary>
        public static event PluginDelegate PluginUnloaded;

        /// <summary>
        /// Occurs when plugins collection gets edited (added or removed plugin).
        /// </summary>
        public static event Action PluginsChanged;

        /// <summary>
        /// Determines whether can load the specified plugin.
        /// </summary>
        /// <param name="pluginDesc">The plugin description.</param>
        /// <returns>True if load it, otherwise false.</returns>
        public delegate bool CanLoadPluginDelegate(ref PluginDescription pluginDesc);

        /// <summary>
        /// Determines whether can load the specified plugin.
        /// </summary>
        public static CanLoadPluginDelegate CanLoadPlugin = DefaultCanLoadPlugin;

        /// <summary>
        /// Plugin related event delegate type.
        /// </summary>
        /// <param name="plugin">The plugin.</param>
        public delegate void PluginDelegate(Plugin plugin);

        /// <summary>
        /// The default implementation for <see cref="CanLoadPlugin"/>.
        /// </summary>
        /// <param name="pluginDesc">The plugin description.</param>
        /// <returns>True if load it, otherwise false.</returns>
        public static bool DefaultCanLoadPlugin(ref PluginDescription pluginDesc)
        {
            return true;
            //return !pluginDesc.DisabledByDefault;
        }

#if FLAX_EDITOR
        internal static void InitializeGamePlugins()
        {
            Profiler.BeginEvent("PluginManager.InitializeGamePlugins");
            for (var i = 0; i < _gamePlugins.Count; i++)
            {
                InvokeInitialize(_gamePlugins[i]);
            }
            Profiler.EndEvent();
        }

        internal static void DeinitializeGamePlugins()
        {
            Profiler.BeginEvent("PluginManager.DeinitializeGamePlugins");
            for (var i = _gamePlugins.Count - 1; i >= 0; i--)
            {
                InvokeDeinitialize(_gamePlugins[i]);
            }
            Profiler.EndEvent();
        }
#endif

        private static void InvokeInitialize(Plugin plugin)
        {
            if (plugin._initialized)
                return;

            try
            {
                Debug.Write(LogType.Info, "Loading plugin " + plugin);

                PluginLoading?.Invoke(plugin);

                plugin.Initialize();
                plugin._initialized = true;

                PluginLoaded?.Invoke(plugin);
            }
            catch (Exception ex)
            {
                Debug.LogException(ex);
                Debug.LogErrorFormat("Failed to initialize plugin {0}. {1}", plugin, ex.Message);
            }
        }

        private static void InvokeDeinitialize(Plugin plugin)
        {
            if (!plugin._initialized)
                return;

            try
            {
                Debug.Write(LogType.Info, "Unloading plugin " + plugin);

                PluginUnloading?.Invoke(plugin);

                plugin.Deinitialize();
                plugin._initialized = false;

                PluginUnloaded?.Invoke(plugin);
            }
            catch (Exception ex)
            {
                Debug.LogException(ex);
                Debug.LogErrorFormat("Failed to deinitialize plugin {0}. {1}", plugin, ex.Message);
            }
        }

        /// <summary>
        /// Returns the first plugin of the provided type.
        /// </summary>
        /// <typeparam name="T">The plugin type.</typeparam>
        /// <returns>The plugin, or null if not loaded.</returns>
        public static T GetPlugin<T>() where T : class
        {
            foreach (Plugin p in _editorPlugins)
            {
                if (p.GetType() == typeof(T))
                    return (T)Convert.ChangeType(p, typeof(T));
            }

            foreach (GamePlugin gp in _gamePlugins)
            {
                if (gp.GetType() == typeof(T))
                    return (T)Convert.ChangeType(gp, typeof(T));
            }

            return null;
        }

        /// <summary>
        /// Return the first plugin using the provided name.
        /// </summary>
        /// /// <param name="name">The plugin name.</param>
        /// <returns>The plugin, or null if not loaded.</returns>
        public static Plugin GetPlugin(string name)
        {
            foreach (Plugin p in _editorPlugins)
            {
                if (p.Description.Name.Equals(name))
                    return p;
            }

            foreach (GamePlugin gp in _gamePlugins)
            {
                if (gp.Description.Name.Equals(name))
                    return gp;
            }

            return null;
        }

        internal static void Internal_LoadPlugin(Type type, bool isEditor)
        {
            if (type == null)
                throw new ArgumentNullException();

            // Create and check if use it
            var plugin = (Plugin)Activator.CreateInstance(type);
            var desc = plugin.Description;
            if (!CanLoadPlugin(ref desc))
            {
                Debug.Write(LogType.Info, "Skip loading plugin " + plugin);
                return;
            }

            if (!isEditor)
            {
                _gamePlugins.Add((GamePlugin)plugin);
#if !FLAX_EDITOR
                InvokeInitialize(plugin);
#endif
            }
#if FLAX_EDITOR
            else
            {
                _editorPlugins.Add(plugin);
                InvokeInitialize(plugin);
            }
#endif
            PluginsChanged?.Invoke();
        }

        internal static void Internal_Dispose(Assembly assembly)
        {
            bool changed = false;

            for (int i = _editorPlugins.Count - 1; i >= 0 && _editorPlugins.Count > 0; i--)
            {
                var plugin = _editorPlugins[i];
                if (plugin.GetType().Assembly == assembly)
                {
                    InvokeDeinitialize(plugin);
                    _editorPlugins.RemoveAt(i);
                    changed = true;
                }
            }

            for (int i = _gamePlugins.Count - 1; i >= 0 && _gamePlugins.Count > 0; i--)
            {
                var plugin = _gamePlugins[i];
                if (plugin.GetType().Assembly == assembly)
                {
                    InvokeDeinitialize(plugin);
                    _gamePlugins.RemoveAt(i);
                    changed = true;
                }
            }

            if (changed)
                PluginsChanged?.Invoke();
        }

        internal static void Internal_Dispose()
        {
            int pluginsCount = _editorPlugins.Count + _gamePlugins.Count;
            if (pluginsCount == 0)
                return;
            Debug.Write(LogType.Info, string.Format("Unloading {0} plugins", pluginsCount));

            for (int i = _editorPlugins.Count - 1; i >= 0 && _editorPlugins.Count > 0; i--)
            {
                var plugin = _editorPlugins[i];
                InvokeDeinitialize(plugin);
                _editorPlugins.RemoveAt(i);
            }

            for (int i = _gamePlugins.Count - 1; i >= 0 && _gamePlugins.Count > 0; i--)
            {
                var plugin = _gamePlugins[i];
                InvokeDeinitialize(plugin);
                _gamePlugins.RemoveAt(i);
            }

            PluginsChanged?.Invoke();
        }
    }
}
