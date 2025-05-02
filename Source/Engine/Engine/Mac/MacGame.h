// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_MAC && !USE_EDITOR

#include "../Base/GameBase.h"

/// <summary>
/// The game class implementation for Mac platform.
/// </summary>
/// <seealso cref="Game" />
class MacGame : public GameBase
{
public:

    // [GameBase]
    static void InitMainWindowSettings(CreateWindowSettings& settings);
};

typedef MacGame Game;

#endif
