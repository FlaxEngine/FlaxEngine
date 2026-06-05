// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WEB

#include "../Base/GameBase.h"

/// <summary>
/// The game class implementation for Web platform.
/// </summary>
/// <seealso cref="Game" />
class WebGame : public GameBase
{
public:
    // [GameBase]
    static void InitMainWindowSettings(CreateWindowSettings& settings);
};

typedef WebGame Game;

#endif
