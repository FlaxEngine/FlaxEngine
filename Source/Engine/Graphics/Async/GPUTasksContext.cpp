// Copyright (c) Wojciech Figat. All rights reserved.

#include "GPUTasksContext.h"
#include "GPUTask.h"
#include "Engine/Core/Log.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Threading/Threading.h"
#include "Engine/Engine/Engine.h"

#define GPU_TASKS_USE_DEDICATED_CONTEXT 0

GPUTasksContext::GPUTasksContext(GPUDevice* device)
{
    // Bump it up to prevent initial state problems with frame index comparison
    _currentSyncPoint = 10;

#if GPU_TASKS_USE_DEDICATED_CONTEXT
	// Create GPU context
	GPU = device->CreateContext(true);
#else
    // Reuse Graphics Device context
    GPU = device->GetMainContext();
#endif
}

GPUTasksContext::~GPUTasksContext()
{
    ASSERT(IsInMainThread());

    // Cancel jobs to sync
    auto tasks = _tasksSyncing;
    _tasksSyncing.Clear();
    for (int32 i = 0; i < tasks.Count(); i++)
    {
        auto task = tasks[i];
        if (task->GetSyncPoint() <= _currentSyncPoint && task->GetState() != TaskState::Finished)
        {
            if (!Engine::IsRequestingExit)
                LOG(Warning, "'{0}' has been canceled before a sync", task->ToString());
            task->CancelSync();
        }
    }

#if GPU_TASKS_USE_DEDICATED_CONTEXT
	SAFE_DELETE(GPU);
#endif
}

void GPUTasksContext::Run(GPUTask* task)
{
    ASSERT(task != nullptr);

    task->Execute(this);

    // Keep only the task that waits for a sync. A task that already ended (finished, failed or canceled) is queued for deletion,
    // so the list would hold a pointer to the deleted object when the next frame comes after its time to live (eg. after a long
    // main thread stall).
    if (task->IsSyncing())
        _tasksSyncing.Add(task);
    else if (task->GetState() == TaskState::Finished)
        _totalTasksDoneCount++;
}

void GPUTasksContext::OnCancelSync(GPUTask* task)
{
    _tasksSyncing.Remove(task);
    if (!Engine::IsRequestingExit)
        LOG(Warning, "'{0}' has been canceled before a sync", task->ToString());
}

void GPUTasksContext::OnFrameBegin()
{
#if GPU_TASKS_USE_DEDICATED_CONTEXT
	GPU->FrameBegin();
#endif

    // Move forward one frame
    ++_currentSyncPoint;

    // Try to flush done jobs
    for (int32 i = 0; i < _tasksSyncing.Count(); i++)
    {
        auto task = _tasksSyncing[i];
        auto state = task->GetState();
        if (EnumHasAllFlags(task->Flags, ObjectFlags::WasMarkedToDelete))
            state = TaskState::Finished;
        if (task->GetSyncPoint() <= _currentSyncPoint && state != TaskState::Finished)
        {
            task->Sync();

            // The sync ends the task and queues it for deletion, so remove it from the list now and not in the next frame
            state = task->GetState();
        }
        if (state == TaskState::Failed || state == TaskState::Canceled)
        {
            _tasksSyncing.RemoveAt(i);
            i--;
        }
        if (state == TaskState::Finished)
        {
            _tasksSyncing.RemoveAt(i);
            i--;
            _totalTasksDoneCount++;
        }
    }
}

void GPUTasksContext::OnFrameEnd()
{
#if GPU_TASKS_USE_DEDICATED_CONTEXT
	GPU->FrameEnd();
#endif
}
