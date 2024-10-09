// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "CoroutineBuilder.h"


ScriptingObjectReference<CoroutineBuilder> CoroutineBuilder::ThenWaitSeconds(float seconds)
{
    return this; //TODO(mtszkarbowiak) Implement waiting for seconds.
}

ScriptingObjectReference<CoroutineBuilder> CoroutineBuilder::ThenWaitFrames(int32 frames)
{
    return this; //TODO(mtszkarbowiak) Implement waiting for frames.
}

ScriptingObjectReference<CoroutineBuilder> CoroutineBuilder::ThenRun(ScriptingObjectReference<CoroutineRunnable> runnable)
{
    return this; //TODO(mtszkarbowiak) Implement running a coroutine.
}


void CoroutineExecutor::Execute(ScriptingObjectReference<CoroutineBuilder> builder)
{
    //TODO(mtszkarbowiak) Implement executing a coroutine.
}
