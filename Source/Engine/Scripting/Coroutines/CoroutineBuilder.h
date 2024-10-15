// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "CoroutineSuspendPoint.h"
#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Types/Variant.h"
#include "Engine/Core/Collections/Array.h"
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
/// Utility class to store coroutine steps and build a coroutine. This class must not be modified once execution has started.
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API CoroutineBuilder final : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(CoroutineBuilder, ScriptingObject);

    /// <summary> Executes the code. </summary>
    API_FUNCTION()
    ScriptingObjectReference<CoroutineBuilder> ThenRun(ScriptingObjectReference<CoroutineRunnable> runnable);

    /// <summary> Suspends the coroutine for the given number of seconds. </summary>
    API_FUNCTION()
    ScriptingObjectReference<CoroutineBuilder> ThenWaitSeconds(float seconds);

    /// <summary> Suspends the coroutine for the given number of frames. </summary>
    API_FUNCTION()
    ScriptingObjectReference<CoroutineBuilder> ThenWaitFrames(int32 frames);

    /// <summary> Suspends the coroutine until the predicate is true. </summary>
    API_FUNCTION()
    ScriptingObjectReference<CoroutineBuilder> ThenWaitUntil(ScriptingObjectReference<CoroutinePredicate> predicate);


    /// <summary> QOL wrapper for running runnable action in native scripts. </summary>
    ScriptingObjectReference<CoroutineBuilder> ThenRunFunc(Function<void()> runnable);

    /// <summary> QOL wrapper for waiting until predicate in native scripts. </summary>
    ScriptingObjectReference<CoroutineBuilder> ThenWaitUntilFunc(Function<void(bool&)> predicate);


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
            RunnableReference     _runnable;
            PredicateReference    _predicate;

            int32                 _framesDelay;
            float                 _secondsDelay;
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

        auto operator=(const Step& other) -> Step&;
        auto operator=(Step&& other) noexcept -> Step&;

        auto GetType()            const -> StepType;
        auto GetRunnable()        const -> const RunnableReference&;
        auto GetPredicate()       const -> const PredicateReference&;
        auto GetFramesDelay()     const -> int32;
        auto GetSecondsDelay()    const -> float;
        auto GetSuspensionPoint() const -> CoroutineSuspendPoint;
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
