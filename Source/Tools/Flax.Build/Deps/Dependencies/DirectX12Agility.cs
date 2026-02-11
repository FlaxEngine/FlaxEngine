// Copyright (c) Wojciech Figat. All rights reserved.

using System.IO;
using System.IO.Compression;
using Flax.Build;

namespace Flax.Deps.Dependencies
{
    /// <summary>
    /// DirectX 12 Agility SDK https://aka.ms/directx12agility
    /// </summary>
    /// <seealso cref="Flax.Deps.Dependency" />
    class DirectX12Agility : Dependency
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
                        TargetArchitecture.x86,
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
            var root = options.IntermediateFolder;
            var packagePath = Path.Combine(root, "package.zip");

            // Download package
            var version = "1.618.5";
            if (!File.Exists(packagePath))
                Downloader.DownloadFileFromUrlToPath("https://www.nuget.org/api/v2/package/Microsoft.Direct3D.D3D12/" + version, packagePath);
            using (ZipArchive archive = ZipFile.Open(packagePath, ZipArchiveMode.Read))
                archive.ExtractToDirectory(root, true);

            // Deploy header files and license file
            var dstIncludePath = Path.Combine(options.ThirdPartyFolder, "DirectX12Agility");
            foreach (var file in new[]
                     {
                         "LICENSE.txt",
                     })
            {
                Utilities.FileCopy(Path.Combine(root, file), Path.Combine(dstIncludePath, Path.GetFileName(file)));
            }
            Utilities.DirectoryCopy(Path.Combine(root, "build/native/include"), dstIncludePath, true, true, "*.h");
        }
    }
}
