// Copyright (c) Wojciech Figat. All rights reserved.

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
        public override void Build(BuildOptions options)
        {
            var root = options.IntermediateFolder;
            var installDir = Path.Combine(root, "install");
            var configuration = "Release";
            var cmakeArgs = string.Format("-DCMAKE_INSTALL_PREFIX=\"{0}\" -DCMAKE_BUILD_TYPE={1} -DENABLE_RTTI=ON -DENABLE_CTEST=OFF -DENABLE_HLSL=ON -DENABLE_SPVREMAPPER=ON -DENABLE_GLSLANG_BINARIES=OFF", installDir, configuration);
            var libsRoot = Path.Combine(installDir, "lib");

            // Get the source
            CloneGitRepoFast(root, "https://github.com/FlaxEngine/glslang.git");

            // Setup the external sources
            // Requires distutils (pip install setuptools)
            Utilities.Run("python", "update_glslang_sources.py", null, root, Utilities.RunOptions.ConsoleLogOutput);

            foreach (var platform in options.Platforms)
            {
                BuildStarted(platform);
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
                    foreach (var architecture in new[] { TargetArchitecture.x64, TargetArchitecture.ARM64 })
                    {
                        var buildDir = Path.Combine(root, "build-" + architecture.ToString());
                        var solutionPath = Path.Combine(buildDir, "glslang.sln");

                        SetupDirectory(buildDir, false);
                        RunCmake(root, platform, architecture, cmakeArgs + $" -B\"{buildDir}\"");
                        Utilities.Run("cmake", string.Format("--build . --config {0} --target install", configuration), null, buildDir, Utilities.RunOptions.ConsoleLogOutput);
                        Deploy.VCEnvironment.BuildSolution(solutionPath, configuration, architecture.ToString());
                        var depsFolder = GetThirdPartyFolder(options, platform, architecture);
                        foreach (var file in outputFiles)
                        {
                            Utilities.FileCopy(file, Path.Combine(depsFolder, Path.GetFileName(file)));
                        }
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
                    var buildDir = root;

                    // Build for Linux
                    RunCmake(root, platform, TargetArchitecture.x64, cmakeArgs);
                    Utilities.Run("cmake", string.Format("--build . --config {0} --target install", configuration), null, buildDir, Utilities.RunOptions.ConsoleLogOutput);
                    Utilities.Run("make", null, null, root, Utilities.RunOptions.ConsoleLogOutput);
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.x64);
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
                    var buildDir = root;

                    // Build for Mac
                    foreach (var architecture in new[] { TargetArchitecture.x64, TargetArchitecture.ARM64 })
                    {
                        RunCmake(root, platform, architecture, cmakeArgs);
                        Utilities.Run("cmake", string.Format("--build . --config {0} --target install", configuration), null, buildDir, Utilities.RunOptions.ConsoleLogOutput);
                        Utilities.Run("make", null, null, root, Utilities.RunOptions.ConsoleLogOutput);
                        var depsFolder = GetThirdPartyFolder(options, platform, architecture);
                        foreach (var file in outputFiles)
                        {
                            var dst = Path.Combine(depsFolder, Path.GetFileName(file));
                            Utilities.FileCopy(file, dst);
                            Utilities.Run("strip", string.Format("\"{0}\"", dst), null, null, Utilities.RunOptions.ConsoleLogOutput);
                        }
                    }
                    break;
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
