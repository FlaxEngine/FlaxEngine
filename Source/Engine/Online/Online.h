// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Delegate.h"
#include "Engine/Scripting/ScriptingType.h"

class IOnlinePlatform;

/// <summary>
/// The online system for communicating with various multiplayer services such as player info, achievements, game lobby or in-game store.
/// </summary>
API_CLASS(Static, Namespace="FlaxEngine.Online") class FLAXENGINE_API Online
{
    DECLARE_SCRIPTING_TYPE_NO_SPAWN(Online);
public:
    /// <summary>
    /// The current online platform.
    /// </summary>
    API_FIELD(ReadOnly) static IOnlinePlatform* Platform;

    /// <summary>
    /// Event called when current online platform gets changed.
    /// </summary>
    API_EVENT() static Action PlatformChanged;

public:
    /// <summary>
    /// Initializes the online system with a given online platform implementation.
    /// </summary>
    /// <remarks>Destroys the current platform (in any already in-use).</remarks>
    /// <param name="platform">The online platform object.</param>
    /// <returns>True if failed, otherwise false.</returns>
    API_FUNCTION() static bool Initialize(IOnlinePlatform* platform);
};
