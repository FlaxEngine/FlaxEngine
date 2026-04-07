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
        case TargetPlatform.Linux:
            return options.Architecture == TargetArchitecture.x64;
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
        options.PrivateDependencies.Add("glslang");
        options.PrivateDependencies.Add("spirv-tools");
        options.PrivateDependencies.Add("lz4");
    }
}
