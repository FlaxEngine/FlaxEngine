// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections;
using System.Collections.Concurrent;
using System.Collections.Generic;
using System.Runtime.CompilerServices;
using System.Threading;
using System.Threading.Tasks;
using FlaxEngine.GUI;

namespace FlaxEngine
{
    /// <summary>
    /// The async tasks scheduler that runs the tasks on a main thread during game update.
    /// </summary>
    /// <seealso cref="System.Threading.Tasks.TaskScheduler" />
    /// <seealso cref="System.IDisposable" />
    internal class MainThreadTaskScheduler : TaskScheduler, IDisposable
    {
        private readonly BlockingCollection<Task> _tasks = new BlockingCollection<Task>();

        internal void Execute()
        {
            while (_tasks.TryTake(out var task))
            {
                TryExecuteTask(task);
            }
        }

        private void Dispose(bool disposing)
        {
            if (!disposing)
                return;
            _tasks.CompleteAdding();
            _tasks.Dispose();
        }

        /// <inheritdoc />
        protected override void QueueTask(Task task)
        {
            _tasks.Add(task);
        }

        /// <inheritdoc />
        protected override bool TryExecuteTaskInline(Task task, bool taskWasPreviouslyQueued)
        {
            return false;
        }

        /// <inheritdoc />
        protected override IEnumerable<Task> GetScheduledTasks()
        {
            return _tasks.ToArray();
        }

        /// <inheritdoc />
        public void Dispose()
        {
            Dispose(true);
            GC.SuppressFinalize(this);
        }
    }

    /// <summary>
    /// C# scripting service.
    /// </summary>
    public static class Scripting
    {
        private static readonly List<Action> UpdateActions = new List<Action>();
        private static readonly MainThreadTaskScheduler MainThreadTaskScheduler = new MainThreadTaskScheduler();

        /// <summary>
        /// Occurs on scripting update.
        /// </summary>
        public static event Action Update;

        /// <summary>
        /// Occurs on scripting 'late' update.
        /// </summary>
        public static event Action LateUpdate;

        /// <summary>
        /// Occurs on scripting `fixed` update.
        /// </summary>
        public static event Action FixedUpdate;

        /// <summary>
        /// Occurs on scripting `draw` update. Called during frame rendering and can be used to invoke custom rendering with GPUDevice.
        /// </summary>
        public static event Action Draw;

        /// <summary>
        /// Occurs when scripting engine is disposing. Engine is during closing and some services may be unavailable (eg. loading scenes). This may be called after the engine fatal error event.
        /// </summary>
        public static event Action Exit;

        /// <summary>
        /// Gets the async tasks scheduler that runs the tasks on a main thread during game update.
        /// </summary>
        public static TaskScheduler MainThreadScheduler => MainThreadTaskScheduler;

        /// <summary>
        /// Calls the given action on the next scripting update.
        /// </summary>
        /// <param name="action">The action to invoke.</param>
        public static void InvokeOnUpdate(Action action)
        {
            lock (UpdateActions)
            {
                UpdateActions.Add(action);
            }
        }

        /// <summary>
        /// Queues the specified work to run on the main thread and returns a <see cref="T:System.Threading.Tasks.Task" /> object that represents that work.
        /// </summary>
        /// <param name="action">The work to execute asynchronously</param>
        /// <returns>A task that represents the work queued to execute in the pool.</returns>
        public static Task RunOnUpdate(Action action)
        {
            return Task.Factory.StartNew(action, CancellationToken.None, TaskCreationOptions.None, MainThreadTaskScheduler);
        }

        private static void OnUnhandledException(object sender, UnhandledExceptionEventArgs e)
        {
            if (e.ExceptionObject is Exception exception)
            {
                Debug.LogError("Unhandled Exception: " + exception.Message);
                Debug.LogException(exception);
            }
        }

        private static void OnUnobservedTaskException(object sender, UnobservedTaskExceptionEventArgs e)
        {
            if (e.Exception != null)
            {
                foreach (var ex in e.Exception.InnerExceptions)
                {
                    Debug.LogError("Unhandled Task Exception: " + ex.Message);
                    Debug.LogException(ex);
                }
            }
        }

        /// <summary>
        /// Initializes Flax API. Called before everything else from native code.
        /// </summary>
        internal static void Init()
        {
#if BUILD_DEBUG
            Debug.Logger.LogHandler.LogWrite(LogType.Info, "Using FlaxAPI in Debug");
#elif BUILD_DEVELOPMENT
            Debug.Logger.LogHandler.LogWrite(LogType.Info, "Using FlaxAPI in Development");
#elif BUILD_RELEASE
            Debug.Logger.LogHandler.LogWrite(LogType.Info, "Using FlaxAPI in Release");
#endif

            AppDomain.CurrentDomain.UnhandledException += OnUnhandledException;
            TaskScheduler.UnobservedTaskException += OnUnobservedTaskException;

            if (!Engine.IsEditor)
            {
                CreateGuiStyle();
            }
        }

        /// <summary>
        /// Sets the managed window as a main game window. Called after creating game window by the native code.
        /// </summary>
        /// <param name="window">The window.</param>
        internal static void SetWindow(Window window)
        {
            // Link it as a GUI root control
            window.GUI.BackgroundColor = Color.Transparent;
            RootControl.GameRoot = window.GUI;
        }

        internal static Type MakeGenericType(Type genericType, Type[] genericArgs)
        {
            return genericType.MakeGenericType(genericArgs);
        }

        internal static void AddDictionaryItem(IDictionary dictionary, object key, object value)
        {
            dictionary.Add(key, value);
        }

        internal static object[] GetDictionaryKeys(IDictionary dictionary)
        {
            var keys = dictionary.Keys;
            var result = new object[keys.Count];
            keys.CopyTo(result, 0);
            return result;
        }

        private static void CreateGuiStyle()
        {
            var style = new Style
            {
                Background = Color.FromBgra(0xFF1C1C1C),
                LightBackground = Color.FromBgra(0xFF2D2D30),
                Foreground = Color.FromBgra(0xFFFFFFFF),
                ForegroundGrey = Color.FromBgra(0xFFA9A9B3),
                ForegroundDisabled = Color.FromBgra(0xFF787883),
                BackgroundHighlighted = Color.FromBgra(0xFF54545C),
                BorderHighlighted = Color.FromBgra(0xFF6A6A75),
                BackgroundSelected = Color.FromBgra(0xFF007ACC),
                BorderSelected = Color.FromBgra(0xFF1C97EA),
                BackgroundNormal = Color.FromBgra(0xFF3F3F46),
                BorderNormal = Color.FromBgra(0xFF54545C),
                TextBoxBackground = Color.FromBgra(0xFF333337),
                ProgressNormal = Color.FromBgra(0xFF0ad328),
                TextBoxBackgroundSelected = Color.FromBgra(0xFF3F3F46),
                SharedTooltip = new Tooltip(),
            };
            style.DragWindow = style.BackgroundSelected * 0.7f;

            Style.Current = style;
        }

        internal static void Internal_Update()
        {
            Update?.Invoke();

            lock (UpdateActions)
            {
                for (int i = 0; i < UpdateActions.Count; i++)
                {
                    try
                    {
                        UpdateActions[i]();
                    }
                    catch (Exception ex)
                    {
                        Debug.LogException(ex);
                    }
                }
                UpdateActions.Clear();
            }

            MainThreadTaskScheduler.Execute();
        }

        internal static void Internal_LateUpdate()
        {
            LateUpdate?.Invoke();
        }

        internal static void Internal_FixedUpdate()
        {
            FixedUpdate?.Invoke();
        }

        internal static void Internal_Draw()
        {
            Draw?.Invoke();
        }

        internal static void Internal_Exit()
        {
            Exit?.Invoke();

            MainThreadTaskScheduler.Dispose();
            Json.JsonSerializer.Dispose();
        }

        /// <summary>
        /// Returns true if game scripts assembly has been loaded.
        /// </summary>
        /// <returns>True if game scripts assembly is loaded, otherwise false.</returns>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern bool HasGameModulesLoaded();

        /// <summary>
        /// Returns true if given type is from one of the game scripts assemblies.
        /// </summary>
        /// <returns>True if the type is from game assembly, otherwise false.</returns>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern bool IsTypeFromGameScripts(Type type);

        /// <summary>
        /// Flushes the removed objects (disposed objects using Object.Destroy).
        /// </summary>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void FlushRemovedObjects();
    }

    /// <summary>
    /// Provides C# scripts profiling methods.
    /// </summary>
    /// <remarks>
    /// Profiler is available in the editor and Debug/Development builds. Release builds don't have profiling tools.
    /// </remarks>
    public static class Profiler
    {
        /// <summary>
        /// Begins profiling a piece of code with a custom label.
        /// </summary>
        /// <param name="name">The name of the event.</param>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void BeginEvent(string name);

        /// <summary>
        /// Ends profiling an event.
        /// </summary>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void EndEvent();

        /// <summary>
        /// Begins GPU profiling a piece of code with a custom label.
        /// </summary>
        /// <param name="name">The name of the event.</param>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void BeginEventGPU(string name);

        /// <summary>
        /// Ends GPU profiling an event.
        /// </summary>
        [MethodImpl(MethodImplOptions.InternalCall)]
        public static extern void EndEventGPU();
    }
}
