// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Input/Enums.h"

/// <summary>
/// Helper class to access display information.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API Screen
{
DECLARE_SCRIPTING_TYPE_NO_SPAWN(Screen);

    /// <summary>
    /// Gets the fullscreen mode.
    /// </summary>
    /// <returns>The value</returns>
    API_PROPERTY() static bool GetIsFullscreen();

    /// <summary>
    /// Sets the fullscreen mode.
    /// </summary>
    /// <remarks>
    /// A fullscreen mode switch may not happen immediately. It will be performed before next frame rendering.
    /// </remarks>
    /// <param name="value">The value.</param>
    API_PROPERTY() static void SetIsFullscreen(bool value);

    /// <summary>
    /// Gets the window size.
    /// </summary>
    /// <returns>The value</returns>
    API_PROPERTY() static Vector2 GetSize();

    /// <summary>
    /// Converts the screen-space position to the game viewport position.
    /// </summary>
    /// <param name="screenPos">The screen-space position.</param>
    /// <returns>The game viewport position.</returns>
    API_FUNCTION() static Vector2 ScreenToGameViewport(const Vector2& screenPos);

    /// <summary>
    /// Converts the game viewport position to the screen-space position.
    /// </summary>
    /// <param name="viewportPos">The game viewport position.</param>
    /// <returns>The screen-space position.</returns>
    API_FUNCTION() static Vector2 GameViewportToScreen(const Vector2& viewportPos);

    /// <summary>
    /// Sets the window size.
    /// </summary>
    /// <remarks>
    /// Resizing may not happen immediately. It will be performed before next frame rendering.
    /// </remarks>
    /// <param name="value">The value.</param>
    API_PROPERTY() static void SetSize(const Vector2& value);

    /// <summary>
    /// Gets the cursor visible flag.
    /// </summary>
    /// <returns>The value.</returns>
    API_PROPERTY() static bool GetCursorVisible();

    /// <summary>
    /// Sets the cursor visible flag.
    /// </summary>
    /// <param name="value">The value.</param>
    API_PROPERTY() static void SetCursorVisible(const bool value);

    /// <summary>
    /// Gets the cursor lock mode.
    /// </summary>
    /// <returns>The current cursor lock mode.</returns>
    API_PROPERTY() static CursorLockMode GetCursorLock();

    /// <summary>
    /// Sets the cursor lock mode.
    /// </summary>
    /// <param name="mode">The mode.</param>
    API_PROPERTY() static void SetCursorLock(CursorLockMode mode);
};
