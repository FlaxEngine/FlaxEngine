// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_WEB

#include "../Base/ThreadBase.h"
#ifdef __EMSCRIPTEN_PTHREADS__
#include <pthread.h>
#include <signal.h>
#endif

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
#ifdef __EMSCRIPTEN_PTHREADS__
        , _thread(0)
#endif
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
    static WebThread* Create(IRunnable* runnable, const String& name, ThreadPriority priority = ThreadPriority::Normal, uint32 stackSize = 0);

#ifdef __EMSCRIPTEN_PTHREADS__
    // [ThreadBase]
    void Join() override
    {
        pthread_join(_thread, nullptr);
        ClearHandleInternal();
    }

protected:
    // [ThreadBase]
    void ClearHandleInternal() override
    {
        _thread = 0;
    }
    void SetPriorityInternal(ThreadPriority priority) override
    {
        // Not supported
    }
    void KillInternal(bool waitForJoin) override
    {
        if (waitForJoin)
            pthread_join(_thread, nullptr);
        pthread_kill(_thread, SIGKILL);
    }

private:
    pthread_t _thread;

    static void* ThreadProc(void* pThis)
    {
        auto thread = (WebThread*)pThis;
        const int32 exitCode = thread->Run();
        return (void*)(uintptr)exitCode;
    }
#else
    // [ThreadBase]
    void Join() override
    {
    }

protected:
    // [ThreadBase]
    void ClearHandleInternal() override
    {
    }
    void SetPriorityInternal(ThreadPriority priority) override
    {
    }
    void KillInternal(bool waitForJoin) override
    {
    }
#endif
};

#endif
