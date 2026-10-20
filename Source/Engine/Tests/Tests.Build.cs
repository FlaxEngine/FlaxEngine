// Copyright (c) Wojciech Figat. All rights reserved.

using System.Collections.Generic;
using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// Engine tests module.
/// </summary>
public class Tests : EngineModule
{
    /// <inheritdoc />
    public Tests()
    {
        Deploy = false;
    }

    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.PrivateDependencies.Add("ModelTool");
        options.PrivateDependencies.Add("ShadersCompilation");

        // DirectX 12 backend tests (with the condition that the Graphics module uses for this backend)
        if (options.Platform.Target == TargetPlatform.Windows && options.Toolchain is Flax.Build.Platforms.WindowsToolchain windowsToolchain && windowsToolchain.SDK != Flax.Build.Platforms.WindowsPlatformSDK.v8_1)
            options.PrivateDependencies.Add("GraphicsDeviceDX12");
    }

    /// <inheritdoc />
    public override void GetFilesToDeploy(List<string> files)
    {
    }
}
