// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "CoroutineBuilder.h"

/// <summary>
/// Utility class that can track and execute coroutines' stages using incoming events.
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API CoroutineExecutor final : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(CoroutineExecutor, ScriptingObject);

    /// <summary>
    /// Adds a coroutine to the executor to be executed.
    /// </summary>
    API_FUNCTION()
    void Execute(ScriptingObjectReference<CoroutineBuilder> builder);

    /// <summary>
    /// Continues the execution of all coroutines at the given suspension point.
    /// </summary>
    API_FUNCTION()
    void Continue(CoroutineSuspensionPointIndex point);


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
        int32            _stepIndex;

    public:
        Execution() = delete;

        explicit Execution(BuilderReference&& builder)
            : _builder{ builder }
            , _stepIndex{ 0 }
        {
        }

        /// <summary>
        /// Executes the next step of the coroutines.
        /// </summary>
        /// <returns>
        /// <c>true</c> if the coroutine should be removed from the executor, because it reached the end of the steps; otherwise, <c>false</c>.
        /// </returns>
        bool ContinueCoroutine(CoroutineSuspensionPointIndex point, const Delta& delta);

    private:
        static bool TryMakeStep(const CoroutineBuilder::Step& step, CoroutineSuspensionPointIndex point, const Delta& delta);
    };

    Array<Execution> _executions;
};
