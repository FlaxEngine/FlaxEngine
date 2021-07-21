// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#include "TaskGraph.h"
#include "JobSystem.h"
#include "Engine/Core/Collections/Sorting.h"
#include "Engine/Profiler/ProfilerCPU.h"

namespace
{
    bool SortTaskGraphSystem(TaskGraphSystem* const& a, TaskGraphSystem* const& b)
    {
        return b->Order > a->Order;
    };
}

TaskGraphSystem::TaskGraphSystem(const SpawnParams& params)
    : PersistentScriptingObject(params)
{
}

void TaskGraphSystem::AddDependency(TaskGraphSystem* system)
{
    _dependencies.Add(system);
}

void TaskGraphSystem::PreExecute(TaskGraph* graph)
{
}

void TaskGraphSystem::Execute(TaskGraph* graph)
{
}

void TaskGraphSystem::PostExecute(TaskGraph* graph)
{
}

TaskGraph::TaskGraph(const SpawnParams& params)
    : PersistentScriptingObject(params)
{
}

const Array<TaskGraphSystem*, InlinedAllocation<64>>& TaskGraph::GetSystems() const
{
    return _systems;
}

void TaskGraph::AddSystem(TaskGraphSystem* system)
{
    _systems.Add(system);
}

void TaskGraph::RemoveSystem(TaskGraphSystem* system)
{
    _systems.Remove(system);
}

void TaskGraph::Execute()
{
    PROFILE_CPU();

    for (auto system : _systems)
        system->PreExecute(this);

    _queue.Clear();
    _remaining.Clear();
    _remaining.Add(_systems);

    while (_remaining.HasItems())
    {
        // Find systems without dependencies or with already executed dependencies
        for (int32 i = _remaining.Count() - 1; i >= 0; i--)
        {
            auto e = _remaining[i];
            bool hasReadyDependencies = true;
            for (auto d : e->_dependencies)
            {
                if (_remaining.Contains(d))
                {
                    hasReadyDependencies = false;
                    break;
                }
            }
            if (hasReadyDependencies)
            {
                _queue.Add(e);
                _remaining.RemoveAt(i);
            }
        }

        // End if no systems left
        if (_queue.IsEmpty())
            break;

        // Execute in order
        Sorting::QuickSort(_queue.Get(), _queue.Count(), &SortTaskGraphSystem);
        JobSystem::SetJobStartingOnDispatch(false);
        _currentLabel = 0;
        for (int32 i = 0; i < _queue.Count(); i++)
        {
            _currentSystem = _queue[i];
            _currentSystem->Execute(this);
        }
        _currentSystem = nullptr;
        _queue.Clear();

        // Wait for async jobs to finish
        JobSystem::SetJobStartingOnDispatch(true);
        JobSystem::Wait(_currentLabel);
    }

    for (auto system : _systems)
        system->PostExecute(this);
}

void TaskGraph::DispatchJob(const Function<void(int32)>& job, int32 jobCount)
{
    ASSERT(_currentSystem);
    _currentLabel = JobSystem::Dispatch(job, jobCount);
}
