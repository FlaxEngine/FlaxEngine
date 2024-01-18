// Writen by Nori_SC
#pragma once
#include "Engine/Scripting/ScriptingObject.h"

#ifdef USE_EDITOR
/// <summary>
/// The design flags use by editor
/// </summary>
API_ENUM(Namespace = "FlaxEngine.Experimental.UI.Editor")
enum class FLAXENGINE_API UIComponentDesignFlags
{
    None = 0,
    Designing = 1 << 0,
    ShowOutline = 1 << 1,
    ExecutePreConstruct = 1 << 2,
    Previewing = 1 << 3
};
#endif // USE_EDITOR
