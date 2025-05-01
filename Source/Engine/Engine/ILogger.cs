// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEngine
{
    /// <summary>
    /// Info message delegate.
    /// </summary>
    /// <param name="level">The log level.</param>
    /// <param name="msg">The message.</param>
    /// <param name="obj">The object.</param>
    /// <param name="stackTrace">The stack trace.</param>
    [HideInEditor]
    public delegate void LogDelegate(LogType level, string msg, Object obj, string stackTrace);

    /// <summary>
    /// Info exception delegate.
    /// </summary>
    /// <param name="exception">The exception.</param>
    /// <param name="obj">The object.</param>
    [HideInEditor]
    public delegate void LogExceptionDelegate(Exception exception, Object obj);

    /// <summary>
    /// Logger interface.
    /// </summary>
    public interface ILogger
    {
        /// <summary>
        /// <para>To selective enable debug log message.</para>
        /// </summary>
        LogType FilterLogType { get; set; }

        /// <summary>
        /// <para>To runtime toggle debug logging [ON/OFF].</para>
        /// </summary>
        bool LogEnabled { get; set; }

        /// <summary>
        /// <para>Set Logger.ILogHandler.</para>
        /// </summary>
        ILogHandler LogHandler { get; set; }

        /// <summary>
        /// <para>Check logging is enabled based on the LogType.</para>
        /// </summary>
        /// <param name="logType"></param>
        /// <returns>
        /// <para>Return true in case logs of LogType will be logged otherwise returns false.</para>
        /// </returns>
        bool IsLogTypeAllowed(LogType logType);

        /// <summary>
        /// <para>Logs message to the Flax Console using default logger.</para>
        /// </summary>
        /// <param name="logType"></param>
        /// <param name="message"></param>
        void Log(LogType logType, object message);

        /// <summary>
        /// <para>Logs message to the Flax Console using default logger.</para>
        /// </summary>
        /// <param name="logType"></param>
        /// <param name="message"></param>
        /// <param name="context"></param>
        void Log(LogType logType, object message, Object context);

        /// <summary>
        /// <para>Logs message to the Flax Console using default logger.</para>
        /// </summary>
        /// <param name="logType"></param>
        /// <param name="message"></param>
        /// <param name="tag"></param>
        void Log(LogType logType, string tag, object message);

        /// <summary>
        /// <para>Logs message to the Flax Console using default logger.</para>
        /// </summary>
        /// <param name="logType"></param>
        /// <param name="message"></param>
        /// <param name="context"></param>
        /// <param name="tag"></param>
        void Log(LogType logType, string tag, object message, Object context);

        /// <summary>
        /// <para>Logs message to the Flax Console using default logger.</para>
        /// </summary>
        /// <param name="message"></param>
        void Log(object message);

        /// <summary>
        /// <para>Logs message to the Flax Console using default logger.</para>
        /// </summary>
        /// <param name="message"></param>
        /// <param name="tag"></param>
        void Log(string tag, object message);

        /// <summary>
        /// <para>Logs message to the Flax Console using default logger.</para>
        /// </summary>
        /// <param name="message"></param>
        /// <param name="context"></param>
        /// <param name="tag"></param>
        void Log(string tag, object message, Object context);

        /// <summary>
        /// <para>A variant of ILogger.Info that logs an error message.</para>
        /// </summary>
        /// <param name="tag"></param>
        /// <param name="message"></param>
        void LogError(string tag, object message);

        /// <summary>
        /// <para>A variant of ILogger.Info that logs an error message.</para>
        /// </summary>
        /// <param name="tag"></param>
        /// <param name="message"></param>
        /// <param name="context"></param>
        void LogError(string tag, object message, Object context);

        /// <summary>
        /// <para>A variant of ILogger.Info that logs an exception message.</para>
        /// </summary>
        /// <param name="exception"></param>
        void LogException(Exception exception);

        /// <summary>
        /// <para>Logs a formatted message.</para>
        /// </summary>
        /// <param name="logType"></param>
        /// <param name="format"></param>
        /// <param name="args"></param>
        void LogFormat(LogType logType, string format, params object[] args);

        /// <summary>
        /// <para>A variant of Logger.Info that logs an warning message.</para>
        /// </summary>
        /// <param name="tag"></param>
        /// <param name="message"></param>
        void LogWarning(string tag, object message);

        /// <summary>
        /// <para>A variant of Logger.Info that logs an warning message.</para>
        /// </summary>
        /// <param name="tag"></param>
        /// <param name="message"></param>
        /// <param name="context"></param>
        void LogWarning(string tag, object message, Object context);

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
