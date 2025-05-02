// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_LINUX

#include "../Unix/UnixThread.h"
#include <signal.h>

/// <summary>
/// Thread object for Linux platform.
/// </summary>
class FLAXENGINE_API LinuxThread : public UnixThread
{
public:

    /// <summary>
    /// Initializes a new instance of the <see cref="LinuxThread"/> class.
    /// </summary>
    /// <param name="runnable">The runnable.</param>
    /// <param name="name">The thread name.</param>
    /// <param name="priority">The thread priority.</param>
    LinuxThread(IRunnable* runnable, const String& name, ThreadPriority priority)
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
    static LinuxThread* Create(IRunnable* runnable, const String& name, ThreadPriority priority = ThreadPriority::Normal, uint32 stackSize = 0)
    {
        return (LinuxThread*)Setup(New<LinuxThread>(runnable, name, priority), stackSize);
    }

protected:

    // [UnixThread]
    int32 Start(pthread_attr_t& attr) override
    {
        const int result = pthread_create(&_thread, &attr, ThreadProc, this);
        if (result == 0)
            pthread_setname_np(_thread, _name.ToStringAnsi().Get());
        return result;
    }
    void KillInternal(bool waitForJoin) override
    {
        if (waitForJoin)
            pthread_join(_thread, nullptr);
        pthread_kill(_thread, SIGKILL);
    }
};

#endif
