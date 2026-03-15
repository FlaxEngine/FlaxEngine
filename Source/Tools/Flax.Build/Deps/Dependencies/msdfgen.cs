// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using Flax.Build;

namespace Flax.Deps.Dependencies
{
    /// <summary>
    /// Multi-channel signed distance field generator. https://github.com/Chlumsky/msdfgen
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
                        TargetPlatform.Android,
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
                        TargetPlatform.iOS,
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
            var configuration = "Release";
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

            // Get the source
            var commit = "1874bcf7d9624ccc85b4bc9a85d78116f690f35b"; // Version 1.13
            CloneGitRepoSingleBranch(root, "https://github.com/Chlumsky/msdfgen.git", "master", commit);

            foreach (var platform in options.Platforms)
            {
                foreach (var architecture in options.Architectures)
                {
                    BuildStarted(platform, architecture);

                    var buildDir = Path.Combine(root, "build-" + architecture);
                    var installDir = Path.Combine(root, "install-" + architecture);
                    var depsFolder = GetThirdPartyFolder(options, platform, architecture);
                    includeDir = Path.Combine(installDir, "include");
                    SetupDirectory(buildDir, true);
                    File.Delete(Path.Combine(root, "CMakeCache.txt"));

                    Dictionary<string, string> envVars = null;
                    var libName = "libmsdfgen-core.a";
                    var cmakeArgs = string.Join(" ", args);
                    switch (platform)
                    {
                    case TargetPlatform.Windows:
                    case TargetPlatform.XboxOne:
                    case TargetPlatform.XboxScarlett:
                        libName = "msdfgen-core.lib";
                        break;
                    case TargetPlatform.Linux:
                        envVars = new Dictionary<string, string>
                        {
                            { "CC", "clang-" + Configuration.LinuxClangMinVer },
                            { "CC_FOR_BUILD", "clang-" + Configuration.LinuxClangMinVer },
                            { "CXX", "clang++-" + Configuration.LinuxClangMinVer },
                            { "CMAKE_BUILD_PARALLEL_LEVEL", CmakeBuildParallel },
                        };
                        cmakeArgs += " -DCMAKE_POSITION_INDEPENDENT_CODE=ON";
                        break;
                    }

                    RunCmake(root, platform, architecture, $"-B\"{buildDir}\" " + cmakeArgs, envVars);
                    BuildCmake(buildDir);
                    Utilities.Run("cmake", $"--install {buildDir} --prefix {installDir} --config {configuration}", null, root, Utilities.RunOptions.DefaultTool);
                    Utilities.FileCopy(Path.Combine(installDir, "lib", libName), Path.Combine(depsFolder, libName));
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
