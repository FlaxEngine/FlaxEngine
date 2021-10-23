// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Types/String.h"
#include "ManagedCLR/MTypes.h"
#include "Engine/Core/Log.h"

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

#if USE_MONO
    /// <summary>
    /// Initializes a new instance of the <see cref="MException"/> class.
    /// </summary>
    /// <param name="exception">The exception.</param>
    explicit MException(MonoException* exception)
        : MException((MonoObject*)exception)
    {
    }
#endif

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
    void Log(const LogType type, const Char* target)
    {
        // Log inner exceptions chain
        auto inner = InnerException;
        while (inner)
        {
            auto stackTrace = inner->StackTrace.HasChars() ? *inner->StackTrace : TEXT("<empty>");
            Log::Logger::Write(LogType::Warning, String::Format(TEXT("Inner exception. {0}\nStack strace:\n{1}\n"), inner->Message, stackTrace));
            inner = inner->InnerException;
        }

        // Send stack trace only to log file
        auto stackTrace = StackTrace.HasChars() ? *StackTrace : TEXT("<empty>");
        Log::Logger::Write(LogType::Warning, String::Format(TEXT("Exception has been thrown during {0}. {1}\nStack strace:\n{2}"), target, Message, stackTrace));
        Log::Logger::Write(type, String::Format(TEXT("Exception has been thrown during {0}.\n{1}"), target, Message));
    }
};
