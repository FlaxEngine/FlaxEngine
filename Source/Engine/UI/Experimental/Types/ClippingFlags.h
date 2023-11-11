#pragma once
/// <summary>
/// Clipping Flags
/// </summary>
API_ENUM(Namespace = "FlaxEngine.Experimental.UI")
enum ClippingFlags
{
    /// <summary>
    /// Keeps the parent flags
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
