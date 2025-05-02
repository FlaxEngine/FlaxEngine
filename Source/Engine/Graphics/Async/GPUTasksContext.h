// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "../GPUContext.h"
#include "Engine/Platform/CriticalSection.h"
#include "Engine/Core/Collections/Array.h"
#include "GPUSyncPoint.h"

class GPUTask;

/// <summary>
/// GPU tasks context
/// </summary>
class GPUTasksContext
{
protected:
    CriticalSection _locker;
    GPUSyncPoint _currentSyncPoint;
    int32 _totalTasksDoneCount = 0;
    Array<GPUTask*> _tasksSyncing;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="GPUTasksContext"/> class.
    /// </summary>
    /// <param name="device">The graphics device.</param>
    GPUTasksContext(GPUDevice* device);

    /// <summary>
    /// Finalizes an instance of the <see cref="GPUTasksContext"/> class.
    /// </summary>
    ~GPUTasksContext();

public:
    /// <summary>
    /// The GPU commands context used for tasks execution (can be only copy/upload without graphics capabilities on some platforms).
    /// </summary>
    GPUContext* GPU;

public:
    /// <summary>
    /// Gets graphics device handle
    /// </summary>
    FORCE_INLINE GPUDevice* GetDevice() const
    {
        return GPU->GetDevice();
    }

    /// <summary>
    /// Gets current synchronization point of that context (CPU position, GPU has some latency)
    /// </summary>
    FORCE_INLINE GPUSyncPoint GetCurrentSyncPoint() const
    {
        return _currentSyncPoint;
    }

    /// <summary>
    /// Gets total amount of tasks done by this context
    /// </summary>
    FORCE_INLINE int32 GetTotalTasksDoneCount() const
    {
        return _totalTasksDoneCount;
    }

    /// <summary>
    /// Perform given task
    /// </summary>
    /// <param name="task">Task to do</param>
    void Run(GPUTask* task);

    void OnCancelSync(GPUTask* task);

public:
    void OnFrameBegin();

    void OnFrameEnd();
};
