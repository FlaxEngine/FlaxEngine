// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System;
using System.IO;

namespace Flax.Build.Platforms
{
    /// <summary>
    /// The Linux build platform implementation.
    /// </summary>
    /// <seealso cref="Platform" />
    /// <seealso cref="UnixPlatform" />
    public class LinuxPlatform : UnixPlatform
    {
        /// <inheritdoc />
        public override TargetPlatform Target => TargetPlatform.Linux;

        /// <inheritdoc />
        public override bool HasRequiredSDKsInstalled { get; }

        /// <inheritdoc />
        public override bool HasSharedLibrarySupport => true;

        /// <summary>
        /// The toolchain folder root.
        /// </summary>
        public readonly string ToolchainRoot;

        /// <summary>
        /// The compiler name.
        /// </summary>
        public readonly string Compiler;

        /// <summary>
        /// Initializes a new instance of the <see cref="LinuxPlatform"/> class.
        /// </summary>
        public LinuxPlatform()
        {
            // Try to use system compiler
            if (Environment.OSVersion.Platform == PlatformID.Unix)
            {
                // Pick the newest compiler (overriden by specified in command line)
                if (Which(Compiler) != null)
                    Compiler = Configuration.Compiler;
                else if (Which("clang++-10") != null)
                    Compiler = "clang++-10";
                else if (Which("clang++-9") != null)
                    Compiler = "clang++-9";
                else if (Which("clang++-8") != null)
                    Compiler = "clang++-8";
                else if (Which("clang++-7") != null)
                    Compiler = "clang++-7";
                else if (Which("clang++-6") != null)
                    Compiler = "clang++-6";
                else if (Which("clang++") != null)
                    Compiler = "clang++";
            }
            if (Compiler != null)
            {
                // System compiler
                ToolchainRoot = string.Empty;
                Log.Verbose($"Using native Linux toolchain (compiler {Compiler})");
                HasRequiredSDKsInstalled = true;
            }
            else
            {
                // Check if Linux toolchain is installed
                string toolchainName = "v13_clang-7.0.1-centos7";
                string toolchainsRoot = Environment.GetEnvironmentVariable("LINUX_TOOLCHAINS_ROOT");
                if (string.IsNullOrEmpty(toolchainsRoot) || !Directory.Exists(Path.Combine(toolchainsRoot, toolchainName)))
                {
                    if (string.IsNullOrEmpty(toolchainsRoot))
                    {
                        Log.Warning("Missing Linux Toolchain. Cannot build for Linux platform.");
                    }

                    return;
                }

                // Installed toolchain
                ToolchainRoot = Path.Combine(toolchainsRoot, toolchainName).Replace('\\', '/');
                Log.Verbose(string.Format("Found Linux Toolchain at {0}", ToolchainRoot));
                HasRequiredSDKsInstalled = true;
            }
        }

        /// <inheritdoc />
        protected override Toolchain CreateToolchain(TargetArchitecture architecture)
        {
            return new LinuxToolchain(this, architecture);
        }

        /// <inheritdoc />
        public override bool CanBuildPlatform(TargetPlatform platform)
        {
            switch (platform)
            {
            case TargetPlatform.Linux: return HasRequiredSDKsInstalled;
            case TargetPlatform.Android: return AndroidSdk.Instance.IsValid && AndroidNdk.Instance.IsValid;
            default: return false;
            }
        }
    }
}
