// Copyright (c) Wojciech Figat. All rights reserved.

#include "Task.h"
#include "ThreadPoolTask.h"
#include "Engine/Core/Log.h"
#include "Engine/Platform/Platform.h"
#include "Engine/Core/Types/DateTime.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Math/Math.h"
#include "Engine/Core/Types/Span.h"
#include "Engine/Profiler/ProfilerCPU.h"

void Task::Start()
{
    if (GetState() != TaskState::Created)
        return;

    OnStart();

    // Change state
    SetState(TaskState::Queued);

    // Add task to the execution queue
    Enqueue();
}

void Task::Cancel()
{
    if (Platform::AtomicRead(&_cancelFlag) == 0)
    {
        // Send event
        OnCancel();

        // Cancel child
        if (_continueWith)
            _continueWith->Cancel();
    }
}

bool Task::Wait(double timeoutMilliseconds) const
{
    PROFILE_CPU();
    double startTime = Platform::GetTimeSeconds() * 0.001;

    // TODO: no active waiting! use a semaphore!

    do
    {
        auto state = GetState();

        // Finished
        if (state == TaskState::Finished)
        {
            // Wait for child if has
            if (_continueWith)
            {
                auto spendTime = Platform::GetTimeSeconds() * 0.001 - startTime;
                return _continueWith->Wait(timeoutMilliseconds - spendTime);
            }

            return false;
        }

        // Failed or canceled
        if (state == TaskState::Failed || state == TaskState::Canceled)
            return true;

        Platform::Sleep(1);
    } while (timeoutMilliseconds <= 0.0 || Platform::GetTimeSeconds() * 0.001 - startTime < timeoutMilliseconds);

    // Timeout reached!
    LOG(Warning, "\'{0}\' has timed out. Wait time: {1} ms", ToString(), timeoutMilliseconds);
    return true;
}

bool Task::WaitAll(const Span<Task*>& tasks, double timeoutMilliseconds)
{
    PROFILE_CPU();
    for (int32 i = 0; i < tasks.Length(); i++)
    {
        if (tasks[i]->Wait())
            return true;
    }
    return false;
}

Task* Task::ContinueWith(Task* task)
{
    ASSERT(task != nullptr && task != this);
    if (_continueWith)
        return _continueWith->ContinueWith(task);
    _continueWith = task;
    return task;
}

Task* Task::ContinueWith(const Action& action, Object* target)
{
    // Get binded functions
    Array<Action::FunctionType> bindings;
    bindings.Resize(action.Count());
    action.GetBindings(bindings.Get(), bindings.Count());

    // Continue with every action
    Task* result = this;
    for (int32 i = 0; i < bindings.Count(); i++)
        result = result->ContinueWith(bindings[i], target);
    return result;
}

Task* Task::ContinueWith(const Function<void()>& action, Object* target)
{
    ASSERT(action.IsBinded());
    return ContinueWith(New<ThreadPoolActionTask>(action, target));
}

Task* Task::ContinueWith(const Function<bool()>& action, Object* target)
{
    ASSERT(action.IsBinded());
    return ContinueWith(New<ThreadPoolActionTask>(action, target));
}

Task* Task::StartNew(Task* task)
{
    ASSERT(task);
    task->Start();
    return task;
}

Task* Task::StartNew(const Function<void()>& action, Object* target)
{
    return StartNew(New<ThreadPoolActionTask>(action, target));
}

Task* Task::StartNew(const Function<void()>::Signature action, Object* target)
{
    return StartNew(New<ThreadPoolActionTask>(action, target));
}

Task* Task::StartNew(const Function<bool()>& action, Object* target)
{
    return StartNew(New<ThreadPoolActionTask>(action, target));
}

Task* Task::StartNew(Function<bool()>::Signature& action, Object* target)
{
    return StartNew(New<ThreadPoolActionTask>(action, target));
}

void Task::Execute()
{
    if (IsCanceled())
        return;
    ASSERT(IsQueued());
    SetState(TaskState::Running);

    // Perform an operation
    bool failed = Run();

    // Process result
    if (IsCancelRequested())
    {
        SetState(TaskState::Canceled);
    }
    else if (failed)
    {
        OnFail();
    }
    else
    {
        OnFinish();
    }
}

void Task::OnStart()
{
}

void Task::OnFinish()
{
    ASSERT(IsRunning() && !IsCancelRequested());
    SetState(TaskState::Finished);

    // Send event further
    if (_continueWith)
        _continueWith->Start();

    OnEnd();
}

void Task::OnFail()
{
    SetState(TaskState::Failed);

    // Send event further
    if (_continueWith)
        _continueWith->OnFail();

    OnEnd();
}

void Task::OnCancel()
{
    // Set flag
    Platform::InterlockedIncrement(&_cancelFlag);
    Platform::MemoryBarrier();

    // If task is active try to wait for a while
    if (IsRunning())
    {
        // Wait for it a little bit
        constexpr double timeout = 10000.0; // 10s
        LOG(Warning, "Cannot cancel \'{0}\' because it's still running, waiting for end with timeout: {1}ms", ToString(), timeout);
        Wait(timeout);
    }

    // Don't call OnEnd twice
    const auto state = GetState();
    if (state != TaskState::Finished && state != TaskState::Failed)
    {
        SetState(TaskState::Canceled);
        OnEnd();
    }
}

void Task::OnEnd()
{
    ASSERT(!IsRunning());

    // Add to delete
    DeleteObject(30.0f, false);
}
