// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using Flax.Build.Graph;
using Flax.Build.NativeCpp;

namespace Flax.Build.Platforms
{
    /// <summary>
    /// The Linux build toolchain implementation.
    /// </summary>
    /// <seealso cref="Platform" />
    /// <seealso cref="UnixToolchain" />
    public class LinuxToolchain : UnixToolchain
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="LinuxToolchain"/> class.
        /// </summary>
        /// <param name="platform">The platform.</param>
        /// <param name="architecture">The target architecture.</param>
        public LinuxToolchain(LinuxPlatform platform, TargetArchitecture architecture)
        : base(platform, architecture, platform.ToolchainRoot, platform.UseSystemCompiler)
        {
            // Setup system paths
            SystemIncludePaths.Add(Path.Combine(ToolsetRoot, "usr", "include"));
            SystemIncludePaths.Add(Path.Combine(ToolsetRoot, "include", "c++", "5.2.0"));
            SystemIncludePaths.Add(Path.Combine(ToolsetRoot, "lib", "clang", "7.0.1", "include"));
        }

        /// <inheritdoc />
        public override void SetupEnvironment(BuildOptions options)
        {
            base.SetupEnvironment(options);

            options.CompileEnv.PreprocessorDefinitions.Add("PLATFORM_LINUX");
            if (Architecture == TargetArchitecture.x64)
                options.CompileEnv.PreprocessorDefinitions.Add("_LINUX64");
        }

        /// <inheritdoc />
        protected override void SetupCompileCppFilesArgs(TaskGraph graph, BuildOptions options, List<string> args, string outputPath)
        {
            base.SetupCompileCppFilesArgs(graph, options, args, outputPath);

            args.Add(string.Format("-target {0}", ArchitectureName));
            args.Add(string.Format("--sysroot=\"{0}\"", ToolsetRoot.Replace('\\', '/')));
        }

        /// <inheritdoc />
        protected override void SetupLinkFilesArgs(TaskGraph graph, BuildOptions options, List<string> args, string outputFilePath)
        {
            base.SetupLinkFilesArgs(graph, options, args, outputFilePath);

            // Speed up build
            //args.Add("-Wl,--as-needed");
            args.Add("-Wl,--hash-style=gnu");
            //args.Add("-Wl,--build-id");

            if (options.LinkEnv.Output == LinkerOutput.SharedLibrary)
            {
                args.Add("-shared");
                args.Add(string.Format("-soname=\"{0}\"", Path.GetFileNameWithoutExtension(outputFilePath)));
            }

            args.Add(string.Format("-target {0}", ArchitectureName));
            args.Add(string.Format("--sysroot=\"{0}\"", ToolsetRoot.Replace('\\', '/')));

            // Link core libraries
            args.Add("-pthread");
            args.Add("-ldl");
            args.Add("-lrt");

            // Link X11
            args.Add("-L/usr/X11R6/lib");
            args.Add("-lX11");
            args.Add("-lXcursor");
            args.Add("-lXinerama");
        }

        /// <inheritdoc />
        protected override Task CreateBinary(TaskGraph graph, BuildOptions options, string outputFilePath)
        {
            var task = base.CreateBinary(graph, options, outputFilePath);
            task.CommandPath = ClangPath;
            return task;
        }
    }
}
