// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Text.RegularExpressions;
using Flax.Build.Graph;
using Flax.Build.NativeCpp;

namespace Flax.Build.Platforms
{
    /// <summary>
    /// The build toolchain for all Unix-like systems.
    /// </summary>
    /// <seealso cref="Toolchain" />
    public abstract class UnixToolchain : Toolchain
    {
        /// <summary>
        /// The toolset root folder path.
        /// </summary>
        protected string ToolsetRoot;

        /// <summary>
        /// The build architecture name.
        /// </summary>
        protected string ArchitectureName;

        /// <summary>
        /// The clang path.
        /// </summary>
        protected string ClangPath;

        /// <summary>
        /// The ar tool path.
        /// </summary>
        protected string ArPath;

        /// <summary>
        /// The LLVM ar tool path.
        /// </summary>
        protected string LlvmArPath;

        /// <summary>
        /// The ranlib tool path.
        /// </summary>
        protected string RanlibPath;

        /// <summary>
        /// The strip tool path.
        /// </summary>
        protected string StripPath;

        /// <summary>
        /// The obj copy tool path.
        /// </summary>
        protected string ObjcopyPath;

        /// <summary>
        /// The ld tool path.
        /// </summary>
        protected string LdPath;

        /// <summary>
        /// The clang tool version.
        /// </summary>
        protected Version ClangVersion;

        /// <summary>
        /// Initializes a new instance of the <see cref="UnixToolchain"/> class.
        /// </summary>
        /// <param name="platform">The platform.</param>
        /// <param name="architecture">The target architecture.</param>
        /// <param name="toolchainRoots">The root folder for the toolchains installation.</param>
        /// <param name="systemCompiler">The system compiler to use. Null if use toolset root.</param>
        /// <param name="toolchainSubDir">The custom toolchain folder location in <paramref name="toolchainRoots"/> directory. If nul the architecture name will be sued.</param>
        protected UnixToolchain(UnixPlatform platform, TargetArchitecture architecture, string toolchainRoots, string systemCompiler, string toolchainSubDir = null)
        : base(platform, architecture)
        {
            ArchitectureName = GetToolchainName(platform.Target, architecture);

            // Build paths
            if (systemCompiler != null)
            {
                ToolsetRoot = toolchainRoots;
                ClangPath = UnixPlatform.Which(systemCompiler);
                ArPath = UnixPlatform.Which("ar");
                LlvmArPath = UnixPlatform.Which("llvm-ar");
                RanlibPath = UnixPlatform.Which("ranlib");
                StripPath = UnixPlatform.Which("strip");
                ObjcopyPath = UnixPlatform.Which("objcopy");
                LdPath = UnixPlatform.Which("ld");
            }
            else
            {
                var exeExtension = Platform.BuildPlatform.ExecutableFileExtension;
                ToolsetRoot = toolchainSubDir == null ? Path.Combine(toolchainRoots, ArchitectureName) : Path.Combine(toolchainRoots, toolchainSubDir);
                ClangPath = Path.Combine(Path.Combine(ToolsetRoot, string.Format("bin/{0}-{1}", ArchitectureName, "clang++"))) + exeExtension;
                if (!File.Exists(ClangPath))
                    ClangPath = Path.Combine(ToolsetRoot, "bin/clang++") + exeExtension;
                ArPath = Path.Combine(Path.Combine(ToolsetRoot, string.Format("bin/{0}-{1}", ArchitectureName, "ar"))) + exeExtension;
                LlvmArPath = Path.Combine(Path.Combine(ToolsetRoot, string.Format("bin/{0}", "llvm-ar"))) + exeExtension;
                RanlibPath = Path.Combine(Path.Combine(ToolsetRoot, string.Format("bin/{0}-{1}", ArchitectureName, "ranlib"))) + exeExtension;
                StripPath = Path.Combine(Path.Combine(ToolsetRoot, string.Format("bin/{0}-{1}", ArchitectureName, "strip"))) + exeExtension;
                ObjcopyPath = Path.Combine(Path.Combine(ToolsetRoot, string.Format("bin/{0}-{1}", ArchitectureName, "objcopy"))) + exeExtension;
                LdPath = Path.Combine(Path.Combine(ToolsetRoot, string.Format("bin/{0}-{1}", ArchitectureName, "ld"))) + exeExtension;
            }

            // Determinate compiler version
            if (!File.Exists(ClangPath))
                throw new Exception(string.Format("Missing Clang ({0})", ClangPath));
            using (var process = new Process())
            {
                process.StartInfo.UseShellExecute = false;
                process.StartInfo.CreateNoWindow = true;
                process.StartInfo.RedirectStandardOutput = true;
                process.StartInfo.RedirectStandardError = true;
                process.StartInfo.FileName = ClangPath;
                process.StartInfo.Arguments = "--version";

                process.Start();
                process.WaitForExit();

                if (process.ExitCode == 0)
                {
                    // Parse the version
                    string data = process.StandardOutput.ReadLine();
                    Regex versionPattern = new Regex("version \\d+(\\.\\d+)+");
                    Match versionMatch = versionPattern.Match(data);
                    if (versionMatch.Value.StartsWith("version "))
                    {
                        var versionString = versionMatch.Value.Replace("version ", "");
                        string[] parts = versionString.Split('.');
                        int major = 0, minor = 0, patch = 0;
                        if (parts.Length >= 1)
                            major = Convert.ToInt32(parts[0]);
                        if (parts.Length >= 2)
                            minor = Convert.ToInt32(parts[1]);
                        if (parts.Length >= 3)
                            patch = Convert.ToInt32(parts[2]);
                        ClangVersion = new Version(major, minor, patch);
                    }
                }
                else
                {
                    throw new Exception(string.Format("Failed to get Clang version ({0})", ClangPath));
                }
            }

            // Check version
            if (ClangVersion.Major < 6)
                throw new Exception(string.Format("Unsupported Clang version {0}. Minimum supported is 6.", ClangVersion));
        }

        private static string GetLibName(string path)
        {
            var libName = Path.GetFileNameWithoutExtension(path);
            if (libName.StartsWith("lib"))
                libName = libName.Substring(3);
            return libName;
        }

        /// <summary>
        /// Gets the name of the toolchain.
        /// </summary>
        /// <param name="platform">The platform.</param>
        /// <param name="architecture">The architecture.</param>
        /// <returns>The toolchain name.</returns>
        public static string GetToolchainName(TargetPlatform platform, TargetArchitecture architecture)
        {
            if (architecture == TargetArchitecture.AnyCPU)
                return string.Empty;

            switch (platform)
            {
            case TargetPlatform.Linux:
                switch (architecture)
                {
                case TargetArchitecture.x64: return "x86_64-unknown-linux-gnu";
                case TargetArchitecture.ARM: return "arm-unknown-linux-gnueabihf";
                case TargetArchitecture.ARM64: return "aarch64-unknown-linux-gnueabi";
                default: throw new InvalidArchitectureException(architecture);
                }
            case TargetPlatform.PS4: return "orbis";
            case TargetPlatform.Android:
                switch (architecture)
                {
                case TargetArchitecture.x86: return "i686-linux-android";
                case TargetArchitecture.x64: return "x86_64-linux-android";
                case TargetArchitecture.ARM: return "armv7a-linux-androideabi";
                case TargetArchitecture.ARM64: return "aarch64-linux-android";
                default: throw new InvalidArchitectureException(architecture);
                }
            default: throw new InvalidPlatformException(platform);
            }
        }

        /// <inheritdoc />
        public override string DllExport => "__attribute__((__visibility__(\\\"default\\\")))";

        /// <inheritdoc />
        public override string DllImport => "";

        /// <inheritdoc />
        public override void LogInfo()
        {
            if (!string.IsNullOrEmpty(ToolsetRoot))
                Log.Info("Toolset root: " + ToolsetRoot);
            Log.Info("Clang version: " + ClangVersion);
        }

        /// <inheritdoc />
        public override void SetupEnvironment(BuildOptions options)
        {
            base.SetupEnvironment(options);

            options.CompileEnv.PreprocessorDefinitions.Add("PLATFORM_UNIX");
            options.CompileEnv.PreprocessorDefinitions.Add("_GNU_SOURCE");
        }

        /// <summary>
        /// Setups the C++ files compilation arguments.
        /// </summary>
        /// <param name="graph">The graph.</param>
        /// <param name="options">The options.</param>
        /// <param name="args">The arguments.</param>
        /// <param name="outputPath">The output directory path (for object files).</param>
        protected virtual void SetupCompileCppFilesArgs(TaskGraph graph, BuildOptions options, List<string> args, string outputPath)
        {
        }

        /// <summary>
        /// Setups the linking files to archive arguments.
        /// </summary>
        /// <param name="graph">The graph.</param>
        /// <param name="options">The options.</param>
        /// <param name="args">The arguments.</param>
        /// <param name="outputFilePath">The output file path.</param>
        protected virtual void SetupArchiveFilesArgs(TaskGraph graph, BuildOptions options, List<string> args, string outputFilePath)
        {
        }

        /// <summary>
        /// Setups the linking files arguments.
        /// </summary>
        /// <param name="graph">The graph.</param>
        /// <param name="options">The options.</param>
        /// <param name="args">The arguments.</param>
        /// <param name="outputFilePath">The output file path.</param>
        protected virtual void SetupLinkFilesArgs(TaskGraph graph, BuildOptions options, List<string> args, string outputFilePath)
        {
        }

        /// <inheritdoc />
        public override CompileOutput CompileCppFiles(TaskGraph graph, BuildOptions options, List<string> sourceFiles, string outputPath)
        {
            var compileEnvironment = options.CompileEnv;
            var output = new CompileOutput();

            // Setup arguments shared by all source files
            var commonArgs = new List<string>();
            SetupCompileCppFilesArgs(graph, options, commonArgs, outputPath);
            {
                commonArgs.Add("-c");
                commonArgs.Add("-pipe");
                commonArgs.Add("-x");
                commonArgs.Add("c++");
                commonArgs.Add("-std=c++14");

                commonArgs.Add("-Wdelete-non-virtual-dtor");
                commonArgs.Add("-fno-math-errno");
                commonArgs.Add("-fdiagnostics-format=msvc");

                if (Architecture == TargetArchitecture.ARM || Architecture == TargetArchitecture.ARM64)
                    commonArgs.Add("-fsigned-char");

                if (compileEnvironment.RuntimeTypeInfo)
                    commonArgs.Add("-frtti");
                else
                    commonArgs.Add("-fno-rtti");

                if (compileEnvironment.TreatWarningsAsErrors)
                    commonArgs.Add("-Wall -Werror");

                // TODO: compileEnvironment.IntrinsicFunctions
                // TODO: compileEnvironment.FunctionLevelLinking
                // TODO: compileEnvironment.FavorSizeOrSpeed
                // TODO: compileEnvironment.RuntimeChecks
                // TODO: compileEnvironment.StringPooling
                // TODO: compileEnvironment.BufferSecurityCheck

                if (compileEnvironment.DebugInformation)
                {
                    commonArgs.Add("-glldb");
                }

                commonArgs.Add("-pthread");

                if (compileEnvironment.Optimization)
                {
                    commonArgs.Add("-O2");
                }
                else
                {
                    commonArgs.Add("-O0");
                }

                if (!compileEnvironment.Inlining)
                {
                    commonArgs.Add("-fno-inline-functions");
                    commonArgs.Add("-fno-inline");
                }

                if (compileEnvironment.EnableExceptions)
                {
                    commonArgs.Add("-fexceptions");
                }
                else
                {
                    commonArgs.Add("-fno-exceptions");
                }
            }

            // Add preprocessor definitions
            foreach (var definition in compileEnvironment.PreprocessorDefinitions)
            {
                commonArgs.Add(string.Format("-D \"{0}\"", definition));
            }

            // Add include paths
            foreach (var includePath in compileEnvironment.IncludePaths)
            {
                commonArgs.Add(string.Format("-I\"{0}\"", includePath.Replace('\\', '/')));
            }

            // Compile all C++ files
            var args = new List<string>();
            foreach (var sourceFile in sourceFiles)
            {
                var sourceFilename = Path.GetFileNameWithoutExtension(sourceFile);
                var task = graph.Add<CompileCppTask>();

                // Use shared arguments
                args.Clear();
                args.AddRange(commonArgs);

                // Object File Name
                var objFile = Path.Combine(outputPath, sourceFilename + ".o");
                args.Add(string.Format("-o \"{0}\"", objFile.Replace('\\', '/')));
                output.ObjectFiles.Add(objFile);
                task.ProducedFiles.Add(objFile);

                // Source File Name
                args.Add("\"" + sourceFile.Replace('\\', '/') + "\"");

                // Request included files to exist
                var includes = IncludesCache.FindAllIncludedFiles(sourceFile);
                task.PrerequisiteFiles.AddRange(includes);

                // Compile
                task.WorkingDirectory = options.WorkingDirectory;
                task.CommandPath = ClangPath;
                task.CommandArguments = string.Join(" ", args);
                task.PrerequisiteFiles.Add(sourceFile);
                task.InfoMessage = Path.GetFileName(sourceFile);
                task.Cost = task.PrerequisiteFiles.Count; // TODO: include source file size estimation to improve tasks sorting
            }

            return output;
        }

        /// <summary>
        /// Links the files into an archive (static library).
        /// </summary>
        protected virtual Task CreateArchive(TaskGraph graph, BuildOptions options, string outputFilePath)
        {
            var linkEnvironment = options.LinkEnv;
            var task = graph.Add<LinkTask>();

            // Setup arguments
            var args = new List<string>();
            {
                args.Add("rcs");

                args.Add(string.Format("-o \"{0}\"", outputFilePath));
            }
            SetupArchiveFilesArgs(graph, options, args, outputFilePath);

            // Input files
            task.PrerequisiteFiles.AddRange(linkEnvironment.InputFiles);
            foreach (var file in linkEnvironment.InputFiles)
            {
                args.Add(string.Format("\"{0}\"", file.Replace('\\', '/')));
            }

            // Use a response file (it can contain any commands that you would specify on the command line)
            bool useResponseFile = true;
            string responseFile = null;
            if (useResponseFile)
            {
                responseFile = Path.Combine(options.IntermediateFolder, Path.GetFileName(outputFilePath) + ".response");
                task.PrerequisiteFiles.Add(responseFile);
                Utilities.WriteFileIfChanged(responseFile, string.Join(Environment.NewLine, args));
            }

            // Link object into archive
            task.WorkingDirectory = options.WorkingDirectory;
            task.CommandPath = ArPath;
            task.CommandArguments = useResponseFile ? string.Format("@\"{0}\"", responseFile) : string.Join(" ", args);
            task.Cost = task.PrerequisiteFiles.Count;
            task.ProducedFiles.Add(outputFilePath);

            // Generate an index to the contents of an archive
            /*task = graph.Add<LinkTask>();
            task.CommandPath = RanlibPath;
            task.CommandArguments = string.Format("\"{0}\"", outputFilePath);
            task.InfoMessage = "Linking " + outputFilePath;
            task.PrerequisiteFiles.Add(outputFilePath);
            task.ProducedFiles.Add(outputFilePath);*/

            return task;
        }

        /// <summary>
        /// Links the files into a binary (executable file or dynamic library).
        /// </summary>
        protected virtual Task CreateBinary(TaskGraph graph, BuildOptions options, string outputFilePath)
        {
            var linkEnvironment = options.LinkEnv;
            var task = graph.Add<LinkTask>();

            // Setup arguments
            var args = new List<string>();
            {
                args.Add(string.Format("-o \"{0}\"", outputFilePath));

                if (!options.LinkEnv.DebugInformation)
                {
                    args.Add("-Wl,--strip-debug");
                }

                // TODO: linkEnvironment.LinkTimeCodeGeneration
                // TODO: linkEnvironment.Optimization
                // TODO: linkEnvironment.UseIncrementalLinking
            }
            SetupLinkFilesArgs(graph, options, args, outputFilePath);

            args.Add("-Wl,--start-group");

            // Input libraries
            var libraryPaths = new HashSet<string>();
            foreach (var library in linkEnvironment.InputLibraries)
            {
                var dir = Path.GetDirectoryName(library);
                var ext = Path.GetExtension(library);
                if (string.IsNullOrEmpty(dir))
                {
                    args.Add(string.Format("\"-l{0}\"", library));
                }
                else if (string.IsNullOrEmpty(ext))
                {
                    // Skip executable
                }
                else if (ext == ".so")
                {
                    // Link against dynamic library
                    task.PrerequisiteFiles.Add(library);
                    libraryPaths.Add(dir);
                    args.Add(string.Format("\"-l{0}\"", GetLibName(library)));
                }
                else
                {
                    task.PrerequisiteFiles.Add(library);
                    args.Add(string.Format("\"{0}\"", GetLibName(library)));
                }
            }
            foreach (var library in options.Libraries)
            {
                var dir = Path.GetDirectoryName(library);
                var ext = Path.GetExtension(library);
                if (string.IsNullOrEmpty(dir))
                {
                    args.Add(string.Format("\"-l{0}\"", library));
                }
                else if (string.IsNullOrEmpty(ext))
                {
                    // Skip executable
                }
                else if (ext == ".so")
                {
                    // Link against dynamic library
                    task.PrerequisiteFiles.Add(library);
                    libraryPaths.Add(dir);
                    args.Add(string.Format("\"-l{0}\"", GetLibName(library)));
                }
                else
                {
                    task.PrerequisiteFiles.Add(library);
                    args.Add(string.Format("\"{0}\"", GetLibName(library)));
                }
            }

            // Input files
            task.PrerequisiteFiles.AddRange(linkEnvironment.InputFiles);
            foreach (var file in linkEnvironment.InputFiles)
            {
                args.Add(string.Format("\"{0}\"", file.Replace('\\', '/')));
            }

            // Additional lib paths
            libraryPaths.AddRange(linkEnvironment.LibraryPaths);
            foreach (var path in libraryPaths)
            {
                args.Add(string.Format("-L\"{0}\"", path.Replace('\\', '/')));
            }

            args.Add("-Wl,--end-group");

            // Use a response file (it can contain any commands that you would specify on the command line)
            bool useResponseFile = true;
            string responseFile = null;
            if (useResponseFile)
            {
                responseFile = Path.Combine(options.IntermediateFolder, Path.GetFileName(outputFilePath) + ".response");
                task.PrerequisiteFiles.Add(responseFile);
                Utilities.WriteFileIfChanged(responseFile, string.Join(Environment.NewLine, args));
            }

            // Link
            task.WorkingDirectory = options.WorkingDirectory;
            task.CommandPath = LdPath;
            task.CommandArguments = useResponseFile ? string.Format("@\"{0}\"", responseFile) : string.Join(" ", args);
            task.InfoMessage = "Linking " + outputFilePath;
            task.Cost = task.PrerequisiteFiles.Count;
            task.ProducedFiles.Add(outputFilePath);

            return task;
        }

        /// <inheritdoc />
        public override void LinkFiles(TaskGraph graph, BuildOptions options, string outputFilePath)
        {
            outputFilePath = outputFilePath.Replace('\\', '/');

            Task linkTask;
            switch (options.LinkEnv.Output)
            {
            case LinkerOutput.Executable:
            case LinkerOutput.SharedLibrary:
                linkTask = CreateBinary(graph, options, outputFilePath);
                break;
            case LinkerOutput.StaticLibrary:
            case LinkerOutput.ImportLibrary:
                linkTask = CreateArchive(graph, options, outputFilePath);
                break;
            default: throw new ArgumentOutOfRangeException();
            }

            if (!options.LinkEnv.DebugInformation)
            {
                // Strip debug symbols
                var task = graph.Add<Task>();
                task.ProducedFiles.Add(outputFilePath);
                task.WorkingDirectory = options.WorkingDirectory;
                task.CommandPath = StripPath;
                task.CommandArguments = string.Format("--strip-debug \"{0}\"", outputFilePath);
                task.InfoMessage = "Striping " + outputFilePath;
                task.Cost = 1;
                task.DisableCache = true;
                task.DependentTasks = new HashSet<Task>();
                task.DependentTasks.Add(linkTask);
            }
        }
    }
}
