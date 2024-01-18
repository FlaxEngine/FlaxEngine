// Writen by Nori_SC
#pragma once
#include "Engine/Scripting/ScriptingObject.h"

/// <summary>
/// Visability flags
/// </summary>
API_ENUM(Namespace = "FlaxEngine.Experimental.UI")
enum UIComponentVisibility
{
    //-------------DONT EDIT VALUES OR ADD ANY----------
    //it will break the serialisation
    //the UI component is packing it in to one int value
    //--------------------------------------------------


    /// <summary>
    /// Visable on the screen
    /// </summary>
    Visible = 0,
    /// <summary>
    /// Hiden on the screen
    /// </summary>
    Hiden = 1,
    /// <summary>
    /// if Events will fire on this 
    /// </summary>
    HitSelf = 2,
    /// <summary>
    /// if Events will fire on children
    /// </summary>
    HitChildren = 4,
    /// <summary>
    /// not visible not hittable and takes up no space in the UI
    /// </summary>
    Collapsed = 8
};
