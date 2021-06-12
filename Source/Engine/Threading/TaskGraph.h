// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Core/Collections/Array.h"

class TaskGraph;

/// <summary>
/// System that can generate work into Task Graph for asynchronous execution.
/// </summary>
API_CLASS(Abstract) class FLAXENGINE_API TaskGraphSystem : public PersistentScriptingObject
{
DECLARE_SCRIPTING_TYPE(TaskGraphSystem);
    friend TaskGraph;
private:
    Array<TaskGraphSystem*, InlinedAllocation<16>> _dependencies;

public:
    /// <summary>
    /// The execution order of the system (systems with higher order are executed earlier).
    /// </summary>
    API_FIELD() int32 Order = 0;

public:
    /// <summary>
    /// Adds the dependency on the system execution. Before this system can be executed the given dependant system has to be executed first.
    /// </summary>
    /// <param name="system">The system to depend on.</param>
    API_FUNCTION() void AddDependency(TaskGraphSystem* system);

    /// <summary>
    /// Called before executing any systems of the graph. Can be used to initialize data (synchronous).
    /// </summary>
    /// <param name="graph">The graph executing the system.</param>
    API_FUNCTION() virtual void PreExecute(TaskGraph* graph);

    /// <summary>
    /// Executes the system logic and schedules the asynchronous work.
    /// </summary>
    /// <param name="graph">The graph executing the system.</param>
    API_FUNCTION() virtual void Execute(TaskGraph* graph);

    /// <summary>
    /// Called after executing all systems of the graph. Can be used to cleanup data (synchronous).
    /// </summary>
    /// <param name="graph">The graph executing the system.</param>
    API_FUNCTION() virtual void PostExecute(TaskGraph* graph);
};

/// <summary>
/// Graph-based asynchronous tasks scheduler for high-performance computing and processing.
/// </summary>
API_CLASS() class FLAXENGINE_API TaskGraph : public PersistentScriptingObject
{
DECLARE_SCRIPTING_TYPE(TaskGraph);
private:
    Array<TaskGraphSystem*, InlinedAllocation<64>> _systems;
    Array<TaskGraphSystem*, InlinedAllocation<64>> _remaining;
    Array<TaskGraphSystem*, InlinedAllocation<64>> _queue;
    TaskGraphSystem* _currentSystem = nullptr;
    int64 _currentLabel = 0;

public:
    /// <summary>
    /// Gets the list of systems.
    /// </summary>
    API_PROPERTY() const Array<TaskGraphSystem*, InlinedAllocation<64>>& GetSystems() const;

    /// <summary>
    /// Adds the system to the graph for the execution.
    /// </summary>
    /// <param name="system">The system to add.</param>
    API_FUNCTION() void AddSystem(TaskGraphSystem* system);

    /// <summary>
    /// Removes the system from the graph.
    /// </summary>
    /// <param name="system">The system to add.</param>
    API_FUNCTION() void RemoveSystem(TaskGraphSystem* system);

    /// <summary>
    /// Schedules the asynchronous systems execution including ordering and dependencies handling.
    /// </summary>
    API_FUNCTION() void Execute();

    /// <summary>
    /// Dispatches the job for the execution.
    /// </summary>
    /// <remarks>Call only from system's Execute method to properly schedule job.</remarks>
    /// <param name="job">The job. Argument is an index of the job execution.</param>
    /// <param name="jobCount">The job executions count.</param>
    API_FUNCTION() void DispatchJob(const Function<void(int32)>& job, int32 jobCount = 1);
};
