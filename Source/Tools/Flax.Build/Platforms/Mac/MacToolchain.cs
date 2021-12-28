// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.Collections.Generic;
using Flax.Build.Graph;
using Flax.Build.NativeCpp;

namespace Flax.Build.Platforms
{
    /// <summary>
    /// The build toolchain for all Mac systems.
    /// </summary>
    /// <seealso cref="Toolchain" />
    public sealed class MacToolchain : Toolchain
    {
        private string MinMacOSXVer = "10.14";
        
        public string ToolchainPath;
        public string SdkPath;
        public string ClangPath;
        public string LinkerPath;
        public string ArchiverPath;

        /// <summary>
        /// Initializes a new instance of the <see cref="MacToolchain"/> class.
        /// </summary>
        /// <param name="platform">The platform.</param>
        /// <param name="architecture">The target architecture.</param>
        public MacToolchain(MacPlatform platform, TargetArchitecture architecture)
        : base(platform, architecture)
        {
            // Setup tools paths
            if (platform.XCodePath.Contains("/Xcode.app"))
            {
                // XCode App
                ToolchainPath = Path.Combine(platform.XCodePath, "Toolchains/XcodeDefault.xctoolchain");
                SdkPath = Path.Combine(platform.XCodePath, "Platforms/MacOSX.platform/Developer");
            }
            else
            {
                // XCode Command Line Tools
                ToolchainPath = SdkPath = platform.XCodePath;
                if (!Directory.Exists(Path.Combine(ToolchainPath, "usr/bin")))
                    throw new Exception("Missing XCode Command Line Tools. Run 'xcode-select --install'.");
            }
            ClangPath = Path.Combine(ToolchainPath, "usr/bin/clang++");
            LinkerPath = Path.Combine(ToolchainPath, "usr/bin/clang++");
            ArchiverPath = Path.Combine(ToolchainPath, "usr/bin/libtool");
            var clangVersion = UnixToolchain.GetClangVersion(ClangPath);
            SdkPath = Path.Combine(SdkPath, "SDKs");
            var sdks = Directory.GetDirectories(SdkPath);
            var sdkPrefix = "MacOSX";
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
            SystemIncludePaths.Add(Path.Combine(ToolchainPath, "usr/include"));
            SystemIncludePaths.Add(Path.Combine(ToolchainPath, "usr/include/c++/v1"));
            SystemIncludePaths.Add(Path.Combine(ToolchainPath, "usr/lib/clang", clangVersion.ToString(3), "include"));
            SystemIncludePaths.Add(Path.Combine(SdkPath, "usr/include"));
            SystemLibraryPaths.Add(Path.Combine(SdkPath, "usr/lib"));
        }

        /// <inheritdoc />
        public override string DllExport => "__attribute__((__visibility__(\\\"default\\\")))";

        /// <inheritdoc />
        public override string DllImport => "";

        /// <inheritdoc />
        public override void LogInfo()
        {
            Log.Info("Toolchain: " + ToolchainPath);
            Log.Info("SDK: " + SdkPath);
            Log.Info("Clang version: " + Utilities.ReadProcessOutput(ClangPath, "--version"));
        }

        /// <inheritdoc />
        public override void SetupEnvironment(BuildOptions options)
        {
            base.SetupEnvironment(options);

            options.CompileEnv.PreprocessorDefinitions.Add("PLATFORM_MAC");
        }

        /// <inheritdoc />
        public override CompileOutput CompileCppFiles(TaskGraph graph, BuildOptions options, List<string> sourceFiles, string outputPath)
        {
            var compileEnvironment = options.CompileEnv;
            var output = new CompileOutput();

            // Setup arguments shared by all source files
            var commonArgs = new List<string>();
            {
                commonArgs.Add("-c");
                commonArgs.Add("-fmessage-length=0");
                commonArgs.Add("-pipe");
                commonArgs.Add("-x");
                commonArgs.Add("c++");
                commonArgs.Add("-std=c++14");
                commonArgs.Add("-stdlib=libc++");
                commonArgs.Add("-mmacosx-version-min=" + MinMacOSXVer);

                commonArgs.Add("-Wdelete-non-virtual-dtor");
                commonArgs.Add("-fno-math-errno");
                commonArgs.Add("-fasm-blocks");
                commonArgs.Add("-fdiagnostics-format=msvc");

                commonArgs.Add("-Wno-absolute-value");
                commonArgs.Add("-Wno-nullability-completeness");

                // Hide all symbols by default
                commonArgs.Add("-fvisibility-inlines-hidden");
                commonArgs.Add("-fvisibility-ms-compat");

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
                    commonArgs.Add("-gdwarf-2");

                commonArgs.Add("-pthread");

                if (compileEnvironment.Optimization)
                    commonArgs.Add("-O3");
                else
                    commonArgs.Add("-O0");

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

            // Setup arguments
            var args = new List<string>();
            {
                args.Add(string.Format("-o \"{0}\"", outputFilePath));
                args.Add("-mmacosx-version-min=" + MinMacOSXVer);

                if (!options.LinkEnv.DebugInformation)
                    args.Add("-Wl,--strip-debug");

                switch (linkEnvironment.Output)
                {
                case LinkerOutput.Executable:
                    break;
                case LinkerOutput.SharedLibrary:
                    args.Add("-dynamiclib");
                    break;
                case LinkerOutput.StaticLibrary:
                case LinkerOutput.ImportLibrary:
                    break;
                default: throw new ArgumentOutOfRangeException();
                }
            }

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
                else if (ext == ".dylib")
                {
                    // Link against dynamic library
                    task.PrerequisiteFiles.Add(library);
                    libraryPaths.Add(dir);
                    args.Add(string.Format("\"-l{0}\"", UnixToolchain.GetLibName(library)));
                }
                else
                {
                    task.PrerequisiteFiles.Add(library);
                    args.Add(string.Format("\"{0}\"", UnixToolchain.GetLibName(library)));
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
                else if (ext == ".dylib")
                {
                    // Link against dynamic library
                    task.PrerequisiteFiles.Add(library);
                    libraryPaths.Add(dir);
                    args.Add(string.Format("\"-l{0}\"", UnixToolchain.GetLibName(library)));
                }
                else
                {
                    task.PrerequisiteFiles.Add(library);
                    args.Add(string.Format("\"{0}\"", UnixToolchain.GetLibName(library)));
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
            switch (linkEnvironment.Output)
            {
            case LinkerOutput.Executable:
            case LinkerOutput.SharedLibrary:
                task.CommandPath = LinkerPath;
                break;
            case LinkerOutput.StaticLibrary:
            case LinkerOutput.ImportLibrary:
                task.CommandPath = ArchiverPath;
                break;
            default: throw new ArgumentOutOfRangeException();
            }
            task.CommandArguments = useResponseFile ? string.Format("@\"{0}\"", responseFile) : string.Join(" ", args);
            task.InfoMessage = "Linking " + outputFilePath;
            task.Cost = task.PrerequisiteFiles.Count;
            task.ProducedFiles.Add(outputFilePath);
        }
    }
}
