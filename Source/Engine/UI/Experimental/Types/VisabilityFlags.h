#pragma once
/// <summary>
/// Visability flags
/// </summary>
API_ENUM(Attribute = "Flags")
enum VisabilityFlags
{
    /// <summary>
    /// Visable on the screen
    /// </summary>
    Visable = 1,
    /// <summary>
    /// Hiden on the screen
    /// </summary>
    Hiden = 2,
    /// <summary>
    /// if Events will fire on this 
    /// </summary>
    HitSelf = 4,
    /// <summary>
    /// if Events will fire on children
    /// </summary>
    HitChildren = 8,
    /// <summary>
    /// not visible not hittable and takes up no space in the UI
    /// </summary>
    Colapsed = 16
};
