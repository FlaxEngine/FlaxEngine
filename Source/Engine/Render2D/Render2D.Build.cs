// Copyright (c) Wojciech Figat. All rights reserved.

using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// 2D graphics rendering module.
/// </summary>
public class Render2D : EngineModule
{
    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.PrivateDependencies.Add("freetype");

        options.PrivateDefinitions.Add("RENDER2D_USE_LINE_AA");
    }
}
