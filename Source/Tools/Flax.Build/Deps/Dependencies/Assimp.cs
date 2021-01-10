// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

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
                default: return new TargetPlatform[0];
                }
            }
        }

        /// <inheritdoc />
        public override void Build(BuildOptions options)
        {
            var root = options.IntermediateFolder;
            var moduleFilename = "assimp.Build.cs";

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
                    RunCmake(root, TargetPlatform.Windows, TargetArchitecture.x64);
                    Deploy.VCEnvironment.BuildSolution(solutionPath, configuration, "x64");
                    var depsFolder = GetThirdPartyFolder(options, TargetPlatform.Windows, TargetArchitecture.x64);
                    foreach (var file in binariesWin)
                    {
                        Utilities.FileCopy(file, Path.Combine(depsFolder, Path.GetFileName(file)));
                    }

                    break;
                }
                case TargetPlatform.Linux:
                {
                    // Build for Linux
                    RunCmake(root, TargetPlatform.Linux, TargetArchitecture.x64);
                    Utilities.Run("make", null, null, root, Utilities.RunOptions.None);
                    var depsFolder = GetThirdPartyFolder(options, TargetPlatform.Linux, TargetArchitecture.x64);
                    Utilities.FileCopy(Path.Combine(root, "lib", "libassimp.so"), Path.Combine(depsFolder, "libassimp.so"));
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
