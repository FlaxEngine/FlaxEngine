// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Delegate.h"

template<typename T>
class Span;

/// <summary>
/// Lightweight multi-threaded jobs execution scheduler. Uses a pool of threads and supports work-stealing concept.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API JobSystem
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(JobSystem);

    /// <summary>
    /// Executes the job (utility to call dispatch and wait for the end).
    /// </summary>
    /// <param name="job">The job. Argument is an index of the job execution.</param>
    /// <param name="jobCount">The job executions count.</param>
    API_FUNCTION() static void Execute(const Function<void(int32)>& job, int32 jobCount = 1);

    /// <summary>
    /// Dispatches the job for the execution.
    /// </summary>
    /// <param name="job">The job. Argument is an index of the job execution.</param>
    /// <param name="jobCount">The job executions count.</param>
    /// <returns>The label identifying this dispatch. Can be used to wait for the execution end.</returns>
    API_FUNCTION() static int64 Dispatch(const Function<void(int32)>& job, int32 jobCount = 1);

    /// <summary>
    /// Dispatches the job for the execution after all of dependant jobs will complete.
    /// </summary>
    /// <param name="job">The job. Argument is an index of the job execution.</param>
    /// <param name="dependencies">The list of dependant jobs that need to complete in order to start executing this job.</param>
    /// <param name="jobCount">The job executions count.</param>
    /// <returns>The label identifying this dispatch. Can be used to wait for the execution end.</returns>
    API_FUNCTION() static int64 Dispatch(const Function<void(int32)>& job, Span<int64> dependencies, int32 jobCount = 1);

    /// <summary>
    /// Waits for all dispatched jobs to finish.
    /// </summary>
    API_FUNCTION() static void Wait();

    /// <summary>
    /// Waits for all dispatched jobs until a given label to finish (i.e. waits for a Dispatch that returned that label).
    /// </summary>
    /// <param name="label">The label.</param>
    API_FUNCTION() static void Wait(int64 label);

    /// <summary>
    /// Sets whether automatically start jobs execution on Dispatch. If disabled jobs won't be executed until it gets re-enabled. Can be used to optimize execution of multiple dispatches that should overlap.
    /// </summary>
    API_FUNCTION() static void SetJobStartingOnDispatch(bool value);

    /// <summary>
    /// Gets the amount of job system threads.
    /// </summary>
    API_PROPERTY() static int32 GetThreadsCount();
};
