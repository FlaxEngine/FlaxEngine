// Copyright (c) Wojciech Figat. All rights reserved.

using System;
using System.IO;
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
        public override void Build(BuildOptions options)
        {
            foreach (var platform in options.Platforms)
            {
                BuildStarted(platform);
                switch (platform)
                {
                case TargetPlatform.Windows:
                {
                    var sdk = WindowsPlatformBase.GetSDKs().Last();
                    var sdkLibLocation = Path.Combine(sdk.Value, "Lib", WindowsPlatformBase.GetSDKVersion(sdk.Key).ToString(), "um");
                    string binLocation = Path.Combine(sdk.Value, "bin", WindowsPlatformBase.GetSDKVersion(sdk.Key).ToString());

                    foreach (var architecture in new[] { TargetArchitecture.x64, TargetArchitecture.ARM64 })
                    {
                        var depsFolder = GetThirdPartyFolder(options, platform, architecture);

                        string dxilLocation = @$"{binLocation}\{architecture}\dxil.dll";
                        string dxcompilerLocation = @$"{binLocation}\{architecture}\dxcompiler.dll";
                        string d3dcompilerLocation = @$"{binLocation}\{architecture}\d3dcompiler_47.dll";
                        Utilities.FileCopy(dxilLocation, Path.Combine(depsFolder, Path.GetFileName(dxilLocation)));
                        Utilities.FileCopy(dxcompilerLocation, Path.Combine(depsFolder, Path.GetFileName(dxcompilerLocation)));
                        Utilities.FileCopy(d3dcompilerLocation, Path.Combine(depsFolder, Path.GetFileName(d3dcompilerLocation)));

                        string dxcompilerLibLocation = @$"{sdkLibLocation}\{architecture}\dxcompiler.lib";
                        string d3dcompilerLibLocation = @$"{sdkLibLocation}\{architecture}\d3dcompiler.lib";
                        Utilities.FileCopy(dxcompilerLibLocation, Path.Combine(depsFolder, Path.GetFileName(dxcompilerLibLocation)));
                        Utilities.FileCopy(d3dcompilerLibLocation, Path.Combine(depsFolder, "d3dcompiler_47.lib"));
                    }
                    break;
                }
                }
            }
        }
    }
}
