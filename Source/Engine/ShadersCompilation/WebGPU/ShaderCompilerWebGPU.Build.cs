// Copyright (c) Wojciech Figat. All rights reserved.

using System.IO;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// WebGPU shaders compiler module.
/// </summary>
public class ShaderCompilerWebGPU : ShaderCompiler
{
    public static bool Use(BuildOptions options)
    {
        // Requires prebuilt tint executable in platform ThirdParty binaries folder
        // https://github.com/google/dawn/releases
        // License: Source/ThirdParty/tint-license.txt (BSD 3-Clause)
        switch (options.Platform.Target)
        {
        case TargetPlatform.Windows:
            return options.Architecture == TargetArchitecture.x64;
        case TargetPlatform.Linux: // TODO: add Linux binary with tint
        case TargetPlatform.Mac:
            return options.Architecture == TargetArchitecture.ARM64;
        default:
            return false;
        }
    }

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.PublicDefinitions.Add("COMPILE_WITH_WEBGPU_SHADER_COMPILER");
        options.PublicDependencies.Add("ShaderCompilerVulkan");

        // Deploy tint executable as a dependency for the shader compilation from SPIR-V into WGSL
        // Tint compiler from: https://github.com/google/dawn/releases
        // License: Source/ThirdParty/tint-license.txt (BSD 3-Clause)
        var depsRoot = options.DepsFolder;
        switch (options.Platform.Target)
        {
        case TargetPlatform.Windows:
            if (options.Architecture == TargetArchitecture.x64)
                options.DependencyFiles.Add(Path.Combine(depsRoot, "tint.exe"));
            break;
        case TargetPlatform.Linux:
            if (options.Architecture == TargetArchitecture.x64)
                options.DependencyFiles.Add(Path.Combine(depsRoot, "tint"));
            break;
        case TargetPlatform.Mac:
            if (options.Architecture == TargetArchitecture.ARM64)
                options.DependencyFiles.Add(Path.Combine(depsRoot, "tint"));
            break;
        }
    }
}
