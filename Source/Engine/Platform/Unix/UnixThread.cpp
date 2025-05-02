// Copyright (c) Wojciech Figat. All rights reserved.

#if PLATFORM_UNIX

#include "UnixThread.h"
#include "Engine/Core/Log.h"
#include "Engine/Threading/IRunnable.h"
#include "Engine/Threading/ThreadRegistry.h"

UnixThread::UnixThread(IRunnable* runnable, const String& name, ThreadPriority priority)
    : ThreadBase(runnable, name, priority)
    , _thread(0)
{
}

UnixThread::~UnixThread()
{
    ASSERT(_thread == 0);
}

int32 UnixThread::Start(pthread_attr_t& attr)
{
    return pthread_create(&_thread, &attr, ThreadProc, this);
}

void* UnixThread::ThreadProc(void* pThis)
{
    auto thread = (UnixThread*)pThis;
#if PLATFORM_APPLE_FAMILY
    // Apple doesn't support creating named thread so assign name here
    {
        pthread_setname_np(StringAnsi(thread->GetName()).Get());
    }
#endif
    const int32 exitCode = thread->Run();
    return (void*)(uintptr)exitCode;
}

UnixThread* UnixThread::Setup(UnixThread* thread, uint32 stackSize)
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    stackSize = thread->GetStackSize(stackSize);
    if (stackSize != 0)
        pthread_attr_setstacksize(&attr, stackSize);
    const int32 result = thread->Start(attr);
    if (result != 0)
    {
        LOG(Warning, "Failed to spawn a thread. Result code: {0}", result);
        Delete(thread);
        return nullptr;
    }

    thread->SetPriorityInternal(thread->GetPriority());

    return thread;
}

void UnixThread::Join()
{
    pthread_join(_thread, nullptr);
    ClearHandleInternal();
}

void UnixThread::ClearHandleInternal()
{
    _thread = 0;
}

void UnixThread::SetPriorityInternal(ThreadPriority priority)
{
    struct sched_param sched;
    Platform::MemoryClear(&sched, sizeof(struct sched_param));
    int32 policy = SCHED_RR;
    pthread_getschedparam(_thread, &policy, &sched);
    sched.sched_priority = GetThreadPriority(priority);
    pthread_setschedparam(_thread, policy, &sched);
}

#endif
