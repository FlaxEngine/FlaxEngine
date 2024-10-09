// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "CoroutineBuilder.h"


ScriptingObjectReference<CoroutineBuilder> CoroutineBuilder::ThenRun(ScriptingObjectReference<CoroutineRunnable> runnable)
{
    return this; //TODO(mtszkarbowiak) Implement running a coroutine.
}

ScriptingObjectReference<CoroutineBuilder> CoroutineBuilder::ThenWaitSeconds(const float seconds)
{
    return this; //TODO(mtszkarbowiak) Implement waiting for seconds.
}

ScriptingObjectReference<CoroutineBuilder> CoroutineBuilder::ThenWaitFrames(int32 frames)
{
    return this; //TODO(mtszkarbowiak) Implement waiting for frames.
}

ScriptingObjectReference<CoroutineBuilder> CoroutineBuilder::ThenWaitUntil(ScriptingObjectReference<CoroutinePredicate> predicate)
{
    return this; //TODO(mtszkarbowiak) Implement waiting until predicate.
}
