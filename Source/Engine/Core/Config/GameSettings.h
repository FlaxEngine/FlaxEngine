// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/Guid.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Collections/Dictionary.h"

/// <summary>
/// Main engine configuration service. Loads and applies game configuration.
/// </summary>
class GameSettings
{
public:

    /// <summary>
    /// The product full name.
    /// </summary>
    static String ProductName;

    /// <summary>
    /// The company full name.
    /// </summary>
    static String CompanyName;

    /// <summary>
    /// The copyright note used for content signing (eg. source code header).
    /// </summary>
    static String CopyrightNotice;

    /// <summary>
    /// The default application icon.
    /// </summary>
    static Guid Icon;

    /// <summary>
    /// Reference to the first scene to load on a game startup.
    /// </summary>
    static Guid FirstScene;

    /// <summary>
    /// True if skip showing splash screen image on the game startup.
    /// </summary>
    static bool NoSplashScreen;

    /// <summary>
    /// Reference to the splash screen image to show on a game startup.
    /// </summary>
    static Guid SplashScreen;

    /// <summary>
    /// The custom settings to use with a game. Can be specified by the user to define game-specific options and be used by the external plugins (used as key-value pair).
    /// </summary>
    static Dictionary<String, Guid> CustomSettings;

    /// <summary>
    /// Loads the game settings (including other settings such as Physics, Input, etc.).
    /// </summary>
    /// <returns>True if failed, otherwise false.</returns>
    static bool Load();
};
