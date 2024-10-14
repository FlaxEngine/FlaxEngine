// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "CoroutineSuspendPoint.h"
#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Types/Variant.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Scripting/ScriptingObjectReference.h"


/// <summary>
/// Reference to a coroutine that can be used to control its execution.
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API CoroutineHandle final : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(CoroutineHandle, ScriptingObject);

    // API_FUNCTION()
    // bool TryCancel();
    // 
    // API_FUNCTION()
    // bool TryPause();
    // 
    // API_FUNCTION()
    // bool TryResume();

    API_FIELD()
    uint64 ID = 0;
};
