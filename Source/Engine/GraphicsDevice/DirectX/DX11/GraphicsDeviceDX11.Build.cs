// Copyright (c) Wojciech Figat. All rights reserved.

using Flax.Build.NativeCpp;

/// <summary>
/// DirectX 11 graphics backend module.
/// </summary>
public class GraphicsDeviceDX11 : GraphicsDeviceBaseModule
{
    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.PublicDefinitions.Add("GRAPHICS_API_DIRECTX11");
        options.OutputFiles.Add("d3d11.lib");
    }
}
