// Writen by Nori_SC
#pragma once
#include "Engine/Scripting/ScriptingObject.h"

/// <summary>
/// Visability flags
/// </summary>
API_ENUM(Namespace = "FlaxEngine.Experimental.UI")
enum UIComponentVisibility
{
    /// <summary>
    /// Visable on the screen
    /// </summary>
    Visible = 1,
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
    Collapsed = 16
};
