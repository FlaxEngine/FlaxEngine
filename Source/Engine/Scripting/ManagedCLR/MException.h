// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/String.h"
#include "Engine/Core/Log.h"
#include "MTypes.h"

/// <summary>
/// Represents errors that occur during script execution.
/// </summary>
class FLAXENGINE_API MException
{
public:
    /// <summary>
    /// Gets a message that describes the current exception.
    /// </summary>
    String Message;

    /// <summary>
    /// Gets a string representation of the immediate frames on the call stack.
    /// </summary>
    String StackTrace;

    /// <summary>
    /// Gets an inner exception. Null if not used.
    /// </summary>
    MException* InnerException;

public:
    /// <summary>
    /// Initializes a new instance of the <see cref="MException"/> class.
    /// </summary>
    /// <param name="exception">The exception object.</param>
    explicit MException(MObject* exception);

    /// <summary>
    /// Disposes a instance of the <see cref="MException"/> class.
    /// </summary>
    ~MException();

public:
    /// <summary>
    /// Sends exception to the log.
    /// </summary>
    /// <param name="type">The log message type.</param>
    /// <param name="target">Execution target name.</param>
    void Log(const LogType type, const Char* target);
};
