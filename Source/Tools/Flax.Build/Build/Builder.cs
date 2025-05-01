// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Flax.Build.Graph;
using Flax.Build.NativeCpp;

namespace Flax.Build
{
    /// <summary>
    /// The modules and targets building utility.
    /// </summary>
    public static partial class Builder
    {
        private static void CleanDirectory(DirectoryInfo dir)
        {
            var subdirs = dir.GetDirectories();
            foreach (var subdir in subdirs)
            {
                CleanDirectory(subdir);
            }

            var files = dir.GetFiles();
            foreach (var file in files)
            {
                try
                {
                    file.Delete();
                }
                catch (UnauthorizedAccessException)
                {
                    File.SetAttributes(file.FullName, FileAttributes.Normal);
                    file.Delete();
                }
                catch
                {
                    // Skip errors
                }
            }

            try
            {
                dir.Delete();
            }
            catch
            {
                // Skip errors
            }
        }

        /// <summary>
        /// Cleans the build system cache and intermediate results.
        /// </summary>
        public static void Clean()
        {
            using (new ProfileEventScope("Clean"))
            {
                // Clear task graph cache
                var project = Globals.Project;
                TaskGraph graph;
                if (project == null)
                {
                    graph = new TaskGraph(Globals.Root);
                    graph.CleanCache();
                    return;
                }
                graph = new TaskGraph(project.ProjectFolderPath);
                graph.CleanCache();

                // Pick targets to clean
                var customBuildTargets = Configuration.BuildTargets;
                var projectTargets = GetProjectTargets(project);
                var targets = customBuildTargets == null ? projectTargets : projectTargets.Where(target => customBuildTargets.Contains(target.Name)).ToArray();
                foreach (var target in targets)
                {
                    // Pick configurations to clean
                    TargetConfiguration[] configurations = Configuration.BuildConfigurations;
                    if (configurations != null)
                    {
                        foreach (var configuration in configurations)
                        {
                            if (!target.Configurations.Contains(configuration))
                                throw new Exception(string.Format("Target {0} does not support {1} configuration.", target.Name, configuration));
                        }
                    }
                    else
                    {
                        configurations = target.Configurations;
                    }

                    foreach (var configuration in configurations)
                    {
                        // Pick platforms to clean
                        TargetPlatform[] platforms = Configuration.BuildPlatforms;
                        if (platforms != null)
                        {
                            foreach (var platform in platforms)
                            {
                                if (!target.Platforms.Contains(platform))
                                    throw new Exception(string.Format("Target {0} does not support {1} platform.", target.Name, platform));
                            }
                        }
                        else
                        {
                            platforms = target.Platforms;
                        }

                        foreach (var targetPlatform in platforms)
                        {
                            if (!Platform.BuildPlatform.CanBuildPlatform(targetPlatform))
                                continue;

                            // Pick architectures to build
                            TargetArchitecture[] architectures = Configuration.BuildArchitectures;
                            if (architectures != null)
                            {
                                foreach (var e in architectures)
                                {
                                    if (!target.Architectures.Contains(e))
                                        throw new Exception(string.Format("Target {0} does not support {1} architecture.", target.Name, e));
                                }
                            }
                            else
                            {
                                architectures = target.GetArchitectures(targetPlatform);
                            }

                            foreach (var architecture in architectures)
                            {
                                if (!Platform.IsPlatformSupported(targetPlatform, architecture))
                                    continue;
                                //throw new Exception(string.Format("Platform {0} {1} is not supported.", targetPlatform, architecture));

                                var platform = Platform.GetPlatform(targetPlatform);
                                var toolchain = platform.GetToolchain(architecture);
                                var targetBuildOptions = GetBuildOptions(target, toolchain.Platform, toolchain, toolchain.Architecture, configuration, project.ProjectFolderPath, string.Empty);

                                // Delete all intermediate files
                                var intermediateFolder = new DirectoryInfo(targetBuildOptions.IntermediateFolder);
                                if (intermediateFolder.Exists)
                                {
                                    Log.Info("Removing: " + targetBuildOptions.IntermediateFolder);
                                    CleanDirectory(intermediateFolder);
                                    intermediateFolder.Create();
                                }

                                // Delete all output files
                                var outputFolder = new DirectoryInfo(targetBuildOptions.OutputFolder);
                                if (outputFolder.Exists)
                                {
                                    Log.Info("Removing: " + targetBuildOptions.OutputFolder);
                                    CleanDirectory(outputFolder);
                                    outputFolder.Create();
                                }
                            }
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Gets the list of targets that can be used when building a given project.
        /// </summary>
        /// <param name="project">The project.</param>
        /// <returns>The array of targets that can be build.</returns>
        public static Target[] GetProjectTargets(ProjectInfo project)
        {
            var rules = GenerateRulesAssembly();
            var sourcePath = Path.Combine(project.ProjectFolderPath, "Source");
            return rules.Targets.Where(target => target.FolderPath.StartsWith(sourcePath)).ToArray();
        }

        /// <summary>
        /// Gets the list of modules that are located under a given project (checks for modules located in the given project Source folder).
        /// </summary>
        /// <param name="project">The project.</param>
        /// <returns>The array of modules that are contained in this project.</returns>
        public static Module[] GetProjectModules(ProjectInfo project)
        {
            var rules = GenerateRulesAssembly();
            var sourcePath = Path.Combine(project.ProjectFolderPath, "Source");
            return rules.Modules.Where(module => module.FolderPath.StartsWith(sourcePath)).ToArray();
        }

        /// <summary>
        /// Gets the project that contains a given module (checks for modules located in the given project Source folder).
        /// </summary>
        /// <param name="module">The module.</param>
        /// <param name="project">The project.</param>
        /// <returns>The found project.</returns>
        public static ProjectInfo GetModuleProject(Module module, ProjectInfo project)
        {
            if (project == null)
                return null;
            var sourcePath = Path.Combine(project.ProjectFolderPath, "Source");
            if (module.FolderPath.StartsWith(sourcePath))
                return project;
            foreach (var reference in project.References)
            {
                var result = GetModuleProject(module, reference.Project);
                if (result != null)
                    return result;
            }
            return null;
        }

        /// <summary>
        /// Gets the project that contains a given module (checks for modules located in the given project Source folder).
        /// </summary>
        /// <param name="module">The module.</param>
        /// <param name="buildData">The build data.</param>
        /// <returns>The found project.</returns>
        public static ProjectInfo GetModuleProject(Module module, BuildData buildData)
        {
            return GetModuleProject(module, buildData.Project);
        }

        /// <summary>
        /// Checks if the project that contains a given module (checks for modules located in the given project Source folder).
        /// </summary>
        /// <param name="module">The module.</param>
        /// <param name="project">The project to check.</param>
        /// <returns>True if project contains that module inside, otherwise it's external or referenced.</returns>
        public static bool IsModuleFromProject(Module module, ProjectInfo project)
        {
            return GetModuleProject(module, project) == project;
        }

        /// <summary>
        /// Builds the targets.
        /// </summary>
        /// <returns>True if failed, otherwise false.</returns>
        public static bool BuildTargets()
        {
            bool failed = false;

            var rules = GenerateRulesAssembly();
            var project = Globals.Project;
            if (project == null)
            {
                Log.Warning("Missing project");
                return true;
            }

            using (new ProfileEventScope("BuildTargets"))
            {
                // Pick targets to build
                var customBuildTargets = Configuration.BuildTargets;
                var projectTargets = GetProjectTargets(project);
                var targets = customBuildTargets == null ? projectTargets : projectTargets.Where(target => customBuildTargets.Contains(target.Name)).ToArray();
                if (customBuildTargets != null)
                {
                    var targetsList = new List<Target>(customBuildTargets.Length);
                    foreach (var customBuildTarget in customBuildTargets)
                    {
                        var target = projectTargets.FirstOrDefault(x => string.Equals(x.Name, customBuildTarget, StringComparison.InvariantCultureIgnoreCase));
                        if (target != null)
                            targetsList.Add(target);
                        else
                            Log.Error("Missing target " + customBuildTarget);
                    }
                    targets = targetsList.ToArray();
                }
                if (targets.Length == 0)
                    Log.Warning("No targets to build");

                using (new ProfileEventScope("LoadIncludesCache"))
                {
                    IncludesCache.LoadCache();
                }

                // Create task graph for building all targets
                var graph = new TaskGraph(project.ProjectFolderPath);
                foreach (var target in targets)
                {
                    target.PreBuild();

                    // Pick configurations to build
                    TargetConfiguration[] configurations = Configuration.BuildConfigurations;
                    if (configurations != null)
                    {
                        foreach (var configuration in configurations)
                        {
                            if (!target.Configurations.Contains(configuration))
                                throw new Exception(string.Format("Target {0} does not support {1} configuration.", target.Name, configuration));
                        }
                    }
                    else
                    {
                        configurations = target.Configurations;
                    }

                    foreach (var configuration in configurations)
                    {
                        // Pick platforms to build
                        TargetPlatform[] platforms = Configuration.BuildPlatforms;
                        if (platforms != null)
                        {
                            foreach (var platform in platforms)
                            {
                                if (!target.Platforms.Contains(platform))
                                    throw new Exception(string.Format("Target {0} does not support {1} platform.", target.Name, platform));
                            }
                        }
                        else
                        {
                            platforms = target.Platforms;
                        }

                        foreach (var targetPlatform in platforms)
                        {
                            // Pick architectures to build
                            TargetArchitecture[] architectures = Configuration.BuildArchitectures;
                            if (architectures != null)
                            {
                                foreach (var e in architectures)
                                {
                                    if (!target.Architectures.Contains(e))
                                        throw new Exception(string.Format("Target {0} does not support {1} architecture.", target.Name, e));
                                }
                            }
                            else
                            {
                                architectures = target.GetArchitectures(targetPlatform);
                            }

                            foreach (var architecture in architectures)
                            {
                                if (!Platform.IsPlatformSupported(targetPlatform, architecture))
                                    continue;
                                //throw new Exception(string.Format("Platform {0} {1} is not supported.", targetPlatform, architecture));

                                var platform = Platform.GetPlatform(targetPlatform);

                                // Special case: building C# bindings only (eg. when building Linux game on Windows without C++ scripting or for C#-only projects)
                                if (Configuration.BuildBindingsOnly || (project.IsCSharpOnlyProject && platform.HasModularBuildSupport))
                                {
                                    Log.Info("Building C# only");
                                    using (new ProfileEventScope(target.Name))
                                    {
                                        Log.Info(string.Format("Building target {0} in {1} for {2} {3}", target.Name, configuration, targetPlatform, architecture));

                                        var buildContext = new Dictionary<Target, BuildData>();
                                        switch (target.Type)
                                        {
                                        case TargetType.NativeCpp:
                                            BuildTargetNativeCppBindingsOnly(rules, graph, target, buildContext, platform, architecture, configuration);
                                            break;
                                        case TargetType.DotNet:
                                        case TargetType.DotNetCore:
                                            BuildTargetDotNet(rules, graph, target, platform, configuration);
                                            break;
                                        default: throw new ArgumentOutOfRangeException();
                                        }
                                    }
                                    continue;
                                }

                                var toolchain = platform.GetToolchain(architecture);
                                using (new ProfileEventScope(target.Name))
                                {
                                    Log.Info(string.Format("Building target {0} in {1} for {2} {3}", target.Name, configuration, targetPlatform, architecture));

                                    var buildContext = new Dictionary<Target, BuildData>();
                                    switch (target.Type)
                                    {
                                    case TargetType.NativeCpp:
                                        BuildTargetNativeCpp(rules, graph, target, buildContext, toolchain, configuration);
                                        break;
                                    case TargetType.DotNet:
                                    case TargetType.DotNetCore:
                                        BuildTargetDotNet(rules, graph, target, toolchain.Platform, configuration);
                                        break;
                                    default: throw new ArgumentOutOfRangeException();
                                    }
                                }
                            }
                        }
                    }
                }

                // Prepare tasks for the execution
                using (new ProfileEventScope("PrepareTasks"))
                {
                    using (new ProfileEventScope("Setup"))
                        graph.Setup();

                    using (new ProfileEventScope("SortTasks"))
                        graph.SortTasks();

                    using (new ProfileEventScope("LoadCache"))
                        graph.LoadCache();
                }

                // Execute tasks
                int executedTasksCount;
                using (new ProfileEventScope("ExecuteTasks"))
                {
                    failed |= graph.Execute(out executedTasksCount);
                }

                if (executedTasksCount != 0)
                {
                    // Save graph execution result cache
                    using (new ProfileEventScope("SaveCache"))
                    {
                        graph.SaveCache();
                    }
                }

                using (new ProfileEventScope("SaveIncludesCache"))
                {
                    IncludesCache.SaveCache();
                }

                foreach (var target in targets)
                {
                    target.PostBuild();
                }
            }

            return failed;
        }
    }
}
