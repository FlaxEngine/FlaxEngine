// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "PluginDescription.h"
#include "Engine/Scripting/ScriptingObject.h"

/// <summary>
/// Base class for game engine editor plugins.
/// </summary>
API_CLASS(Abstract) class FLAXENGINE_API Plugin : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE(Plugin);
private:
    friend class PluginManagerService;
    bool _initialized = false;

protected:
    /// <remarks>
    /// Plugin description. Should be a constant part of the plugin created in constructor and valid before calling <see cref="Initialize"/>.
    /// </remarks>
    API_FIELD() PluginDescription _description;

public:
    /// <summary>
    /// Gets the description.
    /// </summary>
    API_PROPERTY() const PluginDescription& GetDescription() const
    {
        return _description;
    }

    /// <summary>
    /// Initialization method called when this plugin is loaded to the memory and can be used.
    /// </summary>
    API_FUNCTION() virtual void Initialize()
    {
    }

    /// <summary>
    /// Cleanup method called when this plugin is being unloaded or reloaded or engine is closing.
    /// </summary>
    API_FUNCTION() virtual void Deinitialize()
    {
    }
};
