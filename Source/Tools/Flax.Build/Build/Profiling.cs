// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Collections.Generic;
using System.Text;
using System.Threading;
using System.Diagnostics;

namespace Flax.Build
{
    /// <summary>
    /// Inserts a profiling event for the given code scope.
    /// </summary>
    public class ProfileEventScope : IDisposable
    {
        private readonly int _id;

        /// <summary>
        /// Initializes a new instance of the <see cref="ProfileEventScope"/> class.
        /// </summary>
        /// <param name="name">The event name.</param>
        public ProfileEventScope(string name)
        {
            _id = Profiling.Begin(name);
        }

        /// <summary>
        /// Ends the profiling event.
        /// </summary>
        public void Dispose()
        {
            Profiling.End(_id);
        }
    }

    /// <summary>
    /// The build system performance profiling tools.
    /// </summary>
    public static class Profiling
    {
        /// <summary>
        /// The performance event data.
        /// </summary>
        public struct Event
        {
            /// <summary>
            /// The event name.
            /// </summary>
            public string Name;

            /// <summary>
            /// The event start time.
            /// </summary>
            public DateTime StartTime;

            /// <summary>
            /// The event duration.
            /// </summary>
            public TimeSpan Duration;

            /// <summary>
            /// The event call depth (for parent-children events evaluation).
            /// </summary>
            public int Depth;

            /// <summary>
            /// The calling thread id.
            /// </summary>
            public int ThreadId;
        }

        private static int _depth;
        private static readonly List<Event> _events = new List<Event>(1024);
        private static readonly DateTime _startTime = DateTime.Now;
        private static readonly Stopwatch _stopwatch = Stopwatch.StartNew(); // https://stackoverflow.com/questions/1416139/how-to-get-timestamp-of-tick-precision-in-net-c

        /// <summary>
        /// Begins the profiling event.
        /// </summary>
        /// <param name="name">The name.</param>
        /// <returns>The event id. Used by <see cref="End"/> callback.</returns>
        public static int Begin(string name)
        {
            Event e;
            e.Name = name;
            e.StartTime = _startTime.AddTicks(_stopwatch.Elapsed.Ticks);
            e.Duration = TimeSpan.Zero;
            e.Depth = _depth++;
            e.ThreadId = Thread.CurrentThread.ManagedThreadId;
            _events.Add(e);
            return _events.Count - 1;
        }

        /// <summary>
        /// Ends the profiling event.
        /// </summary>
        /// <param name="id">The event identifier returned by <see cref="Begin"/>.</param>
        public static void End(int id)
        {
            var endTime = _startTime.AddTicks(_stopwatch.Elapsed.Ticks);
            var e = _events[id];
            e.Duration = endTime - e.StartTime;
            _events[id] = e;
            _depth--;
        }

        /// <summary>
        /// Logs the recorded profiler events.
        /// </summary>
        public static void LogStats()
        {
            TimeSpan add = TimeSpan.Zero;
            for (int i = 0; i < _events.Count; i++)
            {
                var e = _events[i];

                if (i + 1 < _events.Count && _events[i + 1].Depth == e.Depth && _events[i + 1].Name == e.Name)
                {
                    // Merge two events times
                    add += e.Duration;
                    continue;
                }

                string str = string.Empty;
                while (e.Depth-- > 0)
                    str += "  ";
                str += e.Name;
                str += " ";
                str += e.Duration + add;
                add = TimeSpan.Zero;

                Log.Info(str);
            }
        }

        /// <summary>
        /// Generates the Trace Event file for profiling build system events.
        /// </summary>
        public static void GenerateTraceEventFile()
        {
            var traceEventFile = Configuration.TraceEventFile;
            if (traceEventFile.Length == 0)
            {
                var logFile = Configuration.LogFile.Length != 0 ? Configuration.LogFile : "Cache/Intermediate/Log.txt";
                traceEventFile = Path.ChangeExtension(logFile, ".trace.json");
            }
            var path = Path.GetDirectoryName(traceEventFile);
            if (!string.IsNullOrEmpty(path) && !Directory.Exists(path))
                Directory.CreateDirectory(path);

            Log.Info("Saving trace events to " + traceEventFile);
            var contents = new StringBuilder();

            contents.Append("{\"traceEvents\": [");
            var startTime = _events.Count != 0 ? _events[0].StartTime : DateTime.MinValue;
            for (int i = 0; i < _events.Count; i++)
            {
                var e = _events[i];

                contents.Append($"{{ \"pid\":{e.ThreadId}, \"tid\":1, \"ts\":{(int)((e.StartTime - startTime).TotalMilliseconds * 1000.0)}, \"dur\":{(int)(e.Duration.TotalMilliseconds * 1000.0)}, \"ph\":\"X\", \"name\":\"{e.Name}\", \"args\":{{ \"startTime\":\"{e.StartTime.ToShortTimeString()}\" }} }}\n");

                if (i + 1 != _events.Count)
                    contents.Append(",");
            }
            contents.Append("]}");

            File.WriteAllText(traceEventFile, contents.ToString());
        }
    }
}
