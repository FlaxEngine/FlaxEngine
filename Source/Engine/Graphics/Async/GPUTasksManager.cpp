// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "GPUTasksManager.h"
#include "GPUTask.h"
#include "GPUTasksExecutor.h"
#include "Engine/Graphics/GPUDevice.h"

void GPUTask::Enqueue()
{
    GPUDevice::Instance->TasksManager._tasks.Add(this);
}

GPUTasksManager::GPUTasksManager(GPUDevice* device)
    : _device(device)
    , _executor(nullptr)
    , _bufferIndex(0)
{
    _buffers[0].EnsureCapacity(64);
    _buffers[1].EnsureCapacity(64);

    // Setup executor
    SetExecutor(device->CreateTasksExecutor());
    ASSERT(_executor != nullptr);
}

GPUTasksManager::~GPUTasksManager()
{
    // Ensure that Dispose has been called
    ASSERT(_executor == nullptr);
}

void GPUTasksManager::SetExecutor(GPUTasksExecutor* value)
{
    if (value != _executor && value)
    {
        if (_executor)
        {
            SAFE_DELETE(_executor);
        }

        _executor = value;
    }
}

void GPUTasksManager::Dispose()
{
    // Cancel all performed tasks (that are waiting for sync)
    SAFE_DELETE(_executor);

    // Cleanup
    Task::CancelAll(_buffers[0]);
    Task::CancelAll(_buffers[1]);
    _bufferIndex = 0;
    _tasks.CancelAll();
}

void GPUTasksManager::FrameBegin()
{
    _executor->FrameBegin();
}

void GPUTasksManager::FrameEnd()
{
    _executor->FrameEnd();
}

int32 GPUTasksManager::RequestWork(GPUTask** buffer, int32 maxCount)
{
    const auto b1Index = _bufferIndex;
    const auto b2Index = (_bufferIndex + 1) % 2;
    auto& b1 = _buffers[b1Index];
    auto& b2 = _buffers[b2Index];

    // Take maximum amount of tasks to the buffer at once
    ASSERT(b1.IsEmpty());
    const int32 takenTasksCount = (int32)_tasks.try_dequeue_bulk(b1.Get(), maxCount);
    b2.Add(b1.Get(), takenTasksCount);

    int32 count = 0;

    // Filter taken tasks to keep maxTotalComplexity limit
    b1.Clear();
    int32 i = 0;
    for (; i < b2.Count() && count < maxCount; i++)
    {
        auto task = b2[i];
        const auto state = task->GetState();
        switch (state)
        {
        case TaskState::Failed:
        case TaskState::Canceled:
        case TaskState::Finished:
            // Skip task
            break;
        case TaskState::Queued:
            // Run queued task
            buffer[count++] = task;
            break;
        case TaskState::Created:
        case TaskState::Running:
        default:
            // Keep task for the next RequestWork
            b1.Add(task);
            break;
        }
    }
    const int32 itemsLeft = b2.Count() - i;
    if (itemsLeft > 0)
        b1.Add(&b2[i], itemsLeft);
    b2.Clear();

    // Swap buffers
    _bufferIndex = b2Index;

    return count;
}
