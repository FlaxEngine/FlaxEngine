// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Threading/IRunnable.h"

class Asset;
class LoadingThread;
class ContentLoadTask;

/// <summary>
/// Resources loading thread
/// </summary>
class LoadingThread : public IRunnable
{
protected:
    volatile int64 _exitFlag;
    Thread* _thread;
    int32 _totalTasksDoneCount;

public:
    /// <summary>
    /// Init
    /// </summary>
    LoadingThread();

    /// <summary>
    /// Destructor
    /// </summary>
    ~LoadingThread();

public:
    /// <summary>
    /// Gets the thread identifier.
    /// </summary>
    /// <returns>Thread ID</returns>
    uint64 GetID() const;

public:
    /// <summary>
    /// Returns true if thread has empty exit flag, so it can continue it's work
    /// </summary>
    /// <returns>True if exit flag is empty, otherwise false</returns>
    FORCE_INLINE bool HasExitFlagClear()
    {
        return Platform::AtomicRead(&_exitFlag) == 0;
    }

    /// <summary>
    /// Set exit flag to true so thread must exit
    /// </summary>
    void NotifyExit();

    /// <summary>
    /// Stops the calling thread execution until the loading thread ends its execution.
    /// </summary>
    void Join();

public:
    /// <summary>
    /// Starts thread execution.
    /// </summary>
    /// <param name="name">The thread name.</param>
    /// <returns>True if cannot start, otherwise false</returns>
    bool Start(const String& name);

    /// <summary>
    /// Runs the specified task.
    /// </summary>
    /// <param name="task">The task.</param>
    void Run(ContentLoadTask* task);

public:
    // [IRunnable]
    String ToString() const override;
    int32 Run() override;
    void Exit() override;
};

/// <summary>
/// Content loading manager.
/// </summary>
class ContentLoadingManager
{
    friend ContentLoadTask;
    friend LoadingThread;
    friend Asset;

public:
    /// <summary>
    /// Checks if current execution context is thread used to load assets.
    /// </summary>
    /// <returns>True if execution is in Load Thread, otherwise false.</returns>
    FORCE_INLINE static bool IsInLoadThread()
    {
        return GetCurrentLoadThread() != nullptr;
    }

    /// <summary>
    /// Gets content loading thread handle if current thread is one of them.
    /// </summary>
    /// <returns>Current load thread or null if current thread is different.</returns>
    static LoadingThread* GetCurrentLoadThread();

public:
    /// <summary>
    /// Gets amount of enqueued tasks to perform.
    /// </summary>
    /// <returns>The tasks count.</returns>
    static int32 GetTasksCount();
};
