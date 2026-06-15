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
    /// Uses the Microsoft.Direct3D.DXC NuGet redistributable (SPIR-V codegen enabled).
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

        static string GetNativeArchFolder(TargetArchitecture architecture)
        {
            switch (architecture)
            {
            case TargetArchitecture.x64:
                return "x64";
            case TargetArchitecture.ARM64:
                return "arm64";
            case TargetArchitecture.x86:
                return "x86";
            default:
                throw new System.Exception("Unsupported architecture for DirectX Shader Compiler.");
            }
        }

        /// <inheritdoc />
        public override void Build(BuildOptions options)
        {
            var root = options.IntermediateFolder;
            var packagePath = Path.Combine(root, "Microsoft.Direct3D.DXC.zip");

            // NuGet redistributable includes SPIR-V codegen (Windows SDK dxcompiler.dll does not).
            const string version = "1.9.2602.17";
            if (!File.Exists(packagePath))
                Downloader.DownloadFileFromUrlToPath("https://www.nuget.org/api/v2/package/Microsoft.Direct3D.DXC/" + version, packagePath);
            using (ZipArchive archive = ZipFile.Open(packagePath, ZipArchiveMode.Read))
                archive.ExtractToDirectory(root, true);

            // Deploy headers used by the engine (dxcapi.h, d3d12shader.h, etc.)
            var dstIncludePath = Path.Combine(options.ThirdPartyFolder, "DirectXShaderCompiler");
            Utilities.DirectoryCopy(Path.Combine(root, "build", "native", "include"), dstIncludePath, true, true);

            foreach (var platform in options.Platforms)
            {
                foreach (var architecture in options.Architectures)
                {
                    BuildStarted(platform, architecture);
                    switch (platform)
                    {
                    case TargetPlatform.Windows:
                    {
                        var archFolder = GetNativeArchFolder(architecture);
                        var depsFolder = GetThirdPartyFolder(options, platform, architecture);
                        var nativeRoot = Path.Combine(root, "build", "native");

                        Utilities.FileCopy(Path.Combine(nativeRoot, "bin", archFolder, "dxcompiler.dll"), Path.Combine(depsFolder, "dxcompiler.dll"));
                        Utilities.FileCopy(Path.Combine(nativeRoot, "bin", archFolder, "dxil.dll"), Path.Combine(depsFolder, "dxil.dll"));
                        Utilities.FileCopy(Path.Combine(nativeRoot, "lib", archFolder, "dxcompiler.lib"), Path.Combine(depsFolder, "dxcompiler.lib"));

                        // Legacy D3D shader compiler still comes from the Windows SDK.
                        var sdk = WindowsPlatformBase.GetSDKs().Last();
                        var sdkLibLocation = Path.Combine(sdk.Value, "Lib", WindowsPlatformBase.GetSDKVersion(sdk.Key).ToString(), "um");
                        string binLocation = Path.Combine(sdk.Value, "bin", WindowsPlatformBase.GetSDKVersion(sdk.Key).ToString());
                        string d3dcompilerLocation = @$"{binLocation}\{architecture}\d3dcompiler_47.dll";
                        string d3dcompilerLibLocation = @$"{sdkLibLocation}\{architecture}\d3dcompiler.lib";
                        Utilities.FileCopy(d3dcompilerLocation, Path.Combine(depsFolder, Path.GetFileName(d3dcompilerLocation)));
                        Utilities.FileCopy(d3dcompilerLibLocation, Path.Combine(depsFolder, "d3dcompiler_47.lib"));
                        break;
                    }
                    }
                }
            }
        }
    }
}
