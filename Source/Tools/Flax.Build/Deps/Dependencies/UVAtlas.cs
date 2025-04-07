// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using Flax.Build;

namespace Flax.Deps.Dependencies
{
    /// <summary>
    /// UVAtlas isochart texture atlas https://walbourn.github.io/uvatlas-return-of-the-isochart/
    /// </summary>
    /// <seealso cref="Flax.Deps.Dependency" />
    class UVAtlas : Dependency
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
                default: return new TargetPlatform[0];
                }
            }
        }

        /// <inheritdoc />
        public override void Build(BuildOptions options)
        {
            var root = options.IntermediateFolder;
            var solutionPath = Path.Combine(root, "UVAtlas_2022_Win10.sln");
            var configuration = "Release";
            var outputFileNames = new[]
            {
                "UVAtlas.lib",
                "UVAtlas.pdb",
            };
            var binFolder = Path.Combine(root, "UVAtlas", "Bin", "Desktop_2022_Win10");

            // Get the source
            CloneGitRepoFast(root, "https://github.com/Microsoft/UVAtlas.git");

            foreach (var platform in options.Platforms)
            {
                BuildStarted(platform);
                switch (platform)
                {
                case TargetPlatform.Windows:
                {
                    // Build for Win64
                    foreach (var architecture in new[] { TargetArchitecture.x64, TargetArchitecture.ARM64 })
                    {
                        Deploy.VCEnvironment.BuildSolution(solutionPath, configuration, architecture.ToString(), new Dictionary<string, string>() { { "RestorePackagesConfig", "true" } });
                        var depsFolder = GetThirdPartyFolder(options, TargetPlatform.Windows, architecture);
                        foreach (var file in outputFileNames)
                        {
                            Utilities.FileCopy(Path.Combine(binFolder, architecture.ToString(), "Release", file), Path.Combine(depsFolder, file));
                        }
                    }
                    break;
                }
                }
            }

            // Deploy header files and license file
            var dstIncludePath = Path.Combine(options.ThirdPartyFolder, "UVAtlas");
            foreach (var file in new[]
            {
                "UVAtlas/inc/UVAtlas.h",
                "LICENSE",
            })
            {
                Utilities.FileCopy(Path.Combine(root, file), Path.Combine(dstIncludePath, Path.GetFileName(file)));
            }
        }
    }
}
