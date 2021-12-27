// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#if PLATFORM_MAC

#include "../Base/ThreadBase.h"

/// <summary>
/// Thread object for Mac platform.
/// </summary>
class MacThread : public ThreadBase
{
public:

    MacThread(IRunnable* runnable, const String& name, ThreadPriority priority);
    ~MacThread();
    static MacThread* Create(IRunnable* runnable, const String& name, ThreadPriority priority = ThreadPriority::Normal, uint32 stackSize = 0);

    // [ThreadBase]
    void Join() override;

protected:

    // [ThreadBase]
    void ClearHandleInternal();
    void SetPriorityInternal(ThreadPriority priority);
    void KillInternal(bool waitForJoin);
};

#endif
