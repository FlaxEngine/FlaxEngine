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


    /// <summary>
    /// Checks if the coroutine's execution is still present in the executor.
    /// </summary>
    /// <returns>
    /// <c>false</c> if the coroutine is still being executed, even if paused.
    /// </returns>
    /// <remarks>
    /// No method <c>IsRunning</c> is provided because it would create ambiguity with pause and finish states.
    /// </remarks>
    API_PROPERTY()
    bool HasFinished() const;

    /// <summary>
    /// Checks if the coroutine is currently paused.
    /// </summary>
    /// <returns> <c>true</c> if the coroutine is paused. </returns>
    /// <remarks>
    /// If the coroutine is not being executed, this method will return <c>false</c>.
    /// No method <c>IsRunning</c> is provided because it would create ambiguity with pause and finish states.
    /// </remarks>
    API_PROPERTY()
    bool IsPaused() const;


    /// <summary>
    /// Requests the origin executor to cancel the coroutine.
    /// </summary>
    /// <returns>
    /// <c>true</c> if the coroutine was canceled.
    /// In any other case, including the coroutine already being finished, <c>false</c> is returned.
    /// </returns>
    API_FUNCTION()
    bool Cancel();

    /// <summary>
    /// Requests the origin executor to pause the coroutine.
    /// </summary>
    /// <returns>
    /// <c>true</c> if the coroutine was canceled.
    /// In any other case, including the coroutine already being finished, <c>false</c> is returned.
    /// </returns>
    API_FUNCTION()
    bool Pause();

    /// <summary>
    /// Requests the origin executor to resume the coroutine.
    /// </summary>
    /// <returns>
    /// <c>true</c> if the coroutine was canceled.
    /// In any other case, including the coroutine already being finished, <c>false</c> is returned.
    /// </returns>
    API_FUNCTION()
    bool Resume();


    /// <summary>
    /// Unique identifier of the coroutine execution instance.
    /// </summary>
    API_FIELD(ReadOnly)
    uint64 ExecutionID = 0;

    /// <summary>
    /// The executor that is responsible for the coroutine.
    /// </summary>
    /// <remarks>
    /// Do not change this value manually with native script.
    /// </remarks>
    API_FIELD(ReadOnly)
    ScriptingObjectReference<class CoroutineExecutor> Executor;
};
