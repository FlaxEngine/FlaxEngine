// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "CoroutineSuspensionPointIndex.h"
#include "CoroutineSuspensionPointsFlags.h"
#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Scripting/ScriptingObjectReference.h"


API_CLASS(Sealed) class FLAXENGINE_API CoroutineRunnable final : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(CoroutineRunnable, ScriptingObject);

    API_FIELD()
    CoroutineSuspensionPointIndex ExecutionPoint;
    
    API_EVENT()
    Action OnRun;
};


API_CLASS(Sealed) class FLAXENGINE_API CoroutineBuilder final : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(CoroutineBuilder, ScriptingObject);

    API_FUNCTION()
    ScriptingObjectReference<CoroutineBuilder> ThenWaitSeconds(float seconds);

    API_FUNCTION()
    ScriptingObjectReference<CoroutineBuilder> ThenWaitFrames(int32 frames);

    API_FUNCTION()
    ScriptingObjectReference<CoroutineBuilder> ThenRun(ScriptingObjectReference<CoroutineRunnable> runnable);
};


API_CLASS(Sealed) class FLAXENGINE_API CoroutineExecutor final : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(CoroutineExecutor, ScriptingObject);

    API_FUNCTION()
    void Execute(ScriptingObjectReference<CoroutineBuilder> builder);
};
