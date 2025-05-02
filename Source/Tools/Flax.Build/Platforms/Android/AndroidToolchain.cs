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
        /// Specifies the Android API level to use (eg. 24).
        /// </summary>
        [CommandLine("androidPlatformApi", "<version>", "Specifies the Android API level to use (eg. 24).")]
        public static int AndroidPlatformApi = 24;
    }
}

namespace Flax.Build.Platforms
{
    /// <summary>
    /// The Android build toolchain implementation.
    /// </summary>
    /// <seealso cref="Platform" />
    /// <seealso cref="UnixToolchain" />
    public class AndroidToolchain : UnixToolchain
    {
        /// <summary>
        /// Initializes a new instance of the <see cref="AndroidToolchain"/> class.
        /// </summary>
        /// <param name="platform">The platform.</param>
        /// <param name="architecture">The target architecture.</param>
        /// <param name="toolchainRoot">The toolchain root path.</param>
        public AndroidToolchain(AndroidPlatform platform, TargetArchitecture architecture, string toolchainRoot)
        : base(platform, architecture, toolchainRoot, null, string.Empty)
        {
            var toolchain = ToolsetRoot.Replace('\\', '/');
            SystemIncludePaths.Add(Path.Combine(toolchain, "sources/usr/include/c++/v1").Replace('\\', '/'));
            SystemIncludePaths.Add(Path.Combine(toolchain, "sysroot/usr/include").Replace('\\', '/'));
            SystemIncludePaths.Add(Path.Combine(toolchain, "sysroot/usr/local/include").Replace('\\', '/'));
            SystemIncludePaths.Add(Path.Combine(toolchain, "lib64/clang/9.0.8/include").Replace('\\', '/'));
        }

        /// <summary>
        /// Gets the Android ABI name for a given processor architecture.
        /// </summary>
        /// <param name="architecture">The architecture.</param>
        /// <returns>The Android ABI name.</returns>
        public static string GetAbiName(TargetArchitecture architecture)
        {
            switch (architecture)
            {
            case TargetArchitecture.x86: return "x86";
            case TargetArchitecture.x64: return "x86_64";
            case TargetArchitecture.ARM: return "armeabi-v7a";
            case TargetArchitecture.ARM64: return "arm64-v8a";
            default: throw new InvalidArchitectureException(architecture);
            }
        }

        /// <inheritdoc />
        public override void SetupEnvironment(BuildOptions options)
        {
            base.SetupEnvironment(options);

            options.CompileEnv.PreprocessorDefinitions.Add("PLATFORM_ANDROID");
            options.CompileEnv.PreprocessorDefinitions.Add(string.Format("__ANDROID_API__={0}", Configuration.AndroidPlatformApi));

            options.LinkEnv.InputLibraries.Add("c");
            options.LinkEnv.InputLibraries.Add("z");
            options.LinkEnv.InputLibraries.Add("log");
            options.LinkEnv.InputLibraries.Add("mediandk");
            options.LinkEnv.InputLibraries.Add("android");
        }

        /// <inheritdoc />
        protected override void SetupCompileCppFilesArgs(TaskGraph graph, BuildOptions options, List<string> args, string outputPath)
        {
            base.SetupCompileCppFilesArgs(graph, options, args, outputPath);

            var toolchain = ToolsetRoot.Replace('\\', '/');
            args.Add(string.Format("--target={0}", ArchitectureName));
            args.Add(string.Format("--gcc-toolchain=\"{0}\"", toolchain));
            args.Add(string.Format("--sysroot=\"{0}/sysroot\"", toolchain));

            args.Add("-fpic");
            args.Add("-funwind-tables");
            args.Add("-no-canonical-prefixes");
            if (options.Configuration != TargetConfiguration.Release)
            {
                args.Add("-fstack-protector-strong");
                args.Add("-D_FORTIFY_SOURCE=2");
            }
            else
            {
                args.Add("-D_FORTIFY_SOURCE=1");
            }
            if (options.CompileEnv.DebugInformation)
            {
                args.Add("-g2 -gdwarf-4");
                args.Add("-fno-omit-frame-pointer");
                args.Add("-fno-function-sections");
            }

            switch (Architecture)
            {
            case TargetArchitecture.x86:
                args.Add("-march=atom");
                if (Configuration.AndroidPlatformApi < 24)
                    args.Add("-mstackrealign");
                break;
            case TargetArchitecture.x64:
                args.Add("-march=atom");
                break;
            case TargetArchitecture.ARM:
                args.Add("-march=armv7-a");
                args.Add("-mfloat-abi=softfp");
                args.Add("-mfpu=vfpv3-d16");
                break;
            case TargetArchitecture.ARM64:
                args.Add("-march=armv8-a");
                break;
            default: throw new InvalidArchitectureException(Architecture);
            }
        }

        /// <inheritdoc />
        protected override void SetupLinkFilesArgs(TaskGraph graph, BuildOptions options, List<string> args, string outputFilePath)
        {
            base.SetupLinkFilesArgs(graph, options, args, outputFilePath);

            args.Add("-shared");
            args.Add("-no-canonical-prefixes");
            args.Add("-Wl,-Bsymbolic");
            args.Add("-Wl,--build-id=sha1");
            args.Add("-Wl,-gc-sections");

            if (options.LinkEnv.Output == LinkerOutput.Executable)
            {
                // Prevent linker from stripping the entry point for the Android Native Activity
                args.Add("-u ANativeActivity_onCreate");
            }

            string target;
            switch (Architecture)
            {
            case TargetArchitecture.x86:
                target = "i686-none-linux-android";
                break;
            case TargetArchitecture.x64:
                target = "x86_64-none-linux-android";
                break;
            case TargetArchitecture.ARM:
                target = "armv7-none-linux-androideabi";
                break;
            case TargetArchitecture.ARM64:
                target = "aarch64-none-linux-android";
                break;
            default: throw new InvalidArchitectureException(Architecture);
            }

            var toolchain = ToolsetRoot.Replace('\\', '/');
            args.Add(string.Format("--sysroot=\"{0}/sysroot\"", toolchain));
            args.Add(string.Format("--gcc-toolchain=\"{0}\"", toolchain));
            args.Add("--target=" + target + Configuration.AndroidPlatformApi);
        }

        /// <inheritdoc />
        protected override void SetupArchiveFilesArgs(TaskGraph graph, BuildOptions options, List<string> args, string outputFilePath)
        {
            // Remove -o from the output file specifier
            args[1] = string.Format("\"{0}\"", outputFilePath);

            base.SetupArchiveFilesArgs(graph, options, args, outputFilePath);
        }

        /// <inheritdoc />
        protected override Task CreateArchive(TaskGraph graph, BuildOptions options, string outputFilePath)
        {
            var task = base.CreateArchive(graph, options, outputFilePath);
            task.CommandPath = LlvmArPath;
            return task;
        }

        /// <inheritdoc />
        protected override Task CreateBinary(TaskGraph graph, BuildOptions options, string outputFilePath)
        {
            // Bundle STL library for the executable files for runtime
            if (options.LinkEnv.Output == LinkerOutput.Executable)
            {
                // https://android.googlesource.com/platform/ndk/+/master/docs/BuildSystemMaintainers.md#libc
                var ndkPath = AndroidNdk.Instance.RootPath;
                var libCppSharedPath = Path.Combine(ndkPath, "sources/cxx-stl/llvm-libc++/libs/", GetAbiName(Architecture), "libc++_shared.so"); // NDK24 (and older) location
                if (!File.Exists(libCppSharedPath))
                {
                    libCppSharedPath = Path.Combine(ndkPath, "toolchains/llvm/prebuilt", AndroidSdk.GetHostName(), "sysroot/usr/lib/", GetToolchainName(TargetPlatform.Android, Architecture), "libc++_shared.so"); // NDK25+ location
                    if (!File.Exists(libCppSharedPath))
                        throw new Exception($"Missing Android NDK `libc++_shared.so` for architecture {Architecture}.");
                }
                graph.AddCopyFile(Path.Combine(Path.GetDirectoryName(outputFilePath), "libc++_shared.so"), libCppSharedPath);
            }

            var task = base.CreateBinary(graph, options, outputFilePath);
            task.CommandPath = ClangPath;
            return task;
        }
    }
}
