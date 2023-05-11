#pragma once

#include "Engine/Core/Types/BaseTypes.h"

API_INJECT_CODE(cpp, "#include \"Engine/Platform/ScreenUtils.h\"");

/// <summary>
/// Platform-dependent screen utilties.
/// </summary>
API_CLASS(Static, Name = "ScreenUtils", Tag = "NativeInvokeUseName")
class FLAXENGINE_API ScreenUtilsBase
{
public:
    static struct FLAXENGINE_API ScriptingTypeInitializer TypeInitializer;

    /// <summary>
    /// Gets the pixel color at the specified coordinates.
    /// </summary>
    /// <param name="x">X Coordinate to read.</param>
    /// <param name="y">Y Coordinate to read.</param>
    /// <returns>Pixel color at the specified coordinates.</returns>
    API_FUNCTION() static Color32 GetPixelAt(int32 x, int32 y);

    /// <summary>
    /// Gets the cursor position, in screen cooridnates.
    /// </summary>
    /// <returns>Cursor position, in screen coordinates.</returns>
    API_FUNCTION() static Int2 GetScreenCursorPosition();

    /// <summary>
    /// Blocks mouse input and runs a callback
    /// </summary>
    API_FUNCTION() static void BlockAndReadMouse();
};
