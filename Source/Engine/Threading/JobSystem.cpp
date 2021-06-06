// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "JobSystem.h"
#include "IRunnable.h"
#include "Engine/Core/Collections/RingBuffer.h"
#include "Engine/Platform/CPUInfo.h"
#include "Engine/Platform/Thread.h"
#include "Engine/Platform/ConditionVariable.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Profiler/ProfilerCPU.h"

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
    int32 Count;
};

template<>
struct TIsPODType<JobData>
{
    enum { Value = true };
};

class JobSystemThread : public IRunnable
{
public:
    int32 Index;

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
    Thread* Threads[32] = {};
    int32 ThreadsCount = 0;
    volatile int64 ExitFlag = 0;
    volatile int64 DoneLabel = 0;
    volatile int64 NextLabel = 0;
    CriticalSection JobsLocker;
    ConditionVariable JobsSignal;
    ConditionVariable WaitSignal;
    RingBuffer<JobData, InlinedAllocation<256>> Jobs;
}

bool JobSystemService::Init()
{
    ThreadsCount = Math::Min<int32>(Platform::GetCPUInfo().LogicalProcessorCount, ARRAY_COUNT(Threads));
    for (int32 i = 0; i < ThreadsCount; i++)
    {
        auto runnable = New<JobSystemThread>();
        runnable->Index = i;
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
        if (Threads[i] && Threads[i]->IsRunning())
            Threads[i]->Kill(true);
        Threads[i] = nullptr;
    }
}

int32 JobSystemThread::Run()
{
    Platform::SetThreadAffinityMask(1 << Index);

    JobData data;
    CriticalSection mutex;
    while (Platform::AtomicRead(&ExitFlag) == 0)
    {
        // Try to get a job
        JobsLocker.Lock();
        if (Jobs.Count() != 0)
        {
            auto& front = Jobs.PeekFront();
            data = front;
            front.Index++;
            if (front.Index == front.Count)
            {
                Jobs.PopFront();
            }
        }
        JobsLocker.Unlock();

        if (data.Job.IsBinded())
        {
            // Run job
            data.Job(data.Index);
            data.Job.Unbind();

            if (data.Index + 1 == data.Count)
            {
                // Move forward with the job queue
                Platform::InterlockedIncrement(&DoneLabel);
                WaitSignal.NotifyAll();
            }
        }
        else
        {
            // Wait for signal
            mutex.Lock();
            JobsSignal.Wait(mutex);
            mutex.Unlock();
        }
    }
    return 0;
}

int64 JobSystem::Dispatch(const Function<void(int32)>& job, int32 jobCount)
{
    PROFILE_CPU();
    if (jobCount <= 0)
        return 0;

    JobData data;
    data.Job = job;
    data.Index = 0;
    data.Count = jobCount;

    JobsLocker.Lock();
    const auto label = Platform::InterlockedIncrement(&NextLabel);
    Jobs.PushBack(data);
    JobsLocker.Unlock();

    if (jobCount == 1)
        JobsSignal.NotifyOne();
    else
        JobsSignal.NotifyAll();

    return label;
}

void JobSystem::Wait()
{
    Wait(Platform::AtomicRead(&NextLabel));
}

void JobSystem::Wait(int64 label)
{
    PROFILE_CPU();

    // Early out
    if (label <= Platform::AtomicRead(&DoneLabel))
        return;

    // Wait on signal until input label is not yet done
    CriticalSection mutex;
    while (label > Platform::AtomicRead(&DoneLabel) && Platform::AtomicRead(&ExitFlag) == 0)
    {
        mutex.Lock();
        WaitSignal.Wait(mutex);
        mutex.Unlock();
    }
}
