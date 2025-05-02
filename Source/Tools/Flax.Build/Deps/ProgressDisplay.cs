// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Linq;
using System.Threading;

namespace Flax.Deps
{
    /// <summary>
    /// Class to display download progress
    /// </summary>
    public class ProgressDisplay : IDisposable
    {
        /// <summary>
        /// Current progress
        /// </summary>
        private long _progress;

        /// <summary>
        /// Current display thread
        /// </summary>
        private readonly Thread _asyncDisplayThread;

        /// <summary>
        /// Progress display console position
        /// </summary>
        private readonly int _cursorPos;

        /// <summary>
        /// Queue of diffs from progress
        /// </summary>
        /// <remarks>
        /// Queue last second of data
        /// </remarks>
        private readonly Queue<long> _progressQueue = new Queue<long>();

        /// <summary>
        /// Last progress to calculate progress diff
        /// </summary>
        private long _lastProgress;

        /// <summary>
        /// Should thread be alive
        /// </summary>
        private bool _isAlive = true;

        /// <summary>
        /// Total file size in bytes
        /// </summary>
        public long TotalFileSizeBytes { get; private set; }

        /// <summary>
        /// Current progress of task
        /// </summary>
        public long Progress
        {
            get => _progress;
            set
            {
                if (value < _progress)
                {
                    throw new Exception("Value cannot be smaller then current progress.");
                }

                _progress = value;
            }
        }

        /// <summary>
        /// Gets a value indicating whether can use system console for progress reporting.
        /// </summary>
        public static bool CanUseConsole
        {
            get
            {
                return false;
                /*
                try
                {
                    // App crashes on Azure DevOps with exception:
                    // The handle is invalid.
                    // at
                    // at System.IO.__Error.WinIOError(Int32 errorCode, String maybeFullPath)
                    // at System.Console.GetBufferInfo(Boolean throwOnNoConsole, Boolean & succeeded)
                    // at Flax.Deps.ProgressDisplay..ctor(Int64 totalFileSizeBytes)
                    int a = Console.CursorTop;
                    Console.SetCursorPosition(0, a + 1);
                }
                catch (Exception ex)
                {
                    Console.WriteLine("No ProgressDisplay support.");
                    Console.WriteLine(ex.Message);
                    Console.WriteLine();
                    Console.WriteLine(ex.StackTrace);
                    Console.WriteLine();
                    return false;
                }

                return true;
                */
            }
        }

        /// <summary>
        /// Initializes a new instance of the <see cref="ProgressDisplay"/> class.
        /// </summary>
        /// <param name="totalFileSizeBytes">The total file size bytes.</param>
        public ProgressDisplay(long totalFileSizeBytes)
        {
            TotalFileSizeBytes = totalFileSizeBytes;
            _cursorPos = Console.CursorTop;
            Console.SetCursorPosition(0, _cursorPos + 1);
            _asyncDisplayThread = new Thread(Display);
            _asyncDisplayThread.Start();
        }

        /// <summary>
        /// Updates the progress.
        /// </summary>
        /// <param name="progress">The progress.</param>
        /// <param name="totalLength">The total length.</param>
        public void Update(long progress, long totalLength)
        {
            TotalFileSizeBytes = totalLength <= 0 ? progress : totalLength;
            _progress = progress;
        }

        /// <summary>
        /// Updates console text with current progress.
        /// </summary>
        private void Display()
        {
            lock (_asyncDisplayThread)
            {
                while (_isAlive)
                {
                    // Set queue progress
                    _progressQueue.Enqueue(Progress - _lastProgress);

                    // Remember last cursor position
                    var cursorPosTop = Console.CursorTop;
                    var cursorPosLeft = Console.CursorLeft;

                    // Set cursor position at progress display location
                    Console.SetCursorPosition(0, _cursorPos);
                    var display = $"Downloading: {(Progress / (1024 * 1024f)):0.00} MB / {(TotalFileSizeBytes / (1024 * 1024f)):0.00} - {_progressQueue.Sum() / (1024 * 1024f):0.00} MB/s";
                    // TODO add blank space fillings at the end of line if previous line was longer
                    Console.WriteLine(display);

                    // Set back previous position 
                    Console.SetCursorPosition(cursorPosLeft, cursorPosTop);

                    // Remove old entries from queue
                    if (_progressQueue.Count > 10)
                    {
                        _progressQueue.Dequeue();
                    }

                    _lastProgress = Progress;

                    // Wait 1/10 of second
                    Thread.Sleep(100);
                }
            }
        }

        /// <summary>
        /// Performs application-defined tasks associated with freeing, releasing, or resetting unmanaged resources.
        /// </summary>
        public void Dispose()
        {
            _isAlive = false;
        }
    }
}
