// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.IO;

namespace Flax.Build
{
    /// <summary>
    /// The logging interface for the build system
    /// </summary>
    public static class Log
    {
        private static object _consoleLocker = new object();
        private static FileStream _logFile;
        private static StreamWriter _logFileWriter;
        private static DateTime _startTime;
        private static ConsoleColor _defaultColor;

        /// <summary>
        /// The indent applied to the log messages.
        /// </summary>
        public static string Indent = string.Empty;

        /// <summary>
        /// If set to true the console will be colored for warning and errors messages.
        /// </summary>
        public static bool ApplyConsoleColors = true;

        internal static void Init()
        {
            _startTime = DateTime.Now;
            _defaultColor = Console.ForegroundColor;

            if (!string.IsNullOrEmpty(Configuration.LogFile))
            {
                var path = Path.GetDirectoryName(Configuration.LogFile);
                if (!string.IsNullOrEmpty(path) && !Directory.Exists(path))
                    Directory.CreateDirectory(path);
                _logFile = new FileStream(Configuration.LogFile, FileMode.Create, FileAccess.Write, FileShare.Read);
                _logFileWriter = new StreamWriter(_logFile);
            }
        }

        internal static void Dispose()
        {
            if (_logFile != null)
            {
                _logFileWriter.Dispose();
                _logFile.Dispose();
            }
        }

        /// <summary>
        /// Writes the specified message to the log.
        /// </summary>
        /// <param name="message">The message.</param>
        /// <param name="color">The color.</param>
        /// <param name="consoleLog">If set to <c>true</c> console will get the log.</param>
        public static void Write(string message, ConsoleColor color, bool consoleLog)
        {
            if (consoleLog)
            {
                lock (_consoleLocker)
                {
                    if (ApplyConsoleColors)
                        Console.ForegroundColor = color;

                    Console.WriteLine(Indent + message);

                    if (ApplyConsoleColors)
                        Console.ResetColor();

                    if (System.Diagnostics.Debugger.IsAttached)
                        System.Diagnostics.Debug.WriteLine(Indent + message);
                }
            }
            else if (Configuration.LogFileWithConsole)
                return;

            if (_logFile != null)
            {
                TimeSpan span = DateTime.Now - _startTime;
                string prefix = string.Format("[{0:00}:{1:00}:{2:000}] ", span.Minutes, span.Seconds, span.Milliseconds);

                lock (_logFile)
                {
                    if (Configuration.LogFileWithConsole)
                        _logFileWriter.WriteLine(message);
                    else
                        _logFileWriter.WriteLine(prefix + Indent + message);
                }
            }
        }

        /// <summary>
        /// Logs the verbose message.
        /// </summary>
        /// <param name="message">The message.</param>
        public static void Verbose(string message)
        {
            Write(message, _defaultColor, Configuration.ConsoleLog && Configuration.Verbose);
        }

        /// <summary>
        /// Logs the information.
        /// </summary>
        /// <param name="message">The message.</param>
        public static void Info(string message)
        {
            Write(message, _defaultColor, Configuration.ConsoleLog && !Configuration.LogMessagesOnly);
        }

        /// <summary>
        /// Logs the information.
        /// </summary>
        /// <param name="message">The message.</param>
        public static void Message(string message)
        {
            Write(message, _defaultColor, Configuration.ConsoleLog);
        }

        /// <summary>
        /// Logs the warning message.
        /// </summary>
        /// <param name="message">The message.</param>
        public static void Warning(string message)
        {
            Write(message, ConsoleColor.Yellow, true);
        }

        /// <summary>
        /// Logs the error message.
        /// </summary>
        /// <param name="message">The message.</param>
        public static void Error(string message)
        {
            Write(message, ConsoleColor.Red, true);
        }

        /// <summary>
        /// Logs the verbose message.
        /// </summary>
        /// <param name="message">The message.</param>
        /// <param name="flag">The flag used to indicate whether this log was already sent.</param>
        public static void VerboseOnce(string message, ref bool flag)
        {
            if (flag)
                return;
            flag = true;
            Write(message, _defaultColor, Configuration.ConsoleLog && Configuration.Verbose);
        }

        /// <summary>
        /// Logs the information.
        /// </summary>
        /// <param name="message">The message.</param>
        /// <param name="flag">The flag used to indicate whether this log was already sent.</param>
        public static void InfoOnce(string message, ref bool flag)
        {
            if (flag)
                return;
            flag = true;
            Write(message, _defaultColor, Configuration.ConsoleLog);
        }

        /// <summary>
        /// Logs the warning message.
        /// </summary>
        /// <param name="message">The message.</param>
        /// <param name="flag">The flag used to indicate whether this log was already sent.</param>
        public static void WarningOnce(string message, ref bool flag)
        {
            if (flag)
                return;
            flag = true;
            Write(message, ConsoleColor.Yellow, true);
        }

        /// <summary>
        /// Logs the error message.
        /// </summary>
        /// <param name="message">The message.</param>
        /// <param name="flag">The flag used to indicate whether this log was already sent.</param>
        public static void ErrorOnce(string message, ref bool flag)
        {
            if (flag)
                return;
            flag = true;
            Write(message, ConsoleColor.Red, true);
        }

        /// <summary>
        /// Logs the exception.
        /// </summary>
        /// <param name="ex">The exception.</param>
        public static void Exception(Exception ex)
        {
            Write(string.Format("Exception: {0}", ex.Message), ConsoleColor.Red, true);
            Write("Stack trace:", ConsoleColor.Yellow, true);
            Write(ex.StackTrace, ConsoleColor.Yellow, true);

            if (ex.InnerException != null)
            {
                Warning("Inner exception:");
                Exception(ex.InnerException);
            }
        }
    }
}
