// Copyright (c) Wojciech Figat. All rights reserved.

using System.IO;
using System.IO.Compression;
using System.Linq;
using Flax.Build;
using Flax.Build.Platforms;

namespace Flax.Deps.Dependencies
{
    /// <summary>
    /// DirectX Shader Compiler and tools. https://github.com/microsoft/DirectXShaderCompiler
    /// </summary>
    /// <seealso cref="Flax.Deps.Dependency" />
    class DirectXShaderCompiler : Dependency
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
            var root = options.IntermediateFolder;

            // Download package
            var version = "1.9.2602.24";
            var packagePath = Path.Combine(root, "package.zip");
            if (!File.Exists(packagePath))
                Downloader.DownloadFileFromUrlToPath("https://www.nuget.org/api/v2/package/Microsoft.Direct3D.DXC/" + version, packagePath);
            using (ZipArchive archive = ZipFile.Open(packagePath, ZipArchiveMode.Read))
                archive.ExtractToDirectory(root, true);

            // Deploy header files and license file
            var dstIncludePath = Path.Combine(options.ThirdPartyFolder, "DirectXShaderCompiler");
            foreach (var file in new[]
                     {
                         "LICENCE-MIT.txt",
                         "build/native/include/dxcapi.h",
                     })
            {
                Utilities.FileCopy(Path.Combine(root, file), Path.Combine(dstIncludePath, Path.GetFileName(file)));
            }

            // Deploy binaries
            foreach (var platform in options.Platforms)
            {
                foreach (var architecture in options.Architectures)
                {
                    BuildStarted(platform, architecture);
                    switch (platform)
                    {
                    case TargetPlatform.Windows:
                    {
                        var sdk = WindowsPlatformBase.GetSDKs().Last();
                        var sdkLibLocation = Path.Combine(sdk.Value, "Lib", WindowsPlatformBase.GetSDKVersion(sdk.Key).ToString(), "um");
                        var binLocation = Path.Combine(sdk.Value, "bin", WindowsPlatformBase.GetSDKVersion(sdk.Key).ToString());
                        var depsFolder = GetThirdPartyFolder(options, platform, architecture);

                        // Windows SDK
                        string dxilLocation = @$"{binLocation}\{architecture}\dxil.dll";
                        string dxcompilerLocation = @$"{binLocation}\{architecture}\dxcompiler.dll";
                        string d3dcompilerLocation = @$"{binLocation}\{architecture}\d3dcompiler_47.dll";
                        string dxcompilerLibLocation = @$"{sdkLibLocation}\{architecture}\dxcompiler.lib";
                        string d3dcompilerLibLocation = @$"{sdkLibLocation}\{architecture}\d3dcompiler.lib";

                        // Microsoft.Direct3D.DXC
                        var dxcBin = Path.Combine(root, "build", "native", "bin", architecture.ToString().ToLower());
                        var dxcLib = Path.Combine(root, "build", "native", "lib", architecture.ToString().ToLower());
                        dxilLocation = @$"{dxcBin}\dxil.dll";
                        dxcompilerLocation = @$"{dxcBin}\dxcompiler.dll";
                        dxcompilerLibLocation = @$"{dxcLib}\dxcompiler.lib";

                        Utilities.FileCopy(dxilLocation, Path.Combine(depsFolder, Path.GetFileName(dxilLocation)));
                        Utilities.FileCopy(dxcompilerLocation, Path.Combine(depsFolder, Path.GetFileName(dxcompilerLocation)));
                        Utilities.FileCopy(d3dcompilerLocation, Path.Combine(depsFolder, Path.GetFileName(d3dcompilerLocation)));
                        Utilities.FileCopy(dxcompilerLibLocation, Path.Combine(depsFolder, Path.GetFileName(dxcompilerLibLocation)));
                        Utilities.FileCopy(d3dcompilerLibLocation, Path.Combine(depsFolder, "d3dcompiler_47.lib"));
                        break;
                    }
                    }
                }
            }
        }
    }
}
