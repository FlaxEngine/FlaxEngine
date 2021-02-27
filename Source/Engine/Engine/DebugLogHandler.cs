// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using System.Security;

namespace FlaxEngine
{
    internal sealed class DebugLogHandler : ILogHandler
    {
        /// <summary>
        /// Occurs on sending a log message.
        /// </summary>
        public event LogDelegate SendLog;

        /// <summary>
        /// Occurs on sending a log message.
        /// </summary>
        public event LogExceptionDelegate SendExceptionLog;

        /// <inheritdoc />
        public void LogWrite(LogType logType, string message)
        {
            Internal_LogWrite(logType, message);
        }

        /// <inheritdoc />
        public void LogException(Exception exception, Object context)
        {
            Internal_LogException(exception, Object.GetUnmanagedPtr(context));

            SendExceptionLog?.Invoke(exception, context);
        }

        /// <inheritdoc />
        public void Log(LogType logType, Object context, string message)
        {
            if (message == null)
                return;
#if BUILD_RELEASE
            string stackTrace = null;
#else
            string stackTrace = Environment.StackTrace;
#endif
            Internal_Log(logType, message, Object.GetUnmanagedPtr(context), stackTrace);
            SendLog?.Invoke(logType, message, context, stackTrace);
        }

        internal static void Internal_SendLog(LogType logType, string message, string stackTrace)
        {
            if (message == null)
                return;
            var logger = Debug.Logger;
            if (logger.IsLogTypeAllowed(logType))
            {
                Internal_Log(logType, message, IntPtr.Zero, stackTrace);
                ((DebugLogHandler)logger.LogHandler).SendLog?.Invoke(logType, message, null, stackTrace);
            }
        }

        internal static void Internal_SendLogException(Exception exception)
        {
            Debug.Logger.LogException(exception);
        }

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Internal_LogWrite(LogType level, string msg);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Internal_Log(LogType level, string msg, IntPtr obj, string stackTrace);

        [MethodImpl(MethodImplOptions.InternalCall)]
        internal static extern void Internal_LogException(Exception exception, IntPtr obj);

        [SecuritySafeCritical]
        public static string Internal_GetStackTrace()
        {
            var stackTrace = new StackTrace(1, true);
            return stackTrace.ToString();
        }
    }
}
