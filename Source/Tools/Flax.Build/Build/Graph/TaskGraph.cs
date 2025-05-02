// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Flax.Build.BuildSystem.Graph;

namespace Flax.Build.Graph
{
    /// <summary>
    /// The task execution and processing utility.
    /// </summary>
    public class TaskGraph
    {
        private struct BuildResultCache
        {
            public string CmdLine;
            public int[] FileIndices;
        }

        private readonly List<BuildResultCache> _prevBuildCache = new List<BuildResultCache>();
        private readonly List<string> _prevBuildCacheFiles = new List<string>();
        private readonly Dictionary<string, int> _prevBuildCacheFileIndices = new Dictionary<string, int>();

        /// <summary>
        /// The workspace folder of the task graph.
        /// </summary>
        public readonly string Workspace;

        /// <summary>
        /// Gets the Task Graph cache file path.
        /// </summary>
        public readonly string CachePath;

        /// <summary>
        /// The tasks existing in this graph.
        /// </summary>
        public readonly List<Task> Tasks = new List<Task>();

        /// <summary>
        /// The hash table that maps file path to the task that produces it.
        /// </summary>
        /// <remarks>Cached by <see cref="Setup"/> method.</remarks>
        public readonly Dictionary<string, Task> FileToProducingTaskMap = new Dictionary<string, Task>();

        /// <summary>
        /// Initializes a new instance of the <see cref="TaskGraph"/> class.
        /// </summary>
        /// <param name="workspace">The workspace root folder. Used to locate the build system cache location for the graph.</param>
        public TaskGraph(string workspace)
        {
            Workspace = workspace;
            CachePath = Path.Combine(workspace, Configuration.IntermediateFolder, "TaskGraph.cache");
        }

        /// <summary>
        /// Adds a new task to the graph.
        /// </summary>
        /// <typeparam name="T">The type of the task.</typeparam>
        /// <returns>The created task.</returns>
        public T Add<T>() where T : Task, new()
        {
            var task = new T();
            Tasks.Add(task);
            return task;
        }

        /// <summary>
        /// Adds a new task to the graph that copies a file.
        /// </summary>
        /// <param name="dstFile">The destination file path.</param>
        /// <param name="srcFile">The source file path.</param>
        /// <returns>The created task.</returns>
        public Task AddCopyFile(string dstFile, string srcFile)
        {
            var task = new Task();
            task.PrerequisiteFiles.Add(srcFile);
            task.ProducedFiles.Add(dstFile);
            task.Cost = 1;
            task.WorkingDirectory = Workspace;
            var outputPath = Path.GetDirectoryName(dstFile);
            if (Platform.BuildPlatform.Target == TargetPlatform.Windows)
            {
                task.CommandPath = "xcopy";
                task.CommandArguments = string.Format("/y \"{0}\" \"{1}\"", srcFile.Replace('/', '\\'), outputPath.Replace('/', '\\'));
            }
            else
            {
                task.CommandPath = "cp";
                task.CommandArguments = string.Format("\"{0}\" \"{1}\"", srcFile, outputPath);
            }

            Tasks.Add(task);
            return task;
        }

        /// <summary>
        /// Checks if that copy task is already added in a graph. Use it for logic that might copy the same file multiple times.
        /// </summary>
        /// <param name="dstFile">The destination file path.</param>
        /// <param name="srcFile">The source file path.</param>
        /// <returns>True if has copy task already scheduled in a task graph, otherwise false..</returns>
        public bool HasCopyTask(string dstFile, string srcFile)
        {
            for (int i = Tasks.Count - 1; i >= 0; i--)
            {
                var t = Tasks[i];
                if (t.Cost == 1 &&
                    t.PrerequisiteFiles.Count == 1 && t.PrerequisiteFiles[0] == srcFile &&
                    t.ProducedFiles.Count == 1 && t.ProducedFiles[0] == dstFile)
                {
                    // Already scheduled for copy
                    return true;
                }
            }
            return false;
        }

        /// <summary>
        /// Builds the cache for the task graph. Cached the map for files to provide O(1) lookup for producing task.
        /// </summary>
        public void Setup()
        {
            FileToProducingTaskMap.Clear();
            for (int i = 0; i < Tasks.Count; i++)
            {
                var task = Tasks[i];
                for (int j = 0; j < task.ProducedFiles.Count; j++)
                {
                    FileToProducingTaskMap[task.ProducedFiles[j]] = task;
                }
            }
        }

        /// <summary>
        /// Performs tasks list sorting based on task dependencies and cost heuristics to to improve parallelism of the graph execution.
        /// </summary>
        public void SortTasks()
        {
            // Create a mapping from task to a list of tasks that directly or indirectly depend on it (dependencies collection)
            const int maximumSearchDepth = 6;
            var taskToDependentActionsMap = new Dictionary<Task, HashSet<Task>>();
            for (int depth = 0; depth < maximumSearchDepth; depth++)
            {
                foreach (var task in Tasks)
                {
                    foreach (var prerequisiteFile in task.PrerequisiteFiles)
                    {
                        if (FileToProducingTaskMap.TryGetValue(prerequisiteFile, out var prerequisiteTask))
                        {
                            HashSet<Task> dependentTasks;
                            if (taskToDependentActionsMap.ContainsKey(prerequisiteTask))
                            {
                                dependentTasks = taskToDependentActionsMap[prerequisiteTask];
                            }
                            else
                            {
                                dependentTasks = new HashSet<Task>();
                                taskToDependentActionsMap[prerequisiteTask] = dependentTasks;
                            }

                            // Build dependencies list
                            dependentTasks.Add(task);
                            if (taskToDependentActionsMap.ContainsKey(task))
                            {
                                dependentTasks.UnionWith(taskToDependentActionsMap[task]);
                            }
                        }
                    }
                }
            }

            // Assign the dependencies
            foreach (var e in taskToDependentActionsMap)
            {
                foreach (var c in e.Value)
                {
                    if (c.DependentTasks == null)
                        c.DependentTasks = new HashSet<Task>();
                    c.DependentTasks.Add(e.Key);

                    // Remove any reference to itself (prevent deadlocks)
                    c.DependentTasks.Remove(c);
                }
            }

            // Perform the actual sorting
            Tasks.Sort(Task.Compare);
        }

        /// <summary>
        /// Executes this task graph.
        /// </summary>
        /// <param name="executedTasksCount">The total count of the executed tasks (excluding the cached ones).</param>
        /// <returns>True if failed, otherwise false.</returns>
        public bool Execute(out int executedTasksCount)
        {
            Log.Verbose("");
            Log.Verbose(string.Format("Total {0} tasks", Tasks.Count));

            var executor = new LocalExecutor();
            executedTasksCount = executor.Execute(Tasks);

            var failedCount = 0;
            foreach (var task in Tasks)
            {
                if (task.Failed)
                    failedCount++;
            }

            if (failedCount == 1)
                Log.Error("1 task failed");
            else if (failedCount != 0)
                Log.Error(string.Format("{0} tasks failed", failedCount));
            else
                Log.Info("Done!");

            return failedCount != 0;
        }

        private void LoadCache1(BinaryReader reader, Dictionary<string, Task> tasksCache, ref int validCount, ref int invalidCount)
        {
            var fileIndices = new List<int>();

            int taskCount = reader.ReadInt32();
            for (int i = 0; i < taskCount; i++)
            {
                var cmdLine = reader.ReadString();
                var filesCount = reader.ReadInt32();

                bool allValid = true;
                fileIndices.Clear();
                for (int j = 0; j < filesCount; j++)
                {
                    var file = reader.ReadString();
                    var lastWrite = new DateTime(reader.ReadInt64());

                    int fileIndex = _prevBuildCacheFiles.IndexOf(file);
                    if (fileIndex == -1)
                    {
                        fileIndex = _prevBuildCacheFiles.Count;
                        _prevBuildCacheFiles.Add(file);
                    }

                    fileIndices.Add(fileIndex);

                    if (!allValid)
                        continue;

                    if (File.Exists(file))
                    {
                        if (File.GetLastWriteTime(file) > lastWrite)
                        {
                            allValid = false;
                        }
                    }
                    else if (lastWrite != DateTime.MinValue)
                    {
                        allValid = false;
                    }
                }

                if (allValid)
                {
                    validCount++;

                    if (tasksCache.TryGetValue(cmdLine, out var task))
                    {
                        task.HasValidCachedResults = true;
                    }
                    else
                    {
                        _prevBuildCache.Add(new BuildResultCache
                        {
                            CmdLine = cmdLine,
                            FileIndices = fileIndices.ToArray(),
                        });
                    }
                }
                else
                {
                    invalidCount++;
                }
            }
        }

        private void LoadCache2(BinaryReader reader, Dictionary<string, Task> tasksCache, ref int validCount, ref int invalidCount)
        {
            var fileIndices = new List<int>();
            int pathsCount = reader.ReadInt32();
            var filesDates = new DateTime[pathsCount];
            var filesValid = new bool[pathsCount];
            _prevBuildCacheFiles.Capacity = pathsCount;
            for (int i = 0; i < pathsCount; i++)
            {
                var file = reader.ReadString();
                var lastWrite = new DateTime(reader.ReadInt64());

                var isValid = true;
                var cacheFile = true;
                if (FileCache.Exists(file))
                {
                    if (FileCache.GetLastWriteTime(file) > lastWrite)
                    {
                        isValid = false;
                    }
                }
                else if (lastWrite != DateTime.MinValue)
                {
                    isValid = false;
                }
                else
                    cacheFile = false;

                filesDates[i] = lastWrite;
                filesValid[i] = isValid;
                _prevBuildCacheFiles.Add(file);
                _prevBuildCacheFileIndices.Add(file, i);

                if (!isValid || !cacheFile)
                    FileCache.FileRemoveFromCache(file);
            }

            int taskCount = reader.ReadInt32();
            for (int i = 0; i < taskCount; i++)
            {
                var cmdLine = reader.ReadString();
                var filesCount = reader.ReadInt32();

                bool allValid = true;
                fileIndices.Clear();
                for (int j = 0; j < filesCount; j++)
                {
                    var fileIndex = reader.ReadInt32();
                    allValid &= filesValid[fileIndex];
                    fileIndices.Add(fileIndex);
                }

                if (allValid)
                {
                    validCount++;

                    if (tasksCache.TryGetValue(cmdLine, out var task))
                    {
                        task.HasValidCachedResults = true;
                    }
                    else
                    {
                        _prevBuildCache.Add(new BuildResultCache
                        {
                            CmdLine = cmdLine,
                            FileIndices = fileIndices.ToArray(),
                        });
                    }
                }
                else
                {
                    invalidCount++;
                }
            }
        }

        /// <summary>
        /// Loads the graph execution cache and marks tasks that can be skipped (have still valid results and unmodified prerequisite files).
        /// </summary>
        public void LoadCache()
        {
            var path = CachePath;
            if (!File.Exists(path))
            {
                Log.Verbose("Missing Task Graph cache file");
                return;
            }

            try
            {
                // Builds tasks cache
                var tasksCache = new Dictionary<string, Task>();
                foreach (var task in Tasks)
                {
                    var cmdLine = task.WorkingDirectory + task.CommandPath + task.CommandArguments;
                    tasksCache[cmdLine] = task;
                }

                var validCount = 0;
                var invalidCount = 0;
                using (var stream = new FileStream(CachePath, FileMode.Open))
                using (var reader = new BinaryReader(stream))
                {
                    int version = reader.ReadInt32();
                    switch (version)
                    {
                    case 1:
                        LoadCache1(reader, tasksCache, ref validCount, ref invalidCount);
                        break;
                    case 2:
                        LoadCache2(reader, tasksCache, ref validCount, ref invalidCount);
                        break;
                    default:
                        Log.Error("Unsupported cache version " + version);
                        return;
                    }
                }

                foreach (var task in Tasks)
                {
                    if (task.HasValidCachedResults && task.DependentTasksCount > 0)
                    {
                        // Allow task to use cached results only if all dependency tasks are also cached
                        if (ValidateCachedResults(task))
                        {
                            task.HasValidCachedResults = false;
                        }
                    }
                }

                Log.Info(string.Format("Found {0} valid and {1} invalid actions in Task Graph cache", validCount, invalidCount));
            }
            catch (Exception ex)
            {
                Log.Exception(ex);
                Log.Error("Failed to load task graph cache.");
                Log.Error(path);
                CleanCache();
            }
        }

        private static bool ValidateCachedResults(Task task)
        {
            foreach (var dependentTasks in task.DependentTasks)
            {
                if (!dependentTasks.HasValidCachedResults)
                {
                    return true;
                }

                if (dependentTasks.DependentTasks != null && ValidateCachedResults(dependentTasks))
                {
                    return true;
                }
            }

            return false;
        }

        /// <summary>
        /// Cleans the graph execution cache.
        /// </summary>
        public void CleanCache()
        {
            var path = CachePath;
            if (File.Exists(path))
            {
                Log.Info("Removing: " + path);
                File.Delete(path);
            }
        }

        /// <summary>
        /// Saves the graph execution cache.
        /// </summary>
        public void SaveCache()
        {
            var doneTasks = Tasks.Where(task => task.Result == 0 && !task.DisableCache).ToArray();
            var files = new List<string>();
            var fileIndices = new List<int>();
            foreach (var task in doneTasks)
            {
                var cmdLine = task.WorkingDirectory + task.CommandPath + task.CommandArguments;

                files.Clear();
                files.AddRange(task.ProducedFiles);
                files.AddRange(task.PrerequisiteFiles);

                fileIndices.Clear();
                foreach (var file in files)
                {
                    if (!_prevBuildCacheFileIndices.TryGetValue(file, out int fileIndex))
                    {
                        fileIndex = _prevBuildCacheFiles.Count;
                        _prevBuildCacheFiles.Add(file);
                        _prevBuildCacheFileIndices.Add(file, fileIndex);
                    }

                    fileIndices.Add(fileIndex);
                }

                if (!task.HasValidCachedResults)
                {
                    foreach (var file in task.ProducedFiles)
                        FileCache.FileRemoveFromCache(file);
                }

                _prevBuildCache.Add(new BuildResultCache
                {
                    CmdLine = cmdLine,
                    FileIndices = fileIndices.ToArray(),
                });
            }

            var path = CachePath;
            using (var stream = new FileStream(path, FileMode.Create))
            using (var writer = new BinaryWriter(stream))
            {
                // Version
                writer.Write(2);

                // Paths
                writer.Write(_prevBuildCacheFiles.Count);
                foreach (var file in _prevBuildCacheFiles)
                {
                    // Path
                    writer.Write(file);

                    // Last File Write
                    DateTime lastWrite;
                    if (FileCache.Exists(file))
                        lastWrite = FileCache.GetLastWriteTime(file);
                    else
                        lastWrite = DateTime.MinValue;
                    writer.Write(lastWrite.Ticks);
                }

                // Tasks
                writer.Write(_prevBuildCache.Count);
                foreach (var cache in _prevBuildCache)
                {
                    // Working Dir + Command + Arguments
                    writer.Write(cache.CmdLine);

                    // Files
                    writer.Write(cache.FileIndices.Length);
                    for (var i = 0; i < cache.FileIndices.Length; i++)
                    {
                        writer.Write(cache.FileIndices[i]);
                    }
                }
            }
        }
    }
}
