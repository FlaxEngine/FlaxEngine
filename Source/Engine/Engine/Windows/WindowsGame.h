// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WINDOWS && !USE_EDITOR

#include "../Base/GameBase.h"

/// <summary>
/// The game class implementation for Windows platform.
/// </summary>
/// <seealso cref="Game" />
class WindowsGame : public GameBase
{
public:

    // [GameBase]
    static void InitMainWindowSettings(CreateWindowSettings& settings);
    static bool Init();
    static void BeforeExit();
};

typedef WindowsGame Game;

#endif
