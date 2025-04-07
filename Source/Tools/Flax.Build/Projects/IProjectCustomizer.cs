// Copyright (c) Wojciech Figat. All rights reserved.

namespace Flax.Build.Projects
{
    /// <summary>
    /// Interface for objects that can customize generated project files (eg. insert debugger customization or custom extension settings).
    /// </summary>
    public interface IProjectCustomizer
    {
        /// <summary>
        /// Gets the name of the project architecture for the solution.
        /// </summary>
        /// <param name="architecture">The platform architecture.</param>
        /// <param name="name">The result name.</param>
        void GetSolutionArchitectureName(TargetArchitecture architecture, ref string name);

        /// <summary>
        /// Gets the name of the project architecture.
        /// </summary>
        /// <param name="project">The project.</param>
        /// <param name="platform">The platform.</param>
        /// <param name="architecture">The platform architecture.</param>
        /// <param name="name">The result name.</param>
        void GetProjectArchitectureName(Project project, Platform platform, TargetArchitecture architecture, ref string name);
    }
}
