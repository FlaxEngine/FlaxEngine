// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Flax.Build.Bindings;
using Flax.Build.Graph;
using Flax.Build.NativeCpp;
using System.Text.Json.Serialization;
using System.Text.Json;

namespace Flax.Build
{
    static partial class Builder
    {
        public sealed class BuildData
        {
            public ProjectInfo Project;
            public TaskGraph Graph;
            public RulesAssembly Rules;
            public Target Target;
            public BuildOptions TargetOptions;
            public Platform Platform;
            public Toolchain Toolchain;
            public TargetArchitecture Architecture;
            public TargetConfiguration Configuration;
            public Dictionary<Module, BuildOptions> Modules = new Dictionary<Module, BuildOptions>(256);
            public List<Module> ModulesOrderList = new List<Module>();
            public Dictionary<Module, ModuleInfo> ModulesInfo = new Dictionary<Module, ModuleInfo>(256);
            public Dictionary<TypeInfo, ApiTypeInfo> TypeCache = new Dictionary<TypeInfo, ApiTypeInfo>(1024);
            public IGrouping<string, Module>[] BinaryModules;
            public BuildTargetInfo BuildInfo;
            public Dictionary<ProjectInfo, BuildData> ReferenceBuilds = new Dictionary<ProjectInfo, BuildData>();
            public Dictionary<string, string> PrecompiledHeaderFiles = new();

            public BuildTargetBinaryModuleInfo FinReferenceBuildModule(string name)
            {
                if (BuildInfo != null && BuildInfo.BinaryModules != null)
                {
                    foreach (var e in BuildInfo.BinaryModules)
                    {
                        if (string.Equals(e.Name, name, StringComparison.Ordinal))
                            return e;
                    }
                }
                foreach (var e in ReferenceBuilds)
                {
                    var q = e.Value.FinReferenceBuildModule(name);
                    if (q != null)
                        return q;
                }
                return null;
            }
        }

        public class BuildTargetBinaryModuleInfo
        {
            public string Name;

            [JsonIgnore]
            public string NativePath;

            [JsonPropertyName("NativePath")]
            public string NativePathProcessed;

            [JsonIgnore]
            public string ManagedPath;

            [JsonPropertyName("ManagedPath")]
            public string ManagedPathProcessed;
        }

        public class BuildTargetReferenceInfo
        {
            [JsonIgnore]
            public string ProjectPath;

            [JsonPropertyName("ProjectPath")]
            public string ProjectPathProcessed;

            [JsonIgnore]
            public string Path;

            [JsonPropertyName("Path")]
            public string PathProcessed;
        }

        [JsonSourceGenerationOptions(IncludeFields = true)]
        [JsonSerializable(typeof(BuildTargetBinaryModuleInfo))]
        internal partial class BuildTargetBinaryModuleInfoSourceGenerationContext : JsonSerializerContext
        {
        }

        [JsonSourceGenerationOptions(IncludeFields = true)]
        [JsonSerializable(typeof(BuildTargetReferenceInfo))]
        internal partial class BuildTargetReferenceInfoSourceGenerationContext : JsonSerializerContext
        {
        }

        [JsonSourceGenerationOptions(IncludeFields = true)]
        [JsonSerializable(typeof(BuildTargetInfo))]
        internal partial class BuildTargetInfoSourceGenerationContext : JsonSerializerContext
        {
        }

        public class BuildTargetInfo
        {
            public string Name;
            public string Platform;
            public string Architecture;
            public string Configuration;
            public string HotReloadPostfix;
            public BuildTargetBinaryModuleInfo[] BinaryModules;
            public BuildTargetReferenceInfo[] References;

            internal void AddExternalBinaryModules(ref int i, string projectPath, HashSet<BuildOptions.ExternalModule> externalModules)
            {
                foreach (var externalModule in externalModules)
                {
                    var binaryModuleInfo = new BuildTargetBinaryModuleInfo
                    {
                        NativePath = string.Empty,
                        ManagedPath = string.Empty,
                        Name = externalModule.Name,
                    };

                    switch (externalModule.Type)
                    {
                    case BuildOptions.ExternalModule.Types.Native:
                        binaryModuleInfo.NativePath = externalModule.Path;
                        binaryModuleInfo.NativePathProcessed = ProcessPath(binaryModuleInfo.NativePath, projectPath);
                        break;
                    case BuildOptions.ExternalModule.Types.CSharp:
                        binaryModuleInfo.ManagedPath = externalModule.Path;
                        binaryModuleInfo.ManagedPathProcessed = ProcessPath(binaryModuleInfo.ManagedPath, projectPath);
                        break;
                    }

                    binaryModuleInfo.NativePathProcessed = ProcessPath(binaryModuleInfo.NativePath, projectPath);
                    binaryModuleInfo.ManagedPathProcessed = ProcessPath(binaryModuleInfo.ManagedPath, projectPath);

                    BinaryModules[i++] = binaryModuleInfo;
                }
            }

            internal void AddReferencedBuilds(ref int i, string projectPath, Dictionary<ProjectInfo, BuildData> referencedBuilds)
            {
                foreach (var referenceBuild in referencedBuilds)
                {
                    var reference = referenceBuild.Value;
                    var referenceOutputTargetFilePath = reference.Target.GetOutputFilePath(reference.TargetOptions);
                    var referenceOutputPath = Path.GetDirectoryName(referenceOutputTargetFilePath);
                    var referenceInfo = new BuildTargetReferenceInfo
                    {
                        ProjectPath = reference.Project.ProjectPath,
                        Path = Path.Combine(referenceOutputPath, reference.Target.Name + ".Build.json"),
                    };

                    referenceInfo.ProjectPathProcessed = ProcessPath(referenceInfo.ProjectPath, projectPath);
                    referenceInfo.PathProcessed = ProcessPath(referenceInfo.Path, projectPath);

                    References[i++] = referenceInfo;
                }
            }

            public string Serialize()
            {
                // Null any empty fields to exclude them from serialization
                if (HotReloadPostfix?.Length == 0)
                    HotReloadPostfix = null;
                foreach (var binaryModule in BinaryModules)
                {
                    if (binaryModule.NativePathProcessed?.Length == 0)
                        binaryModule.NativePathProcessed = null;
                    if (binaryModule.ManagedPathProcessed?.Length == 0)
                        binaryModule.ManagedPathProcessed = null;
                }

                // Convert to Json
                var options = new JsonSerializerOptions
                {
                    WriteIndented = true,
                    IncludeFields = true,
                    DefaultIgnoreCondition = JsonIgnoreCondition.WhenWritingNull,
                    TypeInfoResolver = BuildTargetInfoSourceGenerationContext.Default,
                };
                return JsonSerializer.Serialize<BuildTargetInfo>(this, options);
            }

            public static string ProcessPath(string path, string projectPath)
            {
                if (string.IsNullOrEmpty(path))
                    return string.Empty;
                if (path.StartsWith(Globals.EngineRoot))
                    path = "$(EnginePath)" + path.Substring(Globals.EngineRoot.Length);
                else if (path.StartsWith(projectPath))
                    path = "$(ProjectPath)" + path.Substring(projectPath.Length);
                return Utilities.NormalizePath(path);
            }
        }

        private static BuildOptions CollectModules(BuildData buildData, Module module, bool withApi)
        {
            if (!buildData.Modules.TryGetValue(module, out var moduleOptions))
            {
                // Setup module build options
                var outputPath = Path.Combine(buildData.TargetOptions.IntermediateFolder, module.Name);
                if (!buildData.Target.IsPreBuilt && !Directory.Exists(outputPath))
                    Directory.CreateDirectory(outputPath);
                moduleOptions = new BuildOptions
                {
                    Target = buildData.Target,
                    Platform = buildData.Platform,
                    Toolchain = buildData.Toolchain,
                    Architecture = buildData.Architecture,
                    Configuration = buildData.Configuration,
                    CompileEnv = (CompileEnvironment)buildData.TargetOptions.CompileEnv.Clone(),
                    LinkEnv = (LinkEnvironment)buildData.TargetOptions.LinkEnv.Clone(),
                    IntermediateFolder = outputPath,
                    OutputFolder = outputPath,
                    WorkingDirectory = buildData.TargetOptions.WorkingDirectory,
                    HotReloadPostfix = buildData.TargetOptions.HotReloadPostfix,
                    Flags = buildData.TargetOptions.Flags,
                };
                moduleOptions.SourcePaths.Add(module.FolderPath);
                module.Setup(moduleOptions);
                moduleOptions.MergeSourcePathsIntoSourceFiles();

                // Collect dependent modules (private)
                foreach (var moduleName in moduleOptions.PrivateDependencies)
                {
                    // TODO: check circular references

                    var dependencyModule = buildData.Rules.GetModule(moduleName);
                    if (dependencyModule != null)
                    {
                        var dependencyOptions = CollectModules(buildData, dependencyModule, withApi);

                        foreach (var e in dependencyOptions.PublicDefinitions)
                            moduleOptions.PrivateDefinitions.Add(e);
                    }
                    else
                    {
                        Log.Warning(string.Format("Missing module {0} referenced by module {1} (or invalid name specified)", moduleName, module.Name));
                    }
                }

                // Collect dependent modules (public)
                foreach (var moduleName in moduleOptions.PublicDependencies)
                {
                    // TODO: check circular references

                    var dependencyModule = buildData.Rules.GetModule(moduleName);
                    if (dependencyModule != null)
                    {
                        var dependencyOptions = CollectModules(buildData, dependencyModule, withApi);

                        foreach (var e in dependencyOptions.PublicDefinitions)
                            moduleOptions.PublicDefinitions.Add(e);
                    }
                    else
                    {
                        Log.Warning(string.Format("Missing module {0} referenced by module {1} (or invalid name specified)", moduleName, module.Name));
                    }
                }

                if (withApi && !string.IsNullOrEmpty(module.BinaryModuleName))
                {
                    // Process module scripting API
                    BindingsGenerator.ParseModule(buildData, module, moduleOptions);
                }

                // Cache build module
                buildData.Modules.Add(module, moduleOptions);
                buildData.ModulesOrderList.Add(module);
            }

            return moduleOptions;
        }

        public static event Action<TaskGraph, BuildOptions> PreBuild;
        public static event Action<TaskGraph, BuildOptions> PostBuild;
        public static event Action<TaskGraph, BuildData> BuildBindings;

        /// <summary>
        /// Collects the modules required by the given target to build (includes dependencies).
        /// </summary>
        /// <param name="rules">The rules.</param>
        /// <param name="target">The target.</param>
        /// <param name="targetBuildOptions">The target build options.</param>
        /// <param name="platform">The platform.</param>
        /// <param name="toolchain">The toolchain.</param>
        /// <param name="architecture">The architecture.</param>
        /// <param name="configuration">The configuration.</param>
        /// <returns>The list of modules to use for build (unique items).</returns>
        public static Dictionary<Module, BuildOptions> CollectModules(RulesAssembly rules, Platform platform, Target target, BuildOptions targetBuildOptions, Toolchain toolchain, TargetArchitecture architecture, TargetConfiguration configuration)
        {
            return CollectModules(rules, platform, target, targetBuildOptions, toolchain, architecture, configuration, target.Modules);
        }

        /// <summary>
        /// Collects the modules required to build (includes dependencies).
        /// </summary>
        /// <param name="rules">The rules.</param>
        /// <param name="target">The target.</param>
        /// <param name="targetBuildOptions">The target build options.</param>
        /// <param name="platform">The platform.</param>
        /// <param name="toolchain">The toolchain.</param>
        /// <param name="architecture">The architecture.</param>
        /// <param name="configuration">The configuration.</param>
        /// <param name="moduleNames">The list of root modules to start collection.</param>
        /// <returns>The list of modules to use for build (unique items).</returns>
        public static Dictionary<Module, BuildOptions> CollectModules(RulesAssembly rules, Platform platform, Target target, BuildOptions targetBuildOptions, Toolchain toolchain, TargetArchitecture architecture, TargetConfiguration configuration, IEnumerable<string> moduleNames)
        {
            var buildData = new BuildData
            {
                Rules = rules,
                Target = target,
                TargetOptions = targetBuildOptions,
                Platform = platform,
                Toolchain = toolchain,
                Architecture = architecture,
                Configuration = configuration,
            };

            // Collect all modules
            foreach (var moduleName in moduleNames)
            {
                var module = rules.GetModule(moduleName);
                if (module != null)
                {
                    CollectModules(buildData, module, false);
                }
                else
                {
                    Log.Warning(string.Format("Missing module {0} (or invalid name specified)", moduleName));
                }
            }

            return buildData.Modules;
        }

        private static void OnBuildBindings(BuildData buildData, TaskGraph graph)
        {
            if (buildData.Target.IsPreBuilt)
                return;
            using (new ProfileEventScope("BuildBindings"))
            {
                if (EngineConfiguration.WithCSharp(buildData.TargetOptions))
                {
                    BuildTargetBindings(graph, buildData);
                }
                BuildBindings?.Invoke(graph, buildData);
            }
        }

        private static void LinkNativeBinary(BuildData buildData, BuildOptions buildOptions, string outputPath)
        {
            using (new ProfileEventScope("LinkNativeBinary"))
            {
                buildData.Toolchain.LinkFiles(buildData.Graph, buildOptions, outputPath);

                // Produce additional import library if will use binary module references
                var linkerOutput = buildOptions.LinkEnv.Output;
                if (buildData.Toolchain.UseImportLibraryWhenLinking &&
                    !buildData.Toolchain.GeneratesImportLibraryWhenLinking &&
                    buildOptions.Target.UseSymbolsExports &&
                    (linkerOutput == LinkerOutput.Executable || linkerOutput == LinkerOutput.SharedLibrary))
                {
                    buildOptions.LinkEnv.Output = LinkerOutput.ImportLibrary;
                    buildData.Toolchain.LinkFiles(buildData.Graph, buildOptions, Path.ChangeExtension(outputPath, buildData.Toolchain.Platform.StaticLibraryFileExtension));
                    buildOptions.LinkEnv.Output = linkerOutput;
                }
            }
        }

        private static IGrouping<string, Module>[] GetBinaryModules(ProjectInfo project, Target target, Dictionary<Module, BuildOptions> buildModules)
        {
            var modules = new List<Module>();
            switch (target.LinkType)
            {
            case TargetLinkType.Monolithic:
                // Include all modules
                modules.AddRange(buildModules.Keys);
                break;
            case TargetLinkType.Modular:
            {
                // Include all modules from the project that contains this target
                var sourcePath = Path.Combine(project.ProjectFolderPath, "Source");
                foreach (var module in buildModules.Keys)
                {
                    if (module.FolderPath.StartsWith(sourcePath))
                        modules.Add(module);
                }
                break;
            }
            default: throw new ArgumentOutOfRangeException();
            }
            modules.RemoveAll(x => x == null || string.IsNullOrEmpty(x.BinaryModuleName));
            return modules.GroupBy(x => x.BinaryModuleName).ToArray();
        }

        private static BuildOptions BuildModule(BuildData buildData, Module module)
        {
            if (buildData.Modules.TryGetValue(module, out var moduleOptions))
            {
                using (new ProfileEventScope(module.Name))
                {
                    Log.Verbose(string.Format("Building module {0}", module.Name));
                    BuildModuleInner(buildData, module, moduleOptions);
                }
            }
            return moduleOptions;
        }

        internal static void BuildModuleInner(BuildData buildData, Module module, BuildOptions moduleOptions, bool withApi = true)
        {
            // Inherit build environment from dependent modules
            foreach (var moduleName in moduleOptions.PrivateDependencies)
            {
                var dependencyModule = buildData.Rules.GetModule(moduleName);
                if (dependencyModule != null && buildData.Modules.TryGetValue(dependencyModule, out var dependencyOptions))
                {
                    moduleOptions.LinkEnv.InputFiles.AddRange(dependencyOptions.OutputFiles);
                    moduleOptions.DependencyFiles.AddRange(dependencyOptions.DependencyFiles);
                    moduleOptions.OptionalDependencyFiles.AddRange(dependencyOptions.OptionalDependencyFiles);
                    moduleOptions.NugetPackageReferences.AddRange(dependencyOptions.NugetPackageReferences);
                    moduleOptions.PrivateIncludePaths.AddRange(dependencyOptions.PublicIncludePaths);
                    moduleOptions.Libraries.AddRange(dependencyOptions.Libraries);
                    moduleOptions.DelayLoadLibraries.AddRange(dependencyOptions.DelayLoadLibraries);
                    moduleOptions.ScriptingAPI.Add(dependencyOptions.ScriptingAPI);
                    moduleOptions.ExternalModules.AddRange(dependencyOptions.ExternalModules);
                }
            }
            foreach (var moduleName in moduleOptions.PublicDependencies)
            {
                var dependencyModule = buildData.Rules.GetModule(moduleName);
                if (dependencyModule != null && buildData.Modules.TryGetValue(dependencyModule, out var dependencyOptions))
                {
                    moduleOptions.LinkEnv.InputFiles.AddRange(dependencyOptions.OutputFiles);
                    moduleOptions.DependencyFiles.AddRange(dependencyOptions.DependencyFiles);
                    moduleOptions.OptionalDependencyFiles.AddRange(dependencyOptions.OptionalDependencyFiles);
                    moduleOptions.NugetPackageReferences.AddRange(dependencyOptions.NugetPackageReferences);
                    moduleOptions.PublicIncludePaths.AddRange(dependencyOptions.PublicIncludePaths);
                    moduleOptions.Libraries.AddRange(dependencyOptions.Libraries);
                    moduleOptions.DelayLoadLibraries.AddRange(dependencyOptions.DelayLoadLibraries);
                    moduleOptions.ScriptingAPI.Add(dependencyOptions.ScriptingAPI);
                    moduleOptions.ExternalModules.AddRange(dependencyOptions.ExternalModules);
                }
            }

            // Special case some module types
            if (module is DepsModule || module is HeaderOnlyModule)
            {
                // Skip build
            }
            else
            {
                // Setup actual build environment
                module.SetupEnvironment(moduleOptions);
                moduleOptions.MergeSourcePathsIntoSourceFiles();

                // Skip build for C#-only modules
                if (!module.BuildNativeCode)
                    return;

                // Skip build for pre-built targets
                if (buildData.Target.IsPreBuilt)
                {
                    // Setup output file for further linking by referencing targets
                    if (buildData.Target.LinkType != TargetLinkType.Monolithic)
                    {
                        var outputLib = Path.Combine(buildData.TargetOptions.OutputFolder, buildData.Toolchain.Platform.GetLinkOutputFileName(module.Name, moduleOptions.LinkEnv.Output));
                        if (moduleOptions.LinkEnv.Output == LinkerOutput.Executable || moduleOptions.LinkEnv.Output == LinkerOutput.SharedLibrary)
                            moduleOptions.OutputFiles.Add(Path.ChangeExtension(outputLib, buildData.Platform.StaticLibraryFileExtension));
                        else
                            moduleOptions.Libraries.Add(outputLib);
                    }
                    return;
                }

                // Collect all files to compile
                var cppFiles = new List<string>(moduleOptions.SourceFiles.Count / 2);
                for (int i = 0; i < moduleOptions.SourceFiles.Count; i++)
                {
                    if (moduleOptions.SourceFiles[i].EndsWith(".cpp", StringComparison.OrdinalIgnoreCase))
                        cppFiles.Add(moduleOptions.SourceFiles[i]);
                }

                if (!string.IsNullOrEmpty(module.BinaryModuleName) && withApi)
                {
                    // Generate scripting bindings
                    using (new ProfileEventScope("GenerateBindings"))
                    {
                        BindingsGenerator.GenerateBindings(buildData, module, ref moduleOptions, out var bindings);
                        if (bindings.UseBindings)
                        {
                            // Compile C++ file with bindings
                            cppFiles.Add(bindings.GeneratedCppFilePath);
                        }
                    }

                    // Compile C++ file with binary module (only for first module in the binary module to prevent multiple implementations)
                    if (buildData.BinaryModules.FirstOrDefault(x => x.Key == module.BinaryModuleName)?.First(x => !(x is DepsModule || x is HeaderOnlyModule)) == module)
                    {
                        var project = GetModuleProject(module, buildData);
                        var binaryModuleSourcePath = Path.Combine(project.ProjectFolderPath, "Source", module.BinaryModuleName + ".Gen.cpp");
                        if (!cppFiles.Contains(binaryModuleSourcePath) && File.Exists(binaryModuleSourcePath))
                            cppFiles.Add(binaryModuleSourcePath);
                    }
                }

                // If the PCH was already created (eg. by other engine module) then simply reuse the same file
                if (moduleOptions.CompileEnv.PrecompiledHeaderUsage == PrecompiledHeaderFileUsage.CreateManual && buildData.PrecompiledHeaderFiles.TryGetValue(moduleOptions.CompileEnv.PrecompiledHeaderSource, out var pch))
                {
                    moduleOptions.CompileEnv.PrecompiledHeaderUsage = PrecompiledHeaderFileUsage.UseManual;
                    moduleOptions.CompileEnv.PrecompiledHeaderFile =  pch;
                }

                // Compile all source files
                var compilationOutput = buildData.Toolchain.CompileCppFiles(buildData.Graph, moduleOptions, cppFiles, moduleOptions.OutputFolder);
                foreach (var e in compilationOutput.ObjectFiles)
                    moduleOptions.LinkEnv.InputFiles.Add(e);
                if (buildData.TargetOptions.LinkEnv.GenerateDocumentation)
                {
                    // TODO: find better way to add generated doc files to the target linker (module exports the output doc files?)
                    buildData.TargetOptions.LinkEnv.DocumentationFiles.AddRange(compilationOutput.DocumentationFiles);
                }
                if (moduleOptions.CompileEnv.PrecompiledHeaderUsage == PrecompiledHeaderFileUsage.CreateManual && !string.IsNullOrEmpty(compilationOutput.PrecompiledHeaderFile))
                {
                    // Cache PCH file to be used by other modules that reference the same file
                    buildData.PrecompiledHeaderFiles.Add(moduleOptions.CompileEnv.PrecompiledHeaderSource, compilationOutput.PrecompiledHeaderFile);
                }

                if (buildData.Target.LinkType != TargetLinkType.Monolithic)
                {
                    // Use the library includes required by this module
                    moduleOptions.LinkEnv.InputLibraries.AddRange(moduleOptions.Libraries);

                    // Link all object files into module library
                    var outputLib = Path.Combine(buildData.TargetOptions.OutputFolder, buildData.Platform.GetLinkOutputFileName(module.Name + moduleOptions.HotReloadPostfix, moduleOptions.LinkEnv.Output));
                    LinkNativeBinary(buildData, moduleOptions, outputLib);

                    // Expose the import library to modules that include this module
                    if (moduleOptions.LinkEnv.Output == LinkerOutput.Executable || moduleOptions.LinkEnv.Output == LinkerOutput.SharedLibrary)
                    {
                        if (buildData.Toolchain.UseImportLibraryWhenLinking)
                            moduleOptions.OutputFiles.Add(Path.ChangeExtension(outputLib, buildData.Platform.StaticLibraryFileExtension));
                        else
                            moduleOptions.Libraries.Add(outputLib);
                    }
                }
                else
                {
                    // Use direct linking of the module object files into the target
                    moduleOptions.OutputFiles.AddRange(compilationOutput.ObjectFiles);

                    // Forward the library includes required by this module
                    moduleOptions.OutputFiles.AddRange(moduleOptions.LinkEnv.InputFiles);
                }
            }
        }

        internal static void BuildModuleInnerBindingsOnly(BuildData buildData, Module module, BuildOptions moduleOptions)
        {
            // Inherit build environment from dependent modules
            foreach (var moduleName in moduleOptions.PrivateDependencies)
            {
                var dependencyModule = buildData.Rules.GetModule(moduleName);
                if (dependencyModule != null && buildData.Modules.TryGetValue(dependencyModule, out var dependencyOptions))
                {
                    moduleOptions.ScriptingAPI.Add(dependencyOptions.ScriptingAPI);
                    moduleOptions.ExternalModules.AddRange(dependencyOptions.ExternalModules);
                }
            }
            foreach (var moduleName in moduleOptions.PublicDependencies)
            {
                var dependencyModule = buildData.Rules.GetModule(moduleName);
                if (dependencyModule != null && buildData.Modules.TryGetValue(dependencyModule, out var dependencyOptions))
                {
                    moduleOptions.ScriptingAPI.Add(dependencyOptions.ScriptingAPI);
                    moduleOptions.ExternalModules.AddRange(dependencyOptions.ExternalModules);
                }
            }

            // Special case some module types
            if (module is DepsModule || module is HeaderOnlyModule)
            {
                // Skip build
            }
            else
            {
                // Setup actual build environment
                module.SetupEnvironment(moduleOptions);
                moduleOptions.MergeSourcePathsIntoSourceFiles();

                // Skip build for C#-only modules
                if (!module.BuildNativeCode)
                    return;

                // Skip build for pre-built targets
                if (buildData.Target.IsPreBuilt)
                    return;

                if (!string.IsNullOrEmpty(module.BinaryModuleName))
                {
                    // Generate scripting bindings
                    using (new ProfileEventScope("GenerateBindings"))
                    {
                        BindingsGenerator.GenerateBindings(buildData, module, ref moduleOptions, out _);
                    }
                }
            }
        }

        private static void BuildTargetReferenceNativeCpp(Dictionary<Target, BuildData> buildContext, BuildData buildData, ProjectInfo.Reference reference)
        {
            // Find the target from that project that is being referenced
            // Note: projects reference each other but build system uses targets to produce binaries (project can have multiple targets)
            var projectTargets = GetProjectTargets(reference.Project);
            var target = buildData.Target.SelectReferencedTarget(reference.Project, projectTargets);
            if (target == null)
            {
                Log.Verbose("No target selected for build");
                return;
            }
            if (!target.Platforms.Contains(buildData.Platform.Target) || !target.Architectures.Contains(buildData.Architecture))
            {
                Log.Verbose($"Referenced target {reference.Project.Name} doesn't support {buildData.Platform.Target} {buildData.Architecture}");
                return;
            }
            if (!buildContext.TryGetValue(target, out var referencedBuildData))
            {
                Log.Info($"Building referenced target {reference.Project.Name}");

                // Build target
                // Note: use build environment local to the referenced project workspace (separate build cache, separate task graph)
                var graph = new TaskGraph(reference.Project.ProjectFolderPath);
                var skipBuild = target.IsPreBuilt || (Configuration.SkipTargets != null && Configuration.SkipTargets.Contains(target.Name));
                target.PreBuild();
                referencedBuildData = BuildTargetNativeCpp(buildData.Rules, graph, target, buildContext, buildData.Toolchain, buildData.Configuration, true, skipBuild);
                if (!skipBuild)
                {
                    using (new ProfileEventScope("PrepareTasks"))
                    {
                        using (new ProfileEventScope("Setup"))
                            graph.Setup();

                        using (new ProfileEventScope("SortTasks"))
                            graph.SortTasks();

                        using (new ProfileEventScope("LoadCache"))
                            graph.LoadCache();
                    }
                    int executedTasksCount;
                    bool failed;
                    using (new ProfileEventScope("ExecuteTasks"))
                    {
                        failed = graph.Execute(out executedTasksCount);
                    }
                    if (executedTasksCount != 0)
                    {
                        using (new ProfileEventScope("SaveCache"))
                        {
                            graph.SaveCache();
                        }
                    }
                    if (failed)
                        throw new BuildException($"Failed to build target {target.Name}. See log.");
                }
                else
                {
                    Log.Verbose($"Skipping build for target {target.Name}");
                }
                target.PostBuild();
            }

            // Cache build data to be used in calling target build as reference
            buildData.ReferenceBuilds.Add(reference.Project, referencedBuildData);
        }

        private static void BuildTargetReferenceNativeCppBindingsOnly(Dictionary<Target, BuildData> buildContext, BuildData buildData, ProjectInfo.Reference reference)
        {
            // Find the target from that project that is being referenced
            // Note: projects reference each other but build system uses targets to produce binaries (project can have multiple targets)
            var projectTargets = GetProjectTargets(reference.Project);
            var target = buildData.Target.SelectReferencedTarget(reference.Project, projectTargets);
            if (target == null)
            {
                Log.Verbose("No target selected for build");
                return;
            }
            if (!buildContext.TryGetValue(target, out var referencedBuildData))
            {
                Log.Info($"Building referenced target {reference.Project.Name}");

                // Build target
                // Note: use build environment local to the referenced project workspace (separate build cache, separate task graph)
                var graph = new TaskGraph(reference.Project.ProjectFolderPath);
                var skipBuild = target.IsPreBuilt || (Configuration.SkipTargets != null && Configuration.SkipTargets.Contains(target.Name));
                target.PreBuild();
                referencedBuildData = BuildTargetNativeCppBindingsOnly(buildData.Rules, graph, target, buildContext, buildData.Platform, buildData.Architecture, buildData.Configuration, skipBuild);
                if (!skipBuild)
                {
                    using (new ProfileEventScope("PrepareTasks"))
                    {
                        using (new ProfileEventScope("Setup"))
                            graph.Setup();

                        using (new ProfileEventScope("SortTasks"))
                            graph.SortTasks();

                        using (new ProfileEventScope("LoadCache"))
                            graph.LoadCache();
                    }
                    int executedTasksCount;
                    bool failed;
                    using (new ProfileEventScope("ExecuteTasks"))
                    {
                        failed = graph.Execute(out executedTasksCount);
                    }
                    if (executedTasksCount != 0)
                    {
                        using (new ProfileEventScope("SaveCache"))
                        {
                            graph.SaveCache();
                        }
                    }
                    if (failed)
                        throw new BuildException($"Failed to build target {target.Name}. See log.");
                }
                else
                {
                    Log.Verbose($"Skipping build for target {target.Name}");
                }
                target.PostBuild();
            }

            // Cache build data to be used in calling target build as reference
            buildData.ReferenceBuilds.Add(reference.Project, referencedBuildData);
        }

        private static BuildData BuildTargetNativeCpp(RulesAssembly rules, TaskGraph graph, Target target, Dictionary<Target, BuildData> buildContext, Toolchain toolchain, TargetConfiguration configuration, bool isBuildingReference = false, bool skipBuild = false)
        {
            if (buildContext.TryGetValue(target, out var buildData))
                return buildData;

            // Warn if target has no valid modules
            if (target.Modules.Count == 0)
                Log.Warning(string.Format("Target {0} has no modules to build", target.Name));

            // Pick a project
            var project = Globals.Project;
            if (target is ProjectTarget projectTarget)
                project = projectTarget.Project;
            if (project == null)
                throw new Exception($"Cannot build target {target.Name}. The project file is missing (.flaxproj located in the folder above).");

            // Setup build environment for the target
            var targetBuildOptions = GetBuildOptions(target, toolchain.Platform, toolchain, toolchain.Architecture, configuration, project.ProjectFolderPath, skipBuild ? string.Empty : Configuration.HotReloadPostfix);
            if (!isBuildingReference)
                toolchain.LogInfo();

            using (new ProfileEventScope("PreBuild"))
            {
                // Pre build
                toolchain.PreBuild(graph, targetBuildOptions);
                target.PreBuild(graph, targetBuildOptions);
                PreBuild?.Invoke(graph, targetBuildOptions);

                // Ensure that target build directories exist
                if (!target.IsPreBuilt && !Directory.Exists(targetBuildOptions.IntermediateFolder))
                    Directory.CreateDirectory(targetBuildOptions.IntermediateFolder);
                if (!target.IsPreBuilt && !Directory.Exists(targetBuildOptions.OutputFolder))
                    Directory.CreateDirectory(targetBuildOptions.OutputFolder);
            }

            // Setup building common data container
            buildData = new BuildData
            {
                Project = project,
                Graph = graph,
                Rules = rules,
                Target = target,
                TargetOptions = targetBuildOptions,
                Platform = toolchain.Platform,
                Toolchain = toolchain,
                Architecture = toolchain.Architecture,
                Configuration = configuration,
            };
            buildContext.Add(target, buildData);

            // Firstly build all referenced projects (the current targets depends on the referenced projects binaries)
            if (buildData.Target.LinkType == TargetLinkType.Modular)
            {
                using (new ProfileEventScope("References"))
                {
                    foreach (var reference in buildData.Project.References)
                    {
                        using (new ProfileEventScope(reference.Project.Name))
                        {
                            BuildTargetReferenceNativeCpp(buildContext, buildData, reference);
                        }
                    }
                }
            }

            // Collect all modules
            using (new ProfileEventScope("CollectModules"))
            {
                foreach (var moduleName in target.Modules)
                {
                    var module = rules.GetModule(moduleName);
                    if (module != null)
                    {
                        CollectModules(buildData, module, true);
                    }
                    else
                    {
                        Log.Warning(string.Format("Missing module {0} (or invalid name specified)", moduleName));
                    }
                }
            }

            using (new ProfileEventScope("SetupBinaryModules"))
            {
                // Collect all binary modules to include into target
                buildData.BinaryModules = GetBinaryModules(project, target, buildData.Modules);

                // Inject binary modules symbols import/export defines
                var moduleNamesUsed = new HashSet<string>();
                for (int i = 0; i < buildData.BinaryModules.Length; i++)
                {
                    var binaryModule = buildData.BinaryModules[i];
                    var binaryModuleNameUpper = binaryModule.Key.ToUpperInvariant();
                    foreach (var module in binaryModule)
                    {
                        if (buildData.Modules.TryGetValue(module, out var moduleOptions))
                        {
                            if (buildData.Target.LinkType == TargetLinkType.Modular)
                            {
                                // Export symbols from binary module
                                moduleOptions.CompileEnv.PreprocessorDefinitions.Add(binaryModuleNameUpper + (target.UseSymbolsExports ? "_API=" + toolchain.DllExport : "_API="));

                                // Import symbols from binary modules containing the referenced modules (from this project only, external ones are handled via ReferenceBuilds below)
                                foreach (var moduleName in moduleOptions.PrivateDependencies.Concat(moduleOptions.PublicDependencies))
                                {
                                    var dependencyModule = buildData.Rules.GetModule(moduleName);
                                    if (dependencyModule != null &&
                                        !string.IsNullOrEmpty(dependencyModule.BinaryModuleName) &&
                                        dependencyModule.BinaryModuleName != binaryModule.Key &&
                                        !moduleNamesUsed.Contains(dependencyModule.BinaryModuleName) &&
                                        GetModuleProject(dependencyModule, project) != null &&
                                        buildData.Modules.TryGetValue(dependencyModule, out var dependencyOptions))
                                    {
                                        // Import symbols from referenced binary module
                                        moduleOptions.CompileEnv.PreprocessorDefinitions.Add(dependencyModule.BinaryModuleName.ToUpperInvariant() + "_API=" + toolchain.DllImport);

                                        var dependencyModuleBuild = buildData.FinReferenceBuildModule(moduleName);
                                        if (dependencyModuleBuild != null && !string.IsNullOrEmpty(dependencyModuleBuild.NativePath))
                                        {
                                            // Link against the referenced binary module
                                            if (toolchain.UseImportLibraryWhenLinking)
                                                moduleOptions.LinkEnv.InputLibraries.Add(Path.ChangeExtension(dependencyModuleBuild.NativePath, toolchain.Platform.StaticLibraryFileExtension));
                                            else
                                                moduleOptions.LinkEnv.InputLibraries.Add(dependencyModuleBuild.NativePath);
                                        }
                                    }
                                }
                            }
                            else
                            {
                                // Export symbols from all binary modules in the build
                                foreach (var q in buildData.BinaryModules)
                                {
                                    moduleOptions.CompileEnv.PreprocessorDefinitions.Add(q.Key.ToUpperInvariant() + (target.UseSymbolsExports ? "_API=" + toolchain.DllExport : "_API="));
                                }
                            }

                            foreach (var e in buildData.ReferenceBuilds)
                            {
                                foreach (var q in e.Value.BuildInfo.BinaryModules)
                                {
                                    if (!string.IsNullOrEmpty(q.NativePath))
                                    {
                                        if (buildData.Target.LinkType == TargetLinkType.Modular)
                                        {
                                            // Import symbols from referenced binary module
                                            moduleOptions.CompileEnv.PreprocessorDefinitions.Add(q.Name.ToUpperInvariant() + "_API=" + toolchain.DllImport);

                                            // Link against the referenced binary module
                                            if (toolchain.UseImportLibraryWhenLinking)
                                                moduleOptions.LinkEnv.InputLibraries.Add(Path.ChangeExtension(q.NativePath, toolchain.Platform.StaticLibraryFileExtension));
                                            else
                                                moduleOptions.LinkEnv.InputLibraries.Add(q.NativePath);
                                        }
                                        else if (target.UseSymbolsExports)
                                        {
                                            // Export symbols from referenced binary module to be visible further
                                            moduleOptions.CompileEnv.PreprocessorDefinitions.Add(q.Name.ToUpperInvariant() + "_API=" + toolchain.DllExport);
                                        }
                                        else
                                        {
                                            // Skip symbols from referenced binary module (module code injected into monolithic build)
                                            moduleOptions.CompileEnv.PreprocessorDefinitions.Add(q.Name.ToUpperInvariant() + "_API=");
                                        }
                                    }
                                }
                            }
                        }
                    }
                    moduleNamesUsed.Clear();
                }
            }

            // Generate code for binary modules included in the target
            using (new ProfileEventScope("GenerateBindings"))
            {
                if (!target.IsPreBuilt)
                {
                    BindingsGenerator.GenerateBindings(buildData);
                }
            }

            // Build all modules from target binary modules but in order of collecting (from independent to more dependant ones)
            using (new ProfileEventScope("BuildModules"))
            {
                foreach (var module in buildData.ModulesOrderList)
                {
                    if (buildData.BinaryModules.Any(x => x.Contains(module)))
                    {
                        var moduleOptions = BuildModule(buildData, module);

                        // Merge module into target environment
                        buildData.TargetOptions.LinkEnv.InputFiles.AddRange(moduleOptions.OutputFiles);
                        buildData.TargetOptions.DependencyFiles.AddRange(moduleOptions.DependencyFiles);
                        buildData.TargetOptions.OptionalDependencyFiles.AddRange(moduleOptions.OptionalDependencyFiles);
                        buildData.TargetOptions.NugetPackageReferences.AddRange(moduleOptions.NugetPackageReferences);
                        buildData.TargetOptions.Libraries.AddRange(moduleOptions.Libraries);
                        buildData.TargetOptions.DelayLoadLibraries.AddRange(moduleOptions.DelayLoadLibraries);
                        buildData.TargetOptions.ScriptingAPI.Add(moduleOptions.ScriptingAPI);
                        buildData.TargetOptions.ExternalModules.AddRange(moduleOptions.ExternalModules);
                    }
                }
            }

            // Build scripting API bindings
            OnBuildBindings(buildData, graph);

            // Link modules into a target
            var outputTargetFilePath = target.GetOutputFilePath(targetBuildOptions);
            var outputPath = Path.GetDirectoryName(outputTargetFilePath);
            switch (target.LinkType)
            {
            case TargetLinkType.Monolithic:
            {
                if (!buildData.Target.IsPreBuilt)
                    LinkNativeBinary(buildData, targetBuildOptions, outputTargetFilePath);
                break;
            }
            }

            // Generate target build output info
            using (new ProfileEventScope("GenerateBuildInfo"))
            {
                buildData.BuildInfo = new BuildTargetInfo
                {
                    Name = target.Name,
                    Platform = toolchain.Platform.Target.ToString(),
                    Architecture = toolchain.Architecture.ToString(),
                    Configuration = configuration.ToString(),
                    HotReloadPostfix = targetBuildOptions.HotReloadPostfix,
                    BinaryModules = new BuildTargetBinaryModuleInfo[buildData.BinaryModules.Length + targetBuildOptions.ExternalModules.Count],
                    References = new BuildTargetReferenceInfo[buildData.ReferenceBuilds.Count],
                };
                int i;

                // Binary modules
                for (i = 0; i < buildData.BinaryModules.Length; i++)
                {
                    var binaryModule = buildData.BinaryModules[i];
                    var binaryModuleInfo = new BuildTargetBinaryModuleInfo
                    {
                        Name = binaryModule.Key,
                        ManagedPath = EngineConfiguration.WithCSharp(targetBuildOptions) ? Path.Combine(outputPath, binaryModule.Key + ".CSharp.dll") : string.Empty,
                    };
                    switch (target.LinkType)
                    {
                    case TargetLinkType.Monolithic:
                    {
                        if (binaryModule.Any(x => x.BuildNativeCode))
                        {
                            // Target merges all modules into a one native binary
                            binaryModuleInfo.NativePath = outputTargetFilePath;
                        }
                        else
                        {
                            // C#-only binary module
                            binaryModuleInfo.NativePath = string.Empty;
                        }
                        if (!binaryModule.Any(x => x.BuildCSharp))
                        {
                            // Skip C#
                            binaryModuleInfo.ManagedPath = string.Empty;
                        }
                        break;
                    }
                    case TargetLinkType.Modular:
                    {
                        // Every module produces own set of binaries
                        if (binaryModule.Count() != 1)
                            throw new Exception("Cannot output binary if it uses multiple modules.");
                        var module = binaryModule.First();
                        if (module.BuildNativeCode)
                        {
                            var moduleOptions = buildData.Modules[module];
                            var outputLib = Path.Combine(buildData.TargetOptions.OutputFolder, buildData.Platform.GetLinkOutputFileName(module.Name + moduleOptions.HotReloadPostfix, moduleOptions.LinkEnv.Output));
                            binaryModuleInfo.NativePath = outputLib;
                        }
                        else
                        {
                            // C#-only binary module
                            binaryModuleInfo.NativePath = string.Empty;
                        }
                        if (!module.BuildCSharp)
                        {
                            // Skip C#
                            binaryModuleInfo.ManagedPath = string.Empty;
                        }
                        break;
                    }
                    default: throw new ArgumentOutOfRangeException();
                    }

                    binaryModuleInfo.NativePathProcessed = BuildTargetInfo.ProcessPath(binaryModuleInfo.NativePath, project.ProjectFolderPath);
                    binaryModuleInfo.ManagedPathProcessed = BuildTargetInfo.ProcessPath(binaryModuleInfo.ManagedPath, project.ProjectFolderPath);

                    buildData.BuildInfo.BinaryModules[i] = binaryModuleInfo;
                }

                buildData.BuildInfo.AddExternalBinaryModules(ref i, project.ProjectFolderPath, targetBuildOptions.ExternalModules);
                i = 0;
                buildData.BuildInfo.AddReferencedBuilds(ref i, project.ProjectFolderPath, buildData.ReferenceBuilds);

                if (!buildData.Target.IsPreBuilt)
                    Utilities.WriteFileIfChanged(Path.Combine(outputPath, target.Name + ".Build.json"), buildData.BuildInfo.Serialize());
            }

            // Deploy files
            if (!buildData.Target.IsPreBuilt)
            {
                using (new ProfileEventScope("DeployFiles"))
                {
                    foreach (var srcFile in targetBuildOptions.OptionalDependencyFiles.Where(File.Exists).Union(targetBuildOptions.DependencyFiles))
                    {
                        var dstFile = Path.Combine(outputPath, Path.GetFileName(srcFile));
                        graph.AddCopyFile(dstFile, srcFile);
                    }

                    if (targetBuildOptions.NugetPackageReferences.Any())
                    {
                        var nugetPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), ".nuget", "packages");
                        foreach (var reference in targetBuildOptions.NugetPackageReferences)
                        {
                            var path = Path.Combine(nugetPath, reference.Name, reference.Version, "lib", reference.Framework, $"{reference.Name}.dll");
                            if (!File.Exists(path))
                                Utilities.RestoreNugetPackages(graph, target);
                            var dstFile = Path.Combine(outputPath, Path.GetFileName(path));
                            graph.AddCopyFile(dstFile, path);
                        }
                    }
                }
            }

            using (new ProfileEventScope("PostBuild"))
            {
                // Post build
                PostBuild?.Invoke(graph, targetBuildOptions);
                target.PostBuild(graph, targetBuildOptions);
                toolchain.PostBuild(graph, targetBuildOptions);
            }

            return buildData;
        }

        private static BuildData BuildTargetNativeCppBindingsOnly(RulesAssembly rules, TaskGraph graph, Target target, Dictionary<Target, BuildData> buildContext, Platform platform, TargetArchitecture architecture, TargetConfiguration configuration, bool skipBuild = false)
        {
            if (buildContext.TryGetValue(target, out var buildData))
                return buildData;

            // Warn if target has no valid modules
            if (target.Modules.Count == 0)
                Log.Warning(string.Format("Target {0} has no modules to build", target.Name));
            // Pick a project
            var project = Globals.Project;
            if (target is ProjectTarget projectTarget)
                project = projectTarget.Project;
            if (project == null)
                throw new Exception($"Cannot build target {target.Name}. The project file is missing (.flaxproj located in the folder above).");

            // Setup build environment for the target
            var targetBuildOptions = GetBuildOptions(target, platform, platform.TryGetToolchain(architecture), architecture, configuration, project.ProjectFolderPath, skipBuild ? string.Empty : Configuration.HotReloadPostfix);

            using (new ProfileEventScope("PreBuild"))
            {
                // Pre build
                target.PreBuild(graph, targetBuildOptions);

                // Ensure that target build directories exist
                if (!target.IsPreBuilt && !Directory.Exists(targetBuildOptions.IntermediateFolder))
                    Directory.CreateDirectory(targetBuildOptions.IntermediateFolder);
                if (!target.IsPreBuilt && !Directory.Exists(targetBuildOptions.OutputFolder))
                    Directory.CreateDirectory(targetBuildOptions.OutputFolder);
            }

            // Setup building common data container
            buildData = new BuildData
            {
                Project = project,
                Graph = graph,
                Rules = rules,
                Target = target,
                TargetOptions = targetBuildOptions,
                Platform = platform,
                Toolchain = targetBuildOptions.Toolchain,
                Architecture = architecture,
                Configuration = configuration,
            };
            buildContext.Add(target, buildData);

            // Firstly build all referenced projects (the current targets depends on the referenced projects binaries)
            if (buildData.Target.LinkType == TargetLinkType.Modular)
            {
                using (new ProfileEventScope("References"))
                {
                    foreach (var reference in buildData.Project.References)
                    {
                        using (new ProfileEventScope(reference.Project.Name))
                        {
                            if (Configuration.BuildBindingsOnly || reference.Project.IsCSharpOnlyProject || !platform.HasRequiredSDKsInstalled)
                            {
                                BuildTargetReferenceNativeCppBindingsOnly(buildContext, buildData, reference);
                            }
                            else
                            {
                                if (buildData.Toolchain == null)
                                    buildData.Toolchain = platform.GetToolchain(architecture);
                                BuildTargetReferenceNativeCpp(buildContext, buildData, reference);
                            }
                        }
                    }
                }
            }

            // Collect all modules
            using (new ProfileEventScope("CollectModules"))
            {
                foreach (var moduleName in target.Modules)
                {
                    var module = rules.GetModule(moduleName);
                    if (module != null)
                    {
                        CollectModules(buildData, module, true);
                    }
                    else
                    {
                        Log.Warning(string.Format("Missing module {0} (or invalid name specified)", moduleName));
                    }
                }
            }

            using (new ProfileEventScope("SetupBinaryModules"))
            {
                // Collect all binary modules to include into target
                buildData.BinaryModules = GetBinaryModules(project, target, buildData.Modules);
            }

            // Generate code for binary modules included in the target
            using (new ProfileEventScope("GenerateBindings"))
            {
                if (!target.IsPreBuilt)
                {
                    BindingsGenerator.GenerateBindings(buildData);
                }
            }

            // Build all modules from target binary modules but in order of collecting (from independent to more dependant ones)
            using (new ProfileEventScope("BuildModules"))
            {
                foreach (var module in buildData.ModulesOrderList)
                {
                    if (buildData.BinaryModules.Any(x => x.Contains(module)))
                    {
                        if (buildData.Modules.TryGetValue(module, out var moduleOptions))
                        {
                            if (!target.IsPreBuilt)
                            {
                                using (new ProfileEventScope(module.Name))
                                {
                                    Log.Verbose(string.Format("Building module {0}", module.Name));
                                    BuildModuleInnerBindingsOnly(buildData, module, moduleOptions);
                                }
                            }

                            // Merge module into target environment
                            buildData.TargetOptions.ScriptingAPI.Add(moduleOptions.ScriptingAPI);
                            buildData.TargetOptions.ExternalModules.AddRange(moduleOptions.ExternalModules);
                            buildData.TargetOptions.DependencyFiles.AddRange(moduleOptions.DependencyFiles);
                            buildData.TargetOptions.OptionalDependencyFiles.AddRange(moduleOptions.OptionalDependencyFiles);
                            buildData.TargetOptions.NugetPackageReferences.AddRange(moduleOptions.NugetPackageReferences);
                        }
                    }
                }
            }

            // Build scripting API bindings
            OnBuildBindings(buildData, graph);

            // Link modules into a target
            var outputTargetFilePath = target.GetOutputFilePath(targetBuildOptions);
            var outputPath = Path.GetDirectoryName(outputTargetFilePath);

            // Generate target build output info
            using (new ProfileEventScope("GenerateBuildInfo"))
            {
                buildData.BuildInfo = new BuildTargetInfo
                {
                    Name = target.Name,
                    Platform = platform.Target.ToString(),
                    Architecture = architecture.ToString(),
                    Configuration = configuration.ToString(),
                    HotReloadPostfix = targetBuildOptions.HotReloadPostfix,
                    BinaryModules = new BuildTargetBinaryModuleInfo[buildData.BinaryModules.Length + targetBuildOptions.ExternalModules.Count],
                    References = new BuildTargetReferenceInfo[buildData.ReferenceBuilds.Count],
                };
                int i;

                // Binary modules
                for (i = 0; i < buildData.BuildInfo.BinaryModules.Length; i++)
                {
                    var binaryModule = buildData.BinaryModules[i];
                    var binaryModuleInfo = new BuildTargetBinaryModuleInfo
                    {
                        Name = binaryModule.Key,
                        ManagedPath = EngineConfiguration.WithCSharp(targetBuildOptions) ? Path.Combine(outputPath, binaryModule.Key + ".CSharp.dll") : string.Empty,
                    };

                    binaryModuleInfo.NativePathProcessed = string.Empty;
                    binaryModuleInfo.ManagedPathProcessed = BuildTargetInfo.ProcessPath(binaryModuleInfo.ManagedPath, project.ProjectFolderPath);

                    buildData.BuildInfo.BinaryModules[i] = binaryModuleInfo;
                }

                buildData.BuildInfo.AddExternalBinaryModules(ref i, project.ProjectFolderPath, targetBuildOptions.ExternalModules);
                i = 0;
                buildData.BuildInfo.AddReferencedBuilds(ref i, project.ProjectFolderPath, buildData.ReferenceBuilds);

                if (!buildData.Target.IsPreBuilt)
                    Utilities.WriteFileIfChanged(Path.Combine(outputPath, target.Name + ".Build.json"), buildData.BuildInfo.Serialize());
            }

            // Deploy files
            if (!buildData.Target.IsPreBuilt)
            {
                using (new ProfileEventScope("DeployFiles"))
                {
                    foreach (var srcFile in targetBuildOptions.OptionalDependencyFiles.Where(File.Exists).Union(targetBuildOptions.DependencyFiles))
                    {
                        var dstFile = Path.Combine(outputPath, Path.GetFileName(srcFile));
                        graph.AddCopyFile(dstFile, srcFile);
                    }

                    if (targetBuildOptions.NugetPackageReferences.Any())
                    {
                        var nugetPath = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), ".nuget", "packages");
                        foreach (var reference in targetBuildOptions.NugetPackageReferences)
                        {
                            var path = Path.Combine(nugetPath, reference.Name, reference.Version, "lib", reference.Framework, $"{reference.Name}.dll");
                            if (!File.Exists(path))
                                Utilities.RestoreNugetPackages(graph, target);
                            var dstFile = Path.Combine(outputPath, Path.GetFileName(path));
                            graph.AddCopyFile(dstFile, path);
                        }
                    }
                }
            }

            using (new ProfileEventScope("PostBuild"))
            {
                // Post build
                target.PostBuild(graph, targetBuildOptions);
            }

            return buildData;
        }
    }
}
