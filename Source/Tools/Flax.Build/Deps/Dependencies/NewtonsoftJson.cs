// Copyright (c) 2012-2021 Wojciech Figat. All rights reserved.

using System.IO;
using Flax.Build;

namespace Flax.Deps.Dependencies
{
    /// <summary>
    /// JSON framework for .NET http://www.newtonsoft.com/json
    /// </summary>
    /// <seealso cref="Flax.Deps.Dependency" />
    class NewtonsoftJson : Dependency
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
                        TargetPlatform.Linux,
                        TargetPlatform.XboxOne,
                        TargetPlatform.XboxScarlett,
                        TargetPlatform.PS4,
                        TargetPlatform.Switch,
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
            var buildPlatform = "Any CPU";
            var solutionPath = Path.Combine(root, "Src", "Newtonsoft.Json.sln");
            var outputFileNames = new[]
            {
                "Newtonsoft.Json.dll",
                "Newtonsoft.Json.pdb",
                "Newtonsoft.Json.xml",
            };
            var binFolder = Path.Combine(root, "Src", "Newtonsoft.Json", "bin", configuration, "net45");

            // Get the source
            CloneGitRepo(root, "https://github.com/FlaxEngine/Newtonsoft.Json.git");

            // Default build
            GitCheckout(root, "master");
            Deploy.VCEnvironment.BuildSolution(solutionPath, configuration, buildPlatform);
            foreach (var platform in options.Platforms)
            {
                switch (platform)
                {
                case TargetPlatform.Windows:
                case TargetPlatform.Linux:
                {
                    foreach (var file in outputFileNames)
                    {
                        Utilities.FileCopy(Path.Combine(binFolder, file), Path.Combine(options.PlatformsFolder, "DotNet", file));
                    }
                    break;
                }
                }
            }

            // AOT build
            GitCheckout(root, "aot");
            Deploy.VCEnvironment.BuildSolution(solutionPath, configuration, buildPlatform);
            foreach (var platform in options.Platforms)
            {
                switch (platform)
                {
                case TargetPlatform.UWP:
                case TargetPlatform.XboxOne:
                case TargetPlatform.PS4:
                case TargetPlatform.XboxScarlett:
                case TargetPlatform.Switch:
                {
                    var file = "Newtonsoft.Json.dll";
                    Utilities.FileCopy(Path.Combine(binFolder, file), Path.Combine(options.PlatformsFolder, platform.ToString(), "Binaries", file));
                    break;
                }
                }
            }
        }
    }
}
