// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using Flax.Build;

namespace Flax.Deps.Dependencies
{
    /// <summary>
    /// OpenXR loader and headers. https://github.com/KhronosGroup/OpenXR-SDK
    /// </summary>
    /// <seealso cref="Flax.Deps.Dependency" />
    class OpenXR : Dependency
    {
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
                case TargetPlatform.Linux:
                    return new[]
                    {
                        TargetPlatform.Linux,
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
                case TargetPlatform.Linux:
                    return new[]
                    {
                        TargetArchitecture.x64,
                    };
                default: return new TargetArchitecture[0];
                }
            }
        }

        /// <inheritdoc />
        public override void Build(BuildOptions options)
        {
            var root = options.IntermediateFolder;
            var configuration = "Release";
            var filesToKeep = new[]
            {
                "OpenXR.Build.cs",
            };
            var args = new string[]
            {
                "-DDYNAMIC_LOADER=OFF",
                "-DBUILD_API_LAYERS=OFF",
                "-DBUILD_TESTS=OFF",
                "-DBUILD_CONFORMANCE_TESTS=OFF",
                "-DBUILD_WITH_SYSTEM_JSONCPP=OFF",
                "-DINSTALL_SYMBOLS=OFF",
                "-DCMAKE_INSTALL_LIBDIR=lib",
            };

            // Get the source
            var commit = "f2448a8797c85814aa892efc1ab8707900fbcc78"; // Version 1.1.63
            CloneGitRepoSingleBranch(root, "https://github.com/KhronosGroup/OpenXR-SDK.git", "main", commit);

            foreach (var platform in options.Platforms)
            {
                foreach (var architecture in options.Architectures)
                {
                    BuildStarted(platform, architecture);

                    var buildDir = Path.Combine(root, "build-" + architecture);
                    var installDir = Path.Combine(root, "install-" + architecture);
                    var depsFolder = GetThirdPartyFolder(options, platform, architecture);
                    SetupDirectory(buildDir, true);
                    File.Delete(Path.Combine(root, "CMakeCache.txt"));

                    var envVars = new Dictionary<string, string>();
                    var libName = "openxr_loader.lib";
                    var cmakeArgs = string.Join(" ", args);
                    switch (platform)
                    {
                    case TargetPlatform.Linux:
                        envVars["CC"] = "clang-" + LinuxConfiguration.ClangMinVer;
                        envVars["CC_FOR_BUILD"] = "clang-" + LinuxConfiguration.ClangMinVer;
                        envVars["CXX"] = "clang++-" + LinuxConfiguration.ClangMinVer;
                        cmakeArgs += " -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_BUILD_TYPE=" + configuration;
                        libName = "libopenxr_loader.a";
                        break;
                    }
                    RunCmake(root, platform, architecture, $"-B\"{buildDir}\" -Wno-dev " + cmakeArgs, envVars);
                    BuildCmake(buildDir, configuration);
                    Utilities.Run("cmake", $"--install \"{buildDir}\" --prefix \"{installDir}\" --config {configuration}", null, root, Utilities.RunOptions.DefaultTool);
                    Utilities.FileCopy(Path.Combine(installDir, "lib", libName), Path.Combine(depsFolder, libName));
                }
            }

            // Backup files
            var dstIncludePath = Path.Combine(options.ThirdPartyFolder, "OpenXR");
            foreach (var filename in filesToKeep)
            {
                var src = Path.Combine(dstIncludePath, filename);
                var dst = Path.Combine(options.IntermediateFolder, filename + ".tmp");
                Utilities.FileCopy(src, dst);
            }

            // Deploy header files and license
            SetupDirectory(dstIncludePath, true);
            Utilities.FileCopy(Path.Combine(root, "LICENSE"), Path.Combine(dstIncludePath, "LICENSE.txt"));
            Utilities.DirectoryCopy(Path.Combine(root, "include", "openxr"), Path.Combine(dstIncludePath, "openxr"), true, true);
            File.Delete(Path.Combine(dstIncludePath, "openxr", "CMakeLists.txt"));
            foreach (var filename in filesToKeep)
            {
                var src = Path.Combine(options.IntermediateFolder, filename + ".tmp");
                var dst = Path.Combine(dstIncludePath, filename);
                Utilities.FileCopy(src, dst);
            }
        }
    }
}
