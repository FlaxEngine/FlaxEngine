// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WEB

#include "../Base/ThreadBase.h"

/// <summary>
/// Thread object for Web platform.
/// </summary>
class FLAXENGINE_API WebThread : public ThreadBase
{
public:
    /// <summary>
    /// Initializes a new instance of the <see cref="WebThreadThread"/> class.
    /// </summary>
    /// <param name="runnable">The runnable.</param>
    /// <param name="name">The thread name.</param>
    /// <param name="priority">The thread priority.</param>
    WebThread(IRunnable* runnable, const String& name, ThreadPriority priority)
        : ThreadBase(runnable, name, priority)
    {
    }
    
public:
    /// <summary>
    /// Factory method to create a thread with the specified stack size and thread priority
    /// </summary>
    /// <param name="runnable">The runnable object to execute</param>
    /// <param name="name">Name of the thread</param>
    /// <param name="priority">Tells the thread whether it needs to adjust its priority or not. Defaults to normal priority</param>
    /// <param name="stackSize">The size of the stack to create. 0 means use the current thread's stack size</param>
    /// <returns>Pointer to the new thread or null if cannot create it</returns>
    static WebThread* Create(IRunnable* runnable, const String& name, ThreadPriority priority = ThreadPriority::Normal, uint32 stackSize = 0)
    {
        return New<WebThread>(runnable, name, priority);
    }

public:
    // [ThreadBase]
    void Join() override
    {
        // TOOD: impl this
    }

protected:
    // [ThreadBase]
    void ClearHandleInternal() override
    {
        // TOOD: impl this
    }
    void SetPriorityInternal(ThreadPriority priority) override
    {
        // TOOD: impl this
    }
    void KillInternal(bool waitForJoin) override
    {
        // TOOD: impl this
    }
};

#endif
