// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"

/// <summary>
/// Build game header flags.
/// </summary>
enum class GameHeaderFlags
{
    None = 0x0,
    ShowSplashScreen = 0x01,
};

DECLARE_ENUM_OPERATORS(GameHeaderFlags);

#if !USE_EDITOR

#include "ApplicationBase.h"
#include "Engine/Platform/CreateWindowSettings.h"

/// <summary>
/// The main game class.
/// </summary>
/// <seealso cref="ApplicationBase" />
class GameBase : public ApplicationBase
{
public:

    /// <summary>
    /// Determines whether game is showing splash screen.
    /// </summary>
    /// <returns>True if splash screen is in use, otherwise false.</returns>
    static bool IsShowingSplashScreen();

    /// <summary>
    /// Initializes the main window settings.
    /// </summary>
    /// <param name="settings">The settings.</param>
    static void InitMainWindowSettings(CreateWindowSettings& settings);

public:

    // [ApplicationBase]
    static int32 LoadProduct();
    static Window* CreateMainWindow();
    static bool Init();
    static void BeforeRun();
    static void BeforeExit();
};

#endif
