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

            // Copy headers
            var includeDst = Path.Combine(options.ThirdPartyFolder, "fdkaac");
            SetupDirectory(includeDst, true);

            String[] includeDirs =
            {
                Path.Combine(root, "libSYS", "include"),
                Path.Combine(root, "libAACenc", "include"),
                Path.Combine(root, "libAACdec", "include"),
            };

            foreach (var dir in includeDirs)
                Utilities.DirectoryCopy(dir, includeDst, true, true);

            // License
            var licenseSrc = Path.Combine(root, "NOTICE");
            if (File.Exists(licenseSrc))
                Utilities.FileCopy(licenseSrc, Path.Combine(includeDst, "NOTICE"));
        }
    }
}
