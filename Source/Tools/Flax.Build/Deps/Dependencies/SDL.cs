// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
using System.Collections.Generic;
using System.IO;
using System.IO.Compression;
using System.Linq;
using Flax.Build;
using Flax.Build.Platforms;

namespace Flax.Deps.Dependencies
{
    /// <summary>
    /// 
    /// </summary>
    /// <seealso cref="Flax.Deps.Dependency" />
    class SDL : Dependency
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
                        TargetPlatform.iOS,
                    };
                default: return new TargetPlatform[0];
                }
            }
        }

        /// <inheritdoc />
        public override void Build(BuildOptions options)
        {
            string root = options.IntermediateFolder;
            string configuration = "Release";
            const bool buildStatic = true;
            var configs = new string[]
            {
                "-DSDL_TEST_LIBRARY=OFF",
                "-DSDL_CAMERA=OFF",
                "-DSDL_DIALOG=OFF",
                "-DSDL_OFFSCREEN=OFF",
                
                "-DSDL_RENDER=OFF",
                "-DSDL_RENDER_D3D=OFF",
                "-DSDL_RENDER_METAL=OFF",
                "-DSDL_RENDER_VULKAN=OFF",

                "-DSDL_DIRECTX=OFF",
                "-DSDL_VULKAN=OFF",
                "-DSDL_OPENGL=OFF",
                "-DSDL_OPENGLES=OFF",

                "-DSDL_AUDIO=OFF",
                "-DSDL_DISKAUDIO=OFF",
                "-DSDL_WASAPI=OFF",
                "-DSDL_SNDIO=OFF",
                "-DSDL_PULSEAUDIO=OFF",
                "-DSDL_ALSA=OFF",
                "-DSDL_WASAPI=OFF",
            };
            var filesToKeep = new[]
            {
                "SDL.Build.cs",
            };
            List<string> filesToCopy = new List<string>()
            {
                Path.Combine(root, "LICENSE.txt")
            };
            List<string> directoriesToCopy = new List<string>()
            {
                Path.Combine(root, "include", "SDL3"),
            };

            CloneGitRepoFastSince(root, "https://github.com/libsdl-org/SDL.git", new System.DateTime(2024, 7, 15));
            GitResetToCommit(root, "ff7a60db85f53c249cbd277915452b4c67323765");

            foreach (var platform in options.Platforms)
            {
                switch (platform)
                {
                case TargetPlatform.Windows:
                {
                    foreach (var architecture in new TargetArchitecture[] { TargetArchitecture.x64/*, TargetArchitecture.ARM64*/ })
                    {
                        var buildDir = Path.Combine(root, "build-" + architecture.ToString());

                        string[] binariesToCopy;
                        if (buildStatic)
                            binariesToCopy = new[] { "SDL3-static.lib", };
                        else
                            binariesToCopy = new[] { "SDL3.dll", "SDL3.lib", };
                        directoriesToCopy.Add(Path.Combine(buildDir, "include", "SDL3"));
                        
                        var solutionPath = Path.Combine(buildDir, "SDL3.sln");

                        RunCmake(root, platform, architecture, $"-B\"{buildDir}\" -DSDL_SHARED={(!buildStatic ? "ON" : "OFF")} -DSDL_STATIC={(buildStatic ? "ON" : "OFF")} -DCMAKE_POLICY_DEFAULT_CMP0091=NEW -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL " + string.Join(" ", configs));
                        Deploy.VCEnvironment.BuildSolution(solutionPath, configuration, architecture.ToString());

                        // Copy binaries
                        var depsFolder = GetThirdPartyFolder(options, platform, architecture);
                        foreach (var file in binariesToCopy)
                            Utilities.FileCopy(Path.Combine(buildDir, configuration, file), Path.Combine(depsFolder, file == "SDL3-static.lib" ? "SDL3.lib" : file));
                    }
                    break;
                }
                case TargetPlatform.Linux:
                {
                    foreach (var architecture in new TargetArchitecture[] { TargetArchitecture.x64 })
                    {
                        var buildDir = Path.Combine(root, "build-" + architecture.ToString());
                        
                        string[] binariesToCopy;
                        if (buildStatic)
                            binariesToCopy = new[] { "libSDL3.a" };
                        else
                            binariesToCopy = new[] { "libSDL3.so" };
                        directoriesToCopy.Add(Path.Combine(buildDir, "include", "SDL3"));

                        int concurrency = Math.Min(Math.Max(1, (int)(Environment.ProcessorCount * Configuration.ConcurrencyProcessorScale)), Configuration.MaxConcurrency);
                        RunCmake(root, platform, architecture, $"-B\"{buildDir}\" -DCMAKE_BUILD_TYPE=DEBUG -DSDL_SHARED={(!buildStatic ? "ON" : "OFF")} -DSDL_STATIC={(buildStatic ? "ON" : "OFF")} -DCMAKE_POSITION_INDEPENDENT_CODE=ON " + string.Join(" ", configs));
                        BuildCmake(buildDir, configuration, new Dictionary<string, string>() { {"CMAKE_BUILD_PARALLEL_LEVEL", concurrency.ToString()} });

                        // Copy binaries
                        var depsFolder = GetThirdPartyFolder(options, platform, architecture);
                        foreach (var file in binariesToCopy)
                            Utilities.FileCopy(Path.Combine(buildDir, file), Path.Combine(depsFolder, file));
                    }
                    break;
                }
                case TargetPlatform.Mac:
                {
                    // TODO
                    break;
                }
                }
            }

            // Backup files
            var dstPath = Path.Combine(options.ThirdPartyFolder, "SDL");
            foreach (var filename in filesToKeep)
            {
                var src = Path.Combine(dstPath, filename);
                var dst = Path.Combine(options.IntermediateFolder, filename + ".tmp");
                Utilities.FileCopy(src, dst);
            }

            try
            {
                // Setup headers directory
                SetupDirectory(dstPath, true);

                // Deploy files and folders
                foreach (var dir in directoriesToCopy)
                    Utilities.DirectoryCopy(dir, Path.Combine(dstPath, Path.GetFileName(dir)), true, true);
                foreach (var file in filesToCopy)
                    Utilities.FileCopy(file, Path.Combine(dstPath, Path.GetFileName(file)));
            }
            finally
            {
                foreach (var filename in filesToKeep)
                {
                    var src = Path.Combine(options.IntermediateFolder, filename + ".tmp");
                    var dst = Path.Combine(dstPath, filename);
                    Utilities.FileCopy(src, dst);
                }
            }
        }
    }
}
