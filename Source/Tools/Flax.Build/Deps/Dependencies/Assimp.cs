// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using Flax.Build;

namespace Flax.Deps.Dependencies
{
    /// <summary>
    /// Official Open Asset Import Library Repository. Loads 40+ 3D file formats into one unified and clean data structure. http://www.assimp.org
    /// </summary>
    /// <seealso cref="Flax.Deps.Dependency" />
    class Assimp : Dependency
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
            var moduleFilename = "assimp.Build.cs";
            string configHeaderFilePath = null;
            var configs = new string[]
            {
                "-DASSIMP_NO_EXPORT=ON",
                "-DASSIMP_BUILD_ASSIMP_TOOLS=OFF",
                "-DASSIMP_BUILD_TESTS=OFF",
                "-DASSIMP_BUILD_AMF_IMPORTER=FALSE",
                "-DASSIMP_BUILD_AC_IMPORTER=FALSE",
                "-DASSIMP_BUILD_ASE_IMPORTER=FALSE",
                "-DASSIMP_BUILD_ASSBIN_IMPORTER=FALSE",
                "-DASSIMP_BUILD_ASSXML_IMPORTER=FALSE",
                "-DASSIMP_BUILD_DXF_IMPORTER=FALSE",
                "-DASSIMP_BUILD_CSM_IMPORTER=FALSE",
                "-DASSIMP_BUILD_HMP_IMPORTER=FALSE",
                "-DASSIMP_BUILD_NFF_IMPORTER=FALSE",
                "-DASSIMP_BUILD_NDO_IMPORTER=FALSE",
                "-DASSIMP_BUILD_IFC_IMPORTER=FALSE",
                "-DASSIMP_BUILD_FBX_IMPORTER=FALSE",
                "-DASSIMP_BUILD_RAW_IMPORTER=FALSE",
                "-DASSIMP_BUILD_TERRAGEN_IMPORTER=FALSE",
                "-DASSIMP_BUILD_3MF_IMPORTER=FALSE",
                "-DASSIMP_BUILD_STEP_IMPORTER=FALSE",
                "-DASSIMP_BUILD_3DS_IMPORTER=FALSE",
                "-DASSIMP_BUILD_BVH_IMPORTER=FALSE",
                "-DASSIMP_BUILD_IRRMESH_IMPORTER=FALSE",
                "-DASSIMP_BUILD_IRR_IMPORTER=FALSE",
                "-DASSIMP_BUILD_MD2_IMPORTER=FALSE",
                "-DASSIMP_BUILD_MD3_IMPORTER=FALSE",
                "-DASSIMP_BUILD_MD5_IMPORTER=FALSE",
                "-DASSIMP_BUILD_MDC_IMPORTER=FALSE",
                "-DASSIMP_BUILD_MDL_IMPORTER=FALSE",
                "-DASSIMP_BUILD_OFF_IMPORTER=FALSE",
                "-DASSIMP_BUILD_COB_IMPORTER=FALSE",
                "-DASSIMP_BUILD_Q3D_IMPORTER=FALSE",
                "-DASSIMP_BUILD_Q3BSP_IMPORTER=FALSE",
                "-DASSIMP_BUILD_SIB_IMPORTER=FALSE",
                "-DASSIMP_BUILD_SMD_IMPORTER=FALSE",
                "-DASSIMP_BUILD_X3D_IMPORTER=FALSE",
                "-DASSIMP_BUILD_MMD_IMPORTER=FALSE",
            };
            var globalConfig = string.Join(" ", configs);

            // Get the source
            CloneGitRepoSingleBranch(root, "https://github.com/assimp/assimp.git", "master", "10df90ec144179f97803a382e4f07c0570665864");

            foreach (var platform in options.Platforms)
            {
                BuildStarted(platform);
                switch (platform)
                {
                case TargetPlatform.Windows:
                {
                    var configuration = "Release";
                    var binariesWin = new[]
                    {
                        Path.Combine("bin", configuration, "assimp-vc140-md.dll"),
                        Path.Combine("lib", configuration, "assimp-vc140-md.lib"),
                    };

                    // Build for Windows
                    File.Delete(Path.Combine(root, "CMakeCache.txt"));
                    foreach (var architecture in new[] { TargetArchitecture.x64, TargetArchitecture.ARM64 })
                    {
                        var buildDir = Path.Combine(root, "build-" + architecture);
                        var solutionPath = Path.Combine(buildDir, "Assimp.sln");
                        SetupDirectory(buildDir, true);
                        RunCmake(root, platform, architecture, $"-B\"{buildDir}\" -DLIBRARY_SUFFIX=-vc140-md -DUSE_STATIC_CRT=OFF " + globalConfig);
                        Deploy.VCEnvironment.BuildSolution(solutionPath, configuration, architecture.ToString());
                        configHeaderFilePath = Path.Combine(buildDir, "include", "assimp", "config.h");
                        var depsFolder = GetThirdPartyFolder(options, platform, architecture);
                        foreach (var file in binariesWin)
                            Utilities.FileCopy(Path.Combine(buildDir, file), Path.Combine(depsFolder, Path.GetFileName(file)));
                    }

                    break;
                }
                case TargetPlatform.Linux:
                {
                    var envVars = new Dictionary<string, string>
                    {
                        { "CC", "clang-13" },
                        { "CC_FOR_BUILD", "clang-13" },
                        { "CXX", "clang++-13" },
                    };

                    // Build for Linux
                    RunCmake(root, platform, TargetArchitecture.x64, " -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF " + globalConfig, envVars);
                    Utilities.Run("make", null, null, root, Utilities.RunOptions.ThrowExceptionOnError, envVars);
                    configHeaderFilePath = Path.Combine(root, "include", "assimp", "config.h");
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.x64);
                    Utilities.FileCopy(Path.Combine(root, "lib", "libassimp.a"), Path.Combine(depsFolder, "libassimp.a"));
                    break;
                }
                case TargetPlatform.Mac:
                {
                    // Build for Mac
                    foreach (var architecture in new[] { TargetArchitecture.x64, TargetArchitecture.ARM64 })
                    {
                        RunCmake(root, platform, architecture, " -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF " + globalConfig);
                        Utilities.Run("make", null, null, root, Utilities.RunOptions.ThrowExceptionOnError);
                        configHeaderFilePath = Path.Combine(root, "include", "assimp", "config.h");
                        var depsFolder = GetThirdPartyFolder(options, platform, architecture);
                        Utilities.FileCopy(Path.Combine(root, "lib", "libassimp.a"), Path.Combine(depsFolder, "libassimp.a"));
                        Utilities.Run("make", "clean", null, root, Utilities.RunOptions.ThrowExceptionOnError);
                    }
                    break;
                }
                }
            }

            // Deploy header files and license (preserve module build rules file)
            var srcIncludePath = Path.Combine(root, "include", "assimp");
            var dstIncludePath = Path.Combine(options.ThirdPartyFolder, "assimp");
            var moduleFile = Path.Combine(dstIncludePath, moduleFilename);
            var moduleFileBackup = Path.Combine(root, moduleFilename);
            Utilities.FileCopy(moduleFile, moduleFileBackup);
            SetupDirectory(dstIncludePath, true);
            Utilities.FileCopy(moduleFileBackup, moduleFile);
            Utilities.DirectoryCopy(srcIncludePath, dstIncludePath, true, true);
            Utilities.FileCopy(Path.Combine(root, "LICENSE"), Path.Combine(dstIncludePath, "LICENSE"));
            Utilities.FileCopy(configHeaderFilePath, Path.Combine(dstIncludePath, "config.h"));
        }
    }
}
