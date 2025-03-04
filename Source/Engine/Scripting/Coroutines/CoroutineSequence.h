// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "CoroutineSuspendPoint.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Types/Variant.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Scripting/ScriptingObjectReference.h"


/// <summary>
/// Stores code to be executed once previous step is completed.
/// </summary>
/// <remarks>
/// This class is used due to limitations of Flax API.
/// </remarks>
API_CLASS(Sealed) class FLAXENGINE_API CoroutineRunnable final : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(CoroutineRunnable, ScriptingObject);

    /// <summary> Action to be executed. </summary>
    API_EVENT()
    Action OnRun;
};

/// <summary>
/// Stores a predicate to be checked before the coroutine may continue execution.
/// </summary>
/// <remarks>
/// This class is used due to limitations of Flax API.
/// </remarks>
API_CLASS(Sealed) class FLAXENGINE_API CoroutinePredicate final : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(CoroutinePredicate, ScriptingObject);

    /// <summary>
    /// Predicate to be checked. Default value is <c>false</c>.
    /// </summary>
    /// <remarks>
    /// It uses a reference to a boolean due to limitations of Flax API.
    /// </remarks>
    API_EVENT()
    Delegate<bool&> OnCheck;
};

/// <summary>
/// Utility class to store coroutine steps and build a coroutine.
/// </summary>
/// <remarks>
/// Modifying the builder during the execution may cause undefined behavior.
/// One builder may be shared between executions, reducing execution overhead.
/// </remarks>
API_CLASS(Sealed) class FLAXENGINE_API CoroutineSequence final : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(CoroutineSequence, ScriptingObject);

    /// <summary> Executes the code. </summary>
    API_FUNCTION()
    ScriptingObjectReference<CoroutineSequence> ThenRun(ScriptingObjectReference<CoroutineRunnable> runnable);

    /// <summary>
    /// Suspends the coroutine for the given number of seconds.
    /// </summary>
    /// <param name="seconds">
    /// Minimal number of seconds to wait.
    /// 0 is a valid value where the coroutine will be resumed on the closest time accumulation point.
    /// </param>
    API_FUNCTION()
    ScriptingObjectReference<CoroutineSequence> ThenWaitSeconds(float seconds);

    /// <summary> Suspends the coroutine for the given number of frames. </summary>
    /// <param name="frames">
    /// Minimal number of <b>full</b> frames to wait.
    /// 0 is a valid value where the coroutine will be resumed on the closest frame accumulation point.
    /// </param>
    API_FUNCTION()
    ScriptingObjectReference<CoroutineSequence> ThenWaitFrames(int32 frames);

    /// <summary> Suspends the coroutine until the given suspend point. </summary>
    ScriptingObjectReference<CoroutineSequence> ThenWaitForPoint(CoroutineSuspendPoint point);

    /// <summary> Suspends the coroutine until the predicate is true. </summary>
    API_FUNCTION()
    ScriptingObjectReference<CoroutineSequence> ThenWaitUntil(ScriptingObjectReference<CoroutinePredicate> predicate);


    /// <summary> QOL wrapper for running runnable action in native scripts. </summary>
    ScriptingObjectReference<CoroutineSequence> ThenRunFunc(const Function<void()>& runnable);

    /// <summary> QOL wrapper for waiting until predicate in native scripts. </summary>
    ScriptingObjectReference<CoroutineSequence> ThenWaitUntilFunc(const Function<void(bool&)>& predicate);


    using RunnableReference  = ScriptingObjectReference<CoroutineRunnable>;
    using PredicateReference = ScriptingObjectReference<CoroutinePredicate>;

    enum class StepType
    {
        None,
        Run,
        WaitSuspensionPoint,
        WaitSeconds,
        WaitFrames,
        WaitUntil,
    };

    class Step final
    {
        // No std::variant in Cpp 14 :( Need to implement it manually.

        union 
        {
            RunnableReference _runnable;
            PredicateReference _predicate;

            int32 _framesDelay;
            float _secondsDelay;
            CoroutineSuspendPoint _suspensionPoint;
        };

        StepType _type;

    public:
        explicit Step(RunnableReference&& runnable);
        explicit Step(CoroutineSuspendPoint suspensionPoint);
        explicit Step(int32 framesDelay);
        explicit Step(float secondsDelay);
        explicit Step(PredicateReference&& predicate);

        Step(const Step& other);
        Step(Step&& other) noexcept;

    private:
        void Clear();
        void EmplaceCopy(const Step& other);
        void EmplaceMove(Step&& other) noexcept;

    public:
        ~Step();

        Step& operator=(const Step& other);
        Step& operator=(Step&& other) noexcept;


        FORCE_INLINE StepType GetType() const
        {
            return _type;
        }

        FORCE_INLINE const RunnableReference& GetRunnable() const
        {
            ASSERT(_type == StepType::Run);
            return _runnable;
        }

        FORCE_INLINE const PredicateReference& GetPredicate() const
        {
            ASSERT(_type == StepType::WaitUntil);
            return _predicate;
        }

        FORCE_INLINE int32 GetFramesDelay() const
        {
            ASSERT(_type == StepType::WaitFrames);
            return _framesDelay;
        }

        FORCE_INLINE float GetSecondsDelay() const
        {
            ASSERT(_type == StepType::WaitSeconds);
            return _secondsDelay;
        }

        FORCE_INLINE CoroutineSuspendPoint GetSuspensionPoint() const
        {
            ASSERT(_type == StepType::WaitSuspensionPoint);
            return _suspensionPoint;
        }
    };

private:
    Array<Step> _steps;

public:
    /// <summary>
    /// Returns the steps of the coroutine, to be used by the executor.
    /// </summary>
    FORCE_INLINE const Array<Step>& GetSteps() const
    {
        return _steps;
    }
};
