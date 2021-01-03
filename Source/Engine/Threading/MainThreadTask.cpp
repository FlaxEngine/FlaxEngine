// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "MainThreadTask.h"
#include "ConcurrentTaskQueue.h"
#include "Engine/Profiler/ProfilerCPU.h"

ConcurrentTaskQueue<MainThreadTask> MainThreadTasks;

void MainThreadTask::RunAll()
{
    // TODO: use bulk dequeue

    PROFILE_CPU();

    MainThreadTask* task;
    while (MainThreadTasks.try_dequeue(task))
    {
        task->Execute();
    }
}

void MainThreadTask::Enqueue()
{
    MainThreadTasks.Add(this);
}
