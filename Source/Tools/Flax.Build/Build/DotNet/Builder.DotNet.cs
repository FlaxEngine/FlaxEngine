// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;
using Flax.Build.Graph;
using Flax.Build.NativeCpp;
using Flax.Deploy;

namespace Flax.Build
{
    static partial class Builder
    {
        public static event Action<TaskGraph, BuildData, BuildOptions, Task, IGrouping<string, Module>> BuildDotNetAssembly;

        private static void BuildTargetDotNet(RulesAssembly rules, TaskGraph graph, Target target, Platform platform, TargetConfiguration configuration)
        {
            // Check if use custom project file
            if (!string.IsNullOrEmpty(target.CustomExternalProjectFilePath))
            {
                // Use msbuild to compile it
                var task = graph.Add<Task>();
                task.WorkingDirectory = Globals.Root;
                task.InfoMessage = "Building " + Path.GetFileName(target.CustomExternalProjectFilePath);
                task.Cost = 100;
                task.DisableCache = true;
                task.CommandPath = VCEnvironment.MSBuildPath;
                task.CommandArguments = $"\"{target.CustomExternalProjectFilePath}\" /m /p:BuildProjectReferences=false /t:Restore,Build /p:Configuration=\"{configuration}\" /p:RestorePackagesConfig=True /p:Platform=AnyCPU /nologo {VCEnvironment.Verbosity}";
                if (task.CommandPath.EndsWith(" msbuild"))
                {
                    // Special case when using dotnet CLI as msbuild
                    task.CommandPath = task.CommandPath.Substring(0, task.CommandPath.Length - 8);
                    task.CommandArguments = "msbuild " + task.CommandArguments;
                }
                return;
            }

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
            var targetBuildOptions = GetBuildOptions(target, platform, null, TargetArchitecture.AnyCPU, configuration, project.ProjectFolderPath);
            using (new ProfileEventScope("PreBuild"))
            {
                // Pre build
                target.PreBuild(graph, targetBuildOptions);
                PreBuild?.Invoke(graph, targetBuildOptions);

                // Ensure that target build directories exist
                if (!target.IsPreBuilt && !Directory.Exists(targetBuildOptions.IntermediateFolder))
                    Directory.CreateDirectory(targetBuildOptions.IntermediateFolder);
                if (!target.IsPreBuilt && !Directory.Exists(targetBuildOptions.OutputFolder))
                    Directory.CreateDirectory(targetBuildOptions.OutputFolder);
            }

            // Setup building common data container
            var buildData = new BuildData
            {
                Project = project,
                Graph = graph,
                Rules = rules,
                Target = target,
                TargetOptions = targetBuildOptions,
                Platform = platform,
                Architecture = TargetArchitecture.AnyCPU,
                Configuration = configuration,
            };

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

            // Build all modules from target binary modules but in order of collecting (from independent to more dependant ones)
            var sourceFiles = new List<string>();
            using (new ProfileEventScope("BuildModules"))
            {
                foreach (var module in buildData.ModulesOrderList)
                {
                    if (buildData.BinaryModules.Any(x => x.Contains(module)))
                    {
                        var moduleOptions = BuildModule(buildData, module);

                        // Get source files
                        sourceFiles.AddRange(moduleOptions.SourceFiles.Where(x => x.EndsWith(".cs")));

                        // Merge module into target environment
                        buildData.TargetOptions.LinkEnv.InputFiles.AddRange(moduleOptions.OutputFiles);
                        buildData.TargetOptions.DependencyFiles.AddRange(moduleOptions.DependencyFiles);
                        buildData.TargetOptions.OptionalDependencyFiles.AddRange(moduleOptions.OptionalDependencyFiles);
                        buildData.TargetOptions.Libraries.AddRange(moduleOptions.Libraries);
                        buildData.TargetOptions.DelayLoadLibraries.AddRange(moduleOptions.DelayLoadLibraries);
                        buildData.TargetOptions.ScriptingAPI.Add(moduleOptions.ScriptingAPI);
                        buildData.TargetOptions.ExternalModules.AddRange(moduleOptions.ExternalModules);
                    }
                }
            }

            // Build
            var outputTargetFilePath = target.GetOutputFilePath(targetBuildOptions);
            var outputPath = Path.GetDirectoryName(outputTargetFilePath);
            using (new ProfileEventScope("Build"))
            {
                // Cleanup source files
                sourceFiles.RemoveAll(x => x.EndsWith(BuildFilesPostfix));
                sourceFiles.Sort();

                // Build assembly
                BuildDotNet(graph, buildData, targetBuildOptions, target.OutputName, sourceFiles, optimizeAssembly: buildData.TargetOptions.ScriptingAPI.Optimization);
            }

            // Deploy files
            if (!target.IsPreBuilt)
            {
                using (new ProfileEventScope("DeployFiles"))
                {
                    foreach (var srcFile in targetBuildOptions.OptionalDependencyFiles.Where(File.Exists).Union(targetBuildOptions.DependencyFiles))
                    {
                        var dstFile = Path.Combine(outputPath, Path.GetFileName(srcFile));
                        graph.AddCopyFile(dstFile, srcFile);
                    }
                }
            }

            using (new ProfileEventScope("PostBuild"))
            {
                // Post build
                PostBuild?.Invoke(graph, targetBuildOptions);
                target.PostBuild(graph, targetBuildOptions);
            }
        }

        private static void BuildDotNet(TaskGraph graph, BuildData buildData, BuildOptions buildOptions, string name, List<string> sourceFiles, HashSet<string> fileReferences = null, IGrouping<string, Module> binaryModule = null, bool? optimizeAssembly = null)
        {
            // Setup build options
            var buildPlatform = Platform.BuildTargetPlatform;
            var outputPath = Path.GetDirectoryName(buildData.Target.GetOutputFilePath(buildOptions));
            var outputFile = Path.Combine(outputPath, name + ".dll");
            var outputDocFile = Path.Combine(outputPath, name + ".xml");
            var outputGeneratedFiles = Path.Combine(buildOptions.IntermediateFolder);
            string cscPath, referenceAssemblies;
#if USE_NETCORE
            var dotnetSdk = DotNetSdk.Instance;
            if (!dotnetSdk.IsValid)
                throw new DotNetSdk.MissingException();
            string dotnetPath = "dotnet", referenceAnalyzers;
            string[] runtimeVersionNameParts = dotnetSdk.RuntimeVersionName.Split('.');
            string runtimeVersionShort = runtimeVersionNameParts[0] + '.' + runtimeVersionNameParts[1];
#else
            string monoRoot, monoPath;
#endif
            switch (buildPlatform)
            {
            case TargetPlatform.Windows:
            {
#if USE_NETCORE
                dotnetPath = Path.Combine(dotnetSdk.RootPath, "dotnet.exe");
                cscPath = Path.Combine(dotnetSdk.RootPath, @$"sdk\{dotnetSdk.VersionName}\Roslyn\bincore\csc.dll");
                referenceAssemblies = Path.Combine(dotnetSdk.RootPath, @$"packs\Microsoft.NETCore.App.Ref\{dotnetSdk.RuntimeVersionName}\ref\net{runtimeVersionShort}\");
                referenceAnalyzers = Path.Combine(dotnetSdk.RootPath, @$"packs\Microsoft.NETCore.App.Ref\{dotnetSdk.RuntimeVersionName}\analyzers\dotnet\cs\");
#else
                monoRoot = Path.Combine(Globals.EngineRoot, "Source", "Platforms", "Editor", "Windows", "Mono");
                monoPath = Path.Combine(monoRoot, "bin", "mono.exe");
                cscPath = Path.Combine(Path.GetDirectoryName(VCEnvironment.MSBuildPath), "Roslyn", "csc.exe");
                if (!File.Exists(cscPath))
                    cscPath = Path.Combine(monoRoot, "lib", "mono", "4.5", "csc.exe");
                referenceAssemblies = Path.Combine(monoRoot, "lib", "mono", "4.5-api");
#endif
                break;
            }
            case TargetPlatform.Linux:
            {
#if USE_NETCORE
                cscPath = Path.Combine(dotnetSdk.RootPath, $"sdk/{dotnetSdk.VersionName}/Roslyn/bincore/csc.dll");
                referenceAssemblies = Path.Combine(dotnetSdk.RootPath, $"packs/Microsoft.NETCore.App.Ref/{dotnetSdk.RuntimeVersionName}/ref/net{runtimeVersionShort}/");
                referenceAnalyzers = Path.Combine(dotnetSdk.RootPath, $"packs/Microsoft.NETCore.App.Ref/{dotnetSdk.RuntimeVersionName}/analyzers/dotnet/cs/");
#else
                monoRoot = Path.Combine(Globals.EngineRoot, "Source", "Platforms", "Editor", "Linux", "Mono");
                monoPath = Path.Combine(monoRoot, "bin", "mono");
                cscPath = Path.Combine(monoRoot, "lib", "mono", "4.5", "csc.exe");
                referenceAssemblies = Path.Combine(monoRoot, "lib", "mono", "4.5-api");
#endif
                break;
            }
            case TargetPlatform.Mac:
            {
#if USE_NETCORE
                dotnetPath = Path.Combine(dotnetSdk.RootPath, "dotnet");
                cscPath = Path.Combine(dotnetSdk.RootPath, $"sdk/{dotnetSdk.VersionName}/Roslyn/bincore/csc.dll");
                referenceAssemblies = Path.Combine(dotnetSdk.RootPath, $"packs/Microsoft.NETCore.App.Ref/{dotnetSdk.RuntimeVersionName}/ref/net{runtimeVersionShort}/");
                referenceAnalyzers = Path.Combine(dotnetSdk.RootPath, $"packs/Microsoft.NETCore.App.Ref/{dotnetSdk.RuntimeVersionName}/analyzers/dotnet/cs/");
#else
                monoRoot = Path.Combine(Globals.EngineRoot, "Source", "Platforms", "Editor", "Mac", "Mono");
                monoPath = Path.Combine(monoRoot, "bin", "mono");
                cscPath = Path.Combine(monoRoot, "lib", "mono", "4.5", "csc.exe");
                referenceAssemblies = Path.Combine(monoRoot, "lib", "mono", "4.5-api");
#endif
                break;
            }
            default: throw new InvalidPlatformException(buildPlatform);
            }

            if (fileReferences == null)
                fileReferences = buildOptions.ScriptingAPI.FileReferences;
            else
                fileReferences.AddRange(buildOptions.ScriptingAPI.FileReferences);

            // Setup C# compiler arguments
            var args = new List<string>();
            args.Clear();
            args.Add("/nologo");
            args.Add("/target:library");
            args.Add("/platform:AnyCPU");
            args.Add("/debug+");
            args.Add("/debug:portable");
            args.Add("/errorreport:prompt");
            args.Add("/preferreduilang:en-US");
            args.Add("/highentropyva+");
            args.Add("/deterministic");
            args.Add("/nostdlib+");
            args.Add("/errorendlocation");
            args.Add("/utf8output");
            args.Add("/warn:4");
            args.Add("/unsafe");
            args.Add("/fullpaths");
            args.Add("/filealign:512");
#if USE_NETCORE
            args.Add($"/langversion:{dotnetSdk.CSharpLanguageVersion}");
            args.Add(string.Format("/nullable:{0}", buildOptions.ScriptingAPI.CSharpNullableReferences.ToString().ToLowerInvariant()));
            if (buildOptions.ScriptingAPI.CSharpNullableReferences == CSharpNullableReferences.Disable)
                args.Add("-nowarn:8632"); // The annotation for nullable reference types should only be used in code within a '#nullable' annotations context.
#else
            args.Add("/langversion:7.3");
#endif
            if (buildOptions.ScriptingAPI.IgnoreMissingDocumentationWarnings)
                args.Add("-nowarn:1591");
            if (buildOptions.ScriptingAPI.IgnoreSpecificWarnings.Any())
            {
                foreach (var warningString in buildOptions.ScriptingAPI.IgnoreSpecificWarnings)
                {
                    args.Add($"-nowarn:{warningString}");
                }
            }

            // Optimizations prevent debugging, only enable in release builds by default
            var optimize = optimizeAssembly.HasValue ? optimizeAssembly.Value : buildData.Configuration == TargetConfiguration.Release;
            args.Add(optimize ? "/optimize+" : "/optimize-");
#if !USE_NETCORE
            args.Add(string.Format("/reference:\"{0}mscorlib.dll\"", referenceAssemblies));
#endif
            args.Add(string.Format("/out:\"{0}\"", outputFile));
            args.Add(string.Format("/doc:\"{0}\"", outputDocFile));
#if USE_NETCORE
            args.Add(string.Format("/generatedfilesout:\"{0}\"", outputGeneratedFiles));
#endif
            if (buildOptions.ScriptingAPI.Defines.Count != 0)
                args.Add("/define:" + string.Join(";", buildOptions.ScriptingAPI.Defines));
            if (buildData.Configuration == TargetConfiguration.Debug)
                args.Add("/define:DEBUG");
            foreach (var reference in buildOptions.ScriptingAPI.SystemReferences)
                args.Add(string.Format("/reference:\"{0}{1}.dll\"", referenceAssemblies, reference));
            foreach (var reference in fileReferences)
                args.Add(string.Format("/reference:\"{0}\"", reference));
#if USE_NETCORE
            foreach (var systemAnalyzer in buildOptions.ScriptingAPI.SystemAnalyzers)
                args.Add(string.Format("/analyzer:\"{0}{1}.dll\"", referenceAnalyzers, systemAnalyzer));
            foreach (var analyzer in buildOptions.ScriptingAPI.Analyzers)
                args.Add(string.Format("/analyzer:\"{0}\"", analyzer));
#endif
            foreach (var sourceFile in sourceFiles)
                args.Add("\"" + sourceFile + "\"");

#if USE_NETCORE
            // Inject some assembly metadata (similar to msbuild in Visual Studio)
            var assemblyAttributesPath = Path.Combine(buildOptions.IntermediateFolder, name + ".AssemblyAttributes.cs");
            File.WriteAllText(assemblyAttributesPath, $"[assembly: global::System.Runtime.Versioning.TargetFrameworkAttribute(\".NETCoreApp,Version=v{runtimeVersionShort}\", FrameworkDisplayName = \".NET {runtimeVersionShort}\")]\n", Encoding.UTF8);
            args.Add("\"" + assemblyAttributesPath + "\"");
#endif

            // Generate response file with source files paths and compilation arguments
            string responseFile = Path.Combine(buildOptions.IntermediateFolder, name + ".response");
            Utilities.WriteFileIfChanged(responseFile, string.Join(Environment.NewLine, args));

            // Create C# compilation task
            var task = graph.Add<Task>();
            task.PrerequisiteFiles.Add(responseFile);
            task.PrerequisiteFiles.AddRange(sourceFiles);
            task.PrerequisiteFiles.AddRange(fileReferences);
            task.ProducedFiles.Add(outputFile);
            task.WorkingDirectory = buildData.TargetOptions.WorkingDirectory;
            task.InfoMessage = "Compiling " + outputFile;
            task.Cost = task.PrerequisiteFiles.Count;

            // The "/shared" flag enables the compiler server support:
            // https://github.com/dotnet/roslyn/blob/main/docs/compilers/Compiler%20Server.md
#if USE_NETCORE
            task.CommandPath = dotnetPath;
            task.CommandArguments = $"exec \"{cscPath}\" /noconfig /shared @\"{responseFile}\"";
#else
            if (monoPath != null)
            {
                task.CommandPath = monoPath;
                task.CommandArguments = $"\"{cscPath}\" /noconfig @\"{responseFile}\"";
            }
            else
            {
                task.CommandPath = cscPath;
                task.CommandArguments = $"/noconfig /shared @\"{responseFile}\"";
            }
#endif

            BuildDotNetAssembly?.Invoke(graph, buildData, buildOptions, task, binaryModule);

            // Copy referenced assemblies
            foreach (var srcFile in buildOptions.ScriptingAPI.FileReferences)
            {
                var dstFile = Path.Combine(outputPath, Path.GetFileName(srcFile));
                if (dstFile == srcFile || graph.HasCopyTask(dstFile, srcFile))
                    continue;
                graph.AddCopyFile(dstFile, srcFile);

                var srcPdb = Path.ChangeExtension(srcFile, "pdb");
                if (File.Exists(srcPdb))
                    graph.AddCopyFile(Path.ChangeExtension(dstFile, "pdb"), srcPdb);

                var srcXml = Path.ChangeExtension(srcFile, "xml");
                if (File.Exists(srcXml))
                    graph.AddCopyFile(Path.ChangeExtension(dstFile, "xml"), srcXml);
            }
        }

        private static void BuildTargetBindings(TaskGraph graph, BuildData buildData)
        {
            var sourceFiles = new List<string>();
            var fileReferences = new HashSet<string>();
            var buildOptions = buildData.TargetOptions;
            var outputPath = Path.GetDirectoryName(buildData.Target.GetOutputFilePath(buildOptions));
            foreach (var binaryModule in buildData.BinaryModules)
            {
                if (binaryModule.All(x => !x.BuildCSharp))
                    continue;
                var binaryModuleName = binaryModule.Key;
                using (new ProfileEventScope(binaryModuleName))
                {
                    // TODO: add support for extending this code and support generating bindings projects for other scripting languages
                    var project = GetModuleProject(binaryModule.First(), buildData);

                    // Get source files
                    sourceFiles.Clear();
                    foreach (var module in binaryModule)
                        sourceFiles.AddRange(buildData.Modules[module].SourceFiles.Where(x => x.EndsWith(".cs")));
                    sourceFiles.RemoveAll(x => x.EndsWith(BuildFilesPostfix));
                    var moduleGen = Path.Combine(project.ProjectFolderPath, "Source", binaryModuleName + ".Gen.cs");
                    if (!sourceFiles.Contains(moduleGen))
                        sourceFiles.Add(moduleGen);
                    sourceFiles.Sort();

                    // Get references
                    fileReferences.Clear();
                    bool? optimizeAssembly = null;
                    foreach (var module in binaryModule)
                    {
                        if (!buildData.Modules.TryGetValue(module, out var moduleBuildOptions))
                            continue;

                        if (moduleBuildOptions.ScriptingAPI.Optimization.HasValue)
                            optimizeAssembly |= moduleBuildOptions.ScriptingAPI.Optimization;

                        // Find references based on the modules dependencies
                        foreach (var dependencyName in moduleBuildOptions.PublicDependencies.Concat(moduleBuildOptions.PrivateDependencies))
                        {
                            var dependencyModule = buildData.Rules.GetModule(dependencyName);
                            if (dependencyModule != null &&
                                !string.IsNullOrEmpty(dependencyModule.BinaryModuleName) &&
                                dependencyModule.BuildCSharp &&
                                dependencyModule.BinaryModuleName != binaryModuleName &&
                                buildData.Modules.TryGetValue(dependencyModule, out var dependencyModuleOptions))
                            {
                                foreach (var x in buildData.BinaryModules)
                                {
                                    if (x.Key == null || x.Key != dependencyModule.BinaryModuleName)
                                        continue;

                                    // Reference module output binary
                                    fileReferences.Add(Path.Combine(outputPath, dependencyModule.BinaryModuleName + ".CSharp.dll"));
                                }
                                var referencedBuild = buildData.FinReferenceBuildModule(dependencyModule.BinaryModuleName);
                                if (referencedBuild != null && !string.IsNullOrEmpty(referencedBuild.ManagedPath))
                                {
                                    // Reference binary module build for referenced target
                                    fileReferences.Add(referencedBuild.ManagedPath);
                                }
                            }
                        }
                    }

                    // Build assembly
                    BuildDotNet(graph, buildData, buildOptions, binaryModuleName + ".CSharp", sourceFiles, fileReferences, binaryModule, optimizeAssembly);
                }
            }
        }
    }
}
