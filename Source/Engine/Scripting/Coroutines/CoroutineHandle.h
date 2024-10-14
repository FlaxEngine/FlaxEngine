// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Scripting/ScriptingObjectReference.h"

/// <summary>
/// Reference to a coroutine that can be used to control its execution.
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API CoroutineHandle final : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(CoroutineHandle, ScriptingObject);


    API_FUNCTION()
    bool Cancel();
    
    API_FUNCTION()
    bool Pause();
    
    API_FUNCTION()
    bool Resume();


    API_FIELD(ReadOnly)
    uint64 ID = 0;

    API_FIELD(ReadOnly)
    ScriptingObjectReference<class CoroutineExecutor> Executor;
};
