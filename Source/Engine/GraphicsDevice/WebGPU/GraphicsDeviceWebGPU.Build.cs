// Copyright (c) Wojciech Figat. All rights reserved.

using System.IO;
using Flax.Build.NativeCpp;
using Flax.Build.Platforms;

/// <summary>
/// WebGPU graphics backend module.
/// </summary>
public class GraphicsDeviceWebGPU : GraphicsDeviceBaseModule
{
    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        var port = "--use-port=emdawnwebgpu:cpp_bindings=false";
        options.CompileEnv.CustomArgs.Add(port);
        options.LinkEnv.CustomArgs.Add("-sASYNCIFY");
        options.OutputFiles.Add(port);
        options.PublicDefinitions.Add("GRAPHICS_API_WEBGPU");
        options.PrivateIncludePaths.Add(Path.Combine(EmscriptenSdk.Instance.EmscriptenPath, "emscripten/cache/ports/emdawnwebgpu/emdawnwebgpu_pkg/webgpu/include"));
    }
}
