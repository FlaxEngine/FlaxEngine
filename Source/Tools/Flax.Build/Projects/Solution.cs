// Copyright (c) Wojciech Figat. All rights reserved.

namespace Flax.Build.Projects
{
    /// <summary>
    /// The solution file data for generator.
    /// </summary>
    public class Solution
    {
        /// <summary>
        /// The solution name.
        /// </summary>
        public string Name;

        /// <summary>
        /// The solution file path.
        /// </summary>
        public string Path;

        /// <summary>
        /// The workspace root directory path.
        /// </summary>
        public string WorkspaceRootPath;

        /// <summary>
        /// The projects.
        /// </summary>
        public Project[] Projects;

        /// <summary>
        /// The main project to use as solution default (eg. for build and for startup).
        /// </summary>
        public Project MainProject;
    }
}
