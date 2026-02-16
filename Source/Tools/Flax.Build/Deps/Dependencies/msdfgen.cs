// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using Flax.Build;

namespace Flax.Deps.Dependencies
{
    /// <summary>
    /// Official Open Asset Import Library Repository. Loads 40+ 3D file formats into one unified and clean data structure. http://www.assimp.org
    /// </summary>
    /// <seealso cref="Flax.Deps.Dependency" />
    class msdfgen : Dependency
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
                        TargetArchitecture.ARM64,
                    };
                case TargetPlatform.Linux:
                    return new[]
                    {
                        TargetArchitecture.x64,
                        //TargetArchitecture.ARM64,
                    };
                case TargetPlatform.Mac:
                    return new[]
                    {
                        TargetArchitecture.x64,
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
            string includeDir = null;
            var filesToKeep = new[]
            {
                "msdfgen.Build.cs",
            };
            var args = new string[]
            {
                "-DMSDFGEN_USE_VCPKG=OFF",
                "-DMSDFGEN_CORE_ONLY=ON",
                "-DMSDFGEN_DYNAMIC_RUNTIME=ON",
                "-DMSDFGEN_USE_SKIA=OFF",
                "-DBUILD_SHARED_LIBS=OFF",
                "-DMSDFGEN_INSTALL=ON"
            };
            var cmakeArgs = string.Join(" ", args);

            // Get the source
            CloneGitRepoSingleBranch(root, "https://github.com/Chlumsky/msdfgen.git", "master", "1874bcf7d9624ccc85b4bc9a85d78116f690f35b");

            foreach (var platform in options.Platforms)
            {
                foreach (var architecture in options.Architectures)
                {
                    BuildStarted(platform, architecture);
                    switch (platform)
                    {
                    case TargetPlatform.Windows:
                    {
                        var configuration = "Release";

                        // Build for Windows
                        File.Delete(Path.Combine(root, "CMakeCache.txt"));
                        
                        var buildDir = Path.Combine(root, "build-" + architecture);
                        var installDir = Path.Combine(root, "install-" + architecture);
                        SetupDirectory(buildDir, true);
                        RunCmake(root, platform, architecture, $"-B\"{buildDir}\" " + cmakeArgs);
                        BuildCmake(buildDir);
                        Utilities.Run("cmake", $"--install {buildDir} --prefix {installDir} --config {configuration}", null, root, Utilities.RunOptions.DefaultTool);
                        var depsFolder = GetThirdPartyFolder(options, platform, architecture);
                        Utilities.FileCopy(Path.Combine(installDir, "lib", "msdfgen-core.lib"), Path.Combine(depsFolder, "msdfgen-core.lib"));
                        // This will be set multiple times, but the headers are the same anyway.
                        includeDir = Path.Combine(installDir, "include");

                        break;
                    }
                    case TargetPlatform.Linux:
                    {
                        var configuration = "Release";
                        var envVars = new Dictionary<string, string>
                        {
                            { "CC", "clang-" + Configuration.LinuxClangMinVer },
                            { "CC_FOR_BUILD", "clang-" + Configuration.LinuxClangMinVer },
                            { "CXX", "clang++-" + Configuration.LinuxClangMinVer },
                            { "CMAKE_BUILD_PARALLEL_LEVEL", CmakeBuildParallel },
                        };

                        // Build for Linux
                        File.Delete(Path.Combine(root, "CMakeCache.txt"));

                        var buildDir = Path.Combine(root, "build-" + architecture);
                        var installDir = Path.Combine(root, "install-" + architecture);
                        SetupDirectory(buildDir, true);
                        RunCmake(root, platform, architecture, $"-B\"{buildDir}\" " + cmakeArgs + " -DCMAKE_POSITION_INDEPENDENT_CODE=ON", envVars);
                        BuildCmake(buildDir);
                        Utilities.Run("cmake", $"--install {buildDir} --prefix {installDir} --config {configuration}", null, root, Utilities.RunOptions.DefaultTool);
                        var depsFolder = GetThirdPartyFolder(options, platform, architecture);
                        Utilities.FileCopy(Path.Combine(installDir, "lib", "libmsdfgen-core.a"), Path.Combine(depsFolder, "libmsdfgen-core.a"));
                        // This will be set multiple times, but the headers are the same anyway.
                        includeDir = Path.Combine(installDir, "include");
                        
                        break;
                    }
                    case TargetPlatform.Mac:
                    {
                        var configuration = "Release";

                        // Build for Mac
                        File.Delete(Path.Combine(root, "CMakeCache.txt"));

                        var buildDir = Path.Combine(root, "build-" + architecture);
                        var installDir = Path.Combine(root, "install-" + architecture);
                        SetupDirectory(buildDir, true);
                        RunCmake(root, platform, architecture, $"-B\"{buildDir}\" " + cmakeArgs);
                        BuildCmake(buildDir);
                        Utilities.Run("cmake", $"--install {buildDir} --prefix {installDir} --config {configuration}", null, root, Utilities.RunOptions.DefaultTool);
                        var depsFolder = GetThirdPartyFolder(options, platform, architecture);
                        Utilities.FileCopy(Path.Combine(installDir, "lib", "libmsdfgen-core.a"), Path.Combine(depsFolder, "libmsdfgen-core.a"));
                        // This will be set multiple times, but the headers are the same anyway.
                        includeDir = Path.Combine(installDir, "include");
                        
                        break;
                    }
                    }
                }
            }

            // Backup files
            var dstIncludePath = Path.Combine(options.ThirdPartyFolder, "msdfgen");
            foreach (var filename in filesToKeep)
            {
                var src = Path.Combine(dstIncludePath, filename);
                var dst = Path.Combine(options.IntermediateFolder, filename + ".tmp");
                Utilities.FileCopy(src, dst);
            }

            // Deploy header files and license
            SetupDirectory(dstIncludePath, true);
            Utilities.FileCopy(Path.Combine(root, "LICENSE.txt"), Path.Combine(dstIncludePath, "LICENSE.txt"));
            Utilities.DirectoryCopy(includeDir, dstIncludePath, true, true);
            foreach (var filename in filesToKeep)
            {
                var src = Path.Combine(options.IntermediateFolder, filename + ".tmp");
                var dst = Path.Combine(dstIncludePath, filename);
                Utilities.FileCopy(src, dst);
            }
        }
    }
}
