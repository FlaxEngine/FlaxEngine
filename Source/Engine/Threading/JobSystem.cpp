// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

#include "JobSystem.h"
#include "IRunnable.h"
#include "Engine/Platform/CPUInfo.h"
#include "Engine/Platform/Thread.h"
#include "Engine/Platform/ConditionVariable.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Scripting/ManagedCLR/MCore.h"
#if USE_MONO
#include "Engine/Scripting/ManagedCLR/MDomain.h"
#include <ThirdParty/mono-2.0/mono/metadata/appdomain.h>
#include <ThirdParty/mono-2.0/mono/metadata/threads.h>
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
    volatile int64 RefCount = 0;
    volatile int64 JobsLeft = 0;
};

namespace
{
    JobSystemService JobSystemInstance;
    Thread* Threads[PLATFORM_THREADS_LIMIT] = {};
    int32 ThreadsCount = 0;
    bool JobStartingOnDispatch = true;
    volatile int64 ExitFlag = 0;
    volatile int64 JobLabel = 0;
    Dictionary<int64, JobContext> JobContexts;
    CriticalSection JobContextMutex;
    ConditionVariable JobsSignal;
    CriticalSection JobsMutex;
    ConditionVariable WaitSignal;
    CriticalSection WaitMutex;
#if JOB_SYSTEM_USE_MUTEX
    CriticalSection JobsLocker;
    RingBuffer<JobData, InlinedAllocation<256>> Jobs;
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
            if (Threads[i]->IsRunning())
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
    bool attachMonoThread = true;
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
#if USE_MONO
            // Ensure to have C# thread attached to this thead (late init due to MCore being initialized after Job System)
            if (attachMonoThread && !mono_domain_get())
            {
                const auto domain = MCore::GetActiveDomain();
                mono_thread_attach(domain->GetNative());
                attachMonoThread = false;
            }
#endif

            // Run job
            data.Job(data.Index);

            // Move forward with the job queue
            JobContextMutex.Lock();
            JobContext& context = JobContexts.At(data.JobKey);
            Platform::InterlockedDecrement(&context.JobsLeft);
            ASSERT(Platform::AtomicRead(&context.JobsLeft) >= 0)
            if (context.JobsLeft <= 0)
            {
                if (Platform::AtomicRead(&context.RefCount) == 0)
                    JobContexts.Remove(data.JobKey);
            }
            JobContextMutex.Unlock();

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

    JobContextMutex.Lock();
    JobContexts.Add(label, context);
    JobContextMutex.Unlock();

#if JOB_SYSTEM_USE_MUTEX
    JobsLocker.Lock();
    for (data.Index = 0; data.Index < jobCount; data.Index++)
        Jobs.PushBack(data);
    JobsLocker.Unlock();
#else
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
    JobContextMutex.Lock();
    int32 numJobs = JobContexts.Count();
    JobContextMutex.Unlock();

    while (numJobs > 0)
    {
        WaitMutex.Lock();
        WaitSignal.Wait(WaitMutex, 1);
        WaitMutex.Unlock();

        JobContextMutex.Lock();
        numJobs = JobContexts.Count();
        JobContextMutex.Unlock();
    }
#endif
}

void JobSystem::Wait(int64 label)
{
#if JOB_SYSTEM_ENABLED
    PROFILE_CPU();

    JobContextMutex.Lock();
    JobContext* context = JobContexts.TryGet(label);
    if (context)
        Platform::InterlockedIncrement(&context->RefCount);
    JobContextMutex.Unlock();

    // Early out
    if (!context)
        return;

    if (Platform::AtomicRead(&context->JobsLeft) > 0)
    {
        // Wait on signal until input label is not yet done
        do
        {
            WaitMutex.Lock();
            WaitSignal.Wait(WaitMutex, 1);
            WaitMutex.Unlock();
        } while (Platform::AtomicRead(&context->JobsLeft) > 0 && Platform::AtomicRead(&ExitFlag) == 0);
    }

    JobContextMutex.Lock();
    Platform::InterlockedDecrement(&context->RefCount);
    if (Platform::AtomicRead(&context->RefCount) == 0)
        JobContexts.Remove(label);
    JobContextMutex.Unlock();

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
