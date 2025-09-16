// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using Flax.Build;
using Flax.Build.Platforms;

namespace Flax.Deps.Dependencies
{
    /// <summary>
    /// The LibTIFF software provides support for the Tag Image File Format (TIFF), a widely used format for storing image data. https://libtiff.gitlab.io/libtiff/
    /// </summary>
    /// <seealso cref="Flax.Deps.Dependency" />
    class libtiff : Dependency
    {
        /// <inheritdoc />
        public override TargetPlatform[] Platforms
        {
            get
            {
                switch (BuildPlatform)
                {
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
            var moduleFilename = "libtiff.Build.cs";
            string configHeaderFilePath = null;
            var configs = new string[]
            {
                "--enable-static",
    		    "--disable-shared",
    		    "--disable-tools",
    		    "--disable-tests",
    		    "--disable-contrib",
    		    "--disable-docs",
    		    "--disable-jbig",
    		    "--disable-jpeg",
    		    "--disable-lzma",
    		    "--disable-zstd",
    		    "--disable-webp",
    		    "--disable-lerc",
    		    "CFLAGS=\"-O3 -fPIC\"",
            };
            var globalConfig = string.Join(" ", configs);

            // Get the source
            CloneGitRepoFast(root, "https://gitlab.com/libtiff/libtiff.git");

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
                            { "CXX", "clang++" },
                        };

                        Utilities.Run(Path.Combine(root, "autogen.sh"), null, null, root, Utilities.RunOptions.ThrowExceptionOnError, envVars);

                        var toolchain = UnixToolchain.GetToolchainName(platform, TargetArchitecture.x64);
                        Utilities.Run(Path.Combine(root, "configure"), string.Format("--host={0} {1}", toolchain, globalConfig), null, root, Utilities.RunOptions.Default, envVars);

                        Utilities.Run("make", null, null, root, Utilities.RunOptions.ThrowExceptionOnError, envVars);
                        configHeaderFilePath = Path.Combine(root, "libtiff", "tif_config.h");

                        var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.x64);
                        Utilities.FileCopy(Path.Combine(root, "libtiff", ".libs", "libtiff.a"), Path.Combine(depsFolder, "libtiff.a"));
                        Utilities.Run("make", "clean", null, root, Utilities.RunOptions.ThrowExceptionOnError);

                        break;
                    }
                    case TargetPlatform.Mac:
                    {
                        var envVars = new Dictionary<string, string>
                        {
                            { "CC", "clang" },
                            { "CXX", "clang++" },
                        };
                        
                        Utilities.Run(Path.Combine(root, "autogen.sh"), null, null, root, Utilities.RunOptions.ThrowExceptionOnError, envVars);

                        foreach (var architecture in new[] { TargetArchitecture.x64, TargetArchitecture.ARM64 })
                        {
                            var toolchain = MacToolchain.GetToolchainName(platform, architecture);
                            Utilities.Run(Path.Combine(root, "configure"), string.Format("--host={0} {1}", toolchain, globalConfig), null, root, Utilities.RunOptions.Default, envVars);
                            Utilities.Run("make", null, null, root, Utilities.RunOptions.ThrowExceptionOnError, envVars);
                            configHeaderFilePath = Path.Combine(root, "libtiff", "tif_config.h");

                            var depsFolder = GetThirdPartyFolder(options, platform, architecture);
                            Utilities.FileCopy(Path.Combine(root, "libtiff", ".libs", "libtiff.a"), Path.Combine(depsFolder, "libtiff.a"));
                            Utilities.Run("make", "clean", null, root, Utilities.RunOptions.ThrowExceptionOnError);
                        }
                        break;
                    }
                }
            }

            // Deploy header files and license (preserve module build rules file)
            var srcIncludePath = Path.Combine(root, "libtiff");
            var dstIncludePath = Path.Combine(options.ThirdPartyFolder, "libtiff");
            var moduleFile = Path.Combine(dstIncludePath, moduleFilename);
            var moduleFileBackup = Path.Combine(root, moduleFilename);
            Utilities.FileCopy(moduleFile, moduleFileBackup);
            SetupDirectory(dstIncludePath, true);
            Utilities.FileCopy(moduleFileBackup, moduleFile);
            Utilities.DirectoryCopy(srcIncludePath, dstIncludePath, true, true);
            Utilities.FileCopy(Path.Combine(root, "LICENSE.md"), Path.Combine(dstIncludePath, "LICENSE.md"));
            Utilities.FileCopy(configHeaderFilePath, Path.Combine(dstIncludePath, "tif_config.h"));
        }
    }
}
