// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_ANDROID && !USE_EDITOR

#include "../Base/GameBase.h"

/// <summary>
/// The game class implementation for Android platform.
/// </summary>
/// <seealso cref="Game" />
class AndroidGame : public GameBase
{
};

typedef AndroidGame Game;

#endif
