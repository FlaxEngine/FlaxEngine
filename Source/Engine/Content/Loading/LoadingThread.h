// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Threading/IRunnable.h"

/// <summary>
/// Resources loading thread.
/// </summary>
class LoadingThread : public IRunnable
{
protected:
    volatile int64 _exitFlag;
    Thread* _thread;
    int32 _totalTasksDoneCount;

public:
    LoadingThread();
    ~LoadingThread();

public:
    /// <summary>
    /// Set exit flag to true so thread must exit
    /// </summary>
    void NotifyExit();

    /// <summary>
    /// Stops the calling thread execution until the loading thread ends its execution.
    /// </summary>
    void Join();

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
    void Run(class ContentLoadTask* task);

public:
    // [IRunnable]
    String ToString() const override;
    int32 Run() override;
    void Exit() override;
};
