// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Runtime.CompilerServices;

namespace FlaxEngine
{
    /// <summary>
    /// Initializes a new instance of the Logger.
    /// </summary>
    [HideInEditor]
    public class Logger : ILogger
    {
        private const string TagFormat = "{0}: {1}";

        /// <summary>
        /// Create a custom Logger.
        /// </summary>
        /// <param name="logHandler">Pass in default log handler or custom log handler.</param>
        public Logger(ILogHandler logHandler)
        {
            LogHandler = logHandler;
            LogEnabled = true;
            FilterLogType = LogType.Info;
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private static string GetString(object message)
        {
            return message?.ToString() ?? "null";
        }

        /// <summary>
        /// To selective enable debug log message.
        /// </summary>
        public LogType FilterLogType { get; set; }

        /// <summary>
        /// To runtime toggle debug logging [ON/OFF].
        /// </summary>
        public bool LogEnabled { get; set; }

        /// <summary>
        /// Set  Logger.ILogHandler.
        /// </summary>
        public ILogHandler LogHandler { get; set; }

        /// <summary>
        /// Check logging is enabled based on the LogType.
        /// </summary>
        /// <param name="logType">The type of the log message.</param>
        /// <returns>Returns true in case logs of LogType will be logged otherwise returns false.</returns>
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        public bool IsLogTypeAllowed(LogType logType)
        {
            return LogEnabled && logType >= FilterLogType;
        }

        /// <summary>
        /// Logs message to the Flax Console using default logger.
        /// </summary>
        /// <param name="logType">The type of the log message.</param>
        /// <param name="message">String or object to be converted to string representation for display.</param>
        public void Log(LogType logType, object message)
        {
            if (IsLogTypeAllowed(logType))
                LogHandler.Log(logType, null, GetString(message));
        }

        /// <summary>
        /// Logs message to the Flax Console using default logger.
        /// </summary>
        /// <param name="logType">The type of the log message.</param>
        /// <param name="message">String or object to be converted to string representation for display.</param>
        /// <param name="context">Object to which the message applies.</param>
        public void Log(LogType logType, object message, Object context)
        {
            if (IsLogTypeAllowed(logType))
                LogHandler.Log(logType, context, GetString(message));
        }

        /// <summary>
        /// Logs message to the Flax Console using default logger.
        /// </summary>
        /// <param name="logType">The type of the log message.</param>
        /// <param name="tag">
        /// Used to identify the source of a log message. It usually identifies the class where the log call
        /// occurs.
        /// </param>
        /// <param name="message">String or object to be converted to string representation for display.</param>
        public void Log(LogType logType, string tag, object message)
        {
            if (IsLogTypeAllowed(logType))
                LogHandler.Log(logType, null, string.Format(TagFormat, tag, GetString(message)));
        }

        /// <summary>
        /// Logs message to the Flax Console using default logger.
        /// </summary>
        /// <param name="logType">The type of the log message.</param>
        /// <param name="tag">
        /// Used to identify the source of a log message. It usually identifies the class where the log call
        /// occurs.
        /// </param>
        /// <param name="message">String or object to be converted to string representation for display.</param>
        /// <param name="context">Object to which the message applies.</param>
        public void Log(LogType logType, string tag, object message, Object context)
        {
            if (IsLogTypeAllowed(logType))
                LogHandler.Log(logType, context, string.Format(TagFormat, tag, GetString(message)));
        }

        /// <summary>
        /// Logs message to the Flax Console using default logger.
        /// </summary>
        /// <param name="message">String or object to be converted to string representation for display.</param>
        public void Log(object message)
        {
            if (IsLogTypeAllowed(LogType.Info))
                LogHandler.Log(LogType.Info, null, GetString(message));
        }

        /// <summary>
        /// Logs message to the Flax Console using default logger.
        /// </summary>
        /// <param name="tag">
        /// Used to identify the source of a log message. It usually identifies the class where the log call
        /// occurs.
        /// </param>
        /// <param name="message">String or object to be converted to string representation for display.</param>
        public void Log(string tag, object message)
        {
            if (IsLogTypeAllowed(LogType.Info))
                LogHandler.Log(LogType.Info, null, string.Format(TagFormat, tag, GetString(message)));
        }

        /// <summary>
        /// Logs message to the Flax Console using default logger.
        /// </summary>
        /// <param name="tag">
        /// Used to identify the source of a log message. It usually identifies the class where the log call
        /// occurs.
        /// </param>
        /// <param name="message">String or object to be converted to string representation for display.</param>
        /// <param name="context">Object to which the message applies.</param>
        public void Log(string tag, object message, Object context)
        {
            if (IsLogTypeAllowed(LogType.Info))
                LogHandler.Log(LogType.Info, context, string.Format(TagFormat, tag, GetString(message)));
        }

        /// <summary>
        /// A variant of Logger.Info that logs an error message.
        /// </summary>
        /// <param name="tag">
        /// Used to identify the source of a log message. It usually identifies the class where the log call
        /// occurs.
        /// </param>
        /// <param name="message">String or object to be converted to string representation for display.</param>
        public void LogError(string tag, object message)
        {
            if (IsLogTypeAllowed(LogType.Error))
                LogHandler.Log(LogType.Error, null, string.Format(TagFormat, tag, GetString(message)));
        }

        /// <summary>
        /// A variant of Logger.Info that logs an error message.
        /// </summary>
        /// <param name="tag">
        /// Used to identify the source of a log message. It usually identifies the class where the log call
        /// occurs.
        /// </param>
        /// <param name="message">String or object to be converted to string representation for display.</param>
        /// <param name="context">Object to which the message applies.</param>
        public void LogError(string tag, object message, Object context)
        {
            if (IsLogTypeAllowed(LogType.Error))
                LogHandler.Log(LogType.Error, context, string.Format(TagFormat, tag, GetString(message)));
        }

        /// <summary>
        /// A variant of Logger.Info that logs an exception message.
        /// </summary>
        /// <param name="exception">Runtime Exception.</param>
        public void LogException(Exception exception)
        {
            if (LogEnabled)
                LogHandler.LogException(exception, null);
        }

        /// <summary>
        /// A variant of Logger.Info that logs an exception message.
        /// </summary>
        /// <param name="exception">Runtime Exception.</param>
        /// <param name="context">Object to which the message applies.</param>
        public void LogException(Exception exception, Object context)
        {
            if (LogEnabled)
                LogHandler.LogException(exception, context);
        }

        /// <inheritdoc />
        public void Log(LogType logType, Object context, string message)
        {
            if (IsLogTypeAllowed(logType))
                LogHandler.Log(logType, context, message);
        }

        /// <inheritdoc />
        public void LogFormat(LogType logType, string format, params object[] args)
        {
            if (IsLogTypeAllowed(logType))
                LogHandler.Log(logType, null, string.Format(format, args));
        }

        /// <summary>
        /// Logs a formatted message.
        /// </summary>
        /// <param name="logType">The type of the log message.</param>
        /// <param name="format">A composite format string.</param>
        /// <param name="args">Format arguments.</param>
        public void Log(LogType logType, string format, params object[] args)
        {
            if (IsLogTypeAllowed(logType))
                LogHandler.Log(logType, null, string.Format(format, args));
        }

        /// <summary>
        /// Logs a formatted message.
        /// </summary>
        /// <param name="logType">The type of the log message.</param>
        /// <param name="context">Object to which the message applies.</param>
        /// <param name="format">A composite format string.</param>
        /// <param name="args">Format arguments.</param>
        public void Log(LogType logType, Object context, string format, params object[] args)
        {
            if (IsLogTypeAllowed(logType))
                LogHandler.Log(logType, context, string.Format(format, args));
        }

        /// <summary>
        /// A variant of Logger.Info that logs an warning message.
        /// </summary>
        /// <param name="tag">
        /// Used to identify the source of a log message. It usually identifies the class where the log call
        /// occurs.
        /// </param>
        /// <param name="message">String or object to be converted to string representation for display.</param>
        public void LogWarning(string tag, object message)
        {
            if (IsLogTypeAllowed(LogType.Warning))
                LogHandler.Log(LogType.Warning, null, string.Format(TagFormat, tag, GetString(message)));
        }

        /// <summary>
        /// A variant of Logger.Info that logs an warning message.
        /// </summary>
        /// <param name="tag">
        /// Used to identify the source of a log message. It usually identifies the class where the log call
        /// occurs.
        /// </param>
        /// <param name="message">String or object to be converted to string representation for display.</param>
        /// <param name="context">Object to which the message applies.</param>
        public void LogWarning(string tag, object message, Object context)
        {
            if (IsLogTypeAllowed(LogType.Warning))
                LogHandler.Log(LogType.Warning, context, string.Format(TagFormat, tag, GetString(message)));
        }
    }
}
