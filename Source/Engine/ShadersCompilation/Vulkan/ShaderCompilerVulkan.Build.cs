// Copyright (c) Wojciech Figat. All rights reserved.

using Flax.Build.NativeCpp;

/// <summary>
/// Vulkan shaders compiler module.
/// </summary>
public class ShaderCompilerVulkan : ShaderCompiler
{
    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.PublicDefinitions.Add("COMPILE_WITH_VK_SHADER_COMPILER");

        options.PrivateDependencies.Add("glslang");
        options.PrivateDependencies.Add("spirv-tools");
    }
}
