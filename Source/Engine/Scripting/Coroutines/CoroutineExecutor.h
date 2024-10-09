// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "CoroutineBuilder.h"

/// <summary>
/// Utility class that can track and execute coroutines' stages using incoming events.
/// </summary>
API_CLASS(Sealed) class FLAXENGINE_API CoroutineExecutor final : public ScriptingObject
{
    DECLARE_SCRIPTING_TYPE_WITH_CONSTRUCTOR_IMPL(CoroutineExecutor, ScriptingObject);

    API_FUNCTION()
    void Execute(ScriptingObjectReference<CoroutineBuilder> builder);

private:
    class ExecutionState
    {
        ScriptingObjectReference<CoroutineBuilder> _builder;
        int32                                      _stepIndex;
    };
};
