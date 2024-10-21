// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
        public override void Build(BuildOptions options)
        {
            var root = options.IntermediateFolder;
            var version = "1.23.1";
            var dstIncludePath = Path.Combine(options.ThirdPartyFolder, "OpenAL");

            foreach (var platform in options.Platforms)
            {
                BuildStarted(platform);
                switch (platform)
                {
                case TargetPlatform.Windows:
                {
                    var binariesToCopy = new[]
                    {
                        "OpenAL32.lib",
                        "OpenAL32.dll",
                    };

                    string configuration = "Release";

                    // Get the source
                    CloneGitRepo(root, "https://github.com/kcat/openal-soft.git");
                    GitCheckout(root, "master", "d3875f333fb6abe2f39d82caca329414871ae53b"); // 1.23.1

                    // Build for Win64 and ARM64
                    foreach (var architecture in new[] { TargetArchitecture.x64, TargetArchitecture.ARM64 })
                    {
                        var buildDir = Path.Combine(root, "build-" + architecture.ToString());
                        var solutionPath = Path.Combine(buildDir, "OpenAL.sln");

                        RunCmake(root, platform, architecture, $"-B\"{buildDir}\" -DBUILD_SHARED_LIBS=OFF -DCMAKE_C_FLAGS=\"/D_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR /EHsc\" -DCMAKE_CXX_FLAGS=\"/D_DISABLE_CONSTEXPR_MUTEX_CONSTRUCTOR /EHsc\"");
                        Deploy.VCEnvironment.BuildSolution(solutionPath, configuration, architecture.ToString());
                        var depsFolder = GetThirdPartyFolder(options, platform, architecture);
                        foreach (var file in binariesToCopy)
                            Utilities.FileCopy(Path.Combine(buildDir, configuration, file), Path.Combine(depsFolder, Path.GetFileName(file)));
                    }
                    
#if false
                    // Get the binaries
                    var packagePath = Path.Combine(root, "package.zip");
                    if (!File.Exists(packagePath))
                        Downloader.DownloadFileFromUrlToPath("https://openal-soft.org/openal-binaries/openal-soft-" + version + "-bin.zip", packagePath);
                    using (ZipArchive archive = ZipFile.Open(packagePath, ZipArchiveMode.Read))
                    {
                        if (!Directory.Exists(root))
                            archive.ExtractToDirectory(root);
                        root = Path.Combine(root, archive.Entries.First().FullName);
                    }

                    // Deploy Win64 binaries
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.x64);
                    Utilities.FileCopy(Path.Combine(root, "bin", "Win64", "soft_oal.dll"), Path.Combine(depsFolder, "OpenAL32.dll"));
                    Utilities.FileCopy(Path.Combine(root, "libs", "Win64", "OpenAL32.lib"), Path.Combine(depsFolder, "OpenAL32.lib"));

                    // Deploy license
                    Utilities.FileCopy(Path.Combine(root, "COPYING"), Path.Combine(dstIncludePath, "COPYING"), true);

                    // Deploy header files
                    var files = Directory.GetFiles(Path.Combine(root, "include", "AL"));
                    foreach (var file in files)
                    {
                        Utilities.FileCopy(file, Path.Combine(dstIncludePath, Path.GetFileName(file)));
                    }
#endif
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
                        { "CC", "clang-7" },
                        { "CC_FOR_BUILD", "clang-7" }
                    };
                    var config = "-DALSOFT_REQUIRE_ALSA=ON -DALSOFT_REQUIRE_OSS=ON -DALSOFT_REQUIRE_PORTAUDIO=ON -DALSOFT_REQUIRE_PULSEAUDIO=ON -DALSOFT_REQUIRE_JACK=ON -DALSOFT_EMBED_HRTF_DATA=YES";

                    // Get the source
                    var packagePath = Path.Combine(root, "package.zip");
                    File.Delete(packagePath);
                    Downloader.DownloadFileFromUrlToPath("https://openal-soft.org/openal-releases/openal-soft-" + version + ".tar.bz2", packagePath);
                    Utilities.Run("tar", "xjf " + packagePath.Replace('\\', '/'), null, root, Utilities.RunOptions.ConsoleLogOutput);

                    // Use separate build directory
                    root = Path.Combine(root, "openal-soft-" + version);
                    var buildDir = Path.Combine(root, "build");
                    SetupDirectory(buildDir, true);

                    // Build for Linux
                    Utilities.Run("cmake", "-G \"Unix Makefiles\" -DCMAKE_BUILD_TYPE=Release -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DLIBTYPE=STATIC " + config + " ..", null, buildDir, Utilities.RunOptions.ConsoleLogOutput, envVars);
                    Utilities.Run("cmake", "--build .", null, buildDir, Utilities.RunOptions.ConsoleLogOutput, envVars);
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.x64);
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
                    var config = "-DALSOFT_REQUIRE_OBOE=OFF -DALSOFT_REQUIRE_OPENSL=ON -DALSOFT_EMBED_HRTF_DATA=YES";

                    // Get the source
                    var packagePath = Path.Combine(root, "package.zip");
                    File.Delete(packagePath);
                    Downloader.DownloadFileFromUrlToPath("https://openal-soft.org/openal-releases/openal-soft-" + version + ".tar.bz2", packagePath);
                    if (Platform.BuildTargetPlatform == TargetPlatform.Windows)
                    {
                        var sevenZip = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), "7-Zip", "7z.exe");
                        Utilities.Run(sevenZip, "x package.zip", null, root);
                        Utilities.Run(sevenZip, "x package", null, root);
                    }
                    else
                    {
                        Utilities.Run("tar", "xjf " + packagePath.Replace('\\', '/'), null, root, Utilities.RunOptions.ConsoleLogOutput);
                    }

                    // Use separate build directory
                    root = Path.Combine(root, "openal-soft-" + version);
                    var buildDir = Path.Combine(root, "build");
                    SetupDirectory(buildDir, true);

                    // Build
                    RunCmake(buildDir, platform, TargetArchitecture.ARM64, ".. -DLIBTYPE=STATIC -DCMAKE_BUILD_TYPE=Release " + config);
                    BuildCmake(buildDir);
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
                    var config = "-DALSOFT_REQUIRE_COREAUDIO=ON -DALSOFT_EMBED_HRTF_DATA=YES";

                    // Get the source
                    var packagePath = Path.Combine(root, "package.zip");
                    File.Delete(packagePath);
                    Downloader.DownloadFileFromUrlToPath("https://openal-soft.org/openal-releases/openal-soft-" + version + ".tar.bz2", packagePath);
                    Utilities.Run("tar", "xjf " + packagePath.Replace('\\', '/'), null, root, Utilities.RunOptions.ConsoleLogOutput);

                    // Use separate build directory
                    root = Path.Combine(root, "openal-soft-" + version);
                    var buildDir = Path.Combine(root, "build");

                    // Build for Mac
                    foreach (var architecture in new[] { TargetArchitecture.x64, TargetArchitecture.ARM64 })
                    {
                        SetupDirectory(buildDir, true);
                        RunCmake(buildDir, platform, architecture, ".. -DLIBTYPE=STATIC -DCMAKE_BUILD_TYPE=Release " + config);
                        BuildCmake(buildDir);
                        var depsFolder = GetThirdPartyFolder(options, platform, architecture);
                        foreach (var file in binariesToCopy)
                            Utilities.FileCopy(Path.Combine(buildDir, file), Path.Combine(depsFolder, file));
                    }
                    break;
                }
                case TargetPlatform.iOS:
                {
                    var binariesToCopy = new[]
                    {
                        "libopenal.a",
                    };
                    var config = "-DALSOFT_REQUIRE_COREAUDIO=ON -DALSOFT_EMBED_HRTF_DATA=YES";

                    // Get the source
                    var packagePath = Path.Combine(root, "package.zip");
                    if (!File.Exists(packagePath))
                    {
                        Downloader.DownloadFileFromUrlToPath("https://openal-soft.org/openal-releases/openal-soft-" + version + ".tar.bz2", packagePath);
                        Utilities.Run("tar", "xjf " + packagePath.Replace('\\', '/'), null, root, Utilities.RunOptions.ConsoleLogOutput);
                    }

                    // Use separate build directory
                    root = Path.Combine(root, "openal-soft-" + version);
                    var buildDir = Path.Combine(root, "build");

                    // Build for iOS
                    SetupDirectory(buildDir, true);
                    RunCmake(buildDir, platform, TargetArchitecture.ARM64, ".. -DCMAKE_SYSTEM_NAME=iOS -DALSOFT_OSX_FRAMEWORK=ON -DLIBTYPE=STATIC -DCMAKE_BUILD_TYPE=Release " + config);
                    BuildCmake(buildDir);
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.ARM64);
                    foreach (var file in binariesToCopy)
                        Utilities.FileCopy(Path.Combine(buildDir, file), Path.Combine(depsFolder, file));
                    break;
                }
                }
            }
        }
    }
}
