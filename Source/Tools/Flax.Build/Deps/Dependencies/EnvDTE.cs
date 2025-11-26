// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.IO;
using System.IO.Compression;
using System.Linq;
using Flax.Build;
using Flax.Build.Platforms;

namespace Flax.Deps.Dependencies
{
    /// <summary>
    /// Visual Studio EnvDTE COM library. https://learn.microsoft.com/en-us/dotnet/api/envdte?view=visualstudiosdk-2022
    /// </summary>
    /// <seealso cref="Flax.Deps.Dependency" />
    class EnvDTE : Dependency
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
        public override TargetArchitecture[] Architectures
        {
            get
            {
                switch (BuildPlatform)
                {
                case TargetPlatform.Windows:
                return new[]
                {
                        TargetArchitecture.x64,
                        TargetArchitecture.ARM64,
                    };
                default: return new TargetArchitecture[0];
                }
            }
        }

        /// <inheritdoc />
        public override void Build(BuildOptions options)
        {
            options.IntermediateFolder.Replace("/" + GetType().Name, "/Microsoft.VisualStudio.Setup.Configuration.Native");

            // Get the source
            var root = options.IntermediateFolder;
            var packagePath = Path.Combine(root, $"package.zip");
            if (!File.Exists(packagePath))
            {
                Downloader.DownloadFileFromUrlToPath("https://www.nuget.org/api/v2/package/Microsoft.VisualStudio.Setup.Configuration.Native/3.14.2075", packagePath);
            }
            var extractedPath = Path.Combine(root, "extracted");
            if (!Directory.Exists(extractedPath))
            {
                using (ZipArchive archive = ZipFile.Open(packagePath, ZipArchiveMode.Read))
                    archive.ExtractToDirectory(extractedPath);
            }
            root = extractedPath;

            foreach (var platform in options.Platforms)
            {
                foreach (var architecture in options.Architectures)
                {
                    BuildStarted(platform, architecture);
                    switch (platform)
                    {
                    case TargetPlatform.Windows:
                    {
                        var bin = Path.Combine(root, "lib", "native", "v141", architecture.ToString().ToLower());
                        var depsFolder = GetThirdPartyFolder(options, platform, architecture);
                        Utilities.FileCopy(Path.Combine(bin, "Microsoft.VisualStudio.Setup.Configuration.Native.lib"), Path.Combine(depsFolder, "Microsoft.VisualStudio.Setup.Configuration.Native.lib"));

                        var include = Path.Combine(root, "lib", "native", "include");
                        Utilities.FileCopy(Path.Combine(include, "Setup.Configuration.h"), Path.Combine(options.ThirdPartyFolder, "Microsoft.VisualStudio.Setup.Configuration.Native", "Setup.Configuration.h"));
                        break;
                    }
                    }
                }
            }
        }
    }
}
