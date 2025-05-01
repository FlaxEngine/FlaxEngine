// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;

namespace Flax.Build.Graph
{
    /// <summary>
    /// The base class for build system Task Graph.
    /// </summary>
    [Serializable]
    public class Task
    {
        /// <summary>
        /// The collection of the files required by this task to run.
        /// </summary>
        public List<string> PrerequisiteFiles = new List<string>();

        /// <summary>
        /// The collection of the files produced by this task.
        /// </summary>
        public List<string> ProducedFiles = new List<string>();

        /// <summary>
        /// The working directory for any process invoked by this task.
        /// </summary>
        public string WorkingDirectory = string.Empty;

        /// <summary>
        /// The command to call upon task execution.
        /// </summary>
        public Action Command;

        /// <summary>
        /// The command to run to create produced files.
        /// </summary>
        public string CommandPath = string.Empty;

        /// <summary>
        /// Command-line parameters to pass via command line to the invoked process.
        /// </summary>
        public string CommandArguments = string.Empty;

        /// <summary>
        /// The message to print on task execution start.
        /// </summary>
        public string InfoMessage;

        /// <summary>
        /// The estimated action cost. Unitless but might be estimation of milliseconds required to perform this task. It's just an raw estimation (based on input files count or size).
        /// </summary>
        public int Cost = 0;

        /// <summary>
        /// The start time of the task execution.
        /// </summary>
        public DateTime StartTime = DateTime.MinValue;

        /// <summary>
        /// The end time of the task execution.
        /// </summary>
        public DateTime EndTime = DateTime.MinValue;

        /// <summary>
        /// Gets the task execution duration.
        /// </summary>
        public TimeSpan Duration => EndTime != DateTime.MinValue ? EndTime - StartTime : DateTime.Now - StartTime;

        /// <summary>
        /// The dependent tasks cached for this task (limited depth). Might be null.
        /// </summary>
        public HashSet<Task> DependentTasks;

        /// <summary>
        /// Gets the dependent tasks count.
        /// </summary>
        public int DependentTasksCount => DependentTasks?.Count ?? 0;

        /// <summary>
        /// Gets a value indicating whether task results from the previous execution are still valid. Can be used to skip task execution.
        /// </summary>
        public bool HasValidCachedResults;

        /// <summary>
        /// Disables caching this task results.
        /// </summary>
        public bool DisableCache;

        /// <summary>
        /// The task execution result.
        /// </summary>
        public int Result;

        /// <summary>
        /// Gets a value indicating whether task execution has failed.
        /// </summary>
        public bool Failed => Result != 0;

        /// <summary>
        /// Compares two tasks to sort them for the parallel execution.
        /// </summary>
        /// <param name="a">The first task to compare.</param>
        /// <param name="b">The second task to compare.</param>
        public static int Compare(Task a, Task b)
        {
            // Sort by total number of dependent files
            var dependentTasksCountDiff = b.DependentTasksCount - a.DependentTasksCount;
            if (dependentTasksCountDiff != 0)
            {
                return Math.Sign(dependentTasksCountDiff);
            }

            // Sort by the cost
            var costDiff = b.Cost - a.Cost;
            if (costDiff != 0)
            {
                return Math.Sign(costDiff);
            }

            // Sort by amount of input files
            return Math.Sign(b.PrerequisiteFiles.Count - a.PrerequisiteFiles.Count);
        }

        /// <inheritdoc />
        public override string ToString()
        {
            return InfoMessage ?? base.ToString();
        }
    }
}
