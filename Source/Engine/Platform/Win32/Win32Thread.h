// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WIN32

#include "Engine/Platform/Base/ThreadBase.h"

/// <summary>
/// Thread object for Win32 platform.
/// </summary>
class FLAXENGINE_API Win32Thread : public ThreadBase
{
private:

    void* _thread;

public:

    Win32Thread(IRunnable* runnable, const String& name, ThreadPriority priority);

    /// <summary>
    /// Finalizes an instance of the <see cref="Win32Thread"/> class.
    /// </summary>
    ~Win32Thread();

public:

    /// <summary>
    /// Factory method to create a thread with the specified stack size and thread priority
    /// </summary>
    /// <param name="runnable">The runnable object to execute</param>
    /// <param name="name">Name of the thread</param>
    /// <param name="priority">Tells the thread whether it needs to adjust its priority or not. Defaults to normal priority</param>
    /// <param name="stackSize">The size of the stack to create. 0 means use the current thread's stack size</param>
    /// <returns>Pointer to the new thread or null if cannot create it</returns>
    static Win32Thread* Create(IRunnable* runnable, const String& name, ThreadPriority priority = ThreadPriority::Normal, uint32 stackSize = 0);

    void* GetHandle() const
    {
        return _thread;
    }

private:

    bool Start(uint32 stackSize);
    static unsigned long ThreadProc(void* pThis);

public:

    // [ThreadBase]
    void Join() override;

protected:

    // [ThreadBase]
    void ClearHandleInternal() override;
    void SetPriorityInternal(ThreadPriority priority) override;
    void KillInternal(bool waitForJoin) override;
};

#endif
