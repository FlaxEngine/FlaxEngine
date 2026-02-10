// Copyright (c) Wojciech Figat. All rights reserved.

#include "JobSystem.h"
#include "IRunnable.h"
#include "Engine/Platform/CPUInfo.h"
#include "Engine/Platform/Thread.h"
#include "Engine/Platform/ConditionVariable.h"
#include "Engine/Core/Types/Span.h"
#include "Engine/Core/Types/Pair.h"
#include "Engine/Core/Memory/SimpleHeapAllocation.h"
#include "Engine/Core/Collections/RingBuffer.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Profiler/ProfilerCPU.h"
#include "Engine/Profiler/ProfilerMemory.h"
#if USE_CSHARP
#include "Engine/Scripting/ManagedCLR/MCore.h"
#include "Engine/Scripting/Internal/InternalCalls.h"
#endif

#define JOB_SYSTEM_ENABLED 1

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

// Holds a single job dispatch data
struct alignas(int64) JobContext
{
    // The next index of the job to process updated when picking a job by the thread.
    volatile int64 JobIndex = 0;
    // The number of jobs left to process updated after job completion by the thread.
    volatile int64 JobsLeft = 0;
    // The unique label of this job used to identify it. Set to -1 when job is done.
    volatile int64 JobLabel = 0;
    // Utility atomic counter used to indicate that any job is waiting for this one to finish. Then Dependants can be accessed within thread-safe JobsLocker.
    volatile int64 DependantsCount = 0;
    // The number of dependency jobs left to be finished before starting this job.
    volatile int64 DependenciesLeft = 0;
    // The total number of jobs to process (in this context).
    int32 JobsCount = 0;
    // The job function to execute.
    Function<void(int32)> Job;
    // List of dependant jobs to signal when this job is done.
    Array<int64> Dependants;
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
    Thread* Threads[PLATFORM_THREADS_LIMIT / 2] = {};
    int32 ThreadsCount = 0;
    bool JobStartingOnDispatch = true;
    volatile int64 ExitFlag = 0;
    volatile int64 JobLabel = 0;
    volatile int64 JobEndLabel = 0;
    volatile int64 JobStartLabel = 0;
    volatile int64 JobContextsCount = 0;
    uint32 JobContextsSize = 0;
    uint32 JobContextsMask = 0;
    JobContext* JobContexts = nullptr;
    ConditionVariable JobsSignal;
    CriticalSection JobsMutex;
    ConditionVariable WaitSignal;
    CriticalSection WaitMutex;
    CriticalSection JobsLocker;
#define GET_CONTEXT_INDEX(label) (uint32)((label) & (int64)JobContextsMask)
}

bool JobSystemService::Init()
{
    PROFILE_MEM(EngineThreading);

    // Initialize job context storage (fixed-size ring buffer for active jobs tracking)
    JobContextsSize = 256;
    JobContextsMask = JobContextsSize - 1;
    JobContexts = (JobContext*)Platform::Allocate(JobContextsSize * sizeof(JobContext), alignof(JobContext));
    Memory::ConstructItems(JobContexts, (int32)JobContextsSize);

    // Spawn threads
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

    Memory::DestructItems(JobContexts, (int32)JobContextsSize);
    Platform::Free(JobContexts);
    JobContexts = nullptr;
}

int32 JobSystemThread::Run()
{
    // Pin thread to the physical core
    Platform::SetThreadAffinityMask(1ull << Index);

    bool attachCSharpThread = true;
    MONO_THREAD_INFO_TYPE* monoThreadInfo = nullptr;
    while (Platform::AtomicRead(&ExitFlag) == 0)
    {
        // Try to get a job
        int32 jobIndex;
        JobContext* jobContext = nullptr;
        {
            int64 jobOffset = 0;
        RETRY:
            int64 jobStartLabel = Platform::AtomicRead(&JobStartLabel) + jobOffset;
            int64 jobEndLabel = Platform::AtomicRead(&JobEndLabel);
            if (jobStartLabel <= jobEndLabel && jobEndLabel > 0)
            {
                jobContext = &JobContexts[GET_CONTEXT_INDEX(jobStartLabel)];
                if (Platform::AtomicRead(&jobContext->DependenciesLeft) > 0)
                {
                    // This job still waits for dependency so skip it for now and try the next one
                    jobOffset++;
                    jobContext = nullptr;
                    goto RETRY;
                }

                // Move forward with index for a job
                jobIndex = (int32)(Platform::InterlockedIncrement(&jobContext->JobIndex) - 1);
                if (jobIndex < jobContext->JobsCount)
                {
                    // Index is valid
                }
                else if (jobStartLabel < jobEndLabel && jobOffset == 0)
                {
                    // No more jobs inside this context, move to the next one
                    Platform::InterlockedCompareExchange(&JobStartLabel, jobStartLabel + 1, jobStartLabel);
                    jobContext = nullptr;
                    goto RETRY;
                }
                else
                {
                    // No more jobs
                    jobContext = nullptr;
                    if (jobStartLabel < jobEndLabel)
                    {
                        // Try with a different one before going to sleep
                        jobOffset++;
                        goto RETRY;
                    }
                }
            }
        }

        if (jobContext)
        {
#if USE_CSHARP
            // Ensure to have C# thread attached to this thead (late init due to MCore being initialized after Job System)
            if (attachCSharpThread)
            {
                MCore::Thread::Attach();
                attachCSharpThread = false;
                monoThreadInfo = mono_thread_info_attach();
            }
#endif

            // Run job
            jobContext->Job(jobIndex);

            // Move forward with the job queue
            if (Platform::InterlockedDecrement(&jobContext->JobsLeft) <= 0)
            {
                // Mark job as done before processing dependants
                Platform::AtomicStore(&jobContext->JobLabel, -1);

                // Check if any other job waits on this one
                if (Platform::AtomicRead(&jobContext->DependantsCount) != 0)
                {
                    // Update dependant jobs
                    JobsLocker.Lock();
                    for (int64 dependant : jobContext->Dependants)
                    {
                        JobContext& dependantContext = JobContexts[GET_CONTEXT_INDEX(dependant)];
                        if (dependantContext.JobLabel == dependant)
                            Platform::InterlockedDecrement(&dependantContext.DependenciesLeft);
                    }
                    JobsLocker.Unlock();
                }

                // Cleanup completed context
                jobContext->Job.Unbind();
                jobContext->Dependants.Clear();
                Platform::AtomicStore(&jobContext->DependantsCount, 0);
                Platform::AtomicStore(&jobContext->DependenciesLeft, -999); // Mark to indicate deleted context
                Platform::AtomicStore(&jobContext->JobLabel, -1);
                Platform::InterlockedDecrement(&JobContextsCount);

                // Wakeup any thread waiting for the jobs to complete
                WaitSignal.NotifyAll();
            }
        }
        else
        {
            // Wait for signal
            MONO_ENTER_GC_SAFE_WITH_INFO(monoThreadInfo);
            JobsMutex.Lock();
            JobsSignal.Wait(JobsMutex);
            JobsMutex.Unlock();
            MONO_EXIT_GC_SAFE_WITH_INFO;
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
        const int64 label = Dispatch(job, jobCount);
        Wait(label);
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
    while (Platform::InterlockedIncrement(&JobContextsCount) >= JobContextsSize)
    {
        // Too many jobs in flight, wait for some to complete to free up contexts
        PROFILE_CPU_NAMED("JOB SYSTEM OVERFLOW");
        ZoneColor(TracyWaitZoneColor);
        Platform::InterlockedDecrement(&JobContextsCount);
        Platform::Sleep(1);
    }

    // Get a new label
    const int64 label = Platform::InterlockedIncrement(&JobLabel);

    // Build job
    JobContext& context = JobContexts[GET_CONTEXT_INDEX(label)];
    context.Job = job;
    context.JobIndex = 0;
    context.JobsLeft = jobCount;
    context.JobLabel = label;
    context.DependantsCount = 0;
    context.DependenciesLeft = 0;
    context.JobsCount = jobCount;
    context.Dependants.Clear();

    // Move the job queue forward
    Platform::InterlockedIncrement(&JobEndLabel);

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
    PROFILE_MEM(EngineThreading);
#if JOB_SYSTEM_ENABLED
    while (Platform::InterlockedIncrement(&JobContextsCount) >= JobContextsSize)
    {
        // Too many jobs in flight, wait for some to complete to free up contexts
        PROFILE_CPU_NAMED("JOB SYSTEM OVERFLOW");
        ZoneColor(TracyWaitZoneColor);
        Platform::InterlockedDecrement(&JobContextsCount);
        Platform::Sleep(1);
    }

    // Get a new label
    const int64 label = Platform::InterlockedIncrement(&JobLabel);

    // Build job
    JobContext& context = JobContexts[GET_CONTEXT_INDEX(label)];
    context.Job = job;
    context.JobIndex = 0;
    context.JobsLeft = jobCount;
    context.JobLabel = label;
    context.DependantsCount = 0;
    context.DependenciesLeft = 0;
    context.JobsCount = jobCount;
    context.Dependants.Clear();
    {
        JobsLocker.Lock();
        for (int64 dependency : dependencies)
        {
            JobContext& dependencyContext = JobContexts[GET_CONTEXT_INDEX(dependency)];
            if (Platform::AtomicRead(&dependencyContext.JobLabel) == dependency)
            {
                Platform::InterlockedIncrement(&dependencyContext.DependantsCount);
                dependencyContext.Dependants.Add(label);
                context.DependenciesLeft++;
            }
        }
        JobsLocker.Unlock();
    }

    // Move the job queue forward
    Platform::InterlockedIncrement(&JobEndLabel);

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
    PROFILE_CPU();
    ZoneColor(TracyWaitZoneColor);

    int64 numJobs = Platform::AtomicRead(&JobContextsCount);
    while (numJobs > 0)
    {
        WaitMutex.Lock();
        WaitSignal.Wait(WaitMutex, 1);
        WaitMutex.Unlock();

        numJobs = Platform::AtomicRead(&JobContextsCount);
    }
#endif
}

void JobSystem::Wait(int64 label)
{
#if JOB_SYSTEM_ENABLED
    PROFILE_CPU();
    ZoneColor(TracyWaitZoneColor);

    while (Platform::AtomicRead(&ExitFlag) == 0)
    {
        const JobContext& context = JobContexts[GET_CONTEXT_INDEX(label)];
        const bool finished = Platform::AtomicRead(&context.JobLabel) != label || Platform::AtomicRead(&context.JobsLeft) <= 0;

        // Skip if context has been already executed (last job removes it)
        if (finished)
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
    if (value && (Platform::AtomicRead(&JobEndLabel) - Platform::AtomicRead(&JobStartLabel)) > 0)
    {
        // Wake up threads to start processing jobs that may be already in the queue
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
