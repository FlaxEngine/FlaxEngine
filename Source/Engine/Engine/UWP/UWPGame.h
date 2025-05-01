// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_UWP && !USE_EDITOR

#include "../Base/GameBase.h"

/// <summary>
/// The game class implementation for Universal Windows Platform platform.
/// </summary>
/// <seealso cref="Game" />
class UWPGame : public GameBase
{
public:

    // [GameBase]
    static void InitMainWindowSettings(CreateWindowSettings& settings);
};

typedef UWPGame Game;

#endif
