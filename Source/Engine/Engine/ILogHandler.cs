// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Logs handler.
    /// </summary>
    public interface ILogHandler
    {
        /// <summary>
        /// Occurs on sending a log message.
        /// </summary>
        event LogDelegate SendLog;

        /// <summary>
        /// Occurs on sending a exception log message.
        /// </summary>
        event LogExceptionDelegate SendExceptionLog;

        /// <summary>
        /// Logs the raw message to the log.
        /// </summary>
        /// <param name="logType">Type of the log message. Not: fatal will stop the engine. Error may show a message popup.</param>
        /// <param name="message">The message contents.</param>
        void LogWrite(LogType logType, string message);

        /// <summary>
        /// <para>A variant of ILogHandler.LogFormat that logs an exception message.</para>
        /// </summary>
        /// <param name="exception">Runtime Exception.</param>
        /// <param name="context">Object to which the message applies.</param>
        void LogException(Exception exception, Object context);

        /// <summary>
        /// <para>Logs a formatted message.</para>
        /// </summary>
        /// <param name="logType">The type of the log message.</param>
        /// <param name="context">Object to which the message applies.</param>
        /// <param name="message">Message to log.</param>
        void Log(LogType logType, Object context, string message);
    }
}
