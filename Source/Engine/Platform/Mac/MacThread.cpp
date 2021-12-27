// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if PLATFORM_MAC

#include "MacThread.h"
#include "Engine/Threading/IRunnable.h"
#include "Engine/Threading/ThreadRegistry.h"

MacThread::MacThread(IRunnable* runnable, const String& name, ThreadPriority priority)
    : ThreadBase(runnable, name, priority)
{
}

MacThread::~MacThread()
{
}

MacThread* Create(IRunnable* runnable, const String& name, ThreadPriority priority = ThreadPriority::Normal, uint32 stackSize = 0)
{
    // TODO: imp this
}

#endif
