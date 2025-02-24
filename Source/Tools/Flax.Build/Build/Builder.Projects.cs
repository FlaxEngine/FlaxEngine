// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using Flax.Build.NativeCpp;
using Flax.Build.Projects;

namespace Flax.Build
{
    partial class Builder
    {
        private class ProjectTargetsGroup
        {
            public string ProjectName;
            public ProjectInfo Project;
            public TargetType Type;
            public List<Target> Targets;
            public HashSet<ProjectInfo> ProjectDependencies;

            public override string ToString()
            {
                if (Project != null)
                    return $"{ProjectName}: {Project.ProjectPath}";
                return ProjectName;
            }
        }

        private static void SetupProjectConfigurations(Project project, ProjectInfo projectInfo)
        {
            project.Configurations.Clear();

            // Generate a set of configurations to build all targets for all platforms/configurations/architectures
            var rules = GenerateRulesAssembly();
            foreach (var target in project.Targets)
            {
                string targetName = target.ConfigurationName ?? target.Name;

                // When using custom project file try to extract configurations from there
                if (!string.IsNullOrEmpty(target.CustomExternalProjectFilePath))
                {
                    // TODO: implement it in a proper way by parsing the project file to extract the configurations
                    if (target.CustomExternalProjectFilePath.EndsWith(".csproj"))
                    {
                        var targetPlatform = TargetPlatform.Windows;
                        var platformName = targetPlatform.ToString();
                        var platform = Platform.BuildPlatform;
                        var architecture = TargetArchitecture.AnyCPU;
                        var architectureName = "AnyCPU";

                        if (!platform.CanBuildArchitecture(architecture))
                            continue;
                        var toolchain = platform.TryGetToolchain(architecture);

                        var configuration = TargetConfiguration.Debug;
                        var configurationName = "Debug";
                        var configurationText = configurationName;
                        var targetBuildOptions = GetBuildOptions(target, platform, toolchain, architecture, configuration, project.WorkspaceRootPath);
                        targetBuildOptions.Flags |= BuildFlags.GenerateProject;
                        var modules = CollectModules(rules, platform, target, targetBuildOptions, toolchain, architecture, configuration);
                        foreach (var module in modules)
                        {
                            // This merges private module build options into global target - not the best option but helps with syntax highlighting and references collecting
                            module.Key.Setup(targetBuildOptions);
                            module.Key.SetupEnvironment(targetBuildOptions);
                        }
                        project.Configurations.Add(new Project.ConfigurationData
                        {
                            Name = configurationText + '|' + architectureName,
                            Text = configurationText,
                            Platform = targetPlatform,
                            PlatformName = platformName,
                            Architecture = architecture,
                            ArchitectureName = architectureName,
                            Configuration = configuration,
                            ConfigurationName = configurationName,
                            Target = target,
                            TargetBuildOptions = targetBuildOptions,
                            Modules = modules,
                        });
                        configuration = TargetConfiguration.Release;
                        configurationName = "Release";
                        configurationText = configurationName;
                        targetBuildOptions = GetBuildOptions(target, platform, toolchain, architecture, configuration, project.WorkspaceRootPath);
                        targetBuildOptions.Flags |= BuildFlags.GenerateProject;
                        modules = CollectModules(rules, platform, target, targetBuildOptions, toolchain, architecture, configuration);
                        foreach (var module in modules)
                        {
                            // This merges private module build options into global target - not the best option but helps with syntax highlighting and references collecting
                            module.Key.Setup(targetBuildOptions);
                            module.Key.SetupEnvironment(targetBuildOptions);
                        }
                        project.Configurations.Add(new Project.ConfigurationData
                        {
                            Name = configurationText + '|' + architectureName,
                            Text = configurationText,
                            Platform = targetPlatform,
                            PlatformName = platformName,
                            Architecture = architecture,
                            ArchitectureName = architectureName,
                            Configuration = configuration,
                            ConfigurationName = configurationName,
                            Target = target,
                            TargetBuildOptions = targetBuildOptions,
                            Modules = modules,
                        });
                    }
                    else
                        throw new NotImplementedException();
                    continue;
                }

                foreach (var targetPlatform in target.Platforms)
                {
                    string platformName = targetPlatform.ToString();
                    foreach (var configuration in target.Configurations)
                    {
                        string configurationName = configuration.ToString();
                        foreach (var architecture in target.GetArchitectures(targetPlatform))
                        {
                            if (!Platform.IsPlatformSupported(targetPlatform, architecture))
                                continue;
                            var platform = Platform.GetPlatform(targetPlatform, true);
                            if (platform == null)
                                continue;
                            if (!platform.HasRequiredSDKsInstalled && (!projectInfo.IsCSharpOnlyProject || platform != Platform.BuildPlatform))
                                continue;
                            if (!platform.CanBuildArchitecture(architecture))
                                continue;

                            string configurationText = targetName + '.' + platformName + '.' + configurationName;
                            string architectureName = architecture.ToString();
                            if (platform is IProjectCustomizer customizer)
                                customizer.GetProjectArchitectureName(project, platform, architecture, ref architectureName);

                            var toolchain = platform.TryGetToolchain(architecture);
                            var targetBuildOptions = GetBuildOptions(target, platform, toolchain, architecture, configuration, project.WorkspaceRootPath);
                            targetBuildOptions.Flags |= BuildFlags.GenerateProject;
                            var modules = CollectModules(rules, platform, target, targetBuildOptions, toolchain, architecture, configuration);
                            foreach (var module in modules)
                            {
                                // This merges private module build options into global target - not the best option but helps with syntax highlighting and references collecting
                                module.Key.Setup(targetBuildOptions);
                                module.Key.SetupEnvironment(targetBuildOptions);
                            }

                            project.Configurations.Add(new Project.ConfigurationData
                            {
                                Name = configurationText + '|' + architectureName,
                                Text = configurationText,
                                Platform = targetPlatform,
                                PlatformName = platformName,
                                Architecture = architecture,
                                ArchitectureName = architectureName,
                                Configuration = configuration,
                                ConfigurationName = configurationName,
                                Target = target,
                                TargetBuildOptions = targetBuildOptions,
                                Modules = modules,
                            });
                        }
                    }
                }
            }
        }

        /// <summary>
        /// Custom projects generation callback.
        /// </summary>
        public static event Action<List<Project>> GenerateCustomProjects;

        /// <summary>
        /// Generates the projects for the targets and the solution for the workspace.
        /// </summary>
        public static void GenerateProjects()
        {
            using (new ProfileEventScope("GenerateProjects"))
            {
                // Pick the project format
                var projectFormats = new HashSet<ProjectFormat>();
                if (Configuration.ProjectFormatVS2022)
                    projectFormats.Add(ProjectFormat.VisualStudio2022);
                if (Configuration.ProjectFormatVS2019)
                    projectFormats.Add(ProjectFormat.VisualStudio2019);
                if (Configuration.ProjectFormatVS2017)
                    projectFormats.Add(ProjectFormat.VisualStudio2017);
                if (Configuration.ProjectFormatVS2015)
                    projectFormats.Add(ProjectFormat.VisualStudio2015);
                if (Configuration.ProjectFormatVSCode)
                    projectFormats.Add(ProjectFormat.VisualStudioCode);
                if (Configuration.ProjectFormatRider)
                    projectFormats.Add(ProjectFormat.VisualStudio2022);
                if (!string.IsNullOrEmpty(Configuration.ProjectFormatCustom))
                    projectFormats.Add(ProjectFormat.Custom);
                if (projectFormats.Count == 0)
                    projectFormats.Add(Platform.BuildPlatform.DefaultProjectFormat);

                // Always generate VS solution files for project (needed for C# Intellisense support)
                projectFormats.Add(ProjectFormat.VisualStudio2022);

                foreach (ProjectFormat projectFormat in projectFormats)
                    GenerateProject(projectFormat);
            }
        }

        /// <summary>
        /// Generates project files for the specified format.
        /// </summary>
        public static void GenerateProject(ProjectFormat projectFormat)
        {
            using (new ProfileEventScope("GenerateProject" + projectFormat))
            {
                // Setup
                var rules = GenerateRulesAssembly();
                var rootProject = Globals.Project;
                if (rootProject == null)
                    throw new Exception("Missing project.");
                var projectFiles = rootProject.GetAllProjects();
                var workspaceRoot = rootProject.ProjectFolderPath;
                var projectsRoot = Path.Combine(workspaceRoot, "Cache", "Projects");
                var projects = new List<Project>();
                var dotNetProjectGenerator = ProjectGenerator.Create(projectFormat, TargetType.DotNetCore);
                var projectToBinaryModule = new Dictionary<Project, KeyValuePair<string, HashSet<Module>>>();
                var projectToModulesBuildOptions = new Dictionary<Project, Dictionary<Module, BuildOptions>>();
                Project mainSolutionProject = null;
                ProjectGenerator nativeProjectGenerator = ProjectGenerator.Create(projectFormat, TargetType.NativeCpp);
                var solutionName = rootProject.Name;
                var solutionPath = Path.Combine(workspaceRoot, solutionName + '.' + nativeProjectGenerator.SolutionFileExtension);

                // Group targets by project name and sort groups based on the project (ensures that referenced plugin source projects are generated firstly before main source projects)
                var targetGroups = new List<ProjectTargetsGroup>();
                foreach (var target in rules.Targets)
                {
                    int i = 0;
                    for (; i < targetGroups.Count; i++)
                    {
                        if (targetGroups[i].ProjectName == target.ProjectName)
                            break;
                    }
                    if (i == targetGroups.Count)
                    {
                        targetGroups.Add(new ProjectTargetsGroup
                        {
                            ProjectName = target.ProjectName,
                            Type = target.Type,
                            Targets = new List<Target>(),
                        });
                    }
                    var targetGroup = targetGroups[i];
                    if (targetGroup.Type != target.Type)
                        Log.Error(string.Format($"Invalid targets group. Project {target.ProjectName} uses type {target.Type} from target {targetGroup.Targets[0].Name} but target {target.Name} is type of {target.Type}"));
                    if (targetGroup.Project == null && target is ProjectTarget projectTarget)
                        targetGroup.Project = projectTarget.Project;
                    targetGroup.Targets.Add(target);
                }
                foreach (var targetGroup in targetGroups)
                {
                    if (targetGroup.Project == null)
                    {
                        targetGroup.Project = projectFiles.First(x => targetGroup.Targets[0].FolderPath.Contains(x.ProjectFolderPath));
                        if (targetGroup.Project == null)
                        {
                            Log.Error(string.Format($"Invalid target {targetGroup.ProjectName} has no project to link."));
                            return;
                        }
                    }
                    targetGroup.ProjectDependencies = targetGroup.Project.GetAllProjects();
                    targetGroup.ProjectDependencies.Remove(targetGroup.Project);
                }
                targetGroups.Sort((a, b) => a.ProjectDependencies.Count - b.ProjectDependencies.Count);

                // Setup projects for target groups (before actual generation to handle cross-project references like)
                foreach (var e in targetGroups)
                {
                    var projectName = e.ProjectName;
                    using (new ProfileEventScope(projectName))
                    {
                        var targets = e.Targets.ToArray();
                        var type = e.Type;
                        var projectInfo = e.Project;

                        // Create project
                        Project mainProject;
                        var binaryModules = new Dictionary<string, HashSet<Module>>();
                        var modulesBuildOptions = new Dictionary<Module, BuildOptions>();
                        using (new ProfileEventScope("CreateProject"))
                        {
                            var generator = ProjectGenerator.Create(projectFormat, type);
                            var project = mainProject = generator.CreateProject();
                            if (targets[0].CustomExternalProjectFilePath == null)
                                project.Type = TargetType.NativeCpp;
                            project.Name = project.BaseName = projectName;
                            project.Targets = targets;
                            project.SearchPaths = Array.Empty<string>();
                            project.WorkspaceRootPath = projectInfo.ProjectFolderPath;
                            if (targets[0].CustomExternalProjectFilePath == null)
                                project.Path = Path.Combine(projectsRoot, project.Name + '.' + generator.ProjectFileExtension);
                            else
                                project.Path = targets[0].CustomExternalProjectFilePath;
                            if (project.WorkspaceRootPath.StartsWith(rootProject.ProjectFolderPath))
                                project.GroupName = Utilities.NormalizePath(Utilities.MakePathRelativeTo(project.WorkspaceRootPath, rootProject.ProjectFolderPath));
                            else if (projectInfo != Globals.Project)
                                project.GroupName = projectInfo.Name;
                            project.SourceDirectories = new List<string>
                            {
                                Path.GetDirectoryName(targets[0].FilePath)
                            };
                            SetupProjectConfigurations(project, projectInfo);

                            // Get all modules aggregated into all binary modules used in all configurations of this target
                            foreach (var configurationData in project.Configurations)
                            {
                                var configurationBinaryModules = GetBinaryModules(projectInfo, configurationData.Target, configurationData.Modules);
                                foreach (var configurationBinaryModule in configurationBinaryModules)
                                {
                                    // Skip if none of the included binary modules is inside the project workspace (eg. merged external binary modules from engine to game project)
                                    if (!configurationBinaryModule.Any(y => y.FolderPath.StartsWith(project.WorkspaceRootPath)))
                                        continue;

                                    if (!binaryModules.TryGetValue(configurationBinaryModule.Key, out var modules))
                                        binaryModules[configurationBinaryModule.Key] = modules = new HashSet<Module>();
                                    modules.AddRange(configurationBinaryModule);
                                    foreach (var module in configurationBinaryModule)
                                    {
                                        if (!modulesBuildOptions.ContainsKey(module))
                                            modulesBuildOptions.Add(module, configurationData.Modules[module]);
                                    }
                                }

                                foreach (var reference in projectInfo.References)
                                {
                                    var referenceTargets = GetProjectTargets(reference.Project);
                                    foreach (var referenceTarget in referenceTargets)
                                    {
                                        // Skip referenced targets that don't meet this configuration specs (eg. Editor target should skip Android platform)
                                        if (!referenceTarget.Platforms.Contains(configurationData.Platform))
                                            continue;
                                        if (!referenceTarget.Architectures.Contains(configurationData.Architecture))
                                            continue;

                                        try
                                        {
                                            var referenceBuildOptions = GetBuildOptions(referenceTarget, configurationData.TargetBuildOptions.Platform, configurationData.TargetBuildOptions.Toolchain, configurationData.Architecture, configurationData.Configuration, reference.Project.ProjectFolderPath);
                                            referenceBuildOptions.Flags |= BuildFlags.GenerateProject;
                                            var referenceModules = CollectModules(rules, referenceBuildOptions.Platform, referenceTarget, referenceBuildOptions, referenceBuildOptions.Toolchain, referenceBuildOptions.Architecture, referenceBuildOptions.Configuration);
                                            var referenceBinaryModules = referenceModules.Keys.GroupBy(x => x.BinaryModuleName).ToArray();
                                            foreach (var binaryModule in referenceBinaryModules)
                                            {
                                                var binaryModuleName = binaryModule.Key;
                                                if (string.IsNullOrEmpty(binaryModuleName))
                                                    continue;
                                                project.Defines.Add(binaryModuleName.ToUpperInvariant() + "_API=");
                                            }
                                        }
                                        catch
                                        {
                                            // Ignore exceptions
                                        }
                                    }
                                }
                            }

                            foreach (var binaryModule in binaryModules)
                            {
                                project.Defines.Add(binaryModule.Key.ToUpperInvariant() + "_API=");
                            }

                            // Skip using C++ project if any included binary module is C#-only
                            if (binaryModules.Any(x => x.Value.Any(y => y.BuildNativeCode)) && !rootProject.IsCSharpOnlyProject)
                            {
                                projects.Add(project);
                                if (mainSolutionProject == null && projectInfo == rootProject)
                                    mainSolutionProject = project;
                            }
                            else if (targets[0].CustomExternalProjectFilePath != null)
                                projects.Add(project);
                        }

                        // Create API bindings projects
                        foreach (var binaryModule in binaryModules)
                        {
                            var binaryModuleName = binaryModule.Key;

                            // Skip excluded modules with empty BinaryModuleName
                            if (string.IsNullOrEmpty(binaryModuleName))
                                continue;

                            // Skip bindings projects for prebuilt targets (eg. no sources to build/view - just binaries)
                            if (targets[0].IsPreBuilt)
                                continue;

                            // Skip if project of that name has been already added
                            if (projects.Any(x => x.OutputType == TargetOutputType.Library && x.Type == TargetType.DotNetCore && x.BaseName == binaryModuleName))
                                continue;

                            using (new ProfileEventScope(binaryModuleName))
                            {
                                // TODO: add support for extending this code and support generating bindings projects for other scripting languages

                                // Get source files
                                var sourceFiles = new List<string>();
                                var generatedSourceFiles = new List<string>();
                                foreach (var module in binaryModule.Value)
                                {
                                    // C# sources included into build source files
                                    sourceFiles.AddRange(modulesBuildOptions[module].SourceFiles.Where(x => x.EndsWith(".cs")));
                                }
                                sourceFiles.RemoveAll(x => x.EndsWith(BuildFilesPostfix));
                                foreach (var target in targets)
                                {
                                    foreach (var configurationData in mainProject.Configurations)
                                    {
                                        if (target != configurationData.Target)
                                            continue;
                                        foreach (var q in configurationData.Modules)
                                        {
                                            if (!q.Key.FolderPath.StartsWith(target.FolderPath))
                                                continue;

                                            // Generated C# bindings source files (per-target, per-platform, per-configuration)
                                            var intermediateFolder = Path.Combine(q.Value.WorkingDirectory, Configuration.IntermediateFolder, target.Name, configurationData.PlatformName, configurationData.ArchitectureName, configurationData.ConfigurationName);
                                            var path = Path.Combine(intermediateFolder, q.Key.Name, q.Key.Name + ".Bindings.Gen.cs");
                                            if (!File.Exists(path))
                                            {
                                                Directory.CreateDirectory(Path.GetDirectoryName(path));
                                                File.WriteAllText(path, string.Empty, new UTF8Encoding());
                                            }
                                            generatedSourceFiles.Add(path);
                                        }
                                    }
                                }
                                sourceFiles.Add(Path.Combine(targets[0].FolderPath, binaryModuleName + ".Gen.cs"));
                                if (mainSolutionProject == null)
                                {
                                    // Add shaders sources to C# bindings project if not using C++ project
                                    var shadersSourcePath = Path.Combine(projectInfo.ProjectFolderPath, "Source/Shaders");
                                    if (Directory.Exists(shadersSourcePath))
                                        sourceFiles.AddRange(Directory.GetFiles(shadersSourcePath, "*", SearchOption.AllDirectories));
                                }
                                sourceFiles.Sort();

                                // Create project description
                                var project = dotNetProjectGenerator.CreateProject();
                                project.Type = TargetType.DotNetCore;
                                project.Name = project.BaseName = binaryModuleName;
                                if (projects.Any(x => x.Name == project.Name))
                                    project.Name += ".CSharp"; // Prevent overlapping name with native code project
                                project.OutputType = TargetOutputType.Library;
                                project.Targets = targets;
                                project.SearchPaths = Array.Empty<string>();
                                project.WorkspaceRootPath = mainProject.WorkspaceRootPath;
                                project.GroupName = mainProject.GroupName;
                                if (project.WorkspaceRootPath.StartsWith(workspaceRoot))
                                {
                                    // Place project targets binary modules bindings in the Source folder of the project (Visual Studio handles C# source files better in that way)
                                    project.Path = Path.Combine(project.WorkspaceRootPath, "Source", project.Name + '.' + dotNetProjectGenerator.ProjectFileExtension);
                                }
                                else
                                {
                                    // Default project files location
                                    project.Path = Path.Combine(projectsRoot, project.Name + '.' + dotNetProjectGenerator.ProjectFileExtension);
                                }
                                project.SourceFiles = sourceFiles;
                                project.GeneratedSourceFiles = generatedSourceFiles;
                                project.CSharp.UseFlaxVS = true;
                                project.Configurations.AddRange(mainProject.Configurations);
                                projectToBinaryModule.Add(project, binaryModule);
                                projectToModulesBuildOptions.Add(project, modulesBuildOptions);
                                projects.Add(project);
                                if (mainSolutionProject == null && projectInfo == rootProject)
                                    mainSolutionProject = project;
                            }
                        }
                    }
                }

                // Setup cross-project references
                using (new ProfileEventScope("ProjectReferences"))
                {
                    foreach (var project in projects)
                    {
                        if (!projectToBinaryModule.TryGetValue(project, out var binaryModule) ||
                            !projectToModulesBuildOptions.TryGetValue(project, out var modulesBuildOptions))
                            continue;

                        foreach (var module in binaryModule.Value)
                        {
                            if (!modulesBuildOptions.TryGetValue(module, out var moduleBuildOptions))
                                continue;

                            // Combine build options from this module
                            project.CSharp.SystemReferences.AddRange(moduleBuildOptions.ScriptingAPI.SystemReferences);
                            project.CSharp.FileReferences.AddRange(moduleBuildOptions.ScriptingAPI.FileReferences);

                            // Find references based on the modules dependencies (external or from projects)
                            foreach (var dependencyName in moduleBuildOptions.PublicDependencies.Concat(moduleBuildOptions.PrivateDependencies))
                            {
                                var dependencyModule = rules.GetModule(dependencyName);
                                if (dependencyModule != null &&
                                    !string.IsNullOrEmpty(dependencyModule.BinaryModuleName) &&
                                    dependencyModule.BinaryModuleName != binaryModule.Key)
                                {
                                    var dependencyProject = projects.Find(x => x != null && x.Generator.Type == project.Generator.Type && x.BaseName == dependencyModule.BinaryModuleName);
                                    if (dependencyProject != null)
                                    {
                                        // Reference that project
                                        project.Dependencies.Add(dependencyProject);
                                    }
                                    else if (dependencyModule.BinaryModuleName == "FlaxEngine")
                                    {
                                        // TODO: instead of this hack find a way to reference the prebuilt target bindings binary (example: game C# project references FlaxEngine C# prebuilt dll)
                                        project.CSharp.FileReferences.Add(Path.Combine(Globals.EngineRoot, Platform.GetEditorBinaryDirectory(), "Development/FlaxEngine.CSharp.dll"));
                                    }
                                }
                            }
                        }
                    }
                }

                // When generating C#-only projects for Game that uses source-engine distribution replace dependencies on FlaxEngine with fixed dll file refs to fix Intellisense issues
                if (rootProject.IsCSharpOnlyProject)
                {
                    Project flaxDependencyToRemove = null;
                    foreach (var project in projects)
                    {
                        if (project.BaseName != "FlaxEngine")
                        {
                            var flaxDependency = project.Dependencies.FirstOrDefault(x => x.BaseName == "FlaxEngine");
                            if (flaxDependency != null)
                            {
                                project.Dependencies.Remove(flaxDependency);

                                // TODO: instead of this hack find a way to reference the prebuilt target bindings binary (example: game C# project references FlaxEngine C# prebuilt dll)
                                project.CSharp.FileReferences.Add(Path.Combine(Globals.EngineRoot, Platform.GetEditorBinaryDirectory(), "Development/FlaxEngine.CSharp.dll"));

                                // Remove FlaxEngine from projects to prevent duplicated types errors in Intellisense (eg. Actor type defined in both FlaxEngine.CSharp.dll and FlaxEngine.csproj)
                                flaxDependencyToRemove = flaxDependency;
                            }
                        }
                    }
                    if (flaxDependencyToRemove != null)
                    {
                        projects.Remove(flaxDependencyToRemove);
                        foreach (var project in projects)
                            project.Dependencies.Remove(flaxDependencyToRemove);
                    }
                }

                // Setup custom projects
                GenerateCustomProjects?.Invoke(projects);
                nativeProjectGenerator.GenerateCustomProjects(projects);

                // Generate projects
                using (new ProfileEventScope("GenerateProjects"))
                {
                    foreach (var project in projects)
                    {
                        Log.Verbose(project.Name + " -> " + project.Path);
                        project.Generate(solutionPath, project == mainSolutionProject);
                    }
                }

                // Generate C# project for build scripts files
                string rulesProjectName = "BuildScripts";
                using (new ProfileEventScope(rulesProjectName))
                {
                    // Create dummy target
                    var target = new Target
                    {
                        Name = rulesProjectName,
                        ProjectName = rulesProjectName,
                        FilePath = null,
                        FolderPath = null,
                        Type = TargetType.DotNetCore,
                        OutputType = TargetOutputType.Library,
                        Platforms = new[] { Platform.BuildPlatform.Target },
                        Configurations = new[] { TargetConfiguration.Debug },
                        Architectures = new[] { TargetArchitecture.AnyCPU },
                    };

                    // Create project description
                    Project project;
                    using (new ProfileEventScope("CreateProject"))
                    {
                        project = dotNetProjectGenerator.CreateProject();
                        project.Type = TargetType.DotNetCore;
                        project.Name = project.BaseName = rulesProjectName;
                        project.Targets = new[] { target };
                        project.SearchPaths = Array.Empty<string>();
                        project.WorkspaceRootPath = workspaceRoot;
                        project.Path = Path.Combine(projectsRoot, project.Name + '.' + dotNetProjectGenerator.ProjectFileExtension);
                        project.CSharp.OutputPath = Path.Combine(Environment.CurrentDirectory, "Cache", "Intermediate", "Unused");
                        project.SourceFiles = new List<string>();
                        foreach (var e in rules.Targets)
                            project.SourceFiles.Add(e.FilePath);
                        foreach (var e in rules.Modules)
                            project.SourceFiles.Add(e.FilePath);
                        project.CSharp.SystemReferences = new HashSet<string>
                        {
                            "System",
                            "System.Core",
                        };
                        SetupProjectConfigurations(project, rootProject);
                        if (project.Configurations.Count == 0)
                        {
                            // Hardcoded dummy configuration even if platform tools are missing for this platform
                            var platform = Platform.BuildPlatform;
                            var architecture = TargetArchitecture.x64;
                            var configuration = TargetConfiguration.Debug;
                            var buildOptions = GetBuildOptions(target, platform, null, architecture, configuration, project.WorkspaceRootPath);
                            buildOptions.Flags |= BuildFlags.GenerateProject;
                            project.Configurations.Add(new Project.ConfigurationData
                            {
                                Platform = platform.Target,
                                PlatformName = platform.Target.ToString(),
                                Architecture = architecture,
                                ArchitectureName = architecture.ToString(),
                                Configuration = configuration,
                                ConfigurationName = configuration.ToString(),
                                Target = target,
                                TargetBuildOptions = buildOptions,
                                Modules = new Dictionary<Module, BuildOptions>(),
                            });
                        }
                        var c = project.Configurations[0];
                        c.Name = "Debug|AnyCPU";
                        c.Text = "Debug";
                        project.Configurations[0] = c;

                        // Add reference to Flax.Build so rules can be validated in the IDE
                        project.CSharp.FileReferences.Add(Assembly.GetEntryAssembly().Location);
                    }

                    // Generate project
                    using (new ProfileEventScope("GenerateProject"))
                    {
                        Log.Verbose("Project " + rulesProjectName + " -> " + project.Path);
                        dotNetProjectGenerator.GenerateProject(project, solutionPath, project == mainSolutionProject);
                    }

                    projects.Add(project);
                }

                // Generate solution
                using (new ProfileEventScope("Solution"))
                {
                    Solution solution;
                    using (new ProfileEventScope("CreateSolution"))
                    {
                        solution = nativeProjectGenerator.CreateSolution();
                        solution.Name = solutionName;
                        solution.WorkspaceRootPath = workspaceRoot;
                        solution.Path = solutionPath;
                        solution.Projects = projects.ToArray();
                        solution.MainProject = mainSolutionProject;
                    }

                    using (new ProfileEventScope("GenerateSolution"))
                    {
                        Log.Verbose("Solution -> " + solution.Path);
                        nativeProjectGenerator.GenerateSolution(solution);
                    }
                }
            }
        }
    }
}
