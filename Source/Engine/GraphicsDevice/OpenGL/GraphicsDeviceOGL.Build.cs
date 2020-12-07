// Copyright (c) 2012-2020 Wojciech Figat. All rights reserved.

using Flax.Build.NativeCpp;

/// <summary>
/// OpenGL graphics backend module.
/// </summary>
public class GraphicsDeviceOGL : GraphicsDeviceBaseModule
{
    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.PublicDefinitions.Add("GRAPHICS_API_OPENGL");
        //options.PublicDefinitions.Add("GRAPHICS_API_OPENGLES");
    }
}
