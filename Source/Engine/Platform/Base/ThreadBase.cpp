// Copyright (c) Wojciech Figat. All rights reserved.

#include "Engine/Platform/Thread.h"
#include "Engine/Threading/IRunnable.h"
#include "Engine/Threading/ThreadRegistry.h"
#include "Engine/Core/Log.h"
#include "Engine/Scripting/ManagedCLR/MCore.h"
#if TRACY_ENABLE
#include "Engine/Core/Math/Math.h"
#include <ThirdParty/tracy/tracy/Tracy.hpp>
#endif

Delegate<Thread*> ThreadBase::ThreadStarting;
Delegate<Thread*, int32> ThreadBase::ThreadExiting;

ThreadBase::ThreadBase(IRunnable* runnable, const String& name, ThreadPriority priority)
    : _runnable(runnable)
    , _priority(priority)
    , _name(name)
    , _id(0)
    , _isRunning(false)
    , _callAfterWork(true)
{
    ASSERT(_runnable);

#if BUILD_DEBUG
    // Cache name (in case if object gets deleted somewhere)
    _runnableName = _runnable->ToString();
#endif
}

void ThreadBase::SetPriority(ThreadPriority priority)
{
    // Check if value won't change
    if (_priority != priority)
    {
        // Set new value
        _priority = priority;
        SetPriorityInternal(priority);
    }
}

void ThreadBase::Kill(bool waitForJoin)
{
    if (!_isRunning)
    {
        ClearHandleInternal();
        return;
    }
    ASSERT(GetID());
    const auto thread = static_cast<Thread*>(this);

    // Stop runnable object
    if (_callAfterWork && _runnable)
    {
        _runnable->Stop();
    }

    LOG(Info, "Killing thread \'{0}\' ID=0x{1:x}", _name, _id);

    // Kill platform thread
    KillInternal(waitForJoin);
    ClearHandleInternal();

    // End
    if (_callAfterWork)
    {
        _callAfterWork = false;
        _runnable->AfterWork(true);
    }
    _isRunning = false;
    ThreadRegistry::Remove(thread);
}

int32 ThreadBase::Run()
{
    // Setup
    ASSERT(_runnable);
    const auto thread = static_cast<Thread*>(this);
    _id = Platform::GetCurrentThreadID();
#if TRACY_ENABLE
    char threadName[100];
    const int32 threadNameLength = Math::Min<int32>(ARRAY_COUNT(threadName) - 1, _name.Length());
    StringUtils::ConvertUTF162ANSI(*_name, threadName, threadNameLength);
    threadName[threadNameLength] = 0;
    tracy::SetThreadName(threadName);
#endif
    ThreadRegistry::Add(thread);
    ThreadStarting(thread);
    int32 exitCode = 1;
    _isRunning = true;

    LOG(Info, "Thread \'{0}\' ID=0x{1:x} started with priority {2}", _name, _id, ::ToString(GetPriority()));

    if (_runnable->Init())
    {
        exitCode = _runnable->Run();

        if (_callAfterWork) // Prevent from calling this after calling AfterWork since object may be deleted
            _runnable->Exit();
    }

    LOG(Info, "Thread \'{0}\' ID=0x{1:x} exits with code {2}", _name, _id, exitCode);

    // End
    if (_callAfterWork)
    {
        _callAfterWork = false;
        _runnable->AfterWork(false);
    }
    _isRunning = false;
    ThreadExiting(thread, exitCode);
    ThreadRegistry::Remove(thread);
    MCore::Thread::Exit(); // TODO: use mono_thread_detach instead of ext and unlink mono runtime from thread in ThreadExiting delegate
    // mono terminates the native thread..

    return exitCode;
}
