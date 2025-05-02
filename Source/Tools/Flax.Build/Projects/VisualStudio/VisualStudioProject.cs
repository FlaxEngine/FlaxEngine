// Copyright (c) Wojciech Figat. All rights reserved.

using System;

namespace Flax.Build.Projects.VisualStudio
{
    /// <summary>
    /// The generic implementation of the Visual Studio project.
    /// </summary>
    /// <seealso cref="Flax.Build.Projects.Project" />
    public class VisualStudioProject : Project
    {
        /// <summary>
        /// The project unique identifier (from the project file or the generated).
        /// </summary>
        public Guid ProjectGuid;

        /// <summary>
        /// The project parent folder identifier (used in the solution hierarchy).
        /// </summary>
        public Guid FolderGuid;

        /// <summary>
        /// Gets the project type unique identifier. Used by the Visual Studio solution file to identify the project type.
        /// </summary>
        public virtual Guid ProjectTypeGuid
        {
            get
            {
                switch (Type ?? Targets[0].Type)
                {
                case TargetType.NativeCpp: return VisualStudioProjectGenerator.ProjectTypeGuids.WindowsVisualCpp;
                case TargetType.DotNet: return VisualStudioProjectGenerator.ProjectTypeGuids.WindowsCSharp;
                case TargetType.DotNetCore: return VisualStudioProjectGenerator.ProjectTypeGuids.WindowsCSharp;
                default: throw new ArgumentOutOfRangeException();
                }
            }
        }
    }
}
