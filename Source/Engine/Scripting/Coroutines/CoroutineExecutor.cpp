// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "CoroutineExecutor.h"

#include "Engine/Debug/DebugLog.h"


void CoroutineExecutor::ExecuteOnce(ScriptingObjectReference<CoroutineBuilder> builder)
{
    Execution execution{ MoveTemp(builder) };
    _executions.Add(MoveTemp(execution));
}

void CoroutineExecutor::ExecuteRepeats(ScriptingObjectReference<CoroutineBuilder> builder, const int32 repeats)
{
    if (repeats <= 0) 
    {
        DebugLog::LogError(String::Format(
            TEXT("Coroutine must not be dispatched non-positive number of times! Call to repeat {} times will be ignored."), 
            repeats
        ));
        return;
    }

    Execution execution{ MoveTemp(builder), repeats };
    _executions.Add(MoveTemp(execution));
}

void CoroutineExecutor::ExecuteLooped(ScriptingObjectReference<CoroutineBuilder> builder)
{
    Execution execution{ MoveTemp(builder), Execution::InfiniteRepeats };
    _executions.Add(MoveTemp(execution));
}


void CoroutineExecutor::Continue(const CoroutineSuspendPoint point)
{
    const Delta delta{ 0.0f, 1 }; //TODO(mtszkarbowiak) Implement delta time and frames.

    for (int32 i = 0; i < _executions.Count();)
    {
        Execution& execution  = _executions[i];
        const bool reachedEnd = execution.ContinueCoroutine(point, delta);

        if (reachedEnd)
        {
            _executions.RemoveAt(i);
        }
        else
        {
            // Increment the index only if the coroutine was not removed.
            i++;
        }
    }
}


using Step     = CoroutineBuilder::Step;
using StepType = CoroutineBuilder::StepType;


bool CoroutineExecutor::Execution::ContinueCoroutine(
    const CoroutineSuspendPoint point, 
    const Delta& delta
)
{
    while (_stepIndex < _builder->GetSteps().Count())
    {
        const Step& step = _builder->GetSteps()[_stepIndex];

        if (!TryMakeStep(step, point, delta, this->_accumulator))
            return false; // The coroutine is waiting for the next frame or seconds.

        ++_stepIndex;
    }

    if (_repeats == InfiniteRepeats)
    {
        _stepIndex = 0;
        return false; // The coroutine should be executed indefinitely.
    }

    if (_repeats > 1)
    {
        --_repeats;
        _stepIndex = 0;
        return false; // The coroutine should be executed again
    }

    ASSERT(_repeats == 1);
    return true; // The coroutine reached the end of the steps.
}

bool CoroutineExecutor::Execution::TryMakeStep(
    const CoroutineBuilder::Step&       step, 
    const CoroutineSuspendPoint point,
    const Delta&                        delta,
    Delta&                              accumulator
)
{
    switch (step.GetType())
    {
        case StepType::Run:
        {
            const auto& runnable = step.GetRunnable();

            if (runnable->ExecutionPoint != point)
                return false;

            runnable->OnRun();

            return true;
        }

        case StepType::WaitSeconds:
        {
            //TODO(mtszkarbowiak) Add protectors against checking for time at incorrect points.
            // Currently accumulation happens at all points, but it should be done only at the Update point (or other points if needed).

            accumulator.time += delta.time;

            if (step.GetSecondsDelay() > accumulator.time)
                return false;

            accumulator.time = 0.0f; // Reset the time accumulator.
            return true;
        }

        case StepType::WaitFrames:
        {
            //TODO(mtszkarbowiak) Add protectors against checking for time at incorrect points.
            // Currently accumulation happens at all points, but it should be done only at the Update point (or other points if needed).

            accumulator.frames += delta.frames;

            if (step.GetFramesDelay() > accumulator.frames)
                return false;

            accumulator.frames = 0; // Reset the frames accumulator.
            return true;
        }

        case StepType::WaitUntil:
        {
            if (step.GetPredicate()->ExecutionPoint != point)
                return false;

            bool result;
            step.GetPredicate()->OnCheck(result);

            if (result)
                return false;

            return true;
        }

        default:
        {
            CRASH;
        }
    }
};
