// Copyright (c) 2012-2022 Wojciech Figat. All rights reserved.

using Flax.Build;
using Flax.Build.NativeCpp;

/// <summary>
/// 3D graphics rendering module.
/// </summary>
public class Renderer : EngineModule
{
    /// <inheritdoc />
    public override void Setup(BuildOptions options)
    {
        base.Setup(options);

        options.PrivateDependencies.Add("Graphics");
        options.PrivateDependencies.Add("Content");

        if (options.Target.IsEditor)
        {
            options.PublicDefinitions.Add("COMPILE_WITH_PROBES_BAKING");
        }
    }
}
