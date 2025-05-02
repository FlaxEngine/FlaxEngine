// Copyright (c) Wojciech Figat. All rights reserved.

using System.IO;
using Flax.Build;

namespace Flax.Deps.Dependencies
{
    /// <summary>
    /// DirectXMesh geometry processing library https://walbourn.github.io/directxmesh/
    /// </summary>
    /// <seealso cref="Flax.Deps.Dependency" />
    class DirectXMesh : Dependency
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
            var solutionPath = Path.Combine(root, "DirectXMesh_Desktop_2022_Win10.sln");
            var configuration = "Release";
            var outputFileNames = new[]
            {
                "DirectXMesh.lib",
                "DirectXMesh.pdb",
            };
            var binFolder = Path.Combine(root, "DirectXMesh", "Bin", "Desktop_2022_Win10");

            // Get the source
            CloneGitRepoFast(root, "https://github.com/Microsoft/DirectXMesh.git");

            foreach (var platform in options.Platforms)
            {
                BuildStarted(platform);
                switch (platform)
                {
                case TargetPlatform.Windows:
                {
                    foreach (var architecture in new[] { TargetArchitecture.x64, TargetArchitecture.ARM64 })
                    {
                        Deploy.VCEnvironment.BuildSolution(solutionPath, configuration, architecture.ToString());
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
            var dstIncludePath = Path.Combine(options.ThirdPartyFolder, "DirectXMesh");
            foreach (var file in new[]
            {
                "DirectXMesh/DirectXMesh.h",
                "DirectXMesh/DirectXMesh.inl",
                "LICENSE",
            })
            {
                Utilities.FileCopy(Path.Combine(root, file), Path.Combine(dstIncludePath, Path.GetFileName(file)));
            }
        }
    }
}
