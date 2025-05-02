// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Object.h"
#include "Engine/Core/Delegate.h"
#include "Engine/Core/NonCopyable.h"
#include "Engine/Core/Enums.h"
#include "Engine/Core/Types/TimeSpan.h"
#include "Engine/Platform/Platform.h"

/// <summary>
/// Represents the current stage in the lifecycle of a Task.
/// </summary>
enum class TaskState : int64
{
    Created = 0,
    Failed,
    Canceled,
    Queued,
    Running,
    Finished
};

/// <summary>
/// Represents an asynchronous operation.
/// </summary>
class FLAXENGINE_API Task : public Object, public NonCopyable
{
    //
    // Tasks execution and states flow:
    //
    //  Task() [Created]
    //   \/
    // Start() [Queued]
    //   \/
    //  Run()  [Running]
    //    |
    //    ------------------------
    //    \/                     \/
    //  Finish() [Finished]  Fail/Cancel() [Failed/Canceled]
    //    \/                     \/
    //  child.Start()       child.Cancel()
    //    |                      \/
    //    -------------------------
    //    \/
    //   End()
    //

protected:
    /// <summary>
    /// The cancel flag used to indicate that there is request to cancel task operation.
    /// </summary>
    volatile int64 _cancelFlag = 0;

    /// <summary>
    /// The current task state.
    /// </summary>
    volatile TaskState _state = TaskState::Created;

    /// <summary>
    /// The task to start after finish.
    /// </summary>
    Task* _continueWith = nullptr;

    void SetState(TaskState state)
    {
        Platform::AtomicStore((int64 volatile*)&_state, (uint64)state);
    }

public:
    /// <summary>
    /// Gets the task state.
    /// </summary>
    FORCE_INLINE TaskState GetState() const
    {
        return (TaskState)Platform::AtomicRead((int64 const volatile*)&_state);
    }

    /// <summary>
    /// Determines whether the specified object has reference to the given object.
    /// </summary>
    /// <param name="obj">The object.</param>
    /// <returns>True if the specified object has reference to the given object, otherwise false.</returns>
    virtual bool HasReference(Object* obj) const
    {
        return false;
    }

    /// <summary>
    /// Gets the task to start after this one.
    /// </summary>
    FORCE_INLINE Task* GetContinueWithTask() const
    {
        return _continueWith;
    }

public:
    /// <summary>
    /// Checks if operation failed.
    /// </summary>
    FORCE_INLINE bool IsFailed() const
    {
        return GetState() == TaskState::Failed;
    }

    /// <summary>
    /// Checks if operation has been canceled.
    /// </summary>
    FORCE_INLINE bool IsCanceled() const
    {
        return GetState() == TaskState::Canceled;
    }

    /// <summary>
    /// Checks if operation has been queued.
    /// </summary>
    FORCE_INLINE bool IsQueued() const
    {
        return GetState() == TaskState::Queued;
    }

    /// <summary>
    /// Checks if operation is running.
    /// </summary>
    FORCE_INLINE bool IsRunning() const
    {
        return GetState() == TaskState::Running;
    }

    /// <summary>
    /// Checks if operation has been finished.
    /// </summary>
    FORCE_INLINE bool IsFinished() const
    {
        return GetState() == TaskState::Finished;
    }

    /// <summary>
    /// Checks if operation has been ended (via cancel, fail or finish).
    /// </summary>
    bool IsEnded() const
    {
        auto state = GetState();
        return state == TaskState::Failed || state == TaskState::Canceled || state == TaskState::Finished;
    }

    /// <summary>
    /// Returns true if task has been requested to cancel it's operation.
    /// </summary>
    FORCE_INLINE bool IsCancelRequested()
    {
        return Platform::AtomicRead(&_cancelFlag) != 0;
    }

public:
    /// <summary>
    /// Starts this task execution (and will continue with all children).
    /// </summary>
    void Start();

    /// <summary>
    /// Cancels this task (and all child tasks).
    /// </summary>
    void Cancel();

    /// <summary>
    /// Waits the specified timeout for the task to be finished.
    /// </summary>
    /// <param name="timeout">The maximum amount of time to wait for the task to finish it's job. Timeout smaller/equal 0 will result in infinite waiting.</param>
    /// <returns>True if task failed or has been canceled or has timeout, otherwise false.</returns>
    FORCE_INLINE bool Wait(const TimeSpan& timeout) const
    {
        return Wait(timeout.GetTotalMilliseconds());
    }

    /// <summary>
    /// Waits the specified timeout for the task to be finished.
    /// </summary>
    /// <param name="timeoutMilliseconds">The maximum amount of milliseconds to wait for the task to finish it's job. Timeout smaller/equal 0 will result in infinite waiting.</param>
    /// <returns>True if task failed or has been canceled or has timeout, otherwise false.</returns>
    bool Wait(double timeoutMilliseconds = -1) const;

    /// <summary>
    /// Waits for all the tasks from the list.
    /// </summary>
    /// <param name="tasks">The tasks list to wait for.</param>
    /// <param name="timeoutMilliseconds">The maximum amount of milliseconds to wait for the task to finish it's job. Timeout smaller/equal 0 will result in infinite waiting.</param>
    /// <returns>True if any task failed or has been canceled or has timeout, otherwise false.</returns>
    static bool WaitAll(const Span<Task*>& tasks, double timeoutMilliseconds = -1);

    /// <summary>
    /// Waits for all the tasks from the list.
    /// </summary>
    /// <param name="tasks">The tasks list to wait for.</param>
    /// <param name="timeoutMilliseconds">The maximum amount of milliseconds to wait for the task to finish it's job. Timeout smaller/equal 0 will result in infinite waiting.</param>
    /// <returns>True if any task failed or has been canceled or has timeout, otherwise false.</returns>
    template<class T = Task, typename AllocationType = HeapAllocation>
    static bool WaitAll(const Array<T*, AllocationType>& tasks, double timeoutMilliseconds = -1)
    {
        return WaitAll(ToSpan(tasks), timeoutMilliseconds);
    }

public:
    /// <summary>
    /// Continues that task execution with a given task (will call Start on given task after finishing that one).
    /// </summary>
    /// <param name="task">The task to Start after current finish (will propagate OnCancel event if need to).</param>
    /// <returns>Enqueued task.</returns>
    Task* ContinueWith(Task* task);

    /// <summary>
    /// Continues that task execution with a given action (will spawn new async action).
    /// </summary>
    /// <param name="action">Action to run.</param>
    /// <param name="target">The action target object.</param>
    /// <returns>Enqueued task.</returns>
    Task* ContinueWith(const Action& action, Object* target = nullptr);

    /// <summary>
    /// Continues that task execution with a given action (will spawn new async action).
    /// </summary>
    /// <param name="action">Action to run.</param>
    /// <param name="target">The action target object.</param>
    /// <returns>Enqueued task.</returns>
    Task* ContinueWith(const Function<void()>& action, Object* target = nullptr);

    /// <summary>
    /// Continues that task execution with a given action (will spawn new async action).
    /// </summary>
    /// <param name="action">Action to run.</param>
    /// <param name="target">The action target object.</param>
    /// <returns>Enqueued task.</returns>
    Task* ContinueWith(const Function<bool()>& action, Object* target = nullptr);

public:
    /// <summary>
    /// Starts the new task.
    /// </summary>
    /// <param name="task">The task.</param>
    /// <returns>Task</returns>
    static Task* StartNew(Task* task);

    /// <summary>
    /// Starts the new task.
    /// </summary>
    /// <param name="action">The action.</param>
    /// <param name="target">The action target object.</param>
    /// <returns>Task</returns>
    static Task* StartNew(const Function<void()>& action, Object* target = nullptr);

    /// <summary>
    /// Starts the new task.
    /// </summary>
    /// <param name="action">The action.</param>
    /// <param name="target">The action target object.</param>
    /// <returns>Task</returns>
    static Task* StartNew(Function<void()>::Signature action, Object* target = nullptr);

    /// <summary>
    /// Starts the new task.
    /// </summary>
    /// <param name="callee">The callee object.</param>
    /// <returns>Task</returns>
    template<class T, void(T::* Method)()>
    static Task* StartNew(T* callee)
    {
        Function<void()> action;
        action.Bind<T, Method>(callee);
        return StartNew(action, dynamic_cast<Object*>(callee));
    }

    /// <summary>
    /// Starts the new task.
    /// </summary>
    /// <param name="action">The action.</param>
    /// <param name="target">The action target object.</param>
    /// <returns>Task</returns>
    static Task* StartNew(const Function<bool()>& action, Object* target = nullptr);

    /// <summary>
    /// Starts the new task.
    /// </summary>
    /// <param name="action">The action.</param>
    /// <param name="target">The action target object.</param>
    /// <returns>Task</returns>
    static Task* StartNew(Function<bool()>::Signature& action, Object* target = nullptr);

    /// <summary>
    /// Starts the new task.
    /// </summary>
    /// <param name="callee">The callee object.</param>
    /// <returns>Task</returns>
    template<class T, bool(T::*Method)()>
    static Task* StartNew(T* callee)
    {
        Function<bool()> action;
        action.Bind<T, Method>(callee);
        return StartNew(action, dynamic_cast<Object*>(callee));
    }

    /// <summary>
    /// Cancels all the tasks from the list and clears it.
    /// </summary>
    template<class T = Task, typename AllocationType = HeapAllocation>
    static void CancelAll(Array<T*, AllocationType>& tasks)
    {
        for (int32 i = 0; i < tasks.Count(); i++)
            tasks[i]->Cancel();
        tasks.Clear();
    }

protected:
    /// <summary>
    /// Executes this task. It should be called by the task consumer (thread pool or other executor of this task type). It calls run() and handles result).
    /// </summary>
    void Execute();

    /// <summary>
    /// Runs the task specified operations. It does not handle any task related logic, but only performs the actual job.
    /// </summary>
    /// <returns>The task execution result. Returns true if failed, otherwise false.</returns>
    virtual bool Run() = 0;

protected:
    virtual void Enqueue() = 0;
    virtual void OnStart();
    virtual void OnFinish();
    virtual void OnFail();
    virtual void OnCancel();
    virtual void OnEnd();
};
