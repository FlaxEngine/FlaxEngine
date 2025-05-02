// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using Flax.Build.Graph;
using Flax.Build.NativeCpp;

namespace Flax.Build
{
    partial class Configuration
    {
        /// <summary>
        /// Specifies the minimum Windows version to use (eg. 10).
        /// </summary>
        [CommandLine("winMinVer", "<version>", "Specifies the minimum Windows version to use (eg. 10).")]
        public static string WindowsMinVer = "7";
    }
}

namespace Flax.Build.Platforms
{
    /// <summary>
    /// The Microsoft Windows toolchain implementation.
    /// </summary>
    /// <seealso cref="Toolchain" />
    /// <seealso cref="Flax.Build.Platforms.WindowsToolchainBase" />
    public sealed class WindowsToolchain : WindowsToolchainBase
    {
        private Version _minVersion;

        /// <summary>
        /// Initializes a new instance of the <see cref="WindowsToolchain"/> class.
        /// </summary>
        /// <param name="platform">The platform.</param>
        /// <param name="architecture">The target architecture.</param>
        public WindowsToolchain(WindowsPlatform platform, TargetArchitecture architecture)
        : base(platform, architecture, WindowsPlatformToolset.Latest, WindowsPlatformSDK.Latest)
        {
            // Select minimum Windows version
            if (!Version.TryParse(Configuration.WindowsMinVer, out _minVersion))
            {
                if (int.TryParse(Configuration.WindowsMinVer, out var winMinVerMajor))
                    _minVersion = new Version(winMinVerMajor, 0);
                else
                    _minVersion = new Version(7, 0);
            }
        }

        /// <inheritdoc />
        public override void SetupEnvironment(BuildOptions options)
        {
            base.SetupEnvironment(options);

            options.CompileEnv.PreprocessorDefinitions.Add("PLATFORM_WINDOWS");

            int winVer;
            if (_minVersion.Major >= 10)
                winVer = 0x0A00; // Windows 10
            else if (_minVersion.Major == 8 && _minVersion.Minor >= 1)
                winVer = 0x0603; // Windows 8.1
            else if (_minVersion.Major == 8)
                winVer = 0x0602; // Windows 8
            else
                winVer = 0x0601; // Windows 7
            options.CompileEnv.PreprocessorDefinitions.Add($"WINVER=0x{winVer:X4}");

            options.LinkEnv.InputLibraries.Add("dwmapi.lib");
            options.LinkEnv.InputLibraries.Add("kernel32.lib");
            options.LinkEnv.InputLibraries.Add("user32.lib");
            options.LinkEnv.InputLibraries.Add("comdlg32.lib");
            options.LinkEnv.InputLibraries.Add("advapi32.lib");
            options.LinkEnv.InputLibraries.Add("shell32.lib");
            options.LinkEnv.InputLibraries.Add("ole32.lib");
            options.LinkEnv.InputLibraries.Add("oleaut32.lib");
            options.LinkEnv.InputLibraries.Add("delayimp.lib");

            if (options.Architecture == TargetArchitecture.ARM64)
            {
                options.CompileEnv.PreprocessorDefinitions.Add("USE_SOFT_INTRINSICS");
                options.LinkEnv.InputLibraries.Add("softintrin.lib");
            }
        }

        /// <inheritdoc />
        protected override void SetupCompileCppFilesArgs(TaskGraph graph, BuildOptions options, List<string> args)
        {
            base.SetupCompileCppFilesArgs(graph, options, args);

            if (Toolset >= WindowsPlatformToolset.v142 && _minVersion.Major >= 11)
            {
                // Windows 11 requires SSE4.2
                args.Add("/d2archSSE42");
            }
        }

        /// <inheritdoc />
        public override void LinkFiles(TaskGraph graph, BuildOptions options, string outputFilePath)
        {
            // Compile and include resource file if need to
            if (options.Target.Win32ResourceFile != null &&
                !options.Target.IsPreBuilt &&
                (options.LinkEnv.Output == LinkerOutput.Executable || options.LinkEnv.Output == LinkerOutput.SharedLibrary))
            {
                var task = graph.Add<CompileCppTask>();
                var args = new List<string>();
                var sourceFile = options.Target.Win32ResourceFile;

                // Suppress Startup Banner
                args.Add("/nologo");

                // Language
                args.Add("/l 0x0409");

                // Add preprocessor definitions
                foreach (var definition in options.CompileEnv.PreprocessorDefinitions)
                    args.Add(string.Format("/D \"{0}\"", definition));
                args.Add(string.Format("/D \"ORIGINAL_FILENAME=\\\"{0}\\\"\"", Path.GetFileName(outputFilePath)));
                args.Add(string.Format("/D \"PRODUCT_NAME=\\\"{0}\\\"\"", options.Target.ProjectName + " " + options.Target.ConfigurationName));
                args.Add(string.Format("/D \"PRODUCT_NAME_INTERNAL=\\\"{0}\\\"\"", options.Target.Name));

                // Add include paths
                foreach (var includePath in options.CompileEnv.IncludePaths)
                    AddIncludePath(args, includePath);

                // Add the resource file to the produced item list
                var outputFile = Path.Combine(options.IntermediateFolder, Path.GetFileName(outputFilePath) + ".res");
                args.Add(string.Format("/Fo\"{0}\"", outputFile));
                options.LinkEnv.InputFiles.Add(outputFile);

                // Request included files to exist
                task.PrerequisiteFiles.AddRange(IncludesCache.FindAllIncludedFiles(sourceFile));

                // Add the source file
                args.Add(string.Format("\"{0}\"", sourceFile));
                task.ProducedFiles.Add(outputFile);

                task.WorkingDirectory = options.WorkingDirectory;
                task.CommandPath = _resourceCompilerPath;
                task.CommandArguments = string.Join(" ", args);
                task.PrerequisiteFiles.Add(sourceFile);
                task.Cost = 1;
            }

            base.LinkFiles(graph, options, outputFilePath);
        }
    }
}
