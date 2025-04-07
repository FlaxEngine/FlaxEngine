// Copyright (c) Wojciech Figat. All rights reserved.

#include "ThreadPool.h"
#include "IRunnable.h"
#include "Threading.h"
#include "ThreadPoolTask.h"
#include "ConcurrentTaskQueue.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Engine/Globals.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Platform/ConditionVariable.h"
#include "Engine/Platform/CPUInfo.h"
#include "Engine/Platform/Thread.h"

FLAXENGINE_API bool IsInMainThread()
{
    return Globals::MainThreadID == Platform::GetCurrentThreadID();
}

namespace ThreadPoolImpl
{
    volatile int64 ExitFlag = 0;
    Array<Thread*> Threads;
    ConcurrentTaskQueue<ThreadPoolTask> Jobs; // Hello Steve!
    ConditionVariable JobsSignal;
    CriticalSection JobsMutex;
}

String ThreadPoolTask::ToString() const
{
    return String::Format(TEXT("Thread Pool Task ({0})"), (int32)GetState());
}

void ThreadPoolTask::Enqueue()
{
    ThreadPoolImpl::Jobs.Add(this);
    ThreadPoolImpl::JobsSignal.NotifyOne();
}

class ThreadPoolService : public EngineService
{
public:

    ThreadPoolService()
        : EngineService(TEXT("Thread Pool"), -900)
    {
    }

    bool Init() override;
    void BeforeExit() override;
    void Dispose() override;
};

ThreadPoolService ThreadPoolServiceInstance;

bool ThreadPoolService::Init()
{
    // Spawn threads
    const int32 numThreads = Math::Clamp<int32>(Platform::GetCPUInfo().ProcessorCoreCount - 1, 2, PLATFORM_THREADS_LIMIT / 2);
    LOG(Info, "Spawning {0} Thread Pool workers", numThreads);
    for (int32 i = ThreadPoolImpl::Threads.Count(); i < numThreads; i++)
    {
        // Create tread
        auto runnable = New<SimpleRunnable>(true);
        runnable->OnWork.Bind(ThreadPool::ThreadProc);
        auto thread = Thread::Create(runnable, String::Format(TEXT("Thread Pool {0}"), i));
        if (thread == nullptr)
        {
            LOG(Error, "Failed to spawn {0} thread in the Thread Pool", i + 1);
            return true;
        }

        // Add to the list
        ThreadPoolImpl::Threads.Add(thread);
    }

    return false;
}

void ThreadPoolService::BeforeExit()
{
    // Set exit flag and wake up threads
    Platform::AtomicStore(&ThreadPoolImpl::ExitFlag, 1);
    ThreadPoolImpl::JobsSignal.NotifyAll();
}

void ThreadPoolService::Dispose()
{
    // Set exit flag and wake up threads
    Platform::AtomicStore(&ThreadPoolImpl::ExitFlag, 1);
    ThreadPoolImpl::JobsSignal.NotifyAll();

    // Wait some time
    Platform::Sleep(10);

    // Delete threads
    for (int32 i = 0; i < ThreadPoolImpl::Threads.Count(); i++)
    {
        ThreadPoolImpl::Threads[i]->Kill(true);
    }
    ThreadPoolImpl::Threads.ClearDelete();
}

int32 ThreadPool::ThreadProc()
{
    ThreadPoolTask* task;

    // Work until end
    while (Platform::AtomicRead(&ThreadPoolImpl::ExitFlag) == 0)
    {
        // Try to get a job
        if (ThreadPoolImpl::Jobs.try_dequeue(task))
        {
            task->Execute();
        }
        else
        {
            ThreadPoolImpl::JobsMutex.Lock();
            ThreadPoolImpl::JobsSignal.Wait(ThreadPoolImpl::JobsMutex);
            ThreadPoolImpl::JobsMutex.Unlock();
        }
    }

    return 0;
}
