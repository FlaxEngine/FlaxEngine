// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "CoroutineSuspensionPointIndex.h"
#include "CoroutineSuspensionPointsFlags.h"
#include "Engine/Core/Types/BaseTypes.h"
#include "Engine/Core/Types/Variant.h"
#include "Engine/Core/Collections/Array.h"
#include "Engine/Scripting/ScriptingObject.h"
#include "Engine/Scripting/ScriptingObjectReference.h"


/// <summary>
/// Wrapper for a coroutine step with instructions to be executed at the given suspension point.
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API CoroutineRunnable final : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(CoroutineRunnable, ScriptingObject);

    API_FIELD()
    CoroutineSuspensionPointIndex ExecutionPoint = CoroutineSuspensionPointIndex::Update;
    
    API_EVENT()
    Action OnRun;
};

/// <summary>
/// Wrapper for a coroutine check if the coroutine may continue execution at the given suspension point.
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API CoroutinePredicate final : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(CoroutinePredicate, ScriptingObject);

    API_FIELD()
    CoroutineSuspensionPointIndex ExecutionPoint = CoroutineSuspensionPointIndex::Update;

    API_EVENT()
    Delegate<bool&> OnCheck;
};

/// <summary>
/// Utility class to store coroutine steps and build a coroutine. This class must not be modified once execution has started.
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API CoroutineBuilder final : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(CoroutineBuilder, ScriptingObject);

    API_FUNCTION()
    ScriptingObjectReference<CoroutineBuilder> ThenRun(ScriptingObjectReference<CoroutineRunnable> runnable);

    API_FUNCTION()
    ScriptingObjectReference<CoroutineBuilder> ThenWaitSeconds(float seconds);

    API_FUNCTION()
    ScriptingObjectReference<CoroutineBuilder> ThenWaitFrames(int32 frames);

    API_FUNCTION()
    ScriptingObjectReference<CoroutineBuilder> ThenWaitUntil(ScriptingObjectReference<CoroutinePredicate> predicate);


    using RunnableReference  = ScriptingObjectReference<CoroutineRunnable>;
    using PredicateReference = ScriptingObjectReference<CoroutinePredicate>;

    enum class StepType
    {
        None,
        Run,
        WaitSeconds,
        WaitFrames,
        WaitUntil,
    };

    class Step final
    {
        // No std::variant in Cpp 14 :( Need to implement it manually.

        union 
        {
            RunnableReference   _runnable;
            PredicateReference  _predicate;

            int32               _framesDelay;
            float               _secondsDelay;
        };

        StepType _type;

    public:
        explicit Step(RunnableReference&& runnable);
        explicit Step(int32 framesDelay);
        explicit Step(float secondsDelay);
        explicit Step(PredicateReference&& predicate);

        Step(const Step& other);
        Step(Step&& other) noexcept;

    private:
        void Clear();

    public:
        ~Step();

        auto operator=(const Step& other) -> Step&;
        auto operator=(Step&& other) noexcept -> Step&;

        auto GetType()         const -> StepType;
        auto GetRunnable()     const -> const RunnableReference&;
        auto GetPredicate()    const -> const PredicateReference&;
        auto GetFramesDelay()  const -> int32;
        auto GetSecondsDelay() const -> float;
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
