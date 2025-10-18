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
    /// WinPixEventRuntime. https://github.com/microsoft/PixEvents
    /// </summary>
    /// <seealso cref="Flax.Deps.Dependency" />
    class WinPixEventRuntime : Dependency
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
            // Get the source
            var root = options.IntermediateFolder;
            var packagePath = Path.Combine(root, $"package.zip");
            if (!File.Exists(packagePath))
            {
                Downloader.DownloadFileFromUrlToPath("https://www.nuget.org/api/v2/package/WinPixEventRuntime/1.0.240308001", packagePath);
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
                        var bin = Path.Combine(root, "bin", architecture.ToString());
                        var depsFolder = GetThirdPartyFolder(options, platform, architecture);
                        Utilities.FileCopy(Path.Combine(bin, "WinPixEventRuntime.dll"), Path.Combine(depsFolder, "WinPixEventRuntime.dll"));
                        Utilities.FileCopy(Path.Combine(bin, "WinPixEventRuntime.lib"), Path.Combine(depsFolder, "WinPixEventRuntime.lib"));
                        break;
                    }
                    }
                }
            }
        }
    }
}
