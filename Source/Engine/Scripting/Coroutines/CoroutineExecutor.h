// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "CoroutineBuilder.h"
#include "CoroutineHandle.h"

/// <summary>
/// Utility class that can track and execute coroutines' stages using incoming events.
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API CoroutineExecutor final : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(CoroutineExecutor, ScriptingObject);

    /// <summary>
    /// Adds a coroutine to the executor to be executed once.
    /// </summary>
    API_FUNCTION()
    ScriptingObjectReference<CoroutineHandle> ExecuteOnce(ScriptingObjectReference<CoroutineBuilder> builder);

    /// <summary>
    /// Adds a coroutine to the executor to be executed multiple times.
    /// </summary>
    API_FUNCTION()
    ScriptingObjectReference<CoroutineHandle> ExecuteRepeats(ScriptingObjectReference<CoroutineBuilder> builder, int32 repeats);

    /// <summary>
    /// Adds a coroutine to the executor to be executed indefinitely.
    /// </summary>
    API_FUNCTION()
    ScriptingObjectReference<CoroutineHandle> ExecuteLooped(ScriptingObjectReference<CoroutineBuilder> builder);

    //TODO Add coroutine execution handle to allow for manual cancellation of the coroutine.
    //TODO Maybe it would be beneficial to also add pausing and resuming coroutines.


    /// <summary>
    /// Continues the execution of all coroutines at the given suspension point.
    /// </summary>
    /// <param name="point"> Game loop suspension point for the coroutines to try to continue execution. </param>
    /// <param name="deltaTime">
    /// Total time passed since the last Update event.
    /// Warning: Do not use fixed-delta, because only delta time is used for the coroutines to accumulate time.
    /// </param>
    /// <remarks> Manually calling this method may cause undefined behaviors. It is exposed for the engine internals. </remarks>
    API_FUNCTION()
    void Continue(CoroutineSuspendPoint point, float deltaTime);

    /// <summary>
    /// Returns the number of coroutines currently being executed.
    /// </summary>
    API_FUNCTION()
    int32 GetCoroutinesCount() const;

    using ExecutionID = uint64;

private:
    using BuilderReference = ScriptingObjectReference<CoroutineBuilder>;

    struct Delta
    {
        float time;
        int32 frames;
    };

    class Execution final
    {
        BuilderReference _builder;
        Delta            _accumulator;
        ExecutionID      _id;
        int32            _stepIndex;
        int32            _repeats;

    public:
        constexpr static int32 InfiniteRepeats = -1; //TODO Use Nullable instead of sentinel when PR #2969 is merged.

        Execution() = delete;

        explicit Execution(
            BuilderReference&& builder,
            const ExecutionID id,
            const int32 repeats = 1
        )   : _builder{ MoveTemp(builder) }
            , _accumulator{ 0.0f, 0 }
            , _id{ id }
            , _stepIndex{ 0 }
            , _repeats{ repeats }
        {
        }

        /// <summary>
        /// Executes the next step of the coroutines.
        /// </summary>
        /// <returns>
        /// <c>true</c> if the coroutine should be removed from the executor, because it reached the end of the steps; otherwise, <c>false</c>.
        /// </returns>
        bool ContinueCoroutine(CoroutineSuspendPoint point, const Delta& delta);

    private:
        static bool TryMakeStep(
            const CoroutineBuilder::Step& step, 
            CoroutineSuspendPoint point,
            const Delta& delta,
            Delta&       accumulator
        );
    };


    Array<Execution> _executions;

    class UuidGenerator final
    {
        ExecutionID _id{};

    public:
        ExecutionID Generate()
        {
            return _id++;
        }
    }
    _uuidGenerator;
};
