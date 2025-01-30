// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Log.h"
#include "Engine/Core/Config.h"
#include "Engine/Scripting/ScriptingType.h"

class String;
struct Guid;

/// <summary>
/// Log context data structure. Contains different kinds of context data for different situations.
/// </summary>
API_STRUCT(NoDefault) struct FLAXENGINE_API LogContextData
{
    DECLARE_SCRIPTING_TYPE_STRUCTURE(LogContextData)

    /// <summary>
    /// A GUID for an object which this context applies to.
    /// </summary>
    API_FIELD() Guid ObjectID;

    friend bool operator==(const LogContextData& lhs, const LogContextData& rhs)
    {
        return lhs.ObjectID == rhs.ObjectID;
    }
};

template<>
struct TIsPODType<LogContextData>
{
    enum { Value = true };
};

/// <summary>
/// Log context interaction class. Methods are thread local, and as such, the context is as well.
/// This system is used to pass down important information to be logged through large callstacks
/// which don't have any reason to be passing down the information otherwise.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API LogContext
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(LogContext)

    /// <summary>
    /// Adds a log context element to the stack to be displayed in warning and error logs.
    /// </summary>
    /// <param name="id">The ID of the object this context applies to.</param>
    API_FUNCTION() static void Push(const Guid& id);

    /// <summary>
    /// Pops a log context element off of the stack and discards it.
    /// </summary>
    API_FUNCTION() static void Pop();

    /// <summary>
    /// Gets the log context element off the top of stack.
    /// </summary>
    /// <returns>The log context element at the top of the stack.</returns>
    API_FUNCTION() static LogContextData Get();

    /// <summary>
    /// Prints the current log context to the log. Does nothing it 
    /// </summary>
    /// <param name="verbosity">The verbosity of the log.</param>
    API_FUNCTION() static void Print(LogType verbosity);
};

/// <summary>
/// Helper structure used to call LogContext Push/Pop within single code block.
/// </summary>
struct LogContextScope
{
    FORCE_INLINE LogContextScope(const Guid& id)
    {
        LogContext::Push(id);
    }

    FORCE_INLINE ~LogContextScope()
    {
        LogContext::Pop();
    }
};
