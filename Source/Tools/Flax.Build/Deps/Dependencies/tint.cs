// Copyright (c) Wojciech Figat. All rights reserved.

using Flax.Build;
using System.IO;

namespace Flax.Deps.Dependencies
{
    /// <summary>
    /// Tint is a compiler for the WebGPU Shader Language (WGSL) that can be used in standalone to convert shaders from and to WGSL.
    /// https://github.com/google/dawn (mirrors https://dawn.googlesource.com/dawn)
    /// </summary>
    /// <seealso cref="Flax.Deps.Dependency" />
    class tint : Dependency
    {
        /// <inheritdoc />
        public override bool BuildByDefault => false; // Too big to build without explicit need

        /// <inheritdoc />
        public override TargetPlatform[] Platforms
        {
            get
            {
                switch (BuildPlatform)
                {
                case TargetPlatform.Windows:
                    return new[]
                    {
                        TargetPlatform.Windows,
                    };
                case TargetPlatform.Mac:
                    return new[]
                    {
                        TargetPlatform.Mac,
                    };
                default: return new TargetPlatform[0];
                }
            }
        }

        /// <inheritdoc />
        public override TargetArchitecture[] Architectures
        {
            get
            {
                switch (BuildPlatform)
                {
                case TargetPlatform.Windows:
                    return new[]
                    {
                        TargetArchitecture.x64,
                    };
                case TargetPlatform.Mac:
                    return new[]
                    {
                        TargetArchitecture.ARM64,
                    };
                default: return new TargetArchitecture[0];
                }
            }
        }

        /// <inheritdoc />
        public override void Build(BuildOptions options)
        {
            var root = options.IntermediateFolder;
            var config = "Release";
            var buildOptions = new[]
            {
                "DAWN_BUILD_SAMPLES=OFF",
                "DAWN_BUILD_TESTS=OFF",
                "DAWN_BUILD_PROTOBUF=OFF",
                "DAWN_ENABLE_D3D11=OFF",
                "DAWN_ENABLE_D3D12=OFF",
                "DAWN_ENABLE_METAL=OFF",
                "DAWN_ENABLE_NULL=OFF",
                "DAWN_ENABLE_DESKTOP_GL=OFF",
                "DAWN_ENABLE_OPENGLES=OFF",
                "DAWN_ENABLE_VULKAN=OFF",
                "DAWN_ENABLE_SPIRV_VALIDATION=OFF",
                "DAWN_FORCE_SYSTEM_COMPONENT_LOAD=OFF",
                "TINT_BUILD_CMD_TOOLS=ON",
                "TINT_BUILD_SPV_READER=ON",
                "TINT_BUILD_WGSL_READER=OFF",
                "TINT_BUILD_GLSL_WRITER=OFF",
                "TINT_BUILD_GLSL_VALIDATOR=OFF",
                "TINT_BUILD_HLSL_WRITER=OFF",
                "TINT_BUILD_SPV_WRITER=OFF",
                "TINT_BUILD_WGSL_WRITER=ON",
                "TINT_BUILD_NULL_WRITER=OFF",
                "TINT_BUILD_IR_BINARY=OFF",
                "TINT_BUILD_TESTS=OFF",
            };

            // Get the source
            var firstTime = !GitRepositoryExists(root);
            var commit = "536c572abab2602db0b652eda401ebd01e046c11"; // v20260219.200501
            CloneGitRepo(root, "https://github.com/google/dawn.git", commit, null, true);
            if (firstTime)
            {
                // This repo is bloated with submodules and sometimes git fails to properly checkout them all
                for (int i = 0; i < 3; i++)
                    Utilities.Run("git", "submodule update --recursive", null, root, Utilities.RunOptions.ConsoleLogOutput);
            }

            // Build tint
            foreach (var platform in options.Platforms)
            {
                foreach (var architecture in options.Architectures)
                {
                    BuildStarted(platform, architecture);

                    // Build
                    var buildDir = Path.Combine(root, "build");
                    SetupDirectory(buildDir, true);
                    var cmakeArgs = $"-DCMAKE_BUILD_TYPE={config}";
                    foreach (var option in buildOptions)
                        cmakeArgs += $" -D{option}";
                    RunCmake(buildDir, platform, architecture, ".. " + cmakeArgs);
                    BuildCmake(buildDir, config, options: Utilities.RunOptions.ConsoleLogOutput);

                    // Deploy executable
                    var depsFolder = GetThirdPartyFolder(options, platform, architecture);
                    switch (platform)
                    {
                    case TargetPlatform.Windows:
                        Utilities.FileCopy(Path.Combine(buildDir, config, "tint.exe"), Path.Combine(depsFolder, "tint.exe"));
                        break;
                    default:
                        Utilities.FileCopy(Path.Combine(buildDir, "tint"), Path.Combine(depsFolder, "tint"));
                        if (BuildPlatform != TargetPlatform.Windows)
                        {
                            Utilities.Run("chmod", "+x tint", null, depsFolder, Utilities.RunOptions.ConsoleLogOutput);
                            Utilities.Run("strip", "tint", null, depsFolder, Utilities.RunOptions.ConsoleLogOutput);
                        }
                        break;
                    }
                }
            }
        }
    }
}
