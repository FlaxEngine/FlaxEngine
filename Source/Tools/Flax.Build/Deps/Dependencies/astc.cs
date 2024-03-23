// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

using System.IO;
using Flax.Build;

namespace Flax.Deps.Dependencies
{
    /// <summary>
    /// ASTC texture format compression lib.
    /// </summary>
    /// <seealso cref="Flax.Deps.Dependency" />
    class astc : Dependency
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
            var buildDir = Path.Combine(root, "build");

            // Get the source
            var commit = "aeece2f609db959d1c5e43e4f00bd177ea130575"; // 4.6.1
            CloneGitRepo(root, "https://github.com/ARM-software/astc-encoder.git", commit);

            foreach (var platform in options.Platforms)
            {
                switch (platform)
                {
                case TargetPlatform.Windows:
                    foreach (var architecture in new []{ TargetArchitecture.x64 })
                    {
                        var isa = "-DASTCENC_ISA_SSE2=ON";
                        var lib = "astcenc-sse2-static.lib";
                        SetupDirectory(buildDir, true);
                        RunCmake(buildDir, platform, architecture, ".. -DCMAKE_BUILD_TYPE=Release -DASTCENC_CLI=OFF -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL " + isa);
                        BuildCmake(buildDir);
                        var depsFolder = GetThirdPartyFolder(options, platform, architecture);
                        Utilities.FileCopy(Path.Combine(buildDir, "Source/Release", lib), Path.Combine(depsFolder, "astcenc.lib"));
                    }
                    break;
                case TargetPlatform.Mac:
                    foreach (var architecture in new []{ TargetArchitecture.x64, TargetArchitecture.ARM64 })
                    {
                        var isa = architecture == TargetArchitecture.ARM64 ? "-DASTCENC_ISA_NEON=ON" : "-DASTCENC_ISA_SSE2=ON";
                        var lib = architecture == TargetArchitecture.ARM64 ? "libastcenc-neon-static.a" : "libastcenc-sse2-static.a";
                        SetupDirectory(buildDir, true);
                        RunCmake(buildDir, platform, architecture, ".. -DCMAKE_BUILD_TYPE=Release -DASTCENC_UNIVERSAL_BUILD=OFF -DASTCENC_UNIVERSAL_BINARY=OFF " + isa);
                        BuildCmake(buildDir);
                        var depsFolder = GetThirdPartyFolder(options, platform, architecture);
                        Utilities.FileCopy(Path.Combine(buildDir, "Source", lib), Path.Combine(depsFolder, "libastcenc.a"));
                    }
                    break;
                }
            }

            // Copy header and license
            Utilities.FileCopy(Path.Combine(root, "LICENSE.txt"), Path.Combine(options.ThirdPartyFolder, "astc/LICENSE.txt"));
            Utilities.FileCopy(Path.Combine(root, "Source/astcenc.h"), Path.Combine(options.ThirdPartyFolder, "astc/astcenc.h"));
        }
    }
}
