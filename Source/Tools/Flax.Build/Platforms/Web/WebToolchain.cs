// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using Flax.Build.Graph;
using Flax.Build.NativeCpp;

namespace Flax.Build.Platforms
{
    /// <summary>
    /// The build toolchain for Web with Emscripten.
    /// </summary>
    /// <seealso cref="Toolchain" />
    public sealed class WebToolchain : Toolchain
    {
        private string _sysrootPath;
        private string _compilerPath;
        private Version _compilerVersion;

        /// <summary>
        /// Initializes a new instance of the <see cref="WebToolchain"/> class.
        /// </summary>
        /// <param name="platform">The platform.</param>
        /// <param name="architecture">The target architecture.</param>
        public WebToolchain(WebPlatform platform, TargetArchitecture architecture)
        : base(platform, architecture)
        {
            var sdkPath = EmscriptenSdk.Instance.EmscriptenPath;

            // Setup tools
            _compilerPath = Path.Combine(sdkPath, "emscripten", "emcc");
            var clangPath = Path.Combine(sdkPath, "bin", "clang++");
            if (Platform.BuildTargetPlatform == TargetPlatform.Windows)
            {
                _compilerPath += ".bat";
                clangPath += ".exe";
            }

            // Determinate compiler version
            _compilerVersion = UnixToolchain.GetClangVersion(platform.Target, clangPath);

            // Setup system paths
            SystemIncludePaths.Add(Path.Combine(sdkPath, "lib", "clang", _compilerVersion.Major.ToString(), "include"));
            SystemIncludePaths.Add(Path.Combine(sdkPath, "emscripten", "system", "include"));
            SystemIncludePaths.Add(Path.Combine(sdkPath, "emscripten", "system", "lib"));
            _sysrootPath = Path.Combine(sdkPath, "emscripten/cache/sysroot/include");
        }

        public static string GetLibName(string path)
        {
            var libName = Path.GetFileNameWithoutExtension(path);
            if (libName.StartsWith("lib"))
                libName = libName.Substring(3);
            return libName;
        }

        /// <inheritdoc />
        public override string DllExport => "__attribute__((__visibility__(\\\"default\\\")))";

        /// <inheritdoc />
        public override string DllImport => "";

        /// <inheritdoc />
        public override TargetCompiler Compiler => TargetCompiler.Clang;

        /// <inheritdoc />
        public override string NativeCompilerPath => _compilerPath;

        /// <inheritdoc />
        public override void LogInfo()
        {
            Log.Info("Clang version: " + _compilerVersion);
        }

        /// <inheritdoc />
        public override void SetupEnvironment(BuildOptions options)
        {
            base.SetupEnvironment(options);

            options.CompileEnv.PreprocessorDefinitions.Add("PLATFORM_WEB");
            options.CompileEnv.PreprocessorDefinitions.Add("PLATFORM_UNIX");
            options.CompileEnv.PreprocessorDefinitions.Add("__EMSCRIPTEN__");
            options.CompileEnv.EnableExceptions = false;
            options.CompileEnv.CpuArchitecture = CpuArchitecture.None; // TODO: try SIMD support in Emscripten
        }

        private void AddSharedArgs(List<string> args, BuildOptions options, bool debugInformation, bool optimization)
        {
            if (debugInformation)
                args.Add("-g2");
            else
                args.Add("-g0");

            if (options.CompileEnv.FavorSizeOrSpeed == FavorSizeOrSpeed.SmallCode)
                args.Add("-Os");
            if (options.CompileEnv.FavorSizeOrSpeed == FavorSizeOrSpeed.FastCode)
                args.Add("-O3");
            else if (optimization && options.Configuration == TargetConfiguration.Release)
                args.Add("-O3");
            else if (optimization)
                args.Add("-O2");
            else
                args.Add("-O0");

            if (options.CompileEnv.RuntimeTypeInfo)
                args.Add("-frtti");
            else
                args.Add("-fno-rtti");

            if (options.CompileEnv.TreatWarningsAsErrors)
                args.Add("-Wall -Werror");

            if (options.CompileEnv.EnableExceptions)
                args.Add("-fexceptions");
            else
                args.Add("-fno-exceptions");
        }

        /// <inheritdoc />
        public override CompileOutput CompileCppFiles(TaskGraph graph, BuildOptions options, List<string> sourceFiles, string outputPath)
        {
            var output = new CompileOutput();

            // Setup arguments shared by all source files
            var commonArgs = new List<string>();
            commonArgs.AddRange(options.CompileEnv.CustomArgs);
            {
                commonArgs.Add("-c");

                AddSharedArgs(commonArgs, options, options.CompileEnv.DebugInformation, options.CompileEnv.Optimization);
            }

            // Add preprocessor definitions
            foreach (var definition in options.CompileEnv.PreprocessorDefinitions)
            {
                commonArgs.Add(string.Format("-D \"{0}\"", definition));
            }

            // Add include paths
            foreach (var includePath in options.CompileEnv.IncludePaths)
            {
                if (SystemIncludePaths.Contains(includePath)) // TODO: fix SystemIncludePaths so this chack can be removed
                    continue; // Skip system includes as those break compilation (need to fix sys root linking for emscripten)
                commonArgs.Add(string.Format("-I\"{0}\"", includePath.Replace('\\', '/')));
            }

            // Hack for sysroot includes
            commonArgs.Add(string.Format("-I\"{0}\"", _sysrootPath.Replace('\\', '/')));

            // Compile all C/C++ files
            var args = new List<string>();
            foreach (var sourceFile in sourceFiles)
            {
                var sourceFilename = Path.GetFileNameWithoutExtension(sourceFile);
                var task = graph.Add<CompileCppTask>();

                // Use shared arguments
                args.Clear();
                args.AddRange(commonArgs);

                // Language for the file
                args.Add("-x");
                if (sourceFile.EndsWith(".c", StringComparison.OrdinalIgnoreCase))
                    args.Add("c");
                else
                {
                    args.Add("c++");

                    // C++ version
                    switch (options.CompileEnv.CppVersion)
                    {
                    case CppVersion.Cpp14:
                        args.Add("-std=c++14");
                        break;
                    case CppVersion.Cpp17:
                    case CppVersion.Latest:
                        args.Add("-std=c++17");
                        break;
                    case CppVersion.Cpp20:
                        args.Add("-std=c++20");
                        break;
                    }
                }

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
                task.CommandPath = _compilerPath;
                task.CommandArguments = string.Join(" ", args);
                task.PrerequisiteFiles.Add(sourceFile);
                task.InfoMessage = Path.GetFileName(sourceFile);
                task.Cost = task.PrerequisiteFiles.Count; // TODO: include source file size estimation to improve tasks sorting
            }

            return output;
        }

        private Task CreateBinary(TaskGraph graph, BuildOptions options, string outputFilePath)
        {
            var task = graph.Add<LinkTask>();

            // Setup arguments
            var args = new List<string>();
            args.AddRange(options.LinkEnv.CustomArgs);
            {
                args.Add(string.Format("-o \"{0}\"", outputFilePath.Replace('\\', '/')));

                AddSharedArgs(args, options, options.LinkEnv.DebugInformation, options.LinkEnv.Optimization);

            }

            args.Add("-Wl,--start-group");

            // Input libraries
            var libraryPaths = new HashSet<string>();
            var dynamicLibExt = Platform.SharedLibraryFileExtension;
            foreach (var library in options.LinkEnv.InputLibraries.Concat(options.Libraries))
            {
                var dir = Path.GetDirectoryName(library);
                var ext = Path.GetExtension(library);
                if (library.StartsWith("--use-port="))
                {
                    // Ports (https://emscripten.org/docs/compiling/Building-Projects.html#emscripten-ports)
                    args.Add(library);
                }
                else if (string.IsNullOrEmpty(dir))
                {
                    args.Add(string.Format("\"-l{0}\"", library));
                }
                else if (string.IsNullOrEmpty(ext))
                {
                    // Skip executable
                }
                else if (ext == dynamicLibExt)
                {
                    // Link against dynamic library
                    task.PrerequisiteFiles.Add(library);
                    libraryPaths.Add(dir);
                    args.Add(string.Format("\"-l{0}\"", GetLibName(library)));
                }
                else
                {
                    task.PrerequisiteFiles.Add(library);
                    args.Add(string.Format("\"-l{0}\"", GetLibName(library)));
                }
            }

            // Input files (link static libraries last)
            task.PrerequisiteFiles.AddRange(options.LinkEnv.InputFiles);
            foreach (var file in options.LinkEnv.InputFiles.Where(x => !x.EndsWith(".a")).Concat(options.LinkEnv.InputFiles.Where(x => x.EndsWith(".a"))))
            {
                args.Add(string.Format("\"{0}\"", file.Replace('\\', '/')));
            }

            // Additional lib paths
            libraryPaths.AddRange(options.LinkEnv.LibraryPaths);
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
            task.CommandPath = _compilerPath;
            task.CommandArguments = useResponseFile ? string.Format("@\"{0}\"", responseFile) : string.Join(" ", args);
            task.InfoMessage = "Linking " + outputFilePath;
            task.Cost = task.PrerequisiteFiles.Count;
            task.ProducedFiles.Add(outputFilePath);

            return task;
        }

        /// <inheritdoc />
        public override void LinkFiles(TaskGraph graph, BuildOptions options, string outputFilePath)
        {
            outputFilePath = Utilities.NormalizePath(outputFilePath);

            Task linkTask;
            switch (options.LinkEnv.Output)
            {
            case LinkerOutput.Executable:
            case LinkerOutput.SharedLibrary:
                linkTask = CreateBinary(graph, options, outputFilePath);
                break;
            case LinkerOutput.StaticLibrary:
            case LinkerOutput.ImportLibrary:
            default:
                throw new ArgumentOutOfRangeException();
            }
        }
    }
}
