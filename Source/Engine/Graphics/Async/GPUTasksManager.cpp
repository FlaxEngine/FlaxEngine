// Copyright (c) Wojciech Figat. All rights reserved.

#include "GPUTasksManager.h"
#include "GPUTask.h"
#include "GPUTasksExecutor.h"
#include "Engine/Core/Log.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Graphics/GPUDevice.h"

void GPUTask::Execute(GPUTasksContext* context)
{
    ASSERT(IsQueued() && _context == nullptr);
    SetState(TaskState::Running);

    // Perform an operation
    const auto result = run(context);

    // Process result
    if (IsCancelRequested())
    {
        SetState(TaskState::Canceled);
    }
    else if (result != Result::Ok)
    {
        LOG(Warning, "\'{0}\' failed with result: {1}", ToString(), ToString(result));
        OnFail();
    }
    else
    {
        // Save task completion point (for synchronization)
        _syncPoint = context->GetCurrentSyncPoint();
        _context = context;
        if (_syncLatency == 0)
        {
            // No delay on sync
            Sync();
        }
    }
}

String GPUTask::ToString() const
{
    return String::Format(TEXT("GPU Async Task {0} ({1})"), ToString(GetType()), (int32)GetState());
}

void GPUTask::Enqueue()
{
    GPUDevice::Instance->GetTasksManager()->_tasks.Add(this);
}

void GPUTask::OnCancel()
{
    // Check if task is waiting for sync (very likely situation)
    if (IsSyncing())
    {
        // Task has been performed but is waiting for a CPU/GPU sync so we have to cancel that
        _context->OnCancelSync(this);
        _context = nullptr;
        SetState(TaskState::Canceled);
    }

    // Base
    Task::OnCancel();
}

GPUTasksManager::GPUTasksManager()
{
    _buffers[0].EnsureCapacity(64);
    _buffers[1].EnsureCapacity(64);
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

String GPUTasksManager::ToString() const
{
    return TEXT("GPU Tasks Manager");
}
