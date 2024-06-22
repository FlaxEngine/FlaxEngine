// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "JobSystem.h"
#include "IRunnable.h"
#include "Engine/Platform/CPUInfo.h"
#include "Engine/Platform/Thread.h"
#include "Engine/Platform/ConditionVariable.h"
#include "Engine/Core/Collections/Dictionary.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Profiler/ProfilerCPU.h"
#if USE_CSHARP
#include "Engine/Scripting/ManagedCLR/MCore.h"
#endif

// Jobs storage perf info:
// (500 jobs, i7 9th gen)
// JOB_SYSTEM_USE_MUTEX=1, enqueue=130-280 cycles, dequeue=2-6 cycles
// JOB_SYSTEM_USE_MUTEX=0, enqueue=300-700 cycles, dequeue=10-16 cycles
// So using RingBuffer+Mutex+Signals is better than moodycamel::ConcurrentQueue

#define JOB_SYSTEM_ENABLED 1
#define JOB_SYSTEM_USE_MUTEX 1
#define JOB_SYSTEM_USE_STATS 0

#if JOB_SYSTEM_USE_STATS
#include "Engine/Core/Log.h"
#endif
#if JOB_SYSTEM_USE_MUTEX
#include "Engine/Core/Collections/RingBuffer.h"
#else
#include "ConcurrentQueue.h"
#endif

#if JOB_SYSTEM_ENABLED

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
    Function<void(int32)> Job;
    int32 Index;
    int64 JobKey;
};

template<>
struct TIsPODType<JobData>
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

struct JobContext
{
    volatile int64 JobsLeft;
};

template<>
struct TIsPODType<JobContext>
{
    enum { Value = true };
};

namespace
{
    JobSystemService JobSystemInstance;
    Thread* Threads[PLATFORM_THREADS_LIMIT / 2] = {};
    int32 ThreadsCount = 0;
    bool JobStartingOnDispatch = true;
    volatile int64 ExitFlag = 0;
    volatile int64 JobLabel = 0;
    Dictionary<int64, JobContext> JobContexts;
    ConditionVariable JobsSignal;
    CriticalSection JobsMutex;
    ConditionVariable WaitSignal;
    CriticalSection WaitMutex;
    CriticalSection JobsLocker;
#if JOB_SYSTEM_USE_MUTEX
    RingBuffer<JobData> Jobs;
#else
    ConcurrentQueue<JobData> Jobs;
#endif
#if JOB_SYSTEM_USE_STATS
    int64 DequeueCount = 0;
    int64 DequeueSum = 0;
#endif
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
}

int32 JobSystemThread::Run()
{
    Platform::SetThreadAffinityMask(1ull << Index);

    JobData data;
    bool attachCSharpThread = true;
#if !JOB_SYSTEM_USE_MUTEX
    moodycamel::ConsumerToken consumerToken(Jobs);
#endif
    while (Platform::AtomicRead(&ExitFlag) == 0)
    {
        // Try to get a job
#if JOB_SYSTEM_USE_STATS
        const auto start = Platform::GetTimeCycles();
#endif
#if JOB_SYSTEM_USE_MUTEX
        JobsLocker.Lock();
        if (Jobs.Count() != 0)
        {
            data = Jobs.PeekFront();
            Jobs.PopFront();
        }
        JobsLocker.Unlock();
#else
        if (!Jobs.try_dequeue(consumerToken, data))
            data.Job.Unbind();
#endif
#if JOB_SYSTEM_USE_STATS
        Platform::InterlockedIncrement(&DequeueCount);
        Platform::InterlockedAdd(&DequeueSum, Platform::GetTimeCycles() - start);
#endif

        if (data.Job.IsBinded())
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
            data.Job(data.Index);

            // Move forward with the job queue
            bool notifyWaiting = false;
            JobsLocker.Lock();
            JobContext& context = JobContexts.At(data.JobKey);
            if (Platform::InterlockedDecrement(&context.JobsLeft) <= 0)
            {
                ASSERT_LOW_LAYER(context.JobsLeft <= 0);
                JobContexts.Remove(data.JobKey);
                notifyWaiting = true;
            }
            JobsLocker.Unlock();

            if (notifyWaiting)
                WaitSignal.NotifyAll();

            data.Job.Unbind();
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
    PROFILE_CPU();
    if (jobCount <= 0)
        return 0;
#if JOB_SYSTEM_ENABLED
#if JOB_SYSTEM_USE_STATS
    const auto start = Platform::GetTimeCycles();
#endif
    const auto label = Platform::InterlockedAdd(&JobLabel, (int64)jobCount) + jobCount;

    JobData data;
    data.Job = job;
    data.JobKey = label;

    JobContext context;
    context.JobsLeft = jobCount;

#if JOB_SYSTEM_USE_MUTEX
    JobsLocker.Lock();
    JobContexts.Add(label, context);
    for (data.Index = 0; data.Index < jobCount; data.Index++)
        Jobs.PushBack(data);
    JobsLocker.Unlock();
#else
    JobsLocker.Lock();
    JobContexts.Add(label, context);
    JobsLocker.Unlock();
    for (data.Index = 0; data.Index < jobCount; data.Index++)
        Jobs.enqueue(data);
#endif

#if JOB_SYSTEM_USE_STATS
    LOG(Info, "Job enqueue time: {0} cycles", (int64)(Platform::GetTimeCycles() - start));
#endif

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

#if JOB_SYSTEM_USE_STATS
    LOG(Info, "Job average dequeue time: {0} cycles", DequeueSum / DequeueCount);
    DequeueSum = DequeueCount = 0;
#endif
#endif
}

void JobSystem::SetJobStartingOnDispatch(bool value)
{
#if JOB_SYSTEM_ENABLED
    JobStartingOnDispatch = value;

    if (value)
    {
#if JOB_SYSTEM_USE_MUTEX
        JobsLocker.Lock();
        const int32 count = Jobs.Count();
        JobsLocker.Unlock();
#else
        const int32 count = Jobs.Count();
#endif
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
