// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Platform/Thread.h"
#include "Threading.h"
#include "IRunnable.h"

/// <summary>
/// Helper class to spawn custom thread for performing long-time action. Don't use it for short tasks.
/// </summary>
class FLAXENGINE_API ThreadSpawner
{
public:

    /// <summary>
    /// Starts a new thread the specified callback.
    /// </summary>
    /// <param name="callback">The callback function.</param>
    /// <param name="threadName">Name of the thread.</param>
    /// <param name="priority">The thread priority.</param>
    /// <returns>The created thread.</returns>
    static Thread* Start(const Function<int32()>& callback, const String& threadName, ThreadPriority priority = ThreadPriority::Normal)
    {
        auto runnable = New<SimpleRunnable>(true);
        runnable->OnWork = callback;
        return Thread::Create(runnable, threadName, priority);
    }
};
