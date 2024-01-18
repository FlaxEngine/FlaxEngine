//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#pragma once
#include "Engine/Scripting/ScriptingObject.h"

/// <summary>
/// Clipping Flags
/// </summary>
API_ENUM(Namespace = "FlaxEngine.Experimental.UI")
enum FLAXENGINE_API UIComponentClipping
{
    //-------------DONT EDIT VALUES OR ADD ANY----------
    //it will break the serialisation
    //the UI component is packing it in to one int value
    //--------------------------------------------------


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
