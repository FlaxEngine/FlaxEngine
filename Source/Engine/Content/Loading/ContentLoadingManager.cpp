// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "ContentLoadingManager.h"
#include "ContentLoadTask.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Platform/CPUInfo.h"
#include "Engine/Platform/Thread.h"
#include "Engine/Platform/ConditionVariable.h"
#include "Engine/Content/Config.h"
#include "Engine/Engine/EngineService.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Threading/ConcurrentTaskQueue.h"
#if USE_EDITOR && PLATFORM_WINDOWS
#include "Engine/Platform/Win32/IncludeWindowsHeaders.h"
#include <propidlbase.h>
#endif

namespace ContentLoadingManagerImpl
{
    THREADLOCAL LoadingThread* ThisThread = nullptr;
    LoadingThread* MainThread = nullptr;
    Array<LoadingThread*> Threads;
    ConcurrentTaskQueue<ContentLoadTask> Tasks;
    ConditionVariable TasksSignal;
    CriticalSection TasksMutex;
};

using namespace ContentLoadingManagerImpl;

class ContentLoadingManagerService : public EngineService
{
public:
    ContentLoadingManagerService()
        : EngineService(TEXT("Content Loading Manager"), -500)
    {
    }

    bool Init() override;
    void BeforeExit() override;
    void Dispose() override;
};

ContentLoadingManagerService ContentLoadingManagerServiceInstance;

LoadingThread::LoadingThread()
    : _exitFlag(false)
    , _thread(nullptr)
    , _totalTasksDoneCount(0)
{
}

LoadingThread::~LoadingThread()
{
    // Check if has thread attached
    if (_thread != nullptr)
    {
        _thread->Kill(true);
        Delete(_thread);
    }
}

uint64 LoadingThread::GetID() const
{
    return _thread ? _thread->GetID() : 0;
}

void LoadingThread::NotifyExit()
{
    Platform::InterlockedIncrement(&_exitFlag);
}

void LoadingThread::Join()
{
    auto thread = _thread;
    if (thread)
        thread->Join();
}

bool LoadingThread::Start(const String& name)
{
    ASSERT(_thread == nullptr && name.HasChars());

    // Create new thread
    auto thread = Thread::Create(this, name, ThreadPriority::Normal);
    if (thread == nullptr)
        return true;

    _thread = thread;

    return false;
}

void LoadingThread::Run(ContentLoadTask* job)
{
    ASSERT(job);

    job->Execute();
    _totalTasksDoneCount++;
}

String LoadingThread::ToString() const
{
    return String::Format(TEXT("Loading Thread {0}"), GetID());
}

int32 LoadingThread::Run()
{
#if USE_EDITOR && PLATFORM_WINDOWS

    // Initialize COM
    // TODO: maybe add sth to Thread::Create to indicate that thread will use COM stuff
    const auto result = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(result))
    {
        LOG(Error, "Failed to init COM for WIC texture importing! Result: {0:x}", static_cast<uint32>(result));
        return -1;
    }

#endif

    ContentLoadTask* task;
    ThisThread = this;

    while (HasExitFlagClear())
    {
        if (Tasks.try_dequeue(task))
        {
            Run(task);
        }
        else
        {
            TasksMutex.Lock();
            TasksSignal.Wait(TasksMutex);
            TasksMutex.Unlock();
        }
    }

    ThisThread = nullptr;
    return 0;
}

void LoadingThread::Exit()
{
    // Send info
    ASSERT_LOW_LAYER(_thread);
    LOG(Info, "Content thread '{0}' exited. Load calls: {1}", _thread->GetName(), _totalTasksDoneCount);
}

LoadingThread* ContentLoadingManager::GetCurrentLoadThread()
{
    return ThisThread;
}

int32 ContentLoadingManager::GetTasksCount()
{
    return Tasks.Count();
}

bool ContentLoadingManagerService::Init()
{
    ASSERT(ContentLoadingManagerImpl::Threads.IsEmpty() && IsInMainThread());

    // Calculate amount of loading threads to use
    const CPUInfo cpuInfo = Platform::GetCPUInfo();
    const int32 count = Math::Clamp(Math::CeilToInt(LOADING_THREAD_PER_LOGICAL_CORE * (float)cpuInfo.LogicalProcessorCount), 1, 12);
    LOG(Info, "Creating {0} content loading threads...", count);

    // Create loading threads
    MainThread = New<LoadingThread>();
    ThisThread = MainThread;
    Threads.EnsureCapacity(count);
    for (int32 i = 0; i < count; i++)
    {
        auto thread = New<LoadingThread>();
        if (thread->Start(String::Format(TEXT("Load Thread {0}"), i)))
        {
            LOG(Fatal, "Cannot spawn content thread {0}/{1}", i, count);
            Delete(thread);
            return true;
        }
        Threads.Add(thread);
    }

    return false;
}

void ContentLoadingManagerService::BeforeExit()
{
    // Signal threads to end work soon
    for (int32 i = 0; i < Threads.Count(); i++)
        Threads[i]->NotifyExit();
    TasksSignal.NotifyAll();
}

void ContentLoadingManagerService::Dispose()
{
    // Exit all threads
    for (int32 i = 0; i < Threads.Count(); i++)
        Threads[i]->NotifyExit();
    TasksSignal.NotifyAll();
    for (int32 i = 0; i < Threads.Count(); i++)
        Threads[i]->Join();
    Threads.ClearDelete();
    Delete(MainThread);
    MainThread = nullptr;
    ThisThread = nullptr;

    // Cancel all remaining tasks (no chance to execute them)
    Tasks.CancelAll();
}

String ContentLoadTask::ToString() const
{
    return String::Format(TEXT("Content Load Task ({})"), (int32)GetState());
}

void ContentLoadTask::Enqueue()
{
    Tasks.Add(this);
    TasksSignal.NotifyOne();
}

bool ContentLoadTask::Run()
{
    // Perform an operation
    const auto result = run();

    // Process result
    const bool failed = result != Result::Ok;
    if (failed)
    {
        LOG(Warning, "\'{0}\' failed with result: {1}", ToString(), ToString(result));
    }
    return failed;
}
