// Copyright (c) 2012-2023 Wojciech Figat. All rights reserved.

#include "GPUTasksContext.h"
#include "GPUTask.h"
#include "Engine/Core/Log.h"
#include "Engine/Graphics/GPUDevice.h"
#include "Engine/Threading/Threading.h"

#define GPU_TASKS_USE_DEDICATED_CONTEXT 0

GPUTasksContext::GPUTasksContext(GPUDevice* device)
    : _tasksDone(64)
    , _totalTasksDoneCount(0)
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
    auto tasks = _tasksDone;
    _tasksDone.Clear();
    for (int32 i = 0; i < tasks.Count(); i++)
    {
        auto task = tasks[i];
        LOG(Warning, "{0} has been canceled before a sync", task->ToString());
        tasks[i]->CancelSync();
    }

#if GPU_TASKS_USE_DEDICATED_CONTEXT
	SAFE_DELETE(GPU);
#endif
}

void GPUTasksContext::Run(GPUTask* task)
{
    ASSERT(task != nullptr);

    _tasksDone.Add(task);
    task->Execute(this);
}

void GPUTasksContext::OnCancelSync(GPUTask* task)
{
    ASSERT(task != nullptr);

    _tasksDone.Remove(task);

    LOG(Warning, "{0} has been canceled before a sync", task->ToString());
}

void GPUTasksContext::OnFrameBegin()
{
#if GPU_TASKS_USE_DEDICATED_CONTEXT
	GPU->FrameBegin();
#endif

    // Move forward one frame
    ++_currentSyncPoint;

    // Try to flush done jobs
    auto currentSyncPointGPU = _currentSyncPoint - GPU_ASYNC_LATENCY;
    for (int32 i = 0; i < _tasksDone.Count(); i++)
    {
        if (_tasksDone[i]->GetSyncPoint() <= currentSyncPointGPU)
        {
            // TODO: add stats counter and count performed jobs, print to log on exit.

            auto job = _tasksDone[i];
            job->Sync();

            _tasksDone.RemoveAt(i);
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
