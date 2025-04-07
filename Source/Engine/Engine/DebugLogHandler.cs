// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using System.Security;
using System.Runtime.InteropServices;
using System.Runtime.InteropServices.Marshalling;

namespace FlaxEngine
{
    internal partial class DebugLogHandler : ILogHandler
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

        [LibraryImport("FlaxEngine", EntryPoint = "DebugLogHandlerInternal_LogWrite", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(Interop.StringMarshaller))]
        internal static partial void Internal_LogWrite(LogType level, string msg);

        [LibraryImport("FlaxEngine", EntryPoint = "DebugLogHandlerInternal_Log", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(Interop.StringMarshaller))]
        internal static partial void Internal_Log(LogType level, string msg, IntPtr obj, string stackTrace);

        [LibraryImport("FlaxEngine", EntryPoint = "DebugLogHandlerInternal_LogException", StringMarshalling = StringMarshalling.Custom, StringMarshallingCustomType = typeof(Interop.StringMarshaller))]
        internal static partial void Internal_LogException([MarshalUsing(typeof(Interop.ExceptionMarshaller))] Exception exception, IntPtr obj);

        [SecuritySafeCritical]
        public static string Internal_GetStackTrace()
        {
            var stackTrace = new StackTrace(1, true);
            return stackTrace.ToString();
        }
    }
}
