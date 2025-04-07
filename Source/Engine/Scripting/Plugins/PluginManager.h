// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "GamePlugin.h"

/// <summary>
/// Game and Editor plugins management service.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API PluginManager
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(PluginManager);
public:
    /// <summary>
    /// Gets the game plugins.
    /// </summary>
    API_PROPERTY() static const Array<GamePlugin*>& GetGamePlugins();

    /// <summary>
    /// Gets the editor plugins.
    /// </summary>
    API_PROPERTY() static const Array<Plugin*>& GetEditorPlugins();

public:
    /// <summary>
    /// Occurs before loading plugin.
    /// </summary>
    API_EVENT() static Delegate<Plugin*> PluginLoading;

    /// <summary>
    /// Occurs when plugin gets loaded and initialized.
    /// </summary>
    API_EVENT() static Delegate<Plugin*> PluginLoaded;

    /// <summary>
    /// Occurs before unloading plugin.
    /// </summary>
    API_EVENT() static Delegate<Plugin*> PluginUnloading;

    /// <summary>
    /// Occurs when plugin gets unloaded. It should not be used anymore.
    /// </summary>
    API_EVENT() static Delegate<Plugin*> PluginUnloaded;

    /// <summary>
    /// Occurs when plugins collection gets edited (added or removed plugin).
    /// </summary>
    API_EVENT() static Action PluginsChanged;

public:
    /// <summary>
    /// Return the first plugin using the provided name.
    /// </summary>
    // <param name="name">The plugin name.</param>
    /// <returns>The plugin, or null if not loaded.</returns>
    API_FUNCTION() static Plugin* GetPlugin(const StringView& name);

    /// <summary>
    /// Returns the first plugin of the provided type.
    /// </summary>
    /// <param name="type">Type of the plugin to search for. Includes any plugin base class derived from the type.</param>
    /// <returns>The plugin or null.</returns>
    API_FUNCTION() static Plugin* GetPlugin(API_PARAM(Attributes="TypeReference(typeof(Plugin))") const MClass* type);

    /// <summary>
    /// Returns the first plugin of the provided type.
    /// </summary>
    /// <param name="type">Type of the plugin to search for. Includes any plugin base class derived from the type.</param>
    /// <returns>The plugin or null.</returns>
    static Plugin* GetPlugin(const ScriptingTypeHandle& type);

    /// <summary>
    /// Returns the first plugin of the provided type.
    /// </summary>
    /// <returns>The plugin or null.</returns>
    template<typename T>
    FORCE_INLINE static T* GetPlugin()
    {
        return (T*)GetPlugin(T::TypeInitializer);
    }

private:
#if USE_EDITOR
    // Internal bindings
    API_FUNCTION(NoProxy) static void InitializeGamePlugins();
    API_FUNCTION(NoProxy) static void DeinitializeGamePlugins();
#endif
};
