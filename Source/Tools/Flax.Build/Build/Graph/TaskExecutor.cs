// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;

namespace Flax.Build.Graph
{
    /// <summary>
    /// The base class for Task Graph executors that can perform the actual work.
    /// </summary>
    public abstract class TaskExecutor
    {
        /// <summary>
        /// Executes the specified tasks collection using custom execution rules.
        /// </summary>
        /// <param name="tasks">The tasks.</param>
        /// <returns>The total count of the executed tasks (excluding the cached ones).</returns>
        public abstract int Execute(List<Task> tasks);
    }
}
