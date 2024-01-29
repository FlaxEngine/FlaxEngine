//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#pragma once
#include "Engine/Scripting/ScriptingObject.h"

/// <summary>
/// Visability flags
/// </summary>
API_ENUM(Namespace = "FlaxEngine.Experimental.UI", Attributes = "System.Flags")
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
    /// Ignores the raycast on self
    /// </summary>
    IgnoreRaycastSelf = 2,
    /// <summary>
    /// Ignres raycast on choldren
    /// </summary>
    IgnoreRaycastChildren = 4,
    /// <summary>
    /// takes up no space in the UI [Hiden | IgnoreRaycastSelf | IgnoreRaycastChildren]
    /// </summary>
    Collapsed = Hiden | IgnoreRaycastSelf | IgnoreRaycastChildren
};
