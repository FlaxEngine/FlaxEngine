// Writen by Nori_SC
#pragma once
#include "Engine/Scripting/ScriptingObject.h"

/// <summary>
/// Clipping Flags
/// </summary>
API_ENUM(Namespace = "FlaxEngine.Experimental.UI")
enum UIComponentClipping
{
    /// <summary>
    /// Keeps the the last clipping space
    /// </summary>
    Inherit,
    /// <summary>
    /// Ignore clipping
    /// </summary>
    DontClip,
    /// <summary>
    /// Clip children to this element to the to size
    /// </summary>
    ClipToBounds,
};
