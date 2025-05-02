// Copyright (c) Wojciech Figat. All rights reserved.

#include "JobSystem.h"
#include "IRunnable.h"
#include "Engine/Platform/CPUInfo.h"
#include "Engine/Platform/Thread.h"
#include "Engine/Platform/ConditionVariable.h"
#include "Engine/Core/Types/Span.h"
#include "Engine/Core/Types/Pair.h"
#include "Engine/Core/Memory/SimpleHeapAllocation.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Core/Collections/RingBuffer.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Profiler/ProfilerCPU.h"
#if USE_CSHARP
#include "Engine/Scripting/ManagedCLR/MCore.h"
#endif

#define JOB_SYSTEM_ENABLED 1

#if JOB_SYSTEM_ENABLED

// Local allocator for job system memory that uses internal pooling and assumes that JobsLocker is taken (write access owned by the calling thread).
class JobSystemAllocation : public SimpleHeapAllocation<JobSystemAllocation>
{
public:
    static void* Allocate(uintptr size);
    static void Free(void* ptr, uintptr size);
};

class JobSystemService : public EngineService
{
public:
    JobSystemService()
        : EngineService(TEXT("JobSystem"), -800)
    {
    }

    bool Init() override;
    void BeforeExit() override;
    void Dispose() override;
};

struct JobData
{
    int32 Index;
    int64 JobKey;
};

template<>
struct TIsPODType<JobData>
{
    enum { Value = true };
};

struct JobContext
{
    volatile int64 JobsLeft;
    int32 DependenciesLeft;
    Function<void(int32)> Job;
    Array<int64, JobSystemAllocation> Dependants;
};

template<>
struct TIsPODType<JobContext>
{
    enum { Value = false };
};

class JobSystemThread : public IRunnable
{
public:
    uint64 Index;

public:
    // [IRunnable]
    String ToString() const override
    {
        return TEXT("JobSystemThread");
    }

    int32 Run() override;

    void AfterWork(bool wasKilled) override
    {
        Delete(this);
    }
};

namespace
{
    JobSystemService JobSystemInstance;
    Array<Pair<void*, uintptr>> MemPool;
    Thread* Threads[PLATFORM_THREADS_LIMIT / 2] = {};
    int32 ThreadsCount = 0;
    bool JobStartingOnDispatch = true;
    volatile int64 ExitFlag = 0;
    volatile int64 JobLabel = 0;
    Dictionary<int64, JobContext, JobSystemAllocation> JobContexts;
    ConditionVariable JobsSignal;
    CriticalSection JobsMutex;
    ConditionVariable WaitSignal;
    CriticalSection WaitMutex;
    CriticalSection JobsLocker;
    RingBuffer<JobData> Jobs;
}

void* JobSystemAllocation::Allocate(uintptr size)
{
    void* result = nullptr;
    for (int32 i = 0; i < MemPool.Count(); i++)
    {
        if (MemPool.Get()[i].Second == size)
        {
            result = MemPool.Get()[i].First;
            MemPool.RemoveAt(i);
            break;
        }
    }
    if (!result)
        result = Platform::Allocate(size, 16);
    return result;
}

void JobSystemAllocation::Free(void* ptr, uintptr size)
{
    MemPool.Add({ ptr, size });
}

bool JobSystemService::Init()
{
    ThreadsCount = Math::Min<int32>(Platform::GetCPUInfo().LogicalProcessorCount, ARRAY_COUNT(Threads));
    for (int32 i = 0; i < ThreadsCount; i++)
    {
        auto runnable = New<JobSystemThread>();
        runnable->Index = (uint64)i;
        auto thread = Thread::Create(runnable, String::Format(TEXT("Job System {0}"), i), ThreadPriority::AboveNormal);
        if (thread == nullptr)
            return true;
        Threads[i] = thread;
    }
    return false;
}

void JobSystemService::BeforeExit()
{
    Platform::AtomicStore(&ExitFlag, 1);
    JobsSignal.NotifyAll();
}

void JobSystemService::Dispose()
{
    Platform::AtomicStore(&ExitFlag, 1);
    JobsSignal.NotifyAll();
    Platform::Sleep(1);

    for (int32 i = 0; i < ThreadsCount; i++)
    {
        if (Threads[i])
        {
            Threads[i]->Kill(true);
            Delete(Threads[i]);
            Threads[i] = nullptr;
        }
    }

    JobContexts.SetCapacity(0);
    Jobs.Release();
    for (auto& e : MemPool)
        Platform::Free(e.First);
    MemPool.Clear();
}

int32 JobSystemThread::Run()
{
    Platform::SetThreadAffinityMask(1ull << Index);

    JobData data;
    Function<void(int32)> job;
    bool attachCSharpThread = true;
    while (Platform::AtomicRead(&ExitFlag) == 0)
    {
        // Try to get a job
        JobsLocker.Lock();
        if (Jobs.Count() != 0)
        {
            data = Jobs.PeekFront();
            Jobs.PopFront();
            const JobContext& context = ((const Dictionary<int64, JobContext>&)JobContexts).At(data.JobKey);
            job = context.Job;
        }
        JobsLocker.Unlock();

        if (job.IsBinded())
        {
#if USE_CSHARP
            // Ensure to have C# thread attached to this thead (late init due to MCore being initialized after Job System)
            if (attachCSharpThread)
            {
                MCore::Thread::Attach();
                attachCSharpThread = false;
            }
#endif

            // Run job
            job(data.Index);

            // Move forward with the job queue
            bool notifyWaiting = false;
            JobsLocker.Lock();
            JobContext& context = JobContexts.At(data.JobKey);
            if (Platform::InterlockedDecrement(&context.JobsLeft) <= 0)
            {
                // Update any dependant jobs
                for (int64 dependant : context.Dependants)
                {
                    JobContext& dependantContext = JobContexts.At(dependant);
                    if (--dependantContext.DependenciesLeft <= 0)
                    {
                        // Dispatch dependency when it's ready
                        JobData dependantData;
                        dependantData.JobKey = dependant;
                        for (dependantData.Index = 0; dependantData.Index < dependantContext.JobsLeft; dependantData.Index++)
                            Jobs.PushBack(dependantData);
                    }
                }

                // Remove completed context
                JobContexts.Remove(data.JobKey);
                notifyWaiting = true;
            }
            JobsLocker.Unlock();
            if (notifyWaiting)
                WaitSignal.NotifyAll();

            job.Unbind();
        }
        else
        {
            // Wait for signal
            JobsMutex.Lock();
            JobsSignal.Wait(JobsMutex);
            JobsMutex.Unlock();
        }
    }
    return 0;
}

#endif

void JobSystem::Execute(const Function<void(int32)>& job, int32 jobCount)
{
#if JOB_SYSTEM_ENABLED
    // TODO: disable async if called on job thread? or maybe Wait should handle waiting in job thread to do the processing?
    if (jobCount > 1)
    {
        // Async
        const int64 jobWaitHandle = Dispatch(job, jobCount);
        Wait(jobWaitHandle);
    }
    else
#endif
    {
        // Sync
        for (int32 i = 0; i < jobCount; i++)
            job(i);
    }
}

int64 JobSystem::Dispatch(const Function<void(int32)>& job, int32 jobCount)
{
    if (jobCount <= 0)
        return 0;
    PROFILE_CPU();
#if JOB_SYSTEM_ENABLED
    const auto label = Platform::InterlockedAdd(&JobLabel, (int64)jobCount) + jobCount;

    JobData data;
    data.JobKey = label;

    JobContext context;
    context.Job = job;
    context.JobsLeft = jobCount;
    context.DependenciesLeft = 0;

    JobsLocker.Lock();
    JobContexts.Add(label, MoveTemp(context));
    for (data.Index = 0; data.Index < jobCount; data.Index++)
        Jobs.PushBack(data);
    JobsLocker.Unlock();

    if (JobStartingOnDispatch)
    {
        if (jobCount == 1)
            JobsSignal.NotifyOne();
        else
            JobsSignal.NotifyAll();
    }

    return label;
#else
    for (int32 i = 0; i < jobCount; i++)
        job(i);
    return 0;
#endif
}

int64 JobSystem::Dispatch(const Function<void(int32)>& job, Span<int64> dependencies, int32 jobCount)
{
    if (jobCount <= 0)
        return 0;
    PROFILE_CPU();
#if JOB_SYSTEM_ENABLED
    const auto label = Platform::InterlockedAdd(&JobLabel, (int64)jobCount) + jobCount;

    JobData data;
    data.JobKey = label;

    JobContext context;
    context.Job = job;
    context.JobsLeft = jobCount;
    context.DependenciesLeft = 0;

    JobsLocker.Lock();
    for (int64 dependency : dependencies)
    {
        if (JobContext* dependencyContext = JobContexts.TryGet(dependency))
        {
            context.DependenciesLeft++;
            dependencyContext->Dependants.Add(label);
        }
    }
    JobContexts.Add(label, MoveTemp(context));
    if (context.DependenciesLeft == 0)
    {
        // No dependencies left to complete so dispatch now
        for (data.Index = 0; data.Index < jobCount; data.Index++)
            Jobs.PushBack(data);
    }
    JobsLocker.Unlock();

    if (context.DependenciesLeft == 0 && JobStartingOnDispatch)
    {
        if (jobCount == 1)
            JobsSignal.NotifyOne();
        else
            JobsSignal.NotifyAll();
    }

    return label;
#else
    for (int32 i = 0; i < jobCount; i++)
        job(i);
    return 0;
#endif
}

void JobSystem::Wait()
{
#if JOB_SYSTEM_ENABLED
    JobsLocker.Lock();
    int32 numJobs = JobContexts.Count();
    JobsLocker.Unlock();

    while (numJobs > 0)
    {
        WaitMutex.Lock();
        WaitSignal.Wait(WaitMutex, 1);
        WaitMutex.Unlock();

        JobsLocker.Lock();
        numJobs = JobContexts.Count();
        JobsLocker.Unlock();
    }
#endif
}

void JobSystem::Wait(int64 label)
{
#if JOB_SYSTEM_ENABLED
    PROFILE_CPU();

    while (Platform::AtomicRead(&ExitFlag) == 0)
    {
        JobsLocker.Lock();
        const JobContext* context = JobContexts.TryGet(label);
        JobsLocker.Unlock();

        // Skip if context has been already executed (last job removes it)
        if (!context)
            break;

        // Wait on signal until input label is not yet done
        WaitMutex.Lock();
        WaitSignal.Wait(WaitMutex, 1);
        WaitMutex.Unlock();

        // Wake up any thread to prevent stalling in highly multi-threaded environment
        JobsSignal.NotifyOne();
    }
#endif
}

void JobSystem::SetJobStartingOnDispatch(bool value)
{
#if JOB_SYSTEM_ENABLED
    JobStartingOnDispatch = value;
    if (value)
    {
        JobsLocker.Lock();
        const int32 count = Jobs.Count();
        JobsLocker.Unlock();
        if (count == 1)
            JobsSignal.NotifyOne();
        else if (count != 0)
            JobsSignal.NotifyAll();
    }
#endif
}

int32 JobSystem::GetThreadsCount()
{
#if JOB_SYSTEM_ENABLED
    return ThreadsCount;
#else
    return 0;
#endif
}
