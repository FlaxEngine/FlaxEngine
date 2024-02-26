// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
                        TargetPlatform.UWP,
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
            GitCheckout(root, "master", "9a417f506c43e087b84c017260ad673abd6c64e1");

            foreach (var platform in options.Platforms)
            {
                switch (platform)
                {
                case TargetPlatform.Windows:
                {
                    var solutionPath = Path.Combine(root, "DirectXTex_Desktop_2015.sln");
                    var binFolder = Path.Combine(root, "DirectXTex", "Bin", "Desktop_2015");
                    Deploy.VCEnvironment.BuildSolution(solutionPath, configuration, "x64");
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.x64);
                    foreach (var file in outputFileNames)
                    {
                        Utilities.FileCopy(Path.Combine(binFolder, "x64", configuration, file), Path.Combine(depsFolder, file));
                    }
                    break;
                }
                case TargetPlatform.UWP:
                {
                    var solutionPath = Path.Combine(root, "DirectXTex_Windows10_2017.sln");
                    var binFolder = Path.Combine(root, "DirectXTex", "Bin", "Windows10_2017");
                    Deploy.VCEnvironment.BuildSolution(solutionPath, configuration, "x64");
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.x64);
                    foreach (var file in outputFileNames)
                    {
                        Utilities.FileCopy(Path.Combine(binFolder, "x64", configuration, file), Path.Combine(depsFolder, file));
                    }
                    break;
                }
                case TargetPlatform.XboxOne:
                {
                    var solutionPath = Path.Combine(root, "DirectXTex_GXDK_2017.sln");
                    File.Copy(Path.Combine(GetBinariesFolder(options, platform), "DirectXTex_GXDK_2017.sln"), solutionPath, true);
                    var projectFileContents = File.ReadAllText(Path.Combine(GetBinariesFolder(options, platform), "DirectXTex_GXDK_2017.vcxproj"));
                    projectFileContents = projectFileContents.Replace("___VS_TOOLSET___", "v142");
                    var projectPath = Path.Combine(root, "DirectXTex", "DirectXTex_GXDK_2017.vcxproj");
                    File.WriteAllText(projectPath, projectFileContents);
                    var binFolder = Path.Combine(root, "DirectXTex", "Bin", "GXDK_2017");
                    Deploy.VCEnvironment.BuildSolution(solutionPath, configuration, "Gaming.Xbox.XboxOne.x64");
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.x64);
                    foreach (var file in outputFileNames)
                    {
                        Utilities.FileCopy(Path.Combine(binFolder, "Gaming.Xbox.XboxOne.x64", configuration, file), Path.Combine(depsFolder, file));
                    }
                    break;
                }
                case TargetPlatform.XboxScarlett:
                {
                    var solutionPath = Path.Combine(root, "DirectXTex_GXDK_2017.sln");
                    File.Copy(Path.Combine(GetBinariesFolder(options, platform), "DirectXTex_GXDK_2017.sln"), solutionPath, true);
                    var projectFileContents = File.ReadAllText(Path.Combine(GetBinariesFolder(options, platform), "DirectXTex_GXDK_2017.vcxproj"));
                    projectFileContents = projectFileContents.Replace("___VS_TOOLSET___", "v142");
                    var projectPath = Path.Combine(root, "DirectXTex", "DirectXTex_GXDK_2017.vcxproj");
                    File.WriteAllText(projectPath, projectFileContents);
                    var binFolder = Path.Combine(root, "DirectXTex", "Bin", "GXDK_2017");
                    Deploy.VCEnvironment.BuildSolution(solutionPath, configuration, "Gaming.Xbox.Scarlett.x64");
                    var depsFolder = GetThirdPartyFolder(options, platform, TargetArchitecture.x64);
                    foreach (var file in outputFileNames)
                    {
                        Utilities.FileCopy(Path.Combine(binFolder, "Gaming.Xbox.Scarlett.x64", configuration, file), Path.Combine(depsFolder, file));
                    }
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
