// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_MAC

#include "../Base/ThreadBase.h"

/// <summary>
/// Thread object for Mac platform.
/// </summary>
class MacThread : public ThreadBase
{
protected:

    pthread_t _thread;

public:

    MacThread(IRunnable* runnable, const String& name, ThreadPriority priority);
    ~MacThread();
    static MacThread* Create(IRunnable* runnable, const String& name, ThreadPriority priority = ThreadPriority::Normal, uint32 stackSize = 0);

    // [ThreadBase]
    void Join() override;

protected:

    static void* ThreadProc(void* pThis);

    // [ThreadBase]
    void ClearHandleInternal() override;
    void SetPriorityInternal(ThreadPriority priority) override;
    void KillInternal(bool waitForJoin) override;
};

#endif
