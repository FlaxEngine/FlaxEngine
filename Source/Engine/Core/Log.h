// Copyright (c) Wojciech Figat. All rights reserved.

#pragma once

#include "Engine/Core/Singleton.h"
#include "Engine/Core/Delegate.h"
#include "Engine/Core/Types/String.h"
#include "Engine/Core/Types/StringView.h"

// Enable/disable auto flush function
#define LOG_ENABLE_AUTO_FLUSH 1

/// <summary>
/// Sends a formatted message to the log file (message type - describes level of the log (see LogType enum))
/// </summary>
#define LOG(messageType, format, ...) Log::Logger::Write(LogType::messageType, ::String::Format(TEXT(format), ##__VA_ARGS__))

/// <summary>
/// Sends a string message to the log file (message type - describes level of the log (see LogType enum))
/// </summary>
#define LOG_STR(messageType, str) Log::Logger::Write(LogType::messageType, str)

#if LOG_ENABLE_AUTO_FLUSH
// Noop as log is auto-flushed on write
#define LOG_FLUSH()
#else
// Flushes the log file buffer
#define LOG_FLUSH() Log::Logger::Flush()
#endif

/// <summary>
/// The log message types.
/// </summary>
API_ENUM() enum class LogType
{
    /// <summary>
    /// The information log message.
    /// </summary>
    Info = 1,

    /// <summary>
    /// The warning message.
    /// </summary>
    Warning = 2,

    /// <summary>
    /// The error message.
    /// </summary>
    Error = 4,

    /// <summary>
    /// The fatal error.
    /// </summary>
    Fatal = 8,
};

extern const Char* ToString(LogType e);

namespace Log
{
    class Exception;

    /// <summary>
    /// Singleton logger class
    /// </summary>
    /// <seealso cref="Singleton" />
    class FLAXENGINE_API Logger : public Singleton<Logger>
    {
    public:

        /// <summary>
        /// Current log file path. Empty if not used.
        /// </summary>
        static String LogFilePath;

        /// <summary>
        /// Action fired on new log message.
        /// </summary>
        static Delegate<LogType, const StringView&> OnMessage;

        /// <summary>
        /// Action fired on new log error message.
        /// </summary>
        static Delegate<LogType, const StringView&> OnError;

    public:

        /// <summary>
        /// Initializes a logging service.
        /// </summary>
        /// <returns>True if cannot init logging service, otherwise false.</returns>
        static bool Init();

        /// <summary>
        /// Disposes a logging service.
        /// </summary>
        static void Dispose();

    public:

        FORCE_INLINE static bool IsError(const LogType type)
        {
            return type == LogType::Fatal || type == LogType::Error;
        }

    public:

        /// <summary>
        /// Determines whether logging to file is enabled.
        /// </summary>
        /// <returns><c>true</c> if log is enabled; otherwise, <c>false</c>.</returns>
        static bool IsLogEnabled();

        /// <summary>
        /// Flushes log file with a memory buffer.
        /// </summary>
        static void Flush();

        /// <summary>
        /// Writes a series of '=' chars to the log to end a section.
        /// </summary>
        static void WriteFloor();

        /// <summary>
        /// Writes a message to the log file.
        /// </summary>
        /// <param name="type">The message type.</param>
        /// <param name="msg">The message text.</param>
        template<typename... Args>
        FORCE_INLINE static void Write(LogType type, const Char* msg)
        {
            Write(type, StringView(msg));
        }

        /// <summary>
        /// Writes a formatted message to the log file.
        /// </summary>
        /// <param name="type">The message type.</param>
        /// <param name="format">The message format string.</param>
        /// <param name="args">The format arguments.</param>
        template<typename... Args>
        FORCE_INLINE static void Write(LogType type, const Char* format, const Args& ... args)
        {
            Write(type, String::Format(format, args...));
        }

        /// <summary>
        /// Write a message to the log.
        /// </summary>
        /// <param name="type">The message type.</param>
        /// <param name="msg">The message text.</param>
        static void Write(LogType type, const StringView& msg);

        /// <summary>
        /// Writes a custom message to the log.
        /// </summary>
        /// <param name="msg">The message text.</param>
        FORCE_INLINE static void Write(const Char* msg)
        {
            Write(StringView(msg));
        }

        /// <summary>
        /// Writes a custom message to the log.
        /// </summary>
        /// <param name="msg">The message text.</param>
        FORCE_INLINE static void Write(const String& msg)
        {
            Write(StringView(msg));
        }

        /// <summary>
        /// Writes a custom message to the log.
        /// </summary>
        /// <param name="msg">The message text.</param>
        static void Write(const StringView& msg);

        /// <summary>
        /// Writes an exception formatted message to log file.
        /// </summary>
        /// <param name="exception">The exception to write to the file.</param>
        static void Write(const Exception& exception);

    private:

        static void ProcessLogMessage(LogType type, const StringView& msg, fmt_flax::memory_buffer& w);
    };
}
