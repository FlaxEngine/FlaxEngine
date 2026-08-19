using System;
using System.Collections.Generic;
using System.IO;
using Flax.Build;
using Flax.Build.Platforms;

namespace Flax.Deps.Dependencies
{
    /// <summary>
    /// Fraunhofer FDK AAC library build script.
    /// </summary>
    class fdkaac : Dependency
    {
        public override TargetPlatform[] Platforms
        {
            get
            {
                if (BuildPlatform == TargetPlatform.Linux)
                    return new[] { TargetPlatform.Linux };
                return new TargetPlatform[0];
            }
        }

        public override void Build(BuildOptions options)
        {
            var root = options.IntermediateFolder;

            // Get the source
            CloneGitRepoFast(root, "https://github.com/mstorsjo/fdk-aac.git");
            
            var buildDir = Path.Combine(root, "build");

            SetupDirectory(buildDir, true);

            var cmakeFlags = string.Join(" ", new[]
            {
                "-DCMAKE_BUILD_TYPE=Release",
                "-DBUILD_SHARED_LIBS=OFF",
                "-DCMAKE_POSITION_INDEPENDENT_CODE=ON"
            });

            foreach (var platform in options.Platforms)
            {
                BuildStarted(platform);

                switch (platform)
                {
                    case TargetPlatform.Linux:
                        {
                            var env = new Dictionary<string, string>
                        {
                            { "CC", "clang" },
                            { "CXX", "clang++" }
                        };

                            Utilities.Run("cmake", $".. {cmakeFlags}", null, buildDir, Utilities.RunOptions.ThrowExceptionOnError, env);
                            Utilities.Run("make", null, null, buildDir, Utilities.RunOptions.ThrowExceptionOnError, env);

                            // Copy library
                            var libFile = Path.Combine(buildDir, "libfdk-aac.a");
                            var depsLibDir = GetThirdPartyFolder(options, platform, TargetArchitecture.x64);
                            Utilities.FileCopy(libFile, Path.Combine(depsLibDir, "libfdk-aac.a"));

                            break;
                        }
                }
            }

            // Deploy headers and license (preserve module build rules file)
            var moduleFilename = "fdkaac.Build.cs";

            String[] srcIncludePath =
            {
                Path.Combine(root, "libSYS", "include"),
                Path.Combine(root, "libAACenc", "include"),
                Path.Combine(root, "libAACdec", "include"),
            };

            var dstIncludePath = Path.Combine(options.ThirdPartyFolder, "fdkaac");

            // Backup Build file
            var moduleFile = Path.Combine(dstIncludePath, moduleFilename);
            var moduleFileBackup = Path.Combine(root, moduleFilename);
            if (File.Exists(moduleFile))
                Utilities.FileCopy(moduleFile, moduleFileBackup);

            // Clean folder and copy files
            SetupDirectory(dstIncludePath, true);
            Utilities.FileCopy(moduleFileBackup, moduleFile);
            foreach (var dir in srcIncludePath)
                Utilities.DirectoryCopy(dir, dstIncludePath, true, true);
            Utilities.FileCopy(Path.Combine(root, "NOTICE"), Path.Combine(dstIncludePath, "NOTICE"));
        }
    }
}
