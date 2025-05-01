// Copyright (c) Wojciech Figat. All rights reserved.

using Mono.Cecil;
using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Text;

namespace Flax.Build
{
    /// <summary>
    /// .NET Ahead of Time Compilation (AOT) modes.
    /// </summary>
    public enum DotNetAOTModes
    {
        /// <summary>
        /// AOT is not used.
        /// </summary>
        None,

        /// <summary>
        /// Use .NET Native IL Compiler (shorten as ILC) to convert all C# assemblies in native platform executable binary.
        /// </summary>
        ILC,

        /// <summary>
        /// Use Mono AOT to cross-compile all used C# assemblies into native platform shared libraries.
        /// </summary>
        MonoAOTDynamic,

        /// <summary>
        /// Use Mono AOT to cross-compile all used C# assemblies into native platform static libraries which can be linked into a single shared library.
        /// </summary>
        MonoAOTStatic,
    }

    partial class Configuration
    {
        /// <summary>
        /// AOT mode to use by -runDotNetAOT command.
        /// </summary>
        [CommandLine("aotMode", "")]
        public static DotNetAOTModes AOTMode;

        /// <summary>
        /// See SkipUnusedDotnetLibsPackaging in BuildSettings.
        /// </summary>
        [CommandLine("skipUnusedDotnetLibs", "")]
        public static bool SkipUnusedDotnetLibsPackaging = true;

        /// <summary>
        /// Executes AOT process as a part of the game cooking (called by PrecompileAssembliesStep in Editor).
        /// </summary>
        [CommandLine("runDotNetAOT", "")]
        public static void RunDotNetAOT()
        {
            Log.Info("Running .NET AOT in mode " + AOTMode);
            //Configuration.CustomDefines.Add("DOTNET_AOT_DEBUG");
            DotNetAOT.RunAOT();
        }

        /// <summary>
        /// Executes .NET class library stripping process as a part of the game cooking (called by DeployDataStep in Editor).
        /// </summary>
        [CommandLine("runDotNetClassLibStripping", "")]
        public static void RunDotNetClassLibStripping()
        {
            Log.Info("Running .NET class Library stripping");
            DotNetAOT.RunClassLibStripping();
        }
    }

    /// <summary>
    /// The DotNet Ahead of Time Compilation (AOT) feature.
    /// </summary>
    internal static class DotNetAOT
    {
        internal static void RunClassLibStripping()
        {
            var outputPath = Configuration.BinariesFolder; // Provided by DeployDataStep
            if (!Directory.Exists(outputPath))
                throw new Exception("Missing output folder " + outputPath);
            var dotnetOutputPath = Path.Combine(outputPath, "Dotnet");
            if (!Directory.Exists(dotnetOutputPath))
                return;

            // Find input files
            var inputFiles = Directory.GetFiles(outputPath, "*.dll", SearchOption.TopDirectoryOnly).ToList();
            inputFiles.RemoveAll(FilterAssembly);
            for (int i = 0; i < inputFiles.Count; i++)
                inputFiles[i] = Utilities.NormalizePath(inputFiles[i]);
            inputFiles.Sort();

            // Peek class library folder
            var coreLibPaths = Directory.GetFiles(dotnetOutputPath, "System.Private.CoreLib.dll", SearchOption.AllDirectories);
            if (coreLibPaths.Length != 1)
                throw new Exception("Invalid C# class library setup in " + dotnetOutputPath);
            var dotnetLibPath = Utilities.NormalizePath(Path.GetDirectoryName(coreLibPaths[0]));
            Log.Info("Class library found in: " + dotnetLibPath);

            using (var assemblyResolver = new MonoCecil.BasicAssemblyResolver())
            {
                assemblyResolver.SearchDirectories.Add(outputPath);
                assemblyResolver.SearchDirectories.Add(dotnetLibPath);

                // Build list of all used assemblies
                var assembliesPaths = new List<string>();
                foreach (var inputFile in inputFiles)
                {
                    try
                    {
                        BuildAssembliesList(inputFile, assembliesPaths, assemblyResolver, string.Empty, null);
                    }
                    catch (Exception)
                    {
                    }
                }

                // Get all C# class lib assemblies
                var stdLibFiles = Directory.GetFiles(dotnetLibPath, "*.dll", SearchOption.TopDirectoryOnly).ToList();
                stdLibFiles.RemoveAll(FilterAssembly);
                for (int i = 0; i < stdLibFiles.Count; i++)
                    stdLibFiles[i] = Utilities.NormalizePath(stdLibFiles[i]);

                // Remove any unused C# class lib assemblies
                long sizeOfRemoved = 0;
                foreach (var file in stdLibFiles)
                {
                    if (!assembliesPaths.Contains(file))
                    {
                        Log.Info("Removing unused C# assembly: " + Path.GetFileName(file));
                        var fileInfo = new FileInfo(file);
                        sizeOfRemoved += fileInfo.Length;
                        fileInfo.Delete();
                    }
                }
                Log.Info("Removed C# assemblies size: " + (sizeOfRemoved / (1024 * 1024) + " MB"));
            }
        }

        internal static void RunAOT()
        {
            var platform = Configuration.BuildPlatforms[0];
            var arch = Configuration.BuildArchitectures[0];
            var configuration = Configuration.BuildConfigurations[0];
            if (!DotNetSdk.Instance.GetHostRuntime(platform, arch, out var hostRuntime))
                throw new Exception("Missing host runtime");
            var buildPlatform = Platform.GetPlatform(platform);
            var buildToolchain = buildPlatform.GetToolchain(arch);
            var dotnetAotDebug = Configuration.CustomDefines.Contains("DOTNET_AOT_DEBUG") || Environment.GetEnvironmentVariable("DOTNET_AOT_DEBUG") == "1";
            var aotMode = Configuration.AOTMode;
            var outputPath = Configuration.BinariesFolder; // Provided by PrecompileAssembliesStep
            var aotAssembliesPath = Configuration.IntermediateFolder; // Provided by PrecompileAssembliesStep
            if (!Directory.Exists(outputPath))
                throw new Exception("Missing AOT output folder " + outputPath);
            if (!Directory.Exists(aotAssembliesPath))
                throw new Exception("Missing AOT assemblies folder " + aotAssembliesPath);
            var dotnetOutputPath = Path.Combine(outputPath, "Dotnet");
            if (!Directory.Exists(dotnetOutputPath))
                Directory.CreateDirectory(dotnetOutputPath);

            // Find input files
            var inputFiles = Directory.GetFiles(aotAssembliesPath, "*.dll", SearchOption.TopDirectoryOnly).ToList();
            inputFiles.RemoveAll(FilterAssembly);
            for (int i = 0; i < inputFiles.Count; i++)
                inputFiles[i] = Utilities.NormalizePath(inputFiles[i]);
            inputFiles.Sort();

            // Useful references about AOT:
            // .NET Native IL Compiler (shorten as ILC) is used to convert IL into native platform binary
            // https://github.com/dotnet/runtime/blob/main/src/coreclr/nativeaot/docs/README.md
            // https://github.com/dotnet/runtime/blob/main/docs/workflow/building/coreclr/nativeaot.md
            // https://github.com/dotnet/samples/tree/main/core/nativeaot/NativeLibrary
            // http://www.mono-project.com/docs/advanced/runtime/docs/aot/
            // http://www.mono-project.com/docs/advanced/aot/

            if (aotMode == DotNetAOTModes.ILC)
            {
                var runtimeIdentifier = DotNetSdk.GetHostRuntimeIdentifier(platform, arch);
                var runtimeIdentifierParts = runtimeIdentifier.Split('-');
                var enableReflection = true;
                var enableReflectionScan = true;
                var enableStackTrace = true;

                var aotOutputPath = Path.Combine(aotAssembliesPath, "Output");
                if (!Directory.Exists(aotOutputPath))
                    Directory.CreateDirectory(aotOutputPath);

                // TODO: run dotnet nuget installation to get 'runtime.<runtimeIdentifier>.Microsoft.DotNet.ILCompiler' package
                //var ilcRoot = Path.Combine(DotNetSdk.Instance.RootPath, "sdk\\7.0.202\\Sdks\\Microsoft.DotNet.ILCompiler");
                var ilcRoot = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.UserProfile), $".nuget\\packages\\runtime.{runtimeIdentifier}.microsoft.dotnet.ilcompiler\\7.0.4");

                // Build ILC args list
                var ilcArgs = new StringBuilder();
                ilcArgs.AppendLine("--resilient"); // Ignore unresolved types, methods, and assemblies. Defaults to false
                ilcArgs.AppendLine("--nativelib"); // Compile as static or shared library
                if (configuration != TargetConfiguration.Debug)
                    ilcArgs.AppendLine("-O"); // Enable optimizations
                if (configuration == TargetConfiguration.Release)
                    ilcArgs.AppendLine("--Ot"); // Enable optimizations, favor code speed
                if (configuration != TargetConfiguration.Release)
                    ilcArgs.AppendLine("-g"); // Emit debugging information
                string ilcTargetOs = runtimeIdentifierParts[0];
                if (ilcTargetOs == "win")
                    ilcTargetOs = "windows";
                ilcArgs.AppendLine("--targetos:" + ilcTargetOs); // Target OS for cross compilation
                ilcArgs.AppendLine("--targetarch:" + runtimeIdentifierParts[1]); // Target architecture for cross compilation
                var ilcOutputFileName = buildPlatform.SharedLibraryFilePrefix + "AOT" + buildPlatform.SharedLibraryFileExtension;
                var ilcOutputPath = Path.Combine(aotOutputPath, ilcOutputFileName);
                ilcArgs.AppendLine("-o:" + ilcOutputPath); // Output file path
                foreach (var inputFile in inputFiles)
                {
                    ilcArgs.AppendLine(inputFile); // Input file
                    ilcArgs.AppendLine("--root:" + inputFile); // Fully generate given assembly
                }
                ilcArgs.AppendLine("--nowarn:\"1701;1702;IL2121;1701;1702\""); // Disable specific warning messages
                ilcArgs.AppendLine("--initassembly:System.Private.CoreLib"); // Assembly(ies) with a library initializer
                ilcArgs.AppendLine("--initassembly:System.Private.TypeLoader");
                if (enableReflectionScan && enableReflection)
                {
                    ilcArgs.AppendLine("--scanreflection"); // Scan IL for reflection patterns
                }
                if (enableReflection)
                {
                    ilcArgs.AppendLine("--initassembly:System.Private.Reflection.Execution");
                }
                else
                {
                    ilcArgs.AppendLine("--initassembly:System.Private.DisabledReflection");
                    ilcArgs.AppendLine("--reflectiondata:none");
                    ilcArgs.AppendLine("--feature:System.Collections.Generic.DefaultComparers=false");
                    ilcArgs.AppendLine("--feature:System.Reflection.IsReflectionExecutionAvailable=false");
                }
                if (enableReflection || enableStackTrace)
                    ilcArgs.AppendLine("--initassembly:System.Private.StackTraceMetadata");
                if (enableStackTrace)
                    ilcArgs.AppendLine("--stacktracedata"); // Emit data to support generating stack trace strings at runtime
                ilcArgs.AppendLine("--feature:System.Linq.Expressions.CanCompileToIL=false");
                ilcArgs.AppendLine("--feature:System.Linq.Expressions.CanEmitObjectArrayDelegate=false");
                ilcArgs.AppendLine("--feature:System.Linq.Expressions.CanCreateArbitraryDelegates=false");
                // TODO: reference files (-r)
                var referenceFiles = new List<string>();
                referenceFiles.AddRange(Directory.GetFiles(Path.Combine(ilcRoot, "framework"), "*.dll"));
                referenceFiles.AddRange(Directory.GetFiles(Path.Combine(ilcRoot, "sdk"), "*.dll"));
                referenceFiles.Sort();
                foreach (var referenceFile in referenceFiles)
                {
                    ilcArgs.AppendLine("--r:" + referenceFile); // Reference file(s) for compilation
                }
                ilcArgs.AppendLine("--appcontextswitch:RUNTIME_IDENTIFIER=" + runtimeIdentifier); // System.AppContext switches to set (format: 'Key=Value')
                ilcArgs.AppendLine("--appcontextswitch:Microsoft.Extensions.DependencyInjection.VerifyOpenGenericServiceTrimmability=true");
                ilcArgs.AppendLine("--appcontextswitch:System.ComponentModel.TypeConverter.EnableUnsafeBinaryFormatterInDesigntimeLicenseContextSerialization=false");
                ilcArgs.AppendLine("--appcontextswitch:System.Diagnostics.Tracing.EventSource.IsSupported=false");
                ilcArgs.AppendLine("--appcontextswitch:System.Reflection.Metadata.MetadataUpdater.IsSupported=false");
                ilcArgs.AppendLine("--appcontextswitch:System.Resources.ResourceManager.AllowCustomResourceTypes=false");
                ilcArgs.AppendLine("--appcontextswitch:System.Runtime.InteropServices.BuiltInComInterop.IsSupported=false");
                ilcArgs.AppendLine("--appcontextswitch:System.Runtime.InteropServices.EnableConsumingManagedCodeFromNativeHosting=false");
                ilcArgs.AppendLine("--appcontextswitch:System.Runtime.InteropServices.EnableCppCLIHostActivation=false");
                ilcArgs.AppendLine("--appcontextswitch:System.Runtime.Serialization.EnableUnsafeBinaryFormatterSerialization=false");
                ilcArgs.AppendLine("--appcontextswitch:System.StartupHookProvider.IsSupported=false");
                ilcArgs.AppendLine("--appcontextswitch:System.Threading.Thread.EnableAutoreleasePool=false");
                ilcArgs.AppendLine("--appcontextswitch:System.Text.Encoding.EnableUnsafeUTF7Encoding=false");
                ilcArgs.AppendLine("--feature:Microsoft.Extensions.DependencyInjection.VerifyOpenGenericServiceTrimmability=true");
                ilcArgs.AppendLine("--feature:System.ComponentModel.TypeConverter.EnableUnsafeBinaryFormatterInDesigntimeLicenseContextSerialization=false");
                ilcArgs.AppendLine("--feature:System.Diagnostics.Tracing.EventSource.IsSupported=false");
                ilcArgs.AppendLine("--feature:System.Reflection.Metadata.MetadataUpdater.IsSupported=false");
                ilcArgs.AppendLine("--feature:System.Resources.ResourceManager.AllowCustomResourceTypes=false");
                ilcArgs.AppendLine("--feature:System.Runtime.InteropServices.BuiltInComInterop.IsSupported=false");
                ilcArgs.AppendLine("--feature:System.Runtime.InteropServices.EnableConsumingManagedCodeFromNativeHosting=false");
                ilcArgs.AppendLine("--feature:System.Runtime.InteropServices.EnableCppCLIHostActivation=false");
                ilcArgs.AppendLine("--feature:System.Runtime.Serialization.EnableUnsafeBinaryFormatterSerialization=false");
                ilcArgs.AppendLine("--feature:System.StartupHookProvider.IsSupported=false");
                ilcArgs.AppendLine("--feature:System.Threading.Thread.EnableAutoreleasePool=false");
                ilcArgs.AppendLine("--feature:System.Text.Encoding.EnableUnsafeUTF7Encoding=false");
                ilcArgs.AppendLine("--directpinvoke:System.Globalization.Native");
                ilcArgs.AppendLine("--directpinvoke:System.IO.Compression");
                if (buildPlatform is Platforms.WindowsPlatformBase)
                {
                    // Windows-family
                    ilcArgs.AppendLine($"--directpinvokelist:{ilcRoot}\\build\\WindowsAPIs.txt");
                }

                // Developer debug options
                if (dotnetAotDebug)
                {
                    ilcArgs.AppendLine("--verbose"); // Enable verbose logging
                    ilcArgs.AppendLine("--metadatalog:" + Path.Combine(aotAssembliesPath, "DotnetAot.metadata.csv")); // Generate a metadata log file
                    ilcArgs.AppendLine("--exportsfile:" + Path.Combine(aotAssembliesPath, "DotnetAot.exports.txt")); // File to write exported method definitions
                    ilcArgs.AppendLine("--map:" + Path.Combine(aotAssembliesPath, "DotnetAot.map.xml")); // Generate a map file
                    ilcArgs.AppendLine("--mstat:" + Path.Combine(aotAssembliesPath, "DotnetAot.mstat")); // Generate an mstat file
                    ilcArgs.AppendLine("--dgmllog:" + Path.Combine(aotAssembliesPath, "DotnetAot.codegen.dgml.xml")); // Save result of dependency analysis as DGML
                    ilcArgs.AppendLine("--scandgmllog:" + Path.Combine(aotAssembliesPath, "DotnetAot.scan.dgml.xml")); // Save result of scanner dependency analysis as DGML
                }

                // Run ILC
                var ilcResponseFile = Path.Combine(aotAssembliesPath, "AOT.ilc.rsp");
                Utilities.WriteFileIfChanged(ilcResponseFile, string.Join(Environment.NewLine, ilcArgs));
                var ilcPath = Path.Combine(ilcRoot, "tools/ilc.exe");
                if (!File.Exists(ilcPath))
                    throw new Exception("Missing ILC " + ilcPath);
                Utilities.Run(ilcPath, string.Format("@\"{0}\"", ilcResponseFile), null, null, Utilities.RunOptions.AppMustExist | Utilities.RunOptions.ThrowExceptionOnError | Utilities.RunOptions.ConsoleLogOutput);

                // Copy to the destination folder
                Utilities.FileCopy(ilcOutputPath, Path.Combine(outputPath, ilcOutputFileName));
            }
            else if (aotMode == DotNetAOTModes.MonoAOTDynamic || aotMode == DotNetAOTModes.MonoAOTStatic)
            {
                // Peek class library folder
                var coreLibPaths = Directory.GetFiles(aotAssembliesPath, "System.Private.CoreLib.dll", SearchOption.AllDirectories);
                if (coreLibPaths.Length != 1)
                    throw new Exception($"Invalid C# class library setup in '{aotAssembliesPath}' (missing C# dll files)");
                var dotnetLibPath = Utilities.NormalizePath(Path.GetDirectoryName(coreLibPaths[0]));
                Log.Info("Class library found in: " + dotnetLibPath);

                // Build list of assemblies to process (use game assemblies as root to walk over used references from stdlib)
                var assembliesPaths = new List<string>();
                if (Configuration.SkipUnusedDotnetLibsPackaging)
                {
                    // Use game assemblies as root to walk over used references from stdlib
                    using (var assemblyResolver = new MonoCecil.BasicAssemblyResolver())
                    {
                        assemblyResolver.SearchDirectories.Add(aotAssembliesPath);
                        assemblyResolver.SearchDirectories.Add(dotnetLibPath);

                        var warnings = new HashSet<string>();
                        foreach (var inputFile in inputFiles)
                        {
                            try
                            {
                                BuildAssembliesList(inputFile, assembliesPaths, assemblyResolver, string.Empty, warnings);
                            }
                            catch (Exception)
                            {
                                Log.Error($"Failed to load assembly '{inputFile}'");
                                throw;
                            }
                        }
                    }
                }
                else
                {
                    // Use all libs
                    var stdLibFiles = Directory.GetFiles(dotnetLibPath, "*.dll", SearchOption.TopDirectoryOnly).ToList();
                    stdLibFiles.RemoveAll(FilterAssembly);
                    for (int i = 0; i < stdLibFiles.Count; i++)
                        stdLibFiles[i] = Utilities.NormalizePath(stdLibFiles[i]);
                    stdLibFiles.Sort();
                    assembliesPaths.AddRange(stdLibFiles);
                    assembliesPaths.AddRange(inputFiles);
                }

                // Run compilation
                bool failed = false;
                bool validCache = true;
                string platformToolsPath;
                {
                    var options = new Toolchain.CSharpOptions
                    {
                        Action = Toolchain.CSharpOptions.ActionTypes.GetPlatformTools,
                        AssembliesPath = aotAssembliesPath,
                        ClassLibraryPath = dotnetLibPath,
                        EnableToolDebug = dotnetAotDebug,
                    };
                    buildToolchain.CompileCSharp(ref options);
                    platformToolsPath = options.PlatformToolsPath;
                }
                if (!Directory.Exists(platformToolsPath))
                    throw new Exception("Missing platform tools " + platformToolsPath);
                Log.Info("Platform tools found in: " + platformToolsPath);
                var compileAssembly = (string assemblyPath) =>
                {
                    // Determinate whether use debug information for that assembly
                    var useDebug = configuration != TargetConfiguration.Release;
                    if (!dotnetAotDebug && !inputFiles.Contains(assemblyPath))
                    {
                        // Don't use debug for C# stdlib assemblies by default to reduce build size and improve build time (except manually overriden)
                        useDebug = false;
                    }

                    // Get output file path for this assembly (platform can use custom extension)
                    var options = new Toolchain.CSharpOptions
                    {
                        Action = Toolchain.CSharpOptions.ActionTypes.GetOutputFiles,
                        InputFiles = new List<string>() { assemblyPath },
                        OutputFiles = new List<string>(),
                        AssembliesPath = aotAssembliesPath,
                        ClassLibraryPath = dotnetLibPath,
                        PlatformToolsPath = platformToolsPath,
                        EnableDebugSymbols = useDebug,
                        EnableToolDebug = dotnetAotDebug,
                    };
                    buildToolchain.CompileCSharp(ref options);

                    // Skip if output is already generated and is newer than a source assembly
                    if (!File.Exists(options.OutputFiles[0]) || File.GetLastWriteTime(assemblyPath) > File.GetLastWriteTime(options.OutputFiles[0]))
                    {
                        if (dotnetAotDebug)
                        {
                            // Increase log readability when spamming log with verbose mode
                            Log.Info("");
                            Log.Info("");
                        }
                        if (!Directory.Exists(options.PlatformToolsPath))
                            throw new Exception("Missing platform tools " + options.PlatformToolsPath);
                        Log.Info(" * " + assemblyPath);
                        options.Action = Toolchain.CSharpOptions.ActionTypes.MonoCompile;
                        if (buildToolchain.CompileCSharp(ref options))
                        {
                            Log.Error("Failed to run AOT on assembly " + assemblyPath);
                            failed = true;
                            return;
                        }
                        validCache = false;
                    }

                    var assemblyFileName = Path.GetFileName(assemblyPath);
                    if (Configuration.AOTMode == DotNetAOTModes.MonoAOTDynamic)
                    {
                        {
                            // Copy assembly
                            var outputFile = Path.Combine(dotnetOutputPath, assemblyFileName);
                            var deployedFilePath = Path.Combine(dotnetOutputPath, Path.GetFileName(outputFile));
                            if (!File.Exists(deployedFilePath) || File.GetLastWriteTime(outputFile) > File.GetLastWriteTime(deployedFilePath))
                            {
                                Utilities.FileCopy(assemblyPath, outputFile);
                            }
                        }

                        // Copy AOT build products
                        foreach (var outputFile in options.OutputFiles)
                        {
                            // Skip if deployed file is already valid
                            var deployedFilePath = Path.Combine(dotnetOutputPath, Path.GetFileName(outputFile));
                            if (!File.Exists(deployedFilePath) || File.GetLastWriteTime(outputFile) > File.GetLastWriteTime(deployedFilePath))
                            {
                                // Copy to the destination folder
                                Utilities.FileCopy(outputFile, deployedFilePath);
                                if (useDebug && File.Exists(outputFile + ".pdb"))
                                    Utilities.FileCopy(outputFile + ".pdb", Path.Combine(dotnetOutputPath, Path.GetFileName(outputFile + ".pdb")));
                                validCache = false;
                            }
                        }
                    }
                    else
                    {
                        // Copy to the destination folder
                        Utilities.FileCopy(assemblyPath, Path.Combine(dotnetOutputPath, assemblyFileName));
                    }
                };
                if (Configuration.MaxConcurrency > 1 && Configuration.ConcurrencyProcessorScale > 0.0f && !dotnetAotDebug)
                {
                    // Multi-threaded
                    System.Threading.Tasks.Parallel.ForEach(assembliesPaths, compileAssembly);
                }
                else
                {
                    // Single-threaded
                    foreach (var assemblyPath in assembliesPaths)
                        compileAssembly(assemblyPath);
                }
                if (failed)
                    throw new Exception($"Failed to run AOT. See log ({Configuration.LogFile}).");
                var outputAotLib = Path.Combine(outputPath, buildPlatform.SharedLibraryFilePrefix + "AOT" + buildPlatform.SharedLibraryFileExtension);
                if (Configuration.AOTMode == DotNetAOTModes.MonoAOTStatic && (!validCache || !File.Exists(outputAotLib)))
                {
                    // Link static native libraries into a single shared module with whole C# compiled in
                    var options = new Toolchain.CSharpOptions
                    {
                        Action = Toolchain.CSharpOptions.ActionTypes.MonoLink,
                        InputFiles = assembliesPaths,
                        OutputFiles = new List<string>() { outputAotLib },
                        AssembliesPath = aotAssembliesPath,
                        ClassLibraryPath = dotnetLibPath,
                        PlatformToolsPath = platformToolsPath,
                        EnableDebugSymbols = configuration != TargetConfiguration.Release,
                        EnableToolDebug = dotnetAotDebug,
                    };
                    Log.Info(" * " + outputAotLib);
                    if (buildToolchain.CompileCSharp(ref options))
                    {
                        throw new Exception("Mono AOT failed to link static libraries into a shared module");
                    }
                }
            }
            else
            {
                throw new Exception();
            }

            // Deploy license files
            Utilities.FileCopy(Path.Combine(aotAssembliesPath, "LICENSE.TXT"), Path.Combine(dotnetOutputPath, "LICENSE.TXT"));
            Utilities.FileCopy(Path.Combine(aotAssembliesPath, "THIRD-PARTY-NOTICES.TXT"), Path.Combine(dotnetOutputPath, "THIRD-PARTY-NOTICES.TXT"));
        }

        internal static bool FilterAssembly(string x)
        {
            // Skip AOT output products
            if (x.EndsWith(".dll.dll"))
                return true;

            // Skip Flax.Build rules assembly
            var fileName = Path.GetFileName(x);
            if (fileName == Assembler.CacheFileName || fileName == "BuilderRulesCache.dll")
                return true;

            // Skip non-C# DLLs
            try
            {
                using (AssemblyDefinition assembly = AssemblyDefinition.ReadAssembly(x, new ReaderParameters { ReadSymbols = false, InMemory = true, ReadingMode = ReadingMode.Deferred }))
                    return false;
            }
            catch
            {
                return true;
            }
        }

        internal static void BuildAssembliesList(string assemblyPath, List<string> outputList, IAssemblyResolver assemblyResolver, string callerPath, HashSet<string> warnings)
        {
            // Skip if already processed
            if (outputList.Contains(assemblyPath))
                return;
            outputList.Add(assemblyPath);

            // Load assembly metadata
            using (AssemblyDefinition assembly = AssemblyDefinition.ReadAssembly(assemblyPath, new ReaderParameters { ReadSymbols = false, AssemblyResolver = assemblyResolver }))
            {
                foreach (ModuleDefinition assemblyModule in assembly.Modules)
                {
                    // Collected referenced assemblies
                    foreach (AssemblyNameReference assemblyReference in assemblyModule.AssemblyReferences)
                    {
                        BuildAssembliesList(assemblyPath, assemblyReference, outputList, assemblyResolver, callerPath, warnings);
                    }
                }
            }

            // Move to the end of list
            outputList.Remove(assemblyPath);
            outputList.Add(assemblyPath);
        }

        internal static void BuildAssembliesList(AssemblyDefinition assembly, List<string> outputList, IAssemblyResolver assemblyResolver, string callerPath, HashSet<string> warnings)
        {
            // Skip if already processed
            var assemblyPath = Utilities.NormalizePath(assembly.MainModule.FileName);
            if (outputList.Contains(assemblyPath))
                return;
            outputList.Add(assemblyPath);

            foreach (ModuleDefinition assemblyModule in assembly.Modules)
            {
                // Collected referenced assemblies
                foreach (AssemblyNameReference assemblyReference in assemblyModule.AssemblyReferences)
                {
                    BuildAssembliesList(assemblyPath, assemblyReference, outputList, assemblyResolver, callerPath, warnings);
                }
            }

            // Move to the end of list
            outputList.Remove(assemblyPath);
            outputList.Add(assemblyPath);
        }

        internal static void BuildAssembliesList(string assemblyPath, AssemblyNameReference assemblyReference, List<string> outputList, IAssemblyResolver assemblyResolver, string callerPath, HashSet<string> warnings)
        {
            var assemblyName = Path.GetFileName(assemblyPath);
            if (warnings != null)
            {
                // Detect usage of C# API that is not supported in AOT builds
                if (assemblyReference.Name.Contains("System.Linq.Expressions") ||
                    assemblyReference.Name.Contains("System.Reflection.Emit") ||
                    assemblyReference.Name.Contains("System.Reflection.Emit.ILGeneration"))
                {
                    if (!warnings.Contains(assemblyReference.Name))
                    {
                        warnings.Add(assemblyReference.Name);
                        if (callerPath.Length != 0)
                            Log.Warning($"Warning! Assembly '{assemblyName}' (referenced by '{callerPath}') references '{assemblyReference.Name}' which is not supported in AOT builds and might cause error (due to lack of JIT at runtime).");
                        else
                            Log.Warning($"Warning! Assembly '{assemblyName}' references '{assemblyReference.Name}' which is not supported in AOT builds and might cause error (due to lack of JIT at runtime).");
                    }
                }
            }
            if (callerPath.Length != 0)
                callerPath += " - > ";
            callerPath += assemblyName;

            try
            {
                var reference = assemblyResolver.Resolve(assemblyReference);
                BuildAssembliesList(reference, outputList, assemblyResolver, callerPath, warnings);
            }
            catch (Exception)
            {
                Log.Error($"Failed to load assembly '{assemblyReference.FullName}' referenced by '{assemblyPath}'");
                throw;
            }
        }
    }
}
