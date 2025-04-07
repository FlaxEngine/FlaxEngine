// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/Version.h"
#include "Engine/Scripting/ScriptingType.h"

/// <summary>
/// The engine plugin description container.
/// </summary>
API_STRUCT() struct PluginDescription
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(PluginDescription);
public:
    /// <summary>
    /// The name of the plugin.
    /// </summary>
    API_FIELD() String Name;

    /// <summary>
    /// The version of the plugin.
    /// </summary>
    API_FIELD() Version Version;

    /// <summary>
    /// The name of the author of the plugin.
    /// </summary>
    API_FIELD() String Author;

    /// <summary>
    /// The plugin author website URL.
    /// </summary>
    API_FIELD() String AuthorUrl;

    /// <summary>
    /// The homepage URL for the plugin.
    /// </summary>
    API_FIELD() String HomepageUrl;

    /// <summary>
    /// The plugin repository URL (for open-source plugins).
    /// </summary>
    API_FIELD() String RepositoryUrl;

    /// <summary>
    /// The plugin description.
    /// </summary>
    API_FIELD() String Description;

    /// <summary>
    /// The plugin category.
    /// </summary>
    API_FIELD() String Category;

    /// <summary>
    /// True if plugin is during Beta tests (before release).
    /// </summary>
    API_FIELD() bool IsBeta = false;

    /// <summary>
    /// True if plugin is during Alpha tests (early version).
    /// </summary>
    API_FIELD() bool IsAlpha = false;
};
