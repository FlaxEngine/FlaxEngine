// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System;
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
            CloneGitRepo(root, "https://github.com/FlaxEngine/assimp.git");
            GitCheckout(root, "master", "5c900d689a5db5637b98f665fc1e9e9c9ed416b9");

            foreach (var platform in options.Platforms)
            {
                switch (platform)
                {
                case TargetPlatform.Windows:
                {
                    var solutionPath = Path.Combine(root, "Assimp.sln");
                    var configuration = "Release";
                    var binariesWin = new[]
                    {
                        Path.Combine(root, "bin", configuration, "assimp-vc140-md.dll"),
                        Path.Combine(root, "lib", configuration, "assimp-vc140-md.lib"),
                    };

                    // Build for Win64
                    File.Delete(Path.Combine(root, "CMakeCache.txt"));
                    RunCmake(root, platform, TargetArchitecture.x64);
                    Deploy.VCEnvironment.BuildSolution(solutionPath, configuration, "x64");
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.x64);
                    foreach (var file in binariesWin)
                    {
                        Utilities.FileCopy(file, Path.Combine(depsFolder, Path.GetFileName(file)));
                    }

                    break;
                }
                case TargetPlatform.Linux:
                {
                    // Build for Linux
                    RunCmake(root, platform, TargetArchitecture.x64, " -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF " + globalConfig);
                    Utilities.Run("make", null, null, root, Utilities.RunOptions.ThrowExceptionOnError);
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.x64);
                    Utilities.FileCopy(Path.Combine(root, "lib", "libassimp.a"), Path.Combine(depsFolder, "libassimp.a"));
                    Utilities.FileCopy(Path.Combine(root, "lib", "libIrrXML.a"), Path.Combine(depsFolder, "libIrrXML.a"));
                    break;
                }
                case TargetPlatform.Mac:
                {
                    // Build for Mac
                    foreach (var architecture in new[] { TargetArchitecture.x64, TargetArchitecture.ARM64 })
                    {
                        RunCmake(root, platform, architecture, " -DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=OFF " + globalConfig);
                        Utilities.Run("make", null, null, root, Utilities.RunOptions.ThrowExceptionOnError);
                        var depsFolder = GetThirdPartyFolder(options, platform, architecture);
                        Utilities.FileCopy(Path.Combine(root, "lib", "libassimp.a"), Path.Combine(depsFolder, "libassimp.a"));
                        Utilities.FileCopy(Path.Combine(root, "lib", "libIrrXML.a"), Path.Combine(depsFolder, "libIrrXML.a"));
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
        }
    }
}
