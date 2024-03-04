//                                                        UI System
//                                                    writen by Nori_SC
//                                                https://github.com/NoriteSC
#pragma once
#include "Engine/Scripting/ScriptingObject.h"

#ifdef USE_EDITOR
/// <summary>
/// The design flags use by editor
/// </summary>
API_ENUM(Namespace = "FlaxEngine.Experimental.UI.Editor")
enum class FLAXENGINE_API UIComponentDesignFlags : byte
{
    /// <summary>
    /// no flags are is set
    /// </summary>
    None = 0,
    /// <summary>
    /// this flag is set when UI is being edited
    /// </summary>
    Designing = 1 << 0,
    /// <summary>
    /// flag for showing outline
    /// </summary>
    ShowOutline = 1 << 1,
    /// <summary>
    /// this flag is set when UI editor is runing preconstruct on UI elements
    /// </summary>
    ExecutePreConstruct = 1 << 2,
    /// <summary>
    /// Preview flag it is set when is in Preview mode ak the game has started or is rendered to thumnaill, and the Designing is locked
    /// </summary>
    Previewing = 1 << 3
};
DECLARE_ENUM_OPERATORS(UIComponentDesignFlags)

#endif // USE_EDITOR
