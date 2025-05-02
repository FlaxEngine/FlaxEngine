// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_UNIX

#include "Engine/Platform/Base/ThreadBase.h"
#include <pthread.h>

/// <summary>
/// Thread object for Unix platform.
/// </summary>
class FLAXENGINE_API UnixThread : public ThreadBase
{
protected:

    pthread_t _thread;

public:

    /// <summary>
    /// Initializes a new instance of the <see cref="UnixThread"/> class.
    /// </summary>
    /// <param name="runnable">The runnable.</param>
    /// <param name="name">The thread name.</param>
    /// <param name="priority">The thread priority.</param>
    UnixThread(IRunnable* runnable, const String& name, ThreadPriority priority);

    /// <summary>
    /// Finalizes an instance of the <see cref="UnixThread"/> class.
    /// </summary>
    ~UnixThread();

protected:

    virtual int32 Start(pthread_attr_t& attr);

    static void* ThreadProc(void* pThis);

    virtual uint32 GetStackSize(uint32 customStackSize)
    {
        return customStackSize;
    }

    virtual int32 GetThreadPriority(ThreadPriority priority)
    {
        switch (priority)
        {
        case ThreadPriority::Highest:
            return 30;
        case ThreadPriority::AboveNormal:
            return 25;
        case ThreadPriority::Normal:
            return 15;
        case ThreadPriority::BelowNormal:
            return 5;
        case ThreadPriority::Lowest:
            return 1;
        }
        return 0;
    }

    static UnixThread* Setup(UnixThread* thread, uint32 stackSize = 0);

public:

    // [ThreadBase]
    void Join() override;

protected:

    // [ThreadBase]
    void ClearHandleInternal() override;
    void SetPriorityInternal(ThreadPriority priority) override;
};

#endif
