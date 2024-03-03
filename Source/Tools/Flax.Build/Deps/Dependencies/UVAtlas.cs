// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
            var solutionPath = Path.Combine(root, "UVAtlas", "UVAtlas_2015.sln");
            var configuration = "Release";
            var outputFileNames = new[]
            {
                "UVAtlas.lib",
                "UVAtlas.pdb",
            };
            var binFolder = Path.Combine(root, "UVAtlas", "Bin", "Desktop_2015");

            // Get the source
            CloneGitRepoFast(root, "https://github.com/Microsoft/UVAtlas.git");

            foreach (var platform in options.Platforms)
            {
                switch (platform)
                {
                case TargetPlatform.Windows:
                {
                    // Build for Win64
                    Deploy.VCEnvironment.BuildSolution(solutionPath, configuration, "x64");
                    var depsFolder = GetThirdPartyFolder(options, TargetPlatform.Windows, TargetArchitecture.x64);
                    foreach (var file in outputFileNames)
                    {
                        Utilities.FileCopy(Path.Combine(binFolder, "x64", "Release", file), Path.Combine(depsFolder, file));
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
