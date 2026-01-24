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
#include "Engine/Profiler/ProfilerMemory.h"
#include "Engine/Scripting/Internal/InternalCalls.h"

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
#ifdef THREAD_POOL_AFFINITY_MASK
    volatile int64 ThreadIndex = 0;
#endif
}

String ThreadPoolTask::ToString() const
{
    return String::Format(TEXT("Thread Pool Task ({0})"), (int32)GetState());
}

void ThreadPoolTask::Enqueue()
{
    PROFILE_MEM(EngineThreading);
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
    PROFILE_MEM(EngineThreading);

    // Spawn threads
    const CPUInfo cpuInfo = Platform::GetCPUInfo();
    const int32 count = Math::Clamp<int32>(cpuInfo.ProcessorCoreCount - 1, 2, PLATFORM_THREADS_LIMIT / 2);
    LOG(Info, "Spawning {0} Thread Pool workers", count);
    ThreadPoolImpl::Threads.Resize(count);
    for (int32 i = 0; i < count; i++)
    {
        auto runnable = New<SimpleRunnable>(true);
        runnable->OnWork.Bind(ThreadPool::ThreadProc);
        auto thread = Thread::Create(runnable, String::Format(TEXT("Thread Pool {0}"), i));
        if (thread == nullptr)
        {
            LOG(Error, "Failed to spawn {0} thread in the Thread Pool", i + 1);
            return true;
        }
        ThreadPoolImpl::Threads[i] = thread;
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
#ifdef THREAD_POOL_AFFINITY_MASK
    const int64 index = Platform::InterlockedIncrement(&ThreadPoolImpl::ThreadIndex) - 1;
    Platform::SetThreadAffinityMask(THREAD_POOL_AFFINITY_MASK((int32)index));
#endif
    ThreadPoolTask* task;
    MONO_THREAD_INFO_TYPE* monoThreadInfo = nullptr;

    // Work until end
    while (Platform::AtomicRead(&ThreadPoolImpl::ExitFlag) == 0)
    {
        // Try to get a job
        if (ThreadPoolImpl::Jobs.try_dequeue(task))
        {
            task->Execute();
            MONO_THREAD_INFO_GET(monoThreadInfo);
        }
        else
        {
            MONO_ENTER_GC_SAFE_WITH_INFO(monoThreadInfo);
            ThreadPoolImpl::JobsMutex.Lock();
            ThreadPoolImpl::JobsSignal.Wait(ThreadPoolImpl::JobsMutex);
            ThreadPoolImpl::JobsMutex.Unlock();
            MONO_EXIT_GC_SAFE_WITH_INFO;
        }
    }

    return 0;
}
