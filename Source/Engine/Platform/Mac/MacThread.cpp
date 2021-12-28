// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#if PLATFORM_MAC

#include "MacThread.h"
#include "Engine/Core/Log.h"
#include "Engine/Threading/IRunnable.h"
#include "Engine/Threading/ThreadRegistry.h"

MacThread::MacThread(IRunnable* runnable, const String& name, ThreadPriority priority)
    : ThreadBase(runnable, name, priority)
    , _thread(0)
{
}

MacThread::~MacThread()
{
    ASSERT(_thread == 0);
}

MacThread* MacThread::Create(IRunnable* runnable, const String& name, ThreadPriority priority, uint32 stackSize)
{
    auto thread = New<MacThread>(runnable, name, priority);
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    if (stackSize != 0)
        pthread_attr_setstacksize(&attr, stackSize);
    const int32 result = pthread_create(&thread->_thread, &attr, ThreadProc, thread);
    if (result != 0)
    {
        LOG(Warning, "Failed to spawn a thread. Result code: {0}", result);
        Delete(thread);
        return nullptr;
    }
    thread->SetPriorityInternal(thread->GetPriority());
    return thread;
}

void MacThread::Join()
{
    pthread_join(_thread, nullptr);
}

void* MacThread::ThreadProc(void* pThis)
{
    auto thread = (MacThread*)pThis;
    const int32 exitCode = thread->Run();
    return (void*)(uintptr)exitCode;
}

void MacThread::ClearHandleInternal()
{
    _thread = 0;
}

void MacThread::SetPriorityInternal(ThreadPriority priority)
{
    // TODO: impl this
}

void MacThread::KillInternal(bool waitForJoin)
{
    if (waitForJoin)
        pthread_join(_thread, nullptr);
    pthread_kill(_thread, SIGKILL);
}

#endif
