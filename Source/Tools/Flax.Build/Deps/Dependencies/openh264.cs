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

            // Note: assumes the nasm package is installed on the system
            // Get the source
            CloneGitRepoFast(root, "https://github.com/cisco/openh264.git");

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

                            Utilities.Run("make", buildArgs, null, root, Utilities.RunOptions.ThrowExceptionOnError, envVars);

                            // Copy library
                            var libFile = Path.Combine(root, "libopenh264.a");
                            var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.x64);
                            SetupDirectory(depsFolder, true);
                            Utilities.FileCopy(libFile, Path.Combine(depsFolder, "libopenh264.a"));

                            Utilities.Run("make", "clean", null, root, Utilities.RunOptions.Default, envVars);

                            break;
                        }
                }
            }
            
            // Deploy headers and license (preserve module build rules file)
            var moduleFilename = "openh264.Build.cs";
            var srcIncludePath = Path.Combine(root, "codec", "api", "wels");
            var dstIncludePath = Path.Combine(options.ThirdPartyFolder, "openh264");

            // Backup Build file
            var moduleFile = Path.Combine(dstIncludePath, moduleFilename);
            var moduleFileBackup = Path.Combine(root, moduleFilename);
            if (File.Exists(moduleFile))
                Utilities.FileCopy(moduleFile, moduleFileBackup);

            // Clean folder and copy files
            SetupDirectory(dstIncludePath, true);
            Utilities.FileCopy(moduleFileBackup, moduleFile);
            Utilities.DirectoryCopy(srcIncludePath, dstIncludePath, true, true);
            Utilities.FileCopy(Path.Combine(root, "LICENSE"), Path.Combine(dstIncludePath, "LICENSE"));
        }
    }
}
