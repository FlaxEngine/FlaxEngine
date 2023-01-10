// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Object.h"
#include "Engine/Core/NonCopyable.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Threading/ConcurrentTaskQueue.h"
#include "GPUTask.h"

class GPUDevice;
class GPUResource;
class GPUTasksContext;
class GPUTasksExecutor;

/// <summary>
/// Graphics Device work manager.
/// </summary>
class GPUTasksManager : public Object, public NonCopyable
{
    friend GPUDevice;
    friend GPUTask;

private:
    GPUDevice* _device;
    GPUTasksExecutor* _executor;
    ConcurrentTaskQueue<GPUTask> _tasks;
    Array<GPUTask*> _buffers[2];
    int32 _bufferIndex;

private:
    GPUTasksManager(GPUDevice* device);
    ~GPUTasksManager();

public:
    /// <summary>
    /// Gets the parent Graphics Device.
    /// </summary>
    /// <returns>The device.</returns>
    FORCE_INLINE GPUDevice* GetDevice() const
    {
        return _device;
    }

    /// <summary>
    /// Gets the GPU tasks executor.
    /// </summary>
    /// <returns>The tasks executor.</returns>
    FORCE_INLINE GPUTasksExecutor* GetExecutor() const
    {
        return _executor;
    }

    /// <summary>
    /// Sets the GPU tasks executor.
    /// </summary>
    /// <param name="value">The tasks executor.</param>
    void SetExecutor(GPUTasksExecutor* value);

    /// <summary>
    /// Gets the amount of enqueued tasks to perform.
    /// </summary>
    /// <returns>The tasks count.</returns>
    FORCE_INLINE int32 GetTaskCount() const
    {
        return _tasks.Count();
    }

public:
    /// <summary>
    /// Clears asynchronous resources loading queue and cancels all tasks.
    /// </summary>
    void Dispose();

public:
    /// <summary>
    /// On begin rendering frame.
    /// </summary>
    void FrameBegin();

    /// <summary>
    /// On end rendering frame.
    /// </summary>
    void FrameEnd();

public:
    /// <summary>
    /// Requests work to do. Should be used only by GPUTasksExecutor.
    /// </summary>
    /// <param name="buffer">The output buffer.</param>
    /// <param name="maxCount">The maximum allowed amount of tasks to get.</param>
    /// <returns>The amount of tasks added to the buffer.</returns>
    int32 RequestWork(GPUTask** buffer, int32 maxCount);

public:
    // [Object]
    String ToString() const override
    {
        return TEXT("GPU Tasks Manager");
    }
};
