// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Plugin.h"
#include "Engine/Core/Collections/Array.h"

/// <summary>
/// Base class for all plugins used at runtime in game.
/// </summary>
API_CLASS(Abstract) class FLAXENGINE_API GamePlugin : public Plugin
{
    DECLARE_SCRIPTING_TYPE(GamePlugin);
public:
#if USE_EDITOR
    /// <summary>
    /// Function called during game cooking in Editor to collect any assets that this plugin uses. Can be used to inject content for plugins.
    /// </summary>
    /// <returns>The result assets list.</returns>
    API_FUNCTION() virtual Array<Guid> GetReferences() const
    {
        return Array<Guid>();
    }
#endif
};
