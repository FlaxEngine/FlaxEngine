using System;

namespace FlaxEngine;

/// <summary>
/// This attribute allows for specifying initialization and deinitialization order for plugins
/// </summary>
[Serializable]
[AttributeUsage(AttributeTargets.Class)]
public class PluginLoadOrderAttribute : Attribute
{
    /// <summary>
    /// The plugin type to initialize this plugin after.
    /// </summary>
    public Type InitializeAfter;

    /// <summary>
    /// The plugin type to deinitialize this plugin before.
    /// </summary>
    public Type DeinitializeBefore;
}
