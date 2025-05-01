// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Collections.Generic;
using Flax.Build.Graph;
using Flax.Build.NativeCpp;

namespace Flax.Build.Platforms
{
    /// <summary>
    /// The build toolchain for all Apple systems.
    /// </summary>
    /// <seealso cref="UnixToolchain" />
    public abstract class AppleToolchain : UnixToolchain
    {
        public string ToolchainPath;
        public string SdkPath;
        public string LinkerPath;
        public string ArchiverPath;

        /// <summary>
        /// Initializes a new instance of the <see cref="AppleToolchain"/> class.
        /// </summary>
        /// <param name="platform">The platform.</param>
        /// <param name="architecture">The target architecture.</param>
        /// <param name="sdkPrefix">The XCode SDK prefix to use for the target platform..</param>
        public AppleToolchain(ApplePlatform platform, TargetArchitecture architecture, string sdkPrefix)
        : base(platform, architecture)
        {
            // Setup tools paths
            var xCodePath = XCode.Instance.RootPath;
            if (xCodePath.Contains("/Xcode"))
            {
                // XCode App
                ToolchainPath = Path.Combine(xCodePath, "Toolchains/XcodeDefault.xctoolchain");
                SdkPath = Path.Combine(xCodePath, $"Platforms/{sdkPrefix}.platform/Developer");
            }
            else
            {
                // XCode Command Line Tools
                ToolchainPath = SdkPath = xCodePath;
                if (!Directory.Exists(Path.Combine(ToolchainPath, "usr/bin")))
                    throw new Exception("Missing XCode Command Line Tools. Run 'xcode-select --install'.");
            }
            ClangPath = Path.Combine(ToolchainPath, "usr/bin/clang++");
            LinkerPath = Path.Combine(ToolchainPath, "usr/bin/clang++");
            ArchiverPath = Path.Combine(ToolchainPath, "usr/bin/libtool");
            ClangVersion = GetClangVersion(platform.Target, ClangPath);
            SdkPath = Path.Combine(SdkPath, "SDKs");
            var sdks = Directory.GetDirectories(SdkPath);
            var bestSdk = string.Empty;
            var bestSdkVer = new Version(0, 0, 0, 1);
            foreach (var sdk in sdks)
            {
                var name = Path.GetFileName(sdk);
                if (!name.StartsWith(sdkPrefix))
                    continue;
                var versionName = name.Replace(sdkPrefix, "").Replace(".sdk", "");
                if (string.IsNullOrEmpty(versionName))
                    continue;
                if (!versionName.Contains("."))
                    versionName += ".0";
                var version = new Version(versionName);
                if (version > bestSdkVer)
                {
                    bestSdkVer = version;
                    bestSdk = sdk;
                }
            }
            if (bestSdk.Length == 0)
                throw new Exception("Failed to find any valid SDK for " + sdkPrefix);
            SdkPath = bestSdk;

            // Setup system paths
            //SystemIncludePaths.Add(Path.Combine(ToolchainPath, "usr/include"));
            //SystemIncludePaths.Add(Path.Combine(ToolchainPath, "usr/include/c++/v1"));
            //SystemIncludePaths.Add(Path.Combine(ToolchainPath, "usr/lib/clang", ClangVersion.ToString(3), "include"));
            //SystemIncludePaths.Add(Path.Combine(SdkPath, "usr/include"));
            SystemLibraryPaths.Add(Path.Combine(SdkPath, "usr/lib"));
        }

        /// <inheritdoc />
        public override void LogInfo()
        {
            Log.Info("Toolchain: " + ToolchainPath);
            Log.Info("SDK: " + SdkPath);
            Log.Info("Clang version: " + Utilities.ReadProcessOutput(ClangPath, "--version"));
        }

        /// <inheritdoc />
        public override CompileOutput CompileCppFiles(TaskGraph graph, BuildOptions options, List<string> sourceFiles, string outputPath)
        {
            var compileEnvironment = options.CompileEnv;
            var output = new CompileOutput();

            // Setup arguments shared by all source files
            var commonArgs = new List<string>();
            commonArgs.AddRange(options.CompileEnv.CustomArgs);
            {
                commonArgs.Add("-c");
                commonArgs.Add("-fmessage-length=0");
                commonArgs.Add("-pipe");
                commonArgs.Add("-x");
                commonArgs.Add("objective-c++");
                commonArgs.Add("-stdlib=libc++");
                AddArgsCommon(options, commonArgs);
                AddArgsSanitizer(compileEnvironment.Sanitizers, commonArgs);

                switch (compileEnvironment.CppVersion)
                {
                case CppVersion.Cpp14:
                    commonArgs.Add("-std=c++14");
                    break;
                case CppVersion.Cpp17:
                case CppVersion.Latest:
                    commonArgs.Add("-std=c++17");
                    break;
                case CppVersion.Cpp20:
                    commonArgs.Add("-std=c++20");
                    break;
                }

                switch (Architecture)
                {
                case TargetArchitecture.x64:
                    commonArgs.Add("-msse2");
                    break;
                }

                commonArgs.Add("-Wdelete-non-virtual-dtor");
                commonArgs.Add("-fno-math-errno");
                commonArgs.Add("-fasm-blocks");
                commonArgs.Add("-fpascal-strings");
                commonArgs.Add("-fdiagnostics-format=msvc");

                commonArgs.Add("-Wno-absolute-value");
                commonArgs.Add("-Wno-nullability-completeness");
                commonArgs.Add("-Wno-undef-prefix");
                commonArgs.Add("-Wno-expansion-to-defined");

                // Hide all symbols by default
                commonArgs.Add("-fvisibility-inlines-hidden");
                commonArgs.Add("-fvisibility-ms-compat");

                if (compileEnvironment.RuntimeTypeInfo)
                    commonArgs.Add("-frtti");
                else
                    commonArgs.Add("-fno-rtti");

                if (compileEnvironment.TreatWarningsAsErrors)
                    commonArgs.Add("-Wall -Werror");

                if (compileEnvironment.DebugInformation)
                    commonArgs.Add("-gdwarf-2");

                commonArgs.Add("-pthread");

                if (compileEnvironment.Sanitizers.HasFlag(Sanitizer.Address))
                {
					commonArgs.Add("-fno-optimize-sibling-calls");
					commonArgs.Add("-fno-omit-frame-pointer");
                    if (compileEnvironment.Optimization)
                        commonArgs.Add("-O1");
                }
                else
                {
                    if (compileEnvironment.FavorSizeOrSpeed == FavorSizeOrSpeed.FastCode)
                        commonArgs.Add("-Ofast");
                    else if (compileEnvironment.FavorSizeOrSpeed == FavorSizeOrSpeed.SmallCode)
                        commonArgs.Add("-Os");
                    if (compileEnvironment.Optimization)
                        commonArgs.Add("-O3");
                    else
                        commonArgs.Add("-O0");
                }

                if (compileEnvironment.BufferSecurityCheck)
                    commonArgs.Add("-fstack-protector");

                if (!compileEnvironment.Inlining)
                {
                    commonArgs.Add("-fno-inline-functions");
                    commonArgs.Add("-fno-inline");
                }

                if (compileEnvironment.EnableExceptions)
                    commonArgs.Add("-fexceptions");
                else
                    commonArgs.Add("-fno-exceptions");
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

        /// <inheritdoc />
        public override void LinkFiles(TaskGraph graph, BuildOptions options, string outputFilePath)
        {
            var linkEnvironment = options.LinkEnv;
            var task = graph.Add<LinkTask>();
            var isArchive = linkEnvironment.Output == LinkerOutput.StaticLibrary || linkEnvironment.Output == LinkerOutput.ImportLibrary;

            // Setup arguments
            var args = new List<string>();
            args.AddRange(options.LinkEnv.CustomArgs);
            {
                args.Add(string.Format("-o \"{0}\"", outputFilePath));
                AddArgsCommon(options, args);
                AddArgsSanitizer(options.CompileEnv.Sanitizers, args);

                if (isArchive)
                {
                    args.Add("-static");
                }
                else
                {
                    args.Add("-dead_strip");
                    args.Add("-rpath @executable_path/");
                    if (linkEnvironment.Output == LinkerOutput.SharedLibrary)
                        args.Add("-dynamiclib");
                }
            }
            SetupLinkFilesArgs(graph, options, args, outputFilePath);

            // Input libraries
            var libraryPaths = new HashSet<string>();
            var dylibs = new HashSet<string>();
            foreach (var library in linkEnvironment.InputLibraries)
            {
                var dir = Path.GetDirectoryName(library);
                var ext = Path.GetExtension(library);
                if (ext == ".framework")
                {
                    args.Add(string.Format("-framework {0}", library.Substring(0, library.Length - ext.Length)));
                }
                else if (string.IsNullOrEmpty(dir))
                {
                    args.Add(string.Format("\"-l{0}\"", library));
                }
                else if (string.IsNullOrEmpty(ext))
                {
                    // Skip executable
                }
                else if (ext == ".dylib")
                {
                    // Link against dynamic library
                    dylibs.Add(library);
                    task.PrerequisiteFiles.Add(library);
                    libraryPaths.Add(dir);
                    args.Add(string.Format("\"{0}\"", library));
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
                if (ext == ".framework")
                {
                    args.Add(string.Format("-framework {0}", library.Substring(0, library.Length - ext.Length)));
                }
                else if (string.IsNullOrEmpty(dir))
                {
                    args.Add(string.Format("\"-l{0}\"", library));
                }
                else if (string.IsNullOrEmpty(ext))
                {
                    // Skip executable
                }
                else if (ext == ".dylib")
                {
                    // Link against dynamic library
                    dylibs.Add(library);
                    task.PrerequisiteFiles.Add(library);
                    libraryPaths.Add(dir);
                    args.Add(string.Format("\"{0}\"", library));
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
                var ext = Path.GetExtension(file);
                if (ext == ".framework")
                {
                    args.Add(string.Format("-framework {0}", file.Substring(0, file.Length - ext.Length)));
                }
                else
                {
                    args.Add(string.Format("\"{0}\"", file.Replace('\\', '/')));
                }
            }

            // Additional lib paths
            libraryPaths.AddRange(linkEnvironment.LibraryPaths);
            foreach (var path in libraryPaths)
            {
                args.Add(string.Format("-L\"{0}\"", path.Replace('\\', '/')));
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

            // Link
            task.WorkingDirectory = options.WorkingDirectory;
            task.CommandPath = isArchive ? ArchiverPath : LinkerPath;
            task.CommandArguments = useResponseFile ? string.Format("@\"{0}\"", responseFile) : string.Join(" ", args);
            task.InfoMessage = "Linking " + outputFilePath;
            task.Cost = task.PrerequisiteFiles.Count;
            task.ProducedFiles.Add(outputFilePath);

            Task lastTask = task;
            if (options.LinkEnv.Output == LinkerOutput.Executable)
            {
                // Fix rpath for dynamic libraries
                foreach (var library in dylibs)
                {
                    var rpathTask = graph.Add<Task>();
                    rpathTask.ProducedFiles.Add(outputFilePath);
                    rpathTask.WorkingDirectory = options.WorkingDirectory;
                    rpathTask.CommandPath = "install_name_tool";
                    var filename = Path.GetFileName(library);
                    var outputFolder = Path.GetDirectoryName(outputFilePath);
                    rpathTask.CommandArguments = string.Format("-change \"{0}/{1}\" \"@loader_path/{1}\" {2}", outputFolder, filename, outputFilePath);
                    rpathTask.InfoMessage = "Fixing rpath to " + filename;
                    rpathTask.Cost = 1;
                    rpathTask.DisableCache = true;
                    rpathTask.DependentTasks = new HashSet<Task>();
                    rpathTask.DependentTasks.Add(lastTask);
                    lastTask = rpathTask;
                }
            }
            else if (options.LinkEnv.Output == LinkerOutput.SharedLibrary)
            {
                // Fix rpath for dynamic library
                var rpathTask = graph.Add<Task>();
                rpathTask.ProducedFiles.Add(outputFilePath);
                rpathTask.WorkingDirectory = Path.GetDirectoryName(outputFilePath);
                rpathTask.CommandPath = "install_name_tool";
                var filename = Path.GetFileName(outputFilePath);
                rpathTask.CommandArguments = string.Format("-id \"@rpath/{0}\" \"{0}\"", filename);
                rpathTask.InfoMessage = "Fixing rpath id " + filename;
                rpathTask.Cost = 1;
                rpathTask.DisableCache = true;
                rpathTask.DependentTasks = new HashSet<Task>();
                rpathTask.DependentTasks.Add(lastTask);
                lastTask = rpathTask;
            }
            // TODO: fix dylib ID: 'install_name_tool -id @rpath/FlaxEngine.dylib FlaxEngine.dylib'
            if (!options.LinkEnv.DebugInformation)
            {
                // Strip debug symbols
                var stripTask = graph.Add<Task>();
                stripTask.ProducedFiles.Add(outputFilePath);
                stripTask.WorkingDirectory = options.WorkingDirectory;
                stripTask.CommandPath = "strip";
                stripTask.CommandArguments = string.Format("\"{0}\" -S", outputFilePath);
                stripTask.InfoMessage = "Striping " + outputFilePath;
                stripTask.Cost = 1;
                stripTask.DisableCache = true;
                stripTask.DependentTasks = new HashSet<Task>();
                stripTask.DependentTasks.Add(lastTask);
            }
        }

        protected virtual void AddArgsCommon(BuildOptions options, List<string> args)
        {
            args.Add("-isysroot \"" + SdkPath + "\"");
            switch (Architecture)
            {
            case TargetArchitecture.x64:
                args.Add("-arch x86_64");
                break;
            case TargetArchitecture.ARM64:
                args.Add("-arch arm64");
                break;
            }
        }

        protected void AddArgsSanitizer(Sanitizer sanitizers, List<string> args)
        {
            if (sanitizers.HasFlag(Sanitizer.Address))
                args.Add("-fsanitize=address");
            if (sanitizers.HasFlag(Sanitizer.Thread))
                args.Add("-fsanitize=thread");
            if (sanitizers.HasFlag(Sanitizer.Undefined))
                args.Add("-fsanitize=undefined");
        }
    }
}
