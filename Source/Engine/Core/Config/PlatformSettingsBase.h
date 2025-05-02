// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Config/Settings.h"
#include "Engine/Serialization/SerializationFwd.h"

/// <summary>
/// Specifies the display mode of a game window.
/// </summary>
API_ENUM() enum class GameWindowMode
{
    /// <summary>
    /// The window has borders and does not take up the full screen.
    /// </summary>
    Windowed = 0,

    /// <summary>
    /// The window takes up the full screen exclusively.
    /// </summary>
    Fullscreen = 1,

    /// <summary>
    /// The window behaves like in Windowed mode but has no borders.
    /// </summary>
    Borderless = 2,

    /// <summary>
    /// Same as in Borderless, but is of the size of the screen.
    /// </summary>
    FullscreenBorderless = 3
};
