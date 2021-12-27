// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_MAC && !USE_EDITOR

#include "../Base/GameBase.h"

/// <summary>
/// The game class implementation for Mac platform.
/// </summary>
/// <seealso cref="Game" />
class MacGame : public GameBase
{
};

typedef MacGame Game;

#endif
