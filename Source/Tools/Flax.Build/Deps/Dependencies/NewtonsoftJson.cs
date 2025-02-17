// Copyright (c) 2012-2024 Wojciech Figat. All rights reserved.

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
                        TargetPlatform.Linux,
                        TargetPlatform.XboxOne,
                        TargetPlatform.XboxScarlett,
                        TargetPlatform.PS4,
                        TargetPlatform.PS5,
                        TargetPlatform.Switch,
                        TargetPlatform.Mac,
                        TargetPlatform.iOS,
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
            var binFolder = Path.Combine(root, "Src", "Newtonsoft.Json", "bin", configuration, "net8.0");

            // Get the source
            CloneGitRepo(root, "https://github.com/FlaxEngine/Newtonsoft.Json.git");

            // Default build
            GitCheckout(root, "flax-net80");
            Deploy.VCEnvironment.BuildSolution(solutionPath, configuration, buildPlatform);
            {
                var platform = "JIT";
                Log.Info($"Building {GetType().Name} for {platform}");
                foreach (var file in outputFileNames)
                    Utilities.FileCopy(Path.Combine(binFolder, file), Path.Combine(options.PlatformsFolder, "DotNet", file));
            }

            // AOT build (disabled codegen)
            Utilities.ReplaceInFile(Path.Combine(root, "Src", "Newtonsoft.Json", "Newtonsoft.Json.csproj"), "HAVE_REFLECTION_EMIT;", ";");
            Utilities.ReplaceInFile(Path.Combine(root, "Src", "Newtonsoft.Json", "Newtonsoft.Json.csproj"), "HAVE_DYNAMIC;", ";");
            Utilities.ReplaceInFile(Path.Combine(root, "Src", "Newtonsoft.Json", "Newtonsoft.Json.csproj"), "HAVE_EXPRESSIONS;", ";");
            Utilities.ReplaceInFile(Path.Combine(root, "Src", "Newtonsoft.Json", "Newtonsoft.Json.csproj"), "HAVE_REGEX;", ";");
            Utilities.ReplaceInFile(Path.Combine(root, "Src", "Newtonsoft.Json", "Newtonsoft.Json.csproj"), "HAVE_TYPE_DESCRIPTOR;", ";");
            Deploy.VCEnvironment.BuildSolution(solutionPath, configuration, buildPlatform);
            {
                var platform = "AOT";
                Log.Info($"Building {GetType().Name} for {platform}");
                var file = "Newtonsoft.Json.dll";
                Utilities.FileCopy(Path.Combine(binFolder, file), Path.Combine(options.PlatformsFolder, "DotNet/AOT", file));
            }
        }
    }
}
