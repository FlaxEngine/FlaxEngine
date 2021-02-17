// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using Flax.Build.Graph;
using Flax.Build.NativeCpp;

namespace Flax.Build.Platforms
{
    /// <summary>
    /// The Microsoft Windows toolchain implementation.
    /// </summary>
    /// <seealso cref="Toolchain" />
    /// <seealso cref="Flax.Build.Platforms.WindowsToolchainBase" />
    public sealed class WindowsToolchain : WindowsToolchainBase
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="WindowsToolchain"/> class.
        /// </summary>
        /// <param name="platform">The platform.</param>
        /// <param name="architecture">The target architecture.</param>
        public WindowsToolchain(WindowsPlatform platform, TargetArchitecture architecture)
        : base(platform, architecture, WindowsPlatformToolset.Latest, WindowsPlatformSDK.Latest)
        {
        }

        /// <inheritdoc />
        public override void SetupEnvironment(BuildOptions options)
        {
            base.SetupEnvironment(options);

            options.CompileEnv.PreprocessorDefinitions.Add("PLATFORM_WINDOWS");

            options.LinkEnv.InputLibraries.Add("dwmapi.lib");
            options.LinkEnv.InputLibraries.Add("kernel32.lib");
            options.LinkEnv.InputLibraries.Add("user32.lib");
            options.LinkEnv.InputLibraries.Add("comdlg32.lib");
            options.LinkEnv.InputLibraries.Add("advapi32.lib");
            options.LinkEnv.InputLibraries.Add("shell32.lib");
            options.LinkEnv.InputLibraries.Add("ole32.lib");
            options.LinkEnv.InputLibraries.Add("oleaut32.lib");
            options.LinkEnv.InputLibraries.Add("delayimp.lib");
        }

        /// <inheritdoc />
        public override void PreBuild(TaskGraph graph, BuildOptions options)
        {
            base.PreBuild(graph, options);

            // Compile and include resource file if need to
            if (options.Target.Win32ResourceFile != null && !options.Target.IsPreBuilt && options.Target.OutputType == TargetOutputType.Executable)
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
                {
                    args.Add(string.Format("/D \"{0}\"", definition));
                }

                // Add include paths
                foreach (var includePath in options.CompileEnv.IncludePaths)
                {
                    AddIncludePath(args, includePath);
                }

                // Add the resource file to the produced item list
                var outputFile = Path.Combine(options.IntermediateFolder, Path.GetFileNameWithoutExtension(sourceFile) + ".res");
                args.Add(string.Format("/Fo\"{0}\"", outputFile));
                options.LinkEnv.InputFiles.Add(outputFile);

                // Request included files to exist
                var includes = IncludesCache.FindAllIncludedFiles(sourceFile);
                task.PrerequisiteFiles.AddRange(includes);

                // Add the source file
                args.Add(string.Format("\"{0}\"", sourceFile));
                task.ProducedFiles.Add(outputFile);

                task.WorkingDirectory = options.WorkingDirectory;
                task.CommandPath = _resourceCompilerPath;
                task.CommandArguments = string.Join(" ", args);
                task.PrerequisiteFiles.Add(sourceFile);
                task.Cost = 1;
            }
        }
    }
}
