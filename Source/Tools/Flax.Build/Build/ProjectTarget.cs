// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Linq;
using Flax.Build.NativeCpp;

namespace Flax.Build
{
    /// <summary>
    /// The build target that is based on .flaxproj file. Used by engine, game and plugins targets. This target uses standard Flax project structure and is always located in root of the Source directory.
    /// </summary>
    /// <seealso cref="Flax.Build.Target" />
    public abstract class ProjectTarget : Target
    {
        /// <summary>
        /// The loaded project file.
        /// </summary>
        public ProjectInfo Project;

        /// <inheritdoc />
        public override void Init()
        {
            base.Init();

            // Load project
            var projectFiles = Directory.GetFiles(Path.Combine(FolderPath, ".."), "*.flaxproj", SearchOption.TopDirectoryOnly);
            if (projectFiles.Length == 0)
                throw new Exception("Missing project file. Folder: " + FolderPath);
            else if (projectFiles.Length > 1)
                throw new Exception("Too many project files. Don't know which to pick. Folder: " + FolderPath);
            Project = ProjectInfo.Load(projectFiles[0]);

            // Initialize
            ProjectName = OutputName = Project.Name;
        }

        /// <inheritdoc />
        public override Target SelectReferencedTarget(ProjectInfo project, Target[] projectTargets)
        {
            if (IsEditor && !string.IsNullOrEmpty(project.EditorTarget))
            {
                var result = projectTargets.FirstOrDefault(x => x.Name == project.EditorTarget);
                if (result == null)
                    throw new Exception($"Invalid or missing editor target {project.EditorTarget} specified in project {project.Name} (referenced by project {Project.Name}).");
                return result;
            }
            if (!string.IsNullOrEmpty(project.GameTarget))
            {
                var result = projectTargets.FirstOrDefault(x => x.Name == project.GameTarget);
                if (result == null)
                    throw new Exception($"Invalid or missing game target {project.GameTarget} specified in project {project.Name} (referenced by project {Project.Name}).");
                return result;
            }
            return base.SelectReferencedTarget(project, projectTargets);
        }

        /// <inheritdoc />
        public override void SetupTargetEnvironment(BuildOptions options)
        {
            base.SetupTargetEnvironment(options);

            // Add include paths for this project sources and engine third-party sources
            var depsRoot = options.DepsFolder;
            options.CompileEnv.IncludePaths.Add(Path.Combine(Globals.EngineRoot, @"Source\ThirdParty\mono-2.0")); // TODO: let mono module expose it
            options.CompileEnv.IncludePaths.Add(Path.Combine(Globals.EngineRoot, @"Source\ThirdParty"));
            options.CompileEnv.IncludePaths.Add(Path.Combine(Project.ProjectFolderPath, "Source"));
            options.LinkEnv.LibraryPaths.Add(depsRoot);

            // Ensure to propagate no-C# scripting define to the whole codebase
            if (!EngineConfiguration.WithCSharp(options))
            {
                options.CompileEnv.PreprocessorDefinitions.Add("COMPILE_WITHOUT_CSHARP");
            }

            // Add include paths for referenced projects sources
            foreach (var reference in Project.References)
            {
                options.CompileEnv.IncludePaths.Add(Path.Combine(reference.Project.ProjectFolderPath, "Source"));
            }
        }
    }
}
