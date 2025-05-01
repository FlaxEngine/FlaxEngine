// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Plugin.h"

#if USE_EDITOR

/// <summary>
/// Base class for all plugins used in Editor.
/// </summary>
/// <remarks>
/// Plugins should have a public and parameter-less constructor.
/// </remarks>
/// <seealso cref="FlaxEngine.Plugin" />
API_CLASS(Abstract, Namespace="FlaxEditor") class FLAXENGINE_API EditorPlugin : public Plugin
{
    DECLARE_SCRIPTING_TYPE(EditorPlugin);
public:
    void Initialize() override;
    void Deinitialize() override;
};

#endif
