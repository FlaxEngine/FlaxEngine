#pragma once
#include "Engine/Core/Config.h"
#include "Engine/Scripting/ScriptingType.h"
#include "Engine/Threading/ThreadLocal.h"
#include "Engine/Core/Collections/Array.h"

class String;
struct Guid;

/// <summary>
/// Log context data structure. Contains different kinds of context data for different situtations.
/// </summary>
API_STRUCT() struct FLAXENGINE_API LogContextData
{
    DECLARE_SCRIPTING_TYPE_STRUCTURE(LogContextData)

    /// <summary>
    /// A GUID for an object which this context applies to.
    /// </summary>
    API_FIELD() Guid ObjectID;
};

template<>
struct TIsPODType<LogContextData>
{
    enum { Value = true };
};

/// <summary>
/// Stores a stack of log contexts.
/// </summary>
API_STRUCT() struct FLAXENGINE_API LogContextStack
{
    DECLARE_SCRIPTING_TYPE_STRUCTURE(LogContextStack)

    /// <summary>
    /// Log context stack.
    /// </summary>
    Array<LogContextData> Stack;
};

template<>
struct TIsPODType<LogContextStack>
{
    enum { Value = true };
};

/// <summary>
/// Log context interaction class. Methods are thread local, and as such, the context is as well.
/// This system is used to pass down important information to be logged through large callstacks
/// which don't have any reason to be passing down the information otherwise.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API LogContexts
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(LogContexts)

    /// <summary>
    /// Adds a log context element to the stack to be displayed in warning and error logs.
    /// </summary>
    /// <param name="id">The GUID of the object this context applies to.</param>
    API_FUNCTION() static void Set(const Guid& id);

    /// <summary>
    /// Pops a log context element off of the stack and discards it.
    /// </summary>
    API_FUNCTION() static void Clear();

    /// <summary>
    /// Gets the log context element off the top of stack.
    /// </summary>
    /// <returns>The log context element at the top of the stack.</returns>
    API_FUNCTION() static LogContextData Get();

private:
    static ThreadLocal<LogContextStack> CurrentLogContext;
};

/// <summary>
/// Formatting class which will provide a string to apply
/// the current log context to an error or warning.
/// </summary>
API_CLASS(Static) class FLAXENGINE_API LogContextFormatter
{
    DECLARE_SCRIPTING_TYPE_MINIMAL(LogContextFormatter)

public:
    /// <summary>
    /// Returns a string which represents the current log context on the stack.
    /// </summary>
    /// <returns>The formatted string representing the current log context.</returns>
    API_FUNCTION() static String Format();
};
