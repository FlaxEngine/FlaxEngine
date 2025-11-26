// Copyright (c) Wojciech Figat. All rights reserved.
//#define USE_GIT_REPOSITORY
using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Linq;
using Flax.Build;

namespace Flax.Deps.Dependencies
{
    /// <summary>
    /// OpenAL Soft is a software implementation of the OpenAL 3D audio API.
    /// </summary>
    /// <seealso cref="Flax.Deps.Dependency" />
    class OpenAL : Dependency
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
                        TargetPlatform.Android,
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
            var version = "1.24.3";
            var configuration = "Release";
            var cmakeArgs = "-DCMAKE_POLICY_VERSION_MINIMUM=3.5";
            var dstIncludePath = Path.Combine(options.ThirdPartyFolder, "OpenAL");
            var noSSL = true; // OpenAL Soft website has broken certs

#if !USE_GIT_REPOSITORY
            if (options.Platforms.Contains(TargetPlatform.Windows))
#endif
            {
                // Get the source
                CloneGitRepo(root, "https://github.com/kcat/openal-soft.git");
                GitCheckout(root, "master", "dc7d7054a5b4f3bec1dc23a42fd616a0847af948"); // 1.24.3
            }
#if !USE_GIT_REPOSITORY
            else
            {
                // Get the source
                var packagePath = Path.Combine(root, $"package-{version}.zip");
                if (!File.Exists(packagePath))
                {
                    Downloader.DownloadFileFromUrlToPath("https://openal-soft.org/openal-releases/openal-soft-" + version + ".tar.bz2", packagePath, noSSL);
                    if (Platform.BuildTargetPlatform == TargetPlatform.Windows)
                    {
                        // TODO: Maybe use PowerShell Expand-Archive instead?
                        var sevenZip = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), "7-Zip", "7z.exe");
                        Utilities.Run(sevenZip, "x package.zip", null, root);
                        Utilities.Run(sevenZip, "x package", null, root);
                    }
                    else
                    {
                        Utilities.Run("tar", "xjf " + packagePath.Replace('\\', '/'), null, root, Utilities.RunOptions.ConsoleLogOutput);
                    }
                }
            }
#endif

            foreach (var platform in options.Platforms)
            {
                foreach (var architecture in options.Architectures)
                {
                    BuildStarted(platform, architecture);
                    switch (platform)
                    {
                    case TargetPlatform.Windows:
                    {
                        var binariesToCopy = new[]
                        {
                            "OpenAL32.lib",
                            "OpenAL32.dll",
                        };

                        // Build for Windows
                        var buildDir = Path.Combine(root, "build-" + architecture.ToString());
                        var solutionPath = Path.Combine(buildDir, "OpenAL.sln");
                        SetupDirectory(buildDir, true);
                        RunCmake(root, platform, architecture, $"-B\"{buildDir}\" -DBUILD_SHARED_LIBS=OFF -DCMAKE_C_FLAGS=\"/D_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR /EHsc\" -DCMAKE_CXX_FLAGS=\"/D_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR /EHsc\" " + cmakeArgs);
                        Deploy.VCEnvironment.BuildSolution(solutionPath, configuration, architecture.ToString());
                        var depsFolder = GetThirdPartyFolder(options, platform, architecture);
                        foreach (var file in binariesToCopy)
                            Utilities.FileCopy(Path.Combine(buildDir, configuration, file), Path.Combine(depsFolder, Path.GetFileName(file)));
                        break;
                    }
                    case TargetPlatform.Linux:
                    {
                        var binariesToCopy = new[]
                        {
                            "libopenal.a",
                        };
                        var envVars = new Dictionary<string, string>
                        {
                            { "CC", "clang-" + Configuration.LinuxClangMinVer },
                            { "CC_FOR_BUILD", "clang-" + Configuration.LinuxClangMinVer },
                            { "CXX", "clang++-" + Configuration.LinuxClangMinVer },
                            { "CMAKE_BUILD_PARALLEL_LEVEL", CmakeBuildParallel },
                        };
                        var config = $"-DALSOFT_REQUIRE_ALSA=ON " +
                                     $"-DALSOFT_REQUIRE_OSS=ON " +
                                     $"-DALSOFT_REQUIRE_PORTAUDIO=ON " +
                                     $"-DALSOFT_REQUIRE_PULSEAUDIO=ON " +
                                     $"-DALSOFT_REQUIRE_JACK=ON " +
                                     $"-DALSOFT_REQUIRE_PIPEWIRE=ON " +
                                     $"-DALSOFT_EMBED_HRTF_DATA=YES "
                                     + cmakeArgs;

                        // Use separate build directory
#if !USE_GIT_REPOSITORY
                        root = Path.Combine(root, "openal-soft-" + version);
#endif
                        var buildDir = Path.Combine(root, "build-" + architecture.ToString());
                        SetupDirectory(buildDir, true);

                        // Build for Linux
                        RunCmake(root, platform, architecture, $"-B\"{buildDir}\" -DLIBTYPE=STATIC -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DCMAKE_BUILD_TYPE=" + configuration + config, envVars);
                        BuildCmake(buildDir, configuration, envVars);
                        var depsFolder = GetThirdPartyFolder(options, platform, architecture);
                        foreach (var file in binariesToCopy)
                            Utilities.FileCopy(Path.Combine(buildDir, file), Path.Combine(depsFolder, file));
                        break;
                    }
                    case TargetPlatform.Android:
                    {
                        var binariesToCopy = new[]
                        {
                            "libopenal.a",
                        };
                        var envVars = new Dictionary<string, string>
                        {
                            { "CMAKE_BUILD_PARALLEL_LEVEL", CmakeBuildParallel },
                        };
                        var config = "-DALSOFT_REQUIRE_OBOE=OFF -DALSOFT_REQUIRE_OPENSL=ON -DALSOFT_EMBED_HRTF_DATA=YES " + cmakeArgs;

                        // Use separate build directory
#if !USE_GIT_REPOSITORY
                        root = Path.Combine(root, "openal-soft-" + version);
#endif
                        var buildDir = Path.Combine(root, "build-" + architecture.ToString());
                        SetupDirectory(buildDir, true);

                        // Build
                        RunCmake(root, platform, TargetArchitecture.ARM64, $"-B\"{buildDir}\" -DLIBTYPE=STATIC -DCMAKE_BUILD_TYPE=" + configuration + config, envVars);
                        BuildCmake(buildDir, envVars);
                        var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.ARM64);
                        foreach (var file in binariesToCopy)
                            Utilities.FileCopy(Path.Combine(buildDir, file), Path.Combine(depsFolder, file));
                        break;
                    }
                    case TargetPlatform.Mac:
                    {
                        var binariesToCopy = new[]
                        {
                            "libopenal.a",
                        };
                        var envVars = new Dictionary<string, string>
                        {
                            { "CMAKE_BUILD_PARALLEL_LEVEL", CmakeBuildParallel },
                        };
                        var config = " -DALSOFT_REQUIRE_COREAUDIO=ON -DALSOFT_EMBED_HRTF_DATA=YES " + cmakeArgs;

                        // Use separate build directory
#if !USE_GIT_REPOSITORY
                        root = Path.Combine(root, "openal-soft-" + version);
#endif
                        var buildDir = Path.Combine(root, "build-" + architecture.ToString());
                        SetupDirectory(buildDir, true);

                        // Build for Mac
                        RunCmake(root, platform, architecture, $"-B\"{buildDir}\" -DLIBTYPE=STATIC -DCMAKE_BUILD_TYPE=" + configuration + config, envVars);
                        BuildCmake(buildDir, envVars);
                        var depsFolder = GetThirdPartyFolder(options, platform, architecture);
                        foreach (var file in binariesToCopy)
                            Utilities.FileCopy(Path.Combine(buildDir, file), Path.Combine(depsFolder, file));
                        break;
                    }
                    case TargetPlatform.iOS:
                    {
                        var binariesToCopy = new[]
                        {
                            "libopenal.a",
                        };
                        var envVars = new Dictionary<string, string>
                        {
                            { "CMAKE_BUILD_PARALLEL_LEVEL", CmakeBuildParallel },
                        };
                        var config = " -DALSOFT_REQUIRE_COREAUDIO=ON -DALSOFT_EMBED_HRTF_DATA=YES " + cmakeArgs;

                        // Use separate build directory
#if !USE_GIT_REPOSITORY
                        root = Path.Combine(root, "openal-soft-" + version);
#endif
                        var buildDir = Path.Combine(root, "build-" + architecture.ToString());
                        SetupDirectory(buildDir, true);

                        // Build for iOS
                        RunCmake(root, platform, TargetArchitecture.ARM64, $"-B\"{buildDir}\" -DCMAKE_SYSTEM_NAME=iOS -DALSOFT_OSX_FRAMEWORK=ON -DLIBTYPE=STATIC -DCMAKE_BUILD_TYPE=" + configuration + config, envVars);
                        BuildCmake(buildDir, envVars);
                        var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.ARM64);
                        foreach (var file in binariesToCopy)
                            Utilities.FileCopy(Path.Combine(buildDir, file), Path.Combine(depsFolder, file));
                        break;
                    }
                    }
                }
            }

            // Deploy license
            Utilities.FileCopy(Path.Combine(root, "COPYING"), Path.Combine(dstIncludePath, "COPYING"), true);

            // Deploy header files
            var files = Directory.GetFiles(Path.Combine(root, "include", "AL"));
            foreach (var file in files)
            {
                Utilities.FileCopy(file, Path.Combine(dstIncludePath, Path.GetFileName(file)));
            }
        }
    }
}
