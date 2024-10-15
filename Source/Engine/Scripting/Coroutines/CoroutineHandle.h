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
    /// Requests the origin executor to cancel the coroutine.
    /// </summary>
    /// <returns> <c>true</c> if the coroutine was canceled; otherwise, <c>false</c>. </returns>
    /// <remarks> Effect of this method is idempotent. </remarks>
    API_FUNCTION()
    bool Cancel();

    /// <summary>
    /// Requests the origin executor to pause the coroutine.
    /// </summary>
    /// <returns> <c>true</c> if the coroutine was paused; otherwise, <c>false</c>. </returns>
    /// <remarks> Effect of this method is idempotent. </remarks>
    API_FUNCTION()
    bool Pause();

    /// <summary>
    /// Requests the origin executor to resume the coroutine.
    /// </summary>
    /// <returns> <c>true</c> if the coroutine was resumed; otherwise, <c>false</c>. </returns>
    /// <remarks> Effect of this method is idempotent. </remarks>
    API_FUNCTION()
    bool Resume();

    //TODO(mtszkarbowiak) Add check if the coroutine is still running.


    /// <summary>
    /// Unique identifier of the coroutine execution instance.
    /// </summary>
    API_FIELD(ReadOnly)
    uint64 ExecutionID = 0;

    /// <summary>
    /// The executor that is responsible for the coroutine.
    /// </summary>
    API_FIELD(ReadOnly)
    ScriptingObjectReference<class CoroutineExecutor> Executor;
};
