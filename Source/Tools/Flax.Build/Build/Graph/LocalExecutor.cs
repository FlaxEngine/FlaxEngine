// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Threading;
using Flax.Build.Graph;

namespace Flax.Build.BuildSystem.Graph
{
    /// <summary>
    /// The local tasks executor. Uses thread pool to submit a tasks execution in parallel.
    /// </summary>
    /// <seealso cref="Flax.Build.Graph.TaskExecutor" />
    public class LocalExecutor : TaskExecutor
    {
        private int _counter;
        private readonly object _locker = new object();
        private readonly List<Task> _waitingTasks = new List<Task>();
        private readonly HashSet<Task> _executedTasks = new HashSet<Task>();

        /// <summary>
        /// The minimum amount of threads to be used for the parallel execution.
        /// </summary>
        public int ThreadCountMin = 1;

        /// <summary>
        /// The maximum amount of threads to be used for the parallel execution.
        /// </summary>
        public int ThreadCountMax = Configuration.MaxConcurrency;

        /// <summary>
        /// The amount of threads to allocate per processor. Use it to allocate more threads for faster execution or use less to keep reduce CPU usage during build.
        /// </summary>
        public float ProcessorCountScale = Configuration.ConcurrencyProcessorScale;

        /// <inheritdoc />
        public override int Execute(List<Task> tasks)
        {
            // Find tasks to executed
            _waitingTasks.Clear();
            _executedTasks.Clear();
            foreach (var task in tasks)
            {
                if (!task.HasValidCachedResults)
                    _waitingTasks.Add(task);
            }

            int count = _waitingTasks.Count;
            if (count == 0)
                return 0;
            _counter = 0;

            // Calculate amount of threads to spawn for the tasks execution
            int logicalCoresCount = (int)(Environment.ProcessorCount * ProcessorCountScale);
            int threadsCount = Math.Min(Math.Max(ThreadCountMin, Math.Min(ThreadCountMax, logicalCoresCount)), count);

            Log.Info(string.Format("Executing {0} {2} using {1} {3}", count, threadsCount, count == 1 ? "task" : "tasks", threadsCount == 1 ? "thread" : "threads"));

            // Spawn threads
            var threads = new Thread[threadsCount];
            for (int threadIndex = 0; threadIndex < threadsCount; threadIndex++)
            {
                threads[threadIndex] = new Thread(ThreadMain)
                {
                    Name = "Local Executor " + threadIndex,
                };
                threads[threadIndex].Start(threadIndex);
            }

            // Wait for the execution end
            for (int threadIndex = 0; threadIndex < threadsCount; threadIndex++)
            {
                threads[threadIndex].Join();
            }

            return _counter;
        }

        private void ThreadMain(object obj)
        {
            var threadIndex = (int)obj;
            // TODO: set affinity mask?
            var failedTasks = new List<Task>();

            while (true)
            {
                // Try to pick a task for the execution
                Task taskToRun = null;
                lock (_locker)
                {
                    // End when run out of the tasks to perform
                    if (_waitingTasks.Count == 0)
                        break;

                    failedTasks.Clear();
                    foreach (var task in _waitingTasks)
                    {
                        // Check if all its dependencies has been executed
                        bool hasCompletedDependencies = true;
                        bool hasAnyDependencyFailed = false;
                        if (task.DependentTasks != null)
                        {
                            foreach (var dependentTask in task.DependentTasks)
                            {
                                if (_executedTasks.Contains(dependentTask))
                                {
                                    // Handle dependency task execution result
                                    if (dependentTask.Failed)
                                    {
                                        hasAnyDependencyFailed = true;
                                    }
                                }
                                else if (!dependentTask.HasValidCachedResults)
                                {
                                    // Need to execute dependency task before this one
                                    hasCompletedDependencies = false;
                                    break;
                                }
                            }
                        }

                        if (hasAnyDependencyFailed)
                        {
                            // Cannot execute task if one of its dependencies has failed
                            failedTasks.Add(task);
                        }
                        else if (hasCompletedDependencies)
                        {
                            // Pick this task for execution
                            taskToRun = task;
                            break;
                        }
                    }

                    foreach (var task in failedTasks)
                    {
                        _waitingTasks.Remove(task);
                        task.Result = -1;
                        _executedTasks.Add(task);
                    }

                    if (taskToRun != null)
                    {
                        _waitingTasks.Remove(taskToRun);
                    }

                    failedTasks.Clear();
                }

                if (taskToRun != null)
                {
                    // Perform that task
                    taskToRun.StartTime = DateTime.Now;
                    var result = ExecuteTask(taskToRun);
                    taskToRun.EndTime = DateTime.Now;
                    if (result != 0)
                    {
                        Log.Error(string.Format("Task {0} {1} failed with exit code {2}", taskToRun.CommandPath, taskToRun.CommandArguments, result));
                        Log.Error("");
                    }

                    // Cache execution result
                    lock (_locker)
                    {
                        taskToRun.Result = result;
                        _executedTasks.Add(taskToRun);
                        _counter++;
                    }
                }
                else
                {
                    // Wait for other thread to process any dependency
                    Thread.Sleep(10);
                }
            }
        }

        private int ExecuteTask(Task task)
        {
            string name = "Task";
            if (task.ProducedFiles != null && task.ProducedFiles.Count != 0)
                name = Path.GetFileName(task.ProducedFiles[0]);
            var profilerEvent = Profiling.Begin(name);

            var startInfo = new ProcessStartInfo
            {
                WorkingDirectory = task.WorkingDirectory,
                FileName = task.CommandPath,
                Arguments = task.CommandArguments,
                UseShellExecute = false,
                RedirectStandardInput = false,
                RedirectStandardOutput = true,
                RedirectStandardError = true,
                CreateNoWindow = true,
            };

            if (Configuration.Verbose)
            {
                lock (_locker)
                {
                    Log.Verbose("");
                    Log.Verbose(task.CommandPath);
                    Log.Verbose(task.CommandArguments);
                    Log.Verbose("");
                }
            }

            if (task.InfoMessage != null)
            {
                Log.Info(task.InfoMessage);
            }

            Process process = null;
            try
            {
                try
                {
                    process = new Process();
                    process.StartInfo = startInfo;
                    process.OutputDataReceived += ProcessDebugOutput;
                    process.ErrorDataReceived += ProcessDebugOutput;
                    process.Start();

                    process.BeginOutputReadLine();
                    process.BeginErrorReadLine();
                }
                catch (Exception ex)
                {
                    Log.Error("Failed to start local process for task");
                    Log.Exception(ex);
                    return -1;
                }

                // Hang until process end
                process.WaitForExit();

                Profiling.End(profilerEvent);
                return process.ExitCode;
            }
            finally
            {
                Profiling.End(profilerEvent);

                // Ensure to cleanup data
                process?.Close();
            }
        }

        private static void ProcessDebugOutput(object sender, DataReceivedEventArgs e)
        {
            string output = e.Data;
            if (output != null)
            {
                Log.Info(output);
            }
        }
    }
}
