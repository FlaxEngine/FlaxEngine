// Copyright (c) 2012-2024 Flax Engine. All rights reserved.

using System;
using System.Collections.Generic;
using Flax.Build.Projects.VisualStudio;
using Flax.Build.Projects.VisualStudioCode;

namespace Flax.Build.Projects
{
    /// <summary>
    /// The project files generators base class.
    /// </summary>
    public abstract class ProjectGenerator
    {
        /// <summary>
        /// Gets the project file extension (excluding the leading dot).
        /// </summary>
        public abstract string ProjectFileExtension { get; }

        /// <summary>
        /// Gets the solution file extension (excluding the leading dot).
        /// </summary>
        public abstract string SolutionFileExtension { get; }

        /// <summary>
        /// Gets the generator projects target type.
        /// </summary>
        public abstract TargetType? Type { get; }

        /// <summary>
        /// Creates the empty project (factory design pattern).
        /// </summary>
        /// <returns>The empty project.</returns>
        public virtual Project CreateProject()
        {
            return new Project
            {
                Generator = this,
            };
        }

        /// <summary>
        /// Creates the empty solution (factory design pattern).
        /// </summary>
        /// <returns>The empty solution.</returns>
        public virtual Solution CreateSolution()
        {
            return new Solution();
        }

        /// <summary>
        /// Generates the project.
        /// </summary>
        /// <param name="project">The project.</param>
        public abstract void GenerateProject(Project project, string solutionPath, bool isMainProject);

        /// <summary>
        /// Generates the solution.
        /// </summary>
        /// <param name="solution">The solution.</param>
        public abstract void GenerateSolution(Solution solution);

        /// <summary>
        /// Generates the custom projects for the solution.
        /// </summary>
        /// <param name="projects">The projects.</param>
        public virtual void GenerateCustomProjects(List<Project> projects)
        {
        }

        /// <summary>
        /// The custom project types factories. Key is the custom project format name and the value is the <see cref="ProjectGenerator"/> factory function used to spawn it.
        /// </summary>
        public static readonly Dictionary<string, Func<TargetType, ProjectGenerator>> CustomProjectTypes = new Dictionary<string, Func<TargetType, ProjectGenerator>>();

        /// <summary>
        /// Creates the project files generator for the specified project format.
        /// </summary>
        /// <param name="format">The format.</param>
        /// <param name="type">The target project type.</param>
        /// <returns>The generator.</returns>
        public static ProjectGenerator Create(ProjectFormat format, TargetType type)
        {
            // Pick the newest installed Visual Studio version
            if (format == ProjectFormat.VisualStudio)
            {
                if (VisualStudioInstance.HasIDE(VisualStudioVersion.VisualStudio2022))
                {
                    format = ProjectFormat.VisualStudio2022;
                }
                else if (VisualStudioInstance.HasIDE(VisualStudioVersion.VisualStudio2019))
                {
                    format = ProjectFormat.VisualStudio2019;
                }
                else if (VisualStudioInstance.HasIDE(VisualStudioVersion.VisualStudio2017))
                {
                    format = ProjectFormat.VisualStudio2017;
                }
                else if (VisualStudioInstance.HasIDE(VisualStudioVersion.VisualStudio2015))
                {
                    format = ProjectFormat.VisualStudio2015;
                }
                else
                {
                    Log.Warning("Failed to find default Visual Studio installation");
                    format = ProjectFormat.VisualStudio2015;
                }
            }

            switch (format)
            {
            case ProjectFormat.VisualStudio2015:
            case ProjectFormat.VisualStudio2017:
            case ProjectFormat.VisualStudio2019:
            case ProjectFormat.VisualStudio2022:
            {
                VisualStudioVersion vsVersion;
                switch (format)
                {
                case ProjectFormat.VisualStudio2015:
                    vsVersion = VisualStudioVersion.VisualStudio2015;
                    break;
                case ProjectFormat.VisualStudio2017:
                    vsVersion = VisualStudioVersion.VisualStudio2017;
                    break;
                case ProjectFormat.VisualStudio2019:
                    vsVersion = VisualStudioVersion.VisualStudio2019;
                    break;
                case ProjectFormat.VisualStudio2022:
                    vsVersion = VisualStudioVersion.VisualStudio2022;
                    break;
                default: throw new ArgumentOutOfRangeException(nameof(format), format, null);
                }
                switch (type)
                {
                case TargetType.NativeCpp: return new VCProjectGenerator(vsVersion);
                case TargetType.DotNet: return new CSProjectGenerator(vsVersion);
                case TargetType.DotNetCore: return new CSSDKProjectGenerator(vsVersion);
                default: throw new ArgumentOutOfRangeException(nameof(type), type, null);
                }
            }
            case ProjectFormat.VisualStudioCode: return type == TargetType.DotNet 
                ? (ProjectGenerator)new CSProjectGenerator(VisualStudioVersion.VisualStudio2015)
                : (ProjectGenerator)new VisualStudioCodeProjectGenerator();
            case ProjectFormat.XCode: return new XCodeProjectGenerator();
            case ProjectFormat.Custom:
                if (CustomProjectTypes.TryGetValue(Configuration.ProjectFormatCustom, out var factory))
                    return factory(type);
                throw new Exception($"Unknown custom project format type '{Configuration.ProjectFormatCustom}'");
            default: throw new ArgumentOutOfRangeException(nameof(format), "Unknown project format.");
            }
        }
    }
}
