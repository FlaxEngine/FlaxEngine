// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_MAC || PLATFORM_IOS

#include "../Unix/UnixThread.h"
#include <signal.h>

/// <summary>
/// Thread object for Apple platform.
/// </summary>
class FLAXENGINE_API AppleThread : public UnixThread
{
public:

    /// <summary>
    /// Initializes a new instance of the <see cref="AppleThread"/> class.
    /// </summary>
    /// <param name="runnable">The runnable.</param>
    /// <param name="name">The thread name.</param>
    /// <param name="priority">The thread priority.</param>
    AppleThread(IRunnable* runnable, const String& name, ThreadPriority priority)
        : UnixThread(runnable, name, priority)
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
    static AppleThread* Create(IRunnable* runnable, const String& name, ThreadPriority priority = ThreadPriority::Normal, uint32 stackSize = 0)
    {
        return (AppleThread*)Setup(New<AppleThread>(runnable, name, priority), stackSize);
    }

    static int32 GetAppleThreadPriority(ThreadPriority priority)
    {
        switch (priority)
        {
        case ThreadPriority::Highest:
            return 45;
        case ThreadPriority::AboveNormal:
            return 37;
        case ThreadPriority::Normal:
            return 31;
        case ThreadPriority::BelowNormal:
            return 25;
        case ThreadPriority::Lowest:
            return 20;
        }
        return 31;
    }

protected:

    // [UnixThread]
    int32 GetThreadPriority(ThreadPriority priority) override
    {
        return GetAppleThreadPriority(priority);
    }
    int32 Start(pthread_attr_t& attr) override
    {
        return pthread_create(&_thread, &attr, ThreadProc, this);
    }
    void KillInternal(bool waitForJoin) override
    {
        if (waitForJoin)
            pthread_join(_thread, nullptr);
        pthread_kill(_thread, SIGKILL);
    }
};

#endif
