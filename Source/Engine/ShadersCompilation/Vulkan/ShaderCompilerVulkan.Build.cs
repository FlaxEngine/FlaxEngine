// Copyright (c) Wojciech Figat. All rights reserved.

using Flax.Build;
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

        if (options.Platform.Target == TargetPlatform.Windows)
        {
            options.PrivateDefinitions.Add("COMPILE_WITH_VK_DXC_SPIRV");

            var depsRoot = options.DepsFolder;
            options.OutputFiles.Add(System.IO.Path.Combine(depsRoot, "dxcompiler.lib"));
            options.DependencyFiles.Add(System.IO.Path.Combine(depsRoot, "dxcompiler.dll"));
            options.DependencyFiles.Add(System.IO.Path.Combine(depsRoot, "dxil.dll"));
            options.DelayLoadLibraries.Add("dxcompiler.dll");
        }
    }
}
