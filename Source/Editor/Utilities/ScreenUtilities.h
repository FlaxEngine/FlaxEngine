// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Math/Color32.h"
#include "Engine/Core/Math/Vector2.h"
#include "Engine/Core/Delegate.h"

/// <summary>
/// Platform-dependent screen utilities.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API ScreenUtilities
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(ScreenUtilities);

    /// <summary>
    /// Gets the pixel color at the specified coordinates.
    /// </summary>
    /// <param name="pos">Screen-space coordinate to read.</param>
    /// <returns>Pixel color at the specified coordinates.</returns>
    API_FUNCTION() static Color32 GetColorAt(const Float2& pos);

    /// <summary>
    /// Starts async color picking. Color will be returned through PickColorDone event when the actions ends (user selected the final color with a mouse). When action is active, GetColorAt can be used to read the current value.
    /// </summary>
    API_FUNCTION() static void PickColor();

    /// <summary>
    /// Called when PickColor action is finished.
    /// </summary>
    API_EVENT() static Delegate<Color32> PickColorDone;
};
