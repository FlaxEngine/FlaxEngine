// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_LINUX && !USE_EDITOR

#include "../Base/GameBase.h"

/// <summary>
/// The game class implementation for Linux platform.
/// </summary>
/// <seealso cref="Game" />
class LinuxGame : public GameBase
{
public:

    // [GameBase]
    static void InitMainWindowSettings(CreateWindowSettings& settings);
    static Window* CreateMainWindow();
    static bool Init();
};

typedef LinuxGame Game;

#endif
