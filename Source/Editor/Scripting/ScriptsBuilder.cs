// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace FlaxEditor
{
    partial class ScriptsBuilder
    {
        /// <summary>
        /// Compilation end event delegate.
        /// </summary>
        /// <param name="success">False if compilation has failed, otherwise true.</param>
        public delegate void CompilationEndDelegate(bool success);

        /// <summary>
        /// Compilation message events delegate.
        /// </summary>
        /// <param name="message">The message.</param>
        /// <param name="file">The target file.</param>
        /// <param name="line">The target line.</param>
        public delegate void CompilationMessageDelegate(string message, string file, int line);

        /// <summary>
        /// Occurs when compilation ends.
        /// </summary>
        public static event CompilationEndDelegate CompilationEnd;

        /// <summary>
        /// Occurs when compilation success.
        /// </summary>
        public static event Action CompilationSuccess;

        /// <summary>
        /// Occurs when compilation failed.
        /// </summary>
        public static event Action CompilationFailed;

        /// <summary>
        /// Occurs when compilation begins.
        /// </summary>
        public static event Action CompilationBegin;

        /// <summary>
        /// Occurs when compilation just started.
        /// </summary>
        public static event Action CompilationStarted;

        /// <summary>
        /// Occurs when user scripts reload action is called.
        /// </summary>
        public static event Action ScriptsReloadCalled;

        /// <summary>
        /// Occurs when user scripts reload starts.
        /// User objects should be removed at this point to reduce leaks and issues. Game scripts and game editor scripts assemblies will be reloaded.
        /// </summary>
        public static event Action ScriptsReloadBegin;

        /// <summary>
        /// Occurs when user scripts reload is performed (just before the actual reload, scenes are serialized and unloaded). All user objects should be cleanup.
        /// </summary>
        public static event Action ScriptsReload;

        /// <summary>
        /// Occurs when user scripts reload ends.
        /// </summary>
        public static event Action ScriptsReloadEnd;

        /// <summary>
        /// Occurs when engine loads game scripts.
        /// </summary>
        public static event Action ScriptsLoaded;

        /// <summary>
        /// Occurs when code editor starts asynchronous open a file or a solution.
        /// </summary>
        public static event Action CodeEditorAsyncOpenBegin;

        /// <summary>
        /// Occurs when code editor ends asynchronous open a file or a solution.
        /// </summary>
        public static event Action CodeEditorAsyncOpenEnd;

        internal enum EventType
        {
            CompileBegin = 0,
            CompileStarted = 1,
            CompileEndGood = 2,
            CompileEndFailed = 3,
            ReloadCalled = 4,
            ReloadBegin = 5,
            Reload = 6,
            ReloadEnd = 7,
            ScriptsLoaded = 8,
        }

        internal static void Internal_OnEvent(EventType type)
        {
            switch (type)
            {
            case EventType.CompileBegin:
                CompilationBegin?.Invoke();
                break;
            case EventType.CompileStarted:
                CompilationStarted?.Invoke();
                break;
            case EventType.CompileEndGood:
                CompilationEnd?.Invoke(true);
                CompilationSuccess?.Invoke();
                break;
            case EventType.CompileEndFailed:
                CompilationEnd?.Invoke(false);
                CompilationFailed?.Invoke();
                break;
            case EventType.ReloadCalled:
                ScriptsReloadCalled?.Invoke();
                break;
            case EventType.ReloadBegin:
                ScriptsReloadBegin?.Invoke();
                break;
            case EventType.Reload:
                ScriptsReload?.Invoke();
                break;
            case EventType.ReloadEnd:
                ScriptsReloadEnd?.Invoke();
                break;
            case EventType.ScriptsLoaded:
                ScriptsLoaded?.Invoke();
                break;
            }
        }

        internal static void Internal_OnCodeEditorEvent(bool isEnd)
        {
            if (isEnd)
                CodeEditorAsyncOpenEnd?.Invoke();
            else
                CodeEditorAsyncOpenBegin?.Invoke();
        }
    }
}
