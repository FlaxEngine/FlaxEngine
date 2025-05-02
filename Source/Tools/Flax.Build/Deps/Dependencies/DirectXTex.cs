// Copyright (c) Wojciech Figat. All rights reserved.

using System.IO;
using Flax.Build;

namespace Flax.Deps.Dependencies
{
    /// <summary>
    /// DirectXTex texture processing library https://walbourn.github.io/directxtex/
    /// </summary>
    /// <seealso cref="Flax.Deps.Dependency" />
    class DirectXTex : Dependency
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
                        TargetPlatform.XboxOne,
                        TargetPlatform.XboxScarlett,
                    };
                default: return new TargetPlatform[0];
                }
            }
        }

        /// <inheritdoc />
        public override void Build(BuildOptions options)
        {
            var root = options.IntermediateFolder;
            var configuration = "Release";
            var outputFileNames = new[]
            {
                "DirectXTex.lib",
                "DirectXTex.pdb",
            };

            // Get the source
            CloneGitRepo(root, "https://github.com/Microsoft/DirectXTex.git");
            GitCheckout(root, "main", "5cfd711dc5d64cde1e8b27670036535df5c3f922");

            foreach (var platform in options.Platforms)
            {
                BuildStarted(platform);
                switch (platform)
                {
                case TargetPlatform.Windows:
                {
                    var solutionPath = Path.Combine(root, "DirectXTex_Desktop_2022_Win10.sln");
                    var binFolder = Path.Combine(root, "DirectXTex", "Bin", "Desktop_2022_Win10");
                    foreach (var architecture in new[] { TargetArchitecture.x64, TargetArchitecture.ARM64 })
                    {
                        Deploy.VCEnvironment.BuildSolution(solutionPath, configuration, architecture.ToString());
                        var depsFolder = GetThirdPartyFolder(options, platform, architecture);
                        foreach (var file in outputFileNames)
                            Utilities.FileCopy(Path.Combine(binFolder, architecture.ToString(), configuration, file), Path.Combine(depsFolder, file));
                    }
                    break;
                }
                case TargetPlatform.UWP:
                {
                    var solutionPath = Path.Combine(root, "DirectXTex_Windows10_2019.sln");
                    var binFolder = Path.Combine(root, "DirectXTex", "Bin", "Windows10_2019");
                    Deploy.VCEnvironment.BuildSolution(solutionPath, configuration, "x64");
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.x64);
                    foreach (var file in outputFileNames)
                        Utilities.FileCopy(Path.Combine(binFolder, "x64", configuration, file), Path.Combine(depsFolder, file));
                    break;
                }
                case TargetPlatform.XboxOne:
                case TargetPlatform.XboxScarlett:
                {
                    var solutionPath = Path.Combine(root, "DirectXTex_GDK_2022.sln");
                    var binFolder = Path.Combine(root, "DirectXTex", "Bin", "GDK_2022");
                    var xboxName = platform == TargetPlatform.XboxOne ? "Gaming.Xbox.XboxOne.x64" : "Gaming.Xbox.Scarlett.x64";
                    Deploy.VCEnvironment.BuildSolution(solutionPath, configuration, xboxName);
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.x64);
                    foreach (var file in outputFileNames)
                        Utilities.FileCopy(Path.Combine(binFolder, xboxName, configuration, file), Path.Combine(depsFolder, file));
                    break;
                }
                }
            }

            // Deploy header files and license file
            var dstIncludePath = Path.Combine(options.ThirdPartyFolder, "DirectXTex");
            foreach (var file in new[]
            {
                "DirectXTex/DirectXTex.h",
                "DirectXTex/DirectXTex.inl",
                "LICENSE",
            })
            {
                Utilities.FileCopy(Path.Combine(root, file), Path.Combine(dstIncludePath, Path.GetFileName(file)));
            }
        }
    }
}
