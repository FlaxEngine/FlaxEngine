// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using System.IO;
using System.Linq;
using Flax.Build;
using Flax.Build.NativeCpp;
using Flax.Build.Platforms;

/// <summary>
/// Shader compiler module base class.
/// </summary>
public abstract class ShaderCompiler : EngineModule
{
    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.PrivateDefinitions.Add("COMPILE_WITH_SHADER_COMPILER");
    }

    /// <inheritdoc />
    public override void GetFilesToDeploy(List<string> files)
    {
    }
}

/// <summary>
/// Shaders compilation module.
/// </summary>
public class ShadersCompilation : EngineModule
{
    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.PublicDefinitions.Add("COMPILE_WITH_SHADER_COMPILER");

        options.SourcePaths.Clear();
        options.SourceFiles.AddRange(Directory.GetFiles(FolderPath, "*.*", SearchOption.TopDirectoryOnly));
        options.SourcePaths.Add(Path.Combine(FolderPath, "Parser"));

        switch (options.Platform.Target)
        {
        case TargetPlatform.Windows:
            options.PrivateDependencies.Add("ShaderCompilerD3D");
            if (WindowsPlatformBase.GetSDKs().Any(x => x.Key != WindowsPlatformSDK.v8_1))
                options.PrivateDependencies.Add("ShaderCompilerDX");
            options.PrivateDependencies.Add("ShaderCompilerVulkan");
            break;
        case TargetPlatform.Linux:
            options.PrivateDependencies.Add("ShaderCompilerVulkan");
            break;
        case TargetPlatform.Mac:
            options.PrivateDependencies.Add("ShaderCompilerVulkan");
            break;
        default: throw new InvalidPlatformException(options.Platform.Target);
        }

        if (Sdk.HasValid("PS4Sdk"))
            options.PrivateDependencies.Add("ShaderCompilerPS4");
        if (Sdk.HasValid("PS5Sdk"))
            options.PrivateDependencies.Add("ShaderCompilerPS5");
    }

    /// <inheritdoc />
    public override void GetFilesToDeploy(List<string> files)
    {
    }
}
