// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "CoroutineHandle.h"
#include "CoroutineSequence.h"

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
    ScriptingObjectReference<CoroutineHandle> ExecuteOnce(
        ScriptingObjectReference<CoroutineSequence> sequence,
        CoroutineSuspendPoint accumulationPoint
    );

    /// <summary>
    /// Adds a coroutine to the executor to be executed multiple times.
    /// </summary>
    API_FUNCTION()
    ScriptingObjectReference<CoroutineHandle> ExecuteRepeats(
        ScriptingObjectReference<CoroutineSequence> sequence,
        CoroutineSuspendPoint accumulationPoint,
        int32 repeats
    );

    /// <summary>
    /// Adds a coroutine to the executor to be executed indefinitely.
    /// </summary>
    API_FUNCTION()
    ScriptingObjectReference<CoroutineHandle> ExecuteLooped(
        ScriptingObjectReference<CoroutineSequence> sequence,
        CoroutineSuspendPoint accumulationPoint
    );


    /// <summary>
    /// Continues the execution of all coroutines at the given suspension point.
    /// </summary>
    /// <param name="point"> Unlocked game suspension point. </param>
    /// <param name="frames"> Accumulated frames. </param>
    /// <param name="deltaTime"> Accumulated time. </param>
    /// <remarks>
    /// By default, the coroutines accumulate time and frames at the Update point.
    /// </remarks>
    API_FUNCTION()
    void Continue(CoroutineSuspendPoint point, int32 frames, float deltaTime);

    /// <summary>
    /// Returns the number of coroutines currently being executed.
    /// </summary>
    API_PROPERTY()
    int32 GetCoroutinesCount() const;

    using ExecutionID = uint64;

private:
    using SequenceReference = ScriptingObjectReference<CoroutineSequence>;
    using SuspendPoint = CoroutineSuspendPoint;

    struct Delta
    {
        float time;
        int32 frames;
    };

    class Execution final
    {
        SequenceReference _sequence;
        Delta _accumulator;
        ExecutionID _id;
        int32 _stepIndex;
        int32 _repeats;
        SuspendPoint _accumulationPoint;
        bool _isPaused;

    public:
        constexpr static int32 InfiniteRepeats = -1; //TODO Use Nullable instead of sentinel when PR #2969 is merged.

        Execution() = delete;

        explicit Execution(
            SequenceReference&& sequence,
            CoroutineSuspendPoint accumulationPoint,
            ExecutionID id,
            int32 repeats = 1
        );

        /// <summary>
        /// Executes the next step of the coroutines.
        /// </summary>
        /// <returns>
        /// <c>true</c> if the coroutine should be removed from the executor, because it reached the end of the steps; otherwise, <c>false</c>.
        /// </returns>
        bool ContinueCoroutine(CoroutineSuspendPoint point, const Delta& delta);

        auto GetID() const -> ExecutionID;
        auto IsPaused() const -> bool;

        void SetPaused(bool value);

    private:
        FORCE_INLINE static bool TryMakeStep(
            const CoroutineSequence::Step& step, 
            CoroutineSuspendPoint point,
            bool isAccumulating,
            Delta& delta,
            Delta& accumulator
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

public:
    bool HasFinished(const CoroutineHandle& handle) const;

    bool IsPaused(const CoroutineHandle& handle) const;


    bool Cancel(CoroutineHandle& handle);

    bool Pause(CoroutineHandle& handle);

    bool Resume(CoroutineHandle& handle);
};
