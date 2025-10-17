// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.IO;
using Flax.Build;

namespace Flax.Deps.Dependencies
{
    /// <summary>
    /// Khronos reference front-end for GLSL and ESSL, and sample SPIR-V generator
    /// </summary>
    /// <seealso cref="Flax.Deps.Dependency" />
    class glslang : Dependency
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
            var installDir = Path.Combine(root, "install");
            var configuration = "Release";
            var cmakeArgs = $"-DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_INSTALL_PREFIX=\"{installDir}\" -DCMAKE_BUILD_TYPE={configuration} -DENABLE_RTTI=ON -DENABLE_CTEST=OFF -DENABLE_HLSL=ON -DENABLE_SPVREMAPPER=ON -DENABLE_GLSLANG_BINARIES=OFF";
            var libsRoot = Path.Combine(installDir, "lib");

            // Get the source
            CloneGitRepoFast(root, "https://github.com/FlaxEngine/glslang.git");

            // Setup the external sources
            // Requires distutils (pip install setuptools)
            if (Utilities.Run(BuildPlatform != TargetPlatform.Mac ? "python" : "python3", "update_glslang_sources.py", null, root, Utilities.RunOptions.ConsoleLogOutput) != 0)
                throw new Exception("Failed to update glslang sources, make sure setuptools python package is installed.");

            foreach (var platform in options.Platforms)
            {
                foreach (var architecture in options.Architectures)
                {
                    BuildStarted(platform, architecture);

                    var buildDir = Path.Combine(root, "build-" + architecture.ToString());
                    switch (platform)
                    {
                    case TargetPlatform.Windows:
                    {
                        var outputFiles = new[]
                        {
                            Path.Combine(libsRoot, "GenericCodeGen.lib"),
                            Path.Combine(libsRoot, "MachineIndependent.lib"),
                            Path.Combine(libsRoot, "HLSL.lib"),
                            Path.Combine(libsRoot, "OSDependent.lib"),
                            Path.Combine(libsRoot, "OGLCompiler.lib"),
                            Path.Combine(libsRoot, "SPIRV-Tools-opt.lib"),
                            Path.Combine(libsRoot, "SPIRV-Tools.lib"),
                            Path.Combine(libsRoot, "SPIRV.lib"),
                            Path.Combine(libsRoot, "glslang.lib"),
                        };

                        // Build for Windows
                        var solutionPath = Path.Combine(buildDir, "glslang.sln");
                        SetupDirectory(buildDir, false);
                        RunCmake(root, platform, architecture, $"-B\"{buildDir}\" " + cmakeArgs);
                        Deploy.VCEnvironment.BuildSolution(solutionPath, configuration, architecture.ToString());
                        Utilities.Run("cmake", $"--build \"{buildDir}\" --config {configuration} --target install", null, buildDir, Utilities.RunOptions.ConsoleLogOutput);
                        var depsFolder = GetThirdPartyFolder(options, platform, architecture);
                        foreach (var file in outputFiles)
                        {
                            Utilities.FileCopy(file, Path.Combine(depsFolder, Path.GetFileName(file)));
                        }
                        break;
                    }
                    case TargetPlatform.Linux:
                    {
                        var outputFiles = new[]
                        {
                            Path.Combine(libsRoot, "libGenericCodeGen.a"),
                            Path.Combine(libsRoot, "libMachineIndependent.a"),
                            Path.Combine(libsRoot, "libHLSL.a"),
                            Path.Combine(libsRoot, "libOSDependent.a"),
                            Path.Combine(libsRoot, "libOGLCompiler.a"),
                            Path.Combine(libsRoot, "libSPIRV-Tools-opt.a"),
                            Path.Combine(libsRoot, "libSPIRV-Tools.a"),
                            Path.Combine(libsRoot, "libSPIRV.a"),
                            Path.Combine(libsRoot, "libglslang.a"),
                        };

                        // Build for Linux
                        RunCmake(root, platform, architecture, $"-B\"{buildDir}\" " + cmakeArgs);
                        Utilities.Run("make", null, null, buildDir, Utilities.RunOptions.ConsoleLogOutput);
                        Utilities.Run("cmake", $"--build \"{buildDir}\" --config {configuration} --target install", null, buildDir, Utilities.RunOptions.ConsoleLogOutput);
                        var depsFolder = GetThirdPartyFolder(options, platform, architecture);
                        foreach (var file in outputFiles)
                        {
                            var dst = Path.Combine(depsFolder, Path.GetFileName(file));
                            Utilities.FileCopy(file, dst);
                            //Utilities.Run("strip", string.Format("-s \"{0}\"", dst), null, null, Utilities.RunOptions.ConsoleLogOutput);
                        }
                        break;
                    }
                    case TargetPlatform.Mac:
                    {
                        var outputFiles = new[]
                        {
                            Path.Combine(libsRoot, "libGenericCodeGen.a"),
                            Path.Combine(libsRoot, "libMachineIndependent.a"),
                            Path.Combine(libsRoot, "libHLSL.a"),
                            Path.Combine(libsRoot, "libOSDependent.a"),
                            Path.Combine(libsRoot, "libOGLCompiler.a"),
                            Path.Combine(libsRoot, "libSPIRV-Tools-opt.a"),
                            Path.Combine(libsRoot, "libSPIRV-Tools.a"),
                            Path.Combine(libsRoot, "libSPIRV.a"),
                            Path.Combine(libsRoot, "libglslang.a"),
                        };

                        // Build for Mac
                        RunCmake(root, platform, architecture, $"-B\"{buildDir}\" " + cmakeArgs);
                        Utilities.Run("make", null, null, buildDir, Utilities.RunOptions.ConsoleLogOutput);
                        Utilities.Run("cmake", $"--build \"{buildDir}\" --config {configuration} --target install", null, buildDir, Utilities.RunOptions.ConsoleLogOutput);
                        var depsFolder = GetThirdPartyFolder(options, platform, architecture);
                        foreach (var file in outputFiles)
                        {
                            var dst = Path.Combine(depsFolder, Path.GetFileName(file));
                            Utilities.FileCopy(file, dst);
                            Utilities.Run("strip", string.Format("\"{0}\"", dst), null, null, Utilities.RunOptions.ConsoleLogOutput);
                        }
                        break;
                    }
                    }
                }
            }

            // Copy glslang headers
            foreach (var dir in new[]
            {
                "HLSL",
                "Include",
                "MachineIndependent",
                "Public",
                "SPIRV",
            })
            {
                var srcDir = Path.Combine(installDir, "include", "glslang", dir);
                var dstDir = Path.Combine(options.ThirdPartyFolder, "glslang", dir);
                Utilities.DirectoryDelete(dstDir);
                Utilities.DirectoryCopy(srcDir, dstDir);
            }

            // Copy spirv-tools headers
            foreach (var file in Directory.GetFiles(Path.Combine(options.ThirdPartyFolder, "spirv-tools"), "*.h"))
                File.Delete(file);
            foreach (var file in Directory.GetFiles(Path.Combine(options.ThirdPartyFolder, "spirv-tools"), "*.hpp"))
                File.Delete(file);
            foreach (var file in Directory.GetFiles(Path.Combine(installDir, "include", "spirv-tools"), "*.h"))
                Utilities.FileCopy(file, Path.Combine(options.ThirdPartyFolder, "spirv-tools", Path.GetFileName(file)));
            foreach (var file in Directory.GetFiles(Path.Combine(installDir, "include", "spirv-tools"), "*.hpp"))
                Utilities.FileCopy(file, Path.Combine(options.ThirdPartyFolder, "spirv-tools", Path.GetFileName(file)));
        }
    }
}
