// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Log.h"
#include "Engine/Scripting/ManagedCLR/MTypes.h"

/// <summary>
/// Utility class to manage debug and log messages transportation from/to managed/unmanaged data and sending them to log file.
/// </summary>
class FLAXENGINE_API DebugLog
{
public:
    /// <summary>
    /// Sends the log message to the Flax console and the log file.
    /// </summary>
    /// <param name="type">The message type.</param>
    /// <param name="message">The message.</param>
    static void Log(LogType type, const StringView& message);

    /// <summary>
    /// A variant of Debug.Log that logs an info message to the console.
    /// </summary>
    /// <param name="message">The text message to display.</param>
    FORCE_INLINE static void Log(const StringView& message)
    {
        Log(LogType::Info, message);
    }

    /// <summary>
    /// A variant of Debug.Log that logs a warning message to the console.
    /// </summary>
    /// <param name="message">The text message to display.</param>
    FORCE_INLINE static void LogWarning(const StringView& message)
    {
        Log(LogType::Warning, message);
    }

    /// <summary>
    /// A variant of Debug.Log that logs a error message to the console.
    /// </summary>
    /// <param name="message">The text message to display.</param>
    FORCE_INLINE static void LogError(const StringView& message)
    {
        Log(LogType::Error, message);
    }

    /// <summary>
    /// Logs a formatted exception message to the Flax Console.
    /// </summary>
    /// <param name="exceptionObject">Runtime Exception.</param>
    static void LogException(MObject* exceptionObject);

public:
    /// <summary>
    /// Gets the managed stack trace.
    /// </summary>
    /// <returns>The stack trace text.</returns>
    static String GetStackTrace();

    /// <summary>
    /// Throws the exception to the managed world. Can be called only during internal call from the C# world.
    /// </summary>
    /// <param name="msg">The message.</param>
    NO_RETURN static void ThrowException(const char* msg);

    /// <summary>
    /// Throws the null reference to the managed world. Can be called only during internal call from the C# world.
    /// </summary>
    NO_RETURN static void ThrowNullReference();

    /// <summary>
    /// Throws the argument exception to the managed world. Can be called only during internal call from the C# world.
    /// </summary>
    /// <param name="arg">The argument name.</param>
    /// <param name="msg">The message.</param>
    NO_RETURN static void ThrowArgument(const char* arg, const char* msg);

    /// <summary>
    /// Throws the argument null to the managed world. Can be called only during internal call from the C# world.
    /// </summary>
    /// <param name="arg">The argument name.</param>
    NO_RETURN static void ThrowArgumentNull(const char* arg);

    /// <summary>
    /// Throws the argument out of range to the managed world. Can be called only during internal call from the C# world.
    /// </summary>
    /// <param name="arg">The argument name.</param>
    NO_RETURN static void ThrowArgumentOutOfRange(const char* arg);

    /// <summary>
    /// Throws the not supported operation exception to the managed world. Can be called only during internal call from the C# world.
    /// </summary>
    /// <param name="msg">The message.</param>
    NO_RETURN static void ThrowNotSupported(const char* msg);
};
