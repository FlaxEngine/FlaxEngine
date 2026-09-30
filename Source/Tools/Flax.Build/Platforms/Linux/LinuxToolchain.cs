// Copyright (c) Wojciech Figat. All rights reserved.

using Flax.Build.Graph;
using Flax.Build.NativeCpp;
using System;
using System.Collections.Generic;
using System.IO;

namespace Flax.Build
{
    partial class LinuxConfiguration
    {
        /// <summary>
        /// Specifies the minimum Clang compiler version to use on Linux (eg. 10).
        /// </summary>
        [CommandLine("clangMinVer", "<version>", "Specifies the minimum Clang compiler version to use on Linux (eg. 10).")]
        public static string ClangMinVer = "14";
    }
}

namespace Flax.Build.Platforms
{
    /// <summary>
    /// The Linux build toolchain implementation.
    /// </summary>
    /// <seealso cref="Platform" />
    /// <seealso cref="UnixToolchain" />
    public class LinuxToolchain : UnixToolchain
    {
        private List<string> GetPkgConfigLibDirs()
        {
            var targetTriple = Architecture switch
            {
                TargetArchitecture.x86 => "i686-linux-gnu",
                TargetArchitecture.x64 => "x86_64-linux-gnu",
                TargetArchitecture.ARM => "arm-linux-gnueabihf",
                TargetArchitecture.ARM64 => "aarch64-linux-gnu",
                _ => null,
            };

            var result = new List<string>();
            foreach (var path in new[]
            {
                Path.Combine(ToolsetRoot, "usr", "lib", "pkgconfig"),
                Path.Combine(ToolsetRoot, "usr", "share", "pkgconfig"),
                Path.Combine(ToolsetRoot, "lib", "pkgconfig"),
                Path.Combine(ToolsetRoot, "share", "pkgconfig"),
                string.IsNullOrEmpty(targetTriple) ? null : Path.Combine(ToolsetRoot, "usr", "lib", targetTriple, "pkgconfig"),
                string.IsNullOrEmpty(targetTriple) ? null : Path.Combine(ToolsetRoot, "lib", targetTriple, "pkgconfig"),
            })
            {
                if (!string.IsNullOrEmpty(path) && Directory.Exists(path) && !result.Contains(path))
                    result.Add(path);
            }
            return result;
        }

        private static string FindFirstExistingDirectory(params string[] candidates)
        {
            foreach (var candidate in candidates)
            {
                if (!string.IsNullOrEmpty(candidate) && Directory.Exists(candidate))
                    return candidate;
            }
            return null;
        }

        private string TryGetClangResourceIncludePath()
        {
            try
            {
                var resourceDir = Utilities.ReadProcessOutput(ClangPath, "-print-resource-dir");
                if (!string.IsNullOrWhiteSpace(resourceDir))
                {
                    var includePath = Path.Combine(resourceDir.Trim(), "include");
                    if (Directory.Exists(includePath))
                        return includePath;
                }
            }
            catch
            {
            }
            return null;
        }
        /// <summary>
        /// Initializes a new instance of the <see cref="LinuxToolchain"/> class.
        /// </summary>
        /// <param name="platform">The platform.</param>
        /// <param name="architecture">The target architecture.</param>
        public LinuxToolchain(LinuxPlatform platform, TargetArchitecture architecture)
        : base(platform, architecture, platform.ToolchainRoot, platform.Compiler)
        {
            // Check version
            if (Utilities.ParseVersion(LinuxConfiguration.ClangMinVer, out var minClangVer) && ClangVersion < minClangVer)
                Log.Error($"Old Clang version {ClangVersion}. Minimum supported is {minClangVer}.");

            // Setup system paths
            var includePath = FindFirstExistingDirectory(
                Path.Combine(ToolsetRoot, "usr", "include"),
                Path.Combine(ToolsetRoot, "include"));
            if (includePath != null)
            {
                SystemIncludePaths.Add(includePath);

                var cppIncludePath = Path.Combine(includePath, "c++", LibStdCppVersion);
                if (Directory.Exists(cppIncludePath))
                    SystemIncludePaths.Add(cppIncludePath);
                else
                    Log.Verbose($"Missing Clang {ClangVersion} C++ header files location {cppIncludePath}");
            }
            else
            {
                Log.Verbose($"Missing toolset header files location under {ToolsetRoot}");
            }

            var clangIncludePath = TryGetClangResourceIncludePath();
            if (clangIncludePath == null)
            {
                var clangVersionMajor = ClangVersion.Major.ToString();
                var clangVersionFull = ClangVersion.ToString();
                var clangIncludeCandidates = new[]
                {
                    Path.Combine(ToolsetRoot, "usr", "lib", "clang", clangVersionMajor, "include"),
                    Path.Combine(ToolsetRoot, "usr", "lib", "clang", clangVersionFull, "include"),
                    Path.Combine(ToolsetRoot, "lib", "clang", clangVersionMajor, "include"),
                    Path.Combine(ToolsetRoot, "lib", "clang", clangVersionFull, "include"),
                };
                clangIncludePath = FindFirstExistingDirectory(clangIncludeCandidates);
            }

            if (clangIncludePath != null)
                SystemIncludePaths.Add(clangIncludePath);
            else
                Log.Error($"Missing Clang {ClangVersion} header files location (resource-dir and sysroot probes failed).");
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

            // Include any external folders into rpath for proper dlopen (eg. when opening Editor project with plugins)
            if (options.LinkEnv.Output == LinkerOutput.SharedLibrary || options.LinkEnv.Output == LinkerOutput.Executable)
            {
                var originDir = Path.GetDirectoryName(outputFilePath);
                foreach (var lib in options.LinkEnv.InputLibraries)
                {
                    var libDir = Path.GetDirectoryName(lib);
                    if (libDir != originDir)
                        args.Add($"-Wl,-rpath,\"{libDir}\"");
                }
            }

            args.Add(string.Format("-target {0}", ArchitectureName));
            args.Add(string.Format("--sysroot=\"{0}\"", ToolsetRoot.Replace('\\', '/')));

            // Link core libraries
            args.Add("-pthread");
            args.Add("-ldl");
            args.Add("-lrt");
            args.Add("-lz");

            // Link X11
            PkgConfig.AddLibsOrFallback(args, "x11 xcursor xinerama xfixes", new[] { "-lX11", "-lXcursor", "-lXinerama", "-lXfixes" }, ToolsetRoot, GetPkgConfigLibDirs());

            if (EngineConfiguration.WithSDL(options))
            {
                // Link Wayland + GLib for libportal
                PkgConfig.AddLibsOrFallback(args, "wayland-client glib-2.0 gio-2.0 gobject-2.0", new[] { "-lwayland-client", "-lglib-2.0", "-lgio-2.0", "-lgobject-2.0" }, ToolsetRoot, GetPkgConfigLibDirs());
            }
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
