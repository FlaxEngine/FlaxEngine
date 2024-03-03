// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
        : base(platform, architecture, platform.ToolchainRoot, platform.Compiler)
        {
            // Setup system paths
            var includePath = Path.Combine(ToolsetRoot, "usr", "include");
            if (Directory.Exists(includePath))
                SystemIncludePaths.Add(includePath);
            else
                Log.Error($"Missing toolset header files location {includePath}");
            var cppIncludePath = Path.Combine(includePath, "c++", LibStdCppVersion);
            if (Directory.Exists(cppIncludePath))
                SystemIncludePaths.Add(cppIncludePath);
            else
                Log.Verbose($"Missing Clang {ClangVersion} C++ header files location {cppIncludePath}");
            var clangLibPath = Path.Combine(ToolsetRoot, "usr", "lib", "clang");
            var clangIncludePath = Path.Combine(clangLibPath, ClangVersion.Major.ToString(), "include");
            if (!Directory.Exists(clangIncludePath))
            {
                var error = $"Missing Clang {ClangVersion} header files location {clangIncludePath}";
                clangIncludePath = Path.Combine(clangLibPath, ClangVersion.ToString(), "include");
                if (!Directory.Exists(clangIncludePath))
                    Log.Error(error);
            }
            SystemIncludePaths.Add(clangIncludePath);
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

            // Hide all symbols by default
            args.Add("-fvisibility-inlines-hidden");
            args.Add("-fvisibility-ms-compat");

            args.Add(string.Format("-target {0}", ArchitectureName));
            args.Add(string.Format("--sysroot=\"{0}\"", ToolsetRoot.Replace('\\', '/')));

            if (Architecture == TargetArchitecture.x64)
            {
                args.Add("-mssse3");
            }

            if (options.LinkEnv.Output == LinkerOutput.SharedLibrary)
            {
                args.Add("-fPIC");
            }
        }

        /// <inheritdoc />
        protected override void SetupLinkFilesArgs(TaskGraph graph, BuildOptions options, List<string> args, string outputFilePath)
        {
            base.SetupLinkFilesArgs(graph, options, args, outputFilePath);

            args.Add("-Wl,-rpath,\"\\$ORIGIN\"");
            //args.Add("-Wl,--as-needed");
            if (LdKind == "bfd")
                args.Add("-Wl,--copy-dt-needed-entries");
            args.Add("-Wl,--hash-style=gnu");
            //args.Add("-Wl,--build-id");

            if (options.LinkEnv.Output == LinkerOutput.SharedLibrary)
            {
                args.Add("-shared");
                var soname = Path.GetFileNameWithoutExtension(outputFilePath);
                if (soname.StartsWith("lib"))
                    soname = soname.Substring(3);
                //args.Add(string.Format("-Wl,-soname=\"{0}\"", soname));
            }

            args.Add(string.Format("-target {0}", ArchitectureName));
            args.Add(string.Format("--sysroot=\"{0}\"", ToolsetRoot.Replace('\\', '/')));

            // Link core libraries
            args.Add("-pthread");
            args.Add("-ldl");
            args.Add("-lrt");
            args.Add("-lz");

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
