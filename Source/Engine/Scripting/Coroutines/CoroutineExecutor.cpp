// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#include "CoroutineExecutor.h"

#include "Engine/Debug/DebugLog.h"
#include "Engine/Profiler/ProfilerCPU.h"

// Universal shared point for all coroutines to accumulate any kind of time.
// Update is used, because it is the only point that is guaranteed to be called exactly once every frame.
constexpr static CoroutineSuspendPoint DeltaAccumulationPoint = CoroutineSuspendPoint::Update;


ScriptingObjectReference<CoroutineHandle> CoroutineExecutor::ExecuteOnce(ScriptingObjectReference<CoroutineBuilder> builder)
{
    const ExecutionID id = _uuidGenerator.Generate();
    Execution execution{ MoveTemp(builder), id };
    _executions.Add(MoveTemp(execution));

    ScriptingObjectReference<CoroutineHandle> handle = NewObject<CoroutineHandle>();
    handle->ExecutionID = id;
    handle->Executor    = ScriptingObjectReference<CoroutineExecutor>{ this };
    return handle;
}

ScriptingObjectReference<CoroutineHandle> CoroutineExecutor::ExecuteRepeats(ScriptingObjectReference<CoroutineBuilder> builder, const int32 repeats)
{
    if (repeats <= 0) 
    {
        DebugLog::LogError(String::Format(
            TEXT("Coroutine must not be dispatched non-positive number of times! Call to repeat {} times will be ignored."), 
            repeats
        ));
        return nullptr;
    }

    const ExecutionID id = _uuidGenerator.Generate();
    Execution execution{ MoveTemp(builder), id, repeats };
    _executions.Add(MoveTemp(execution));

    ScriptingObjectReference<CoroutineHandle> handle = NewObject<CoroutineHandle>();
    handle->ExecutionID = id;
    handle->Executor    = ScriptingObjectReference<CoroutineExecutor>{ this };
    return handle;
}

ScriptingObjectReference<CoroutineHandle> CoroutineExecutor::ExecuteLooped(ScriptingObjectReference<CoroutineBuilder> builder)
{
    const ExecutionID id = _uuidGenerator.Generate();
    Execution execution{ MoveTemp(builder), id, Execution::InfiniteRepeats };
    _executions.Add(MoveTemp(execution));

    ScriptingObjectReference<CoroutineHandle> handle = NewObject<CoroutineHandle>();
    handle->ExecutionID = id;
    handle->Executor    = ScriptingObjectReference<CoroutineExecutor>{ this };
    return handle;
}


void CoroutineExecutor::Continue(
    const CoroutineSuspendPoint point, 
    const float deltaTime)
{
    PROFILE_CPU();

    const Delta delta{ deltaTime, 1 };

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

int32 CoroutineExecutor::GetCoroutinesCount() const
{
    return _executions.Count();
}

using Step     = CoroutineBuilder::Step;
using StepType = CoroutineBuilder::StepType;

CoroutineExecutor::Execution::Execution(
    BuilderReference&& builder, 
    const ExecutionID  id, 
    const int32        repeats
)   : _builder{ MoveTemp(builder) }
    , _accumulator{ 0.0f, 0 }
    , _id{ id }
    , _stepIndex{ 0 }
    , _repeats{ repeats }
    , _isPaused{ false }
{
}

bool CoroutineExecutor::Execution::ContinueCoroutine(
    const CoroutineSuspendPoint point, 
    const Delta& delta
)
{
    if (_isPaused)
        return false;

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

CoroutineExecutor::ExecutionID CoroutineExecutor::Execution::GetID() const
{
    return _id;
}

bool CoroutineExecutor::Execution::IsPaused() const
{
    return _isPaused;
}

void CoroutineExecutor::Execution::SetPaused(const bool value)
{
    _isPaused = value;
}

bool CoroutineExecutor::Execution::TryMakeStep(
    const CoroutineBuilder::Step& step, 
    const CoroutineSuspendPoint   point,
    const Delta&                  delta,
    Delta&                        accumulator
)
{
    switch (step.GetType())
    {
        case StepType::Run:
        {
            step.GetRunnable()->OnRun();
            return true;
        }

        case StepType::WaitSuspensionPoint:
        {
            return step.GetSuspensionPoint() == point;
        }

        case StepType::WaitSeconds:
        {
            if (point != DeltaAccumulationPoint)
                return false;

            accumulator.time += delta.time;

            if (step.GetSecondsDelay() >= accumulator.time)
                return false;

            accumulator.time = 0.0f; // Reset the time accumulator.
            return true;
        }

        case StepType::WaitFrames:
        {
            if (point != DeltaAccumulationPoint)
                return false;

            accumulator.frames += delta.frames;

            if (step.GetFramesDelay() >= accumulator.frames)
                return false;

            accumulator.frames = 0; // Reset the frames accumulator.
            return true;
        }

        case StepType::WaitUntil:
        {
            bool result = false;
            step.GetPredicate()->OnCheck(result);
            return result;
        }

        case StepType::None:
        default:
        {
            CRASH;
        }

        //TODO Optimize filtering accumulation steps by caching the expected suspend point. (Or filtering it by bit-field)
    }
}

// Cancel, Pause and Resume currently have O(n) based on the number of coroutines.
// Subject to change if the number of coroutines becomes a bottleneck.

bool CoroutineExecutor::Cancel(CoroutineHandle& handle)
{
    PROFILE_CPU();

    for (int32 i = 0; i < _executions.Count(); i++)
    {
        if (_executions[i].GetID() != handle.ExecutionID)
            continue;

        _executions.RemoveAt(i);
        handle.Executor = nullptr;

        return true;
    }

    return false;
}

bool CoroutineExecutor::Pause(CoroutineHandle& handle)
{
    PROFILE_CPU();

    for (int32 i = 0; i < _executions.Count(); i++)
    {
        if (_executions[i].GetID() != handle.ExecutionID)
            continue;

        const bool wasPaused = _executions[i].IsPaused();
        _executions[i].SetPaused(true);
        return !wasPaused;
    }

    return false;
}

bool CoroutineExecutor::Resume(CoroutineHandle& handle)
{
    PROFILE_CPU();

    for (int32 i = 0; i < _executions.Count(); i++)
    {
        if (_executions[i].GetID() != handle.ExecutionID)
            continue;

        const bool wasPaused = _executions[i].IsPaused();
        _executions[i].SetPaused(false);
        return wasPaused;
    }

    return false;
};
