// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
            options.CompileEnv.IncludePaths.Add(Path.Combine(Globals.EngineRoot, @"Source\ThirdParty"));
            options.LinkEnv.LibraryPaths.Add(depsRoot);

            // Ensure to propagate global configuration defines to the whole codebase
            if (!EngineConfiguration.WithCSharp(options))
            {
                options.CompileEnv.PreprocessorDefinitions.Add("COMPILE_WITHOUT_CSHARP");
            }
            else if (!EngineConfiguration.WithDotNet(options))
            {
                options.CompileEnv.PreprocessorDefinitions.Add("COMPILE_WITH_MONO");
            }
            if (EngineConfiguration.WithLargeWorlds(options))
            {
                options.CompileEnv.PreprocessorDefinitions.Add("USE_LARGE_WORLDS");
                options.ScriptingAPI.Defines.Add("USE_LARGE_WORLDS");
            }
            if (EngineConfiguration.WithSDL(options))
            {
                options.CompileEnv.PreprocessorDefinitions.Add("PLATFORM_SDL");
                options.ScriptingAPI.Defines.Add("PLATFORM_SDL");
            }

            // Add include paths for this and all referenced projects sources
            foreach (var project in Project.GetAllProjects())
            {
                options.CompileEnv.IncludePaths.Add(Path.Combine(project.ProjectFolderPath, "Source"));
            }
        }
    }
}
