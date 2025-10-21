using System.Collections.Generic;
using System.IO;
using Flax.Build;
using Flax.Build.Platforms;

namespace Flax.Deps.Dependencies
{
    /// <summary>
    /// Open Source H.264 Codec (OpenH264)
    /// </summary>
    class openh264 : Dependency
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
            var repoPath = Path.Combine(root, "openh264");

            // Note: assumes the nasm package is installed on the system

            // Get the source
            CloneGitRepoFast(repoPath, "https://github.com/cisco/openh264.git");

            string buildArgs = "ARCH=x86_64 OS=linux BUILDTYPE=Release";

            foreach (var platform in options.Platforms)
            {
                BuildStarted(platform);

                switch (platform)
                {
                    case TargetPlatform.Linux:
                    {
                        var envVars = new Dictionary<string, string>
                        {
                            { "CC", "clang" },
                            { "CXX", "clang++" }
                        };

                        Utilities.Run("make", buildArgs, null, repoPath, Utilities.RunOptions.ThrowExceptionOnError, envVars);

                        // Copy library
                        var libFile = Path.Combine(repoPath, "libopenh264.a");
                        var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.x64);
                        SetupDirectory(depsFolder, true);
                        Utilities.FileCopy(libFile, Path.Combine(depsFolder, "libopenh264.a"));

                        Utilities.Run("make", "clean", null, repoPath, Utilities.RunOptions.Default, envVars);

                        break;
                    }
                }
            }

            // Copy headers
            var includeSrc = Path.Combine(repoPath, "codec", "api", "wels");
            var includeDst = Path.Combine(options.ThirdPartyFolder, "openh264");
            SetupDirectory(includeDst, true);
            Utilities.DirectoryCopy(includeSrc, includeDst, true, true);

            // Copy license
            var licenseSrc = Path.Combine(repoPath, "LICENSE");
            if (File.Exists(licenseSrc))
                Utilities.FileCopy(licenseSrc, Path.Combine(includeDst, "LICENSE"));
        }
    }
}
