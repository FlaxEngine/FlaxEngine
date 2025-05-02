// Copyright (c) Wojciech Figat. All rights reserved.

#include "MainThreadTask.h"
#include "Engine/Platform/CriticalSection.h"
#include "Engine/Profiler/ProfilerCPU.h"

namespace
{
    CriticalSection Locker;
    Array<MainThreadTask*> Waiting;
    Array<MainThreadTask*> Queue;
}

void MainThreadTask::RunAll(float dt)
{
    PROFILE_CPU();
    Locker.Lock();
    for (int32 i = Waiting.Count() - 1; i >= 0; i--)
    {
        auto task = Waiting[i];
        task->InitialDelay -= dt;
        if (task->InitialDelay < ZeroTolerance)
        {
            Waiting.RemoveAt(i);
            Queue.Add(task);
        }
    }
    for (int32 i = 0; i < Queue.Count(); i++)
    {
        Queue[i]->Execute();
    }
    Queue.Clear();
    Locker.Unlock();
}

String MainThreadTask::ToString() const
{
    return String::Format(TEXT("Main Thread Task ({0})"), (int32)GetState());
}

void MainThreadTask::Enqueue()
{
    Locker.Lock();
    if (InitialDelay <= ZeroTolerance)
        Queue.Add(this);
    else
        Waiting.Add(this);
    Locker.Unlock();
}

bool MainThreadActionTask::Run()
{
    if (_action1.IsBinded())
    {
        _action1();
        return false;
    }
    if (_action2.IsBinded())
    {
        return _action2();
    }
    return true;
}

bool MainThreadActionTask::HasReference(Object* obj) const
{
    return obj == _target;
}
